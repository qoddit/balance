#include "net.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <stdatomic.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#define MAX_INCOMING 32768

typedef struct PeerConn {
    int fd;
    pthread_t thread;
    bool thread_started;
    pthread_mutex_t send_mu;
    _Atomic bool alive;
    bool incoming;
    bool tunnel_active;
    char username[BALANCE_MAX_USERNAME];
    char endpoint[BALANCE_MAX_ENDPOINT];
    uint8_t ed_pub[32];
    uint8_t key[32];
    uint8_t tx_nonce[4];
    uint8_t rx_nonce[4];
    uint64_t tx_seq;
    uint64_t rx_seq;
    char tunnel_id[BALANCE_MAX_TUNNEL];
    char transfers[16][65];
    size_t transfer_count;
    struct PeerConn *next;
    struct NetContext *net;
} PeerConn;

typedef struct {
    NetEventFn cb;
    void *opaque;
    char tunnel_id[BALANCE_MAX_TUNNEL];
    PeerConn *peer;
} PendingMap;

struct NetContext {
    Vault *vault;
    NetEventFn cb;
    void *opaque;
    int listen_fd;
    uint16_t listen_port;
    pthread_t listener;
    _Atomic bool stop;
    pthread_mutex_t peers_mu;
    PeerConn *peers;
    pthread_mutex_t workers_mu;
    pthread_t workers[64];
    size_t worker_count;
    _Atomic uint64_t tx_bytes;
    _Atomic uint64_t rx_bytes;
    _Atomic uint64_t tx_frames;
    _Atomic uint64_t rx_frames;
    pthread_mutex_t stats_mu;
    uint64_t stats_last_ms;
    uint64_t stats_last_tx_bytes;
    uint64_t stats_last_rx_bytes;
};

static void emit(NetContext *n, const NetEvent *ev) { if(n && n->cb && ev)n->cb(ev,n->opaque); }
static void copy_text(char *dst,size_t cap,const char *src){if(!dst||cap==0)return;if(!src)src="";snprintf(dst,cap,"%s",src);}
static void put16(uint8_t*p,uint16_t x){p[0]=(uint8_t)(x>>8);p[1]=(uint8_t)x;}
static uint16_t get16(const uint8_t*p){return (uint16_t)(((uint16_t)p[0]<<8)|p[1]);}
static void put32(uint8_t*p,uint32_t x){p[0]=(uint8_t)(x>>24);p[1]=(uint8_t)(x>>16);p[2]=(uint8_t)(x>>8);p[3]=(uint8_t)x;}
static uint32_t get32(const uint8_t*p){return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];}
static void put64(uint8_t*p,uint64_t x){for(int i=0;i<8;++i)p[7-i]=(uint8_t)(x>>(i*8));}
static uint64_t get64(const uint8_t*p){uint64_t x=0;for(int i=0;i<8;++i)x=(x<<8)|p[i];return x;}

static int read_full(int fd, void *buf, size_t n){uint8_t*p=buf;while(n){ssize_t r=recv(fd,p,n,0);if(r<=0)return BALANCE_ERR;p+=(size_t)r;n-=(size_t)r;}return BALANCE_OK;}
static int write_full(int fd, const void *buf, size_t n){const uint8_t*p=buf;while(n){ssize_t w=send(fd,p,n,MSG_NOSIGNAL);if(w<=0)return BALANCE_ERR;p+=(size_t)w;n-=(size_t)w;}return BALANCE_OK;}
static int recv_frame(int fd,uint8_t **out,size_t*out_len){uint8_t h[4];if(read_full(fd,h,4U)!=BALANCE_OK)return BALANCE_ERR;uint32_t n=get32(h);if(n==0U||n>BALANCE_MAX_FRAME+128U)return BALANCE_ERR;uint8_t*b=malloc(n);if(!b)return BALANCE_ERR;if(read_full(fd,b,n)!=BALANCE_OK){free(b);return BALANCE_ERR;}*out=b;*out_len=n;return BALANCE_OK;}
static int send_frame(int fd,const uint8_t*b,size_t n){if(n>UINT32_MAX)return BALANCE_ERR;uint8_t h[4];put32(h,(uint32_t)n);return write_full(fd,h,4U)==BALANCE_OK&&write_full(fd,b,n)==BALANCE_OK?BALANCE_OK:BALANCE_ERR;}

static int build_hello(Vault *v, EVP_PKEY *eph, uint8_t **out, size_t*out_len){
    uint8_t ep[32];if(crypto_x25519_public(eph,ep)!=BALANCE_OK)return BALANCE_ERR;size_t un=strlen(v->username);if(un>=BALANCE_MAX_USERNAME)return BALANCE_ERR;uint8_t msg[16+BALANCE_MAX_USERNAME+32+32];size_t ml=0;memcpy(msg+ml,"BALANCE-H2",10U);ml+=10U;put16(msg+ml,2U);ml+=2U;put16(msg+ml,(uint16_t)un);ml+=2U;memcpy(msg+ml,v->username,un);ml+=un;memcpy(msg+ml,v->ed_pub,32U);ml+=32U;memcpy(msg+ml,ep,32U);ml+=32U;uint8_t sig[64];if(crypto_ed25519_sign(v->ed_private,msg,ml,sig)!=BALANCE_OK)return BALANCE_ERR;size_t n=2U+2U+un+32U+32U+64U;uint8_t*p=malloc(n);if(!p)return BALANCE_ERR;put16(p,2U);put16(p+2,(uint16_t)un);memcpy(p+4,v->username,un);memcpy(p+4+un,v->ed_pub,32U);memcpy(p+36+un,ep,32U);memcpy(p+68+un,sig,64U);*out=p;*out_len=n;return BALANCE_OK;
}

typedef struct { uint16_t version; char username[BALANCE_MAX_USERNAME]; uint8_t ed_pub[32]; uint8_t eph_pub[32]; uint8_t sig[64]; } Hello;
static int parse_hello(const uint8_t*p,size_t n,Hello*h){if(!p||!h||n<4U+32U+32U+64U)return BALANCE_ERR;memset(h,0,sizeof(*h));h->version=get16(p);uint16_t un=get16(p+2);if(h->version!=2U||un>=BALANCE_MAX_USERNAME||4U+(size_t)un+128U!=n)return BALANCE_ERR;memcpy(h->username,p+4,un);memcpy(h->ed_pub,p+4+un,32U);memcpy(h->eph_pub,p+36+un,32U);memcpy(h->sig,p+68+un,64U);return BALANCE_OK;}
static int verify_hello(const Hello*h){EVP_PKEY*p=EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519,NULL,h->ed_pub,32U);if(!p)return BALANCE_ERR;uint8_t msg[16+BALANCE_MAX_USERNAME+64];size_t ml=0;memcpy(msg+ml,"BALANCE-H2",10U);ml+=10U;put16(msg+ml,h->version);ml+=2U;size_t un=strlen(h->username);put16(msg+ml,(uint16_t)un);ml+=2U;memcpy(msg+ml,h->username,un);ml+=un;memcpy(msg+ml,h->ed_pub,32U);ml+=32U;memcpy(msg+ml,h->eph_pub,32U);ml+=32U;int rc=crypto_ed25519_verify(p,msg,ml,h->sig);EVP_PKEY_free(p);return rc;}
static int derive_session(const Hello*a,const Hello*b,const uint8_t shared[32],uint8_t key_a[32],uint8_t key_b[32],uint8_t n_a[4],uint8_t n_b[4]){
    const Hello *lo=a,*hi=b;if(memcmp(a->ed_pub,b->ed_pub,32U)>0){lo=b;hi=a;}uint8_t tr[64];memcpy(tr,lo->ed_pub,32U);memcpy(tr+32,hi->ed_pub,32U);uint8_t salt[32];if(crypto_sha256(tr,64U,salt)!=BALANCE_OK)return BALANCE_ERR;const uint8_t info[]="balance-session-v2";uint8_t material[72];if(crypto_hkdf(shared,32U,salt,32U,info,sizeof(info)-1U,material,sizeof(material))!=BALANCE_OK)return BALANCE_ERR;memcpy(key_a,material,32U);memcpy(n_a,material+64U,4U);memcpy(key_b,material+32U,32U);memcpy(n_b,material+68U,4U);secure_zero(material,sizeof(material));return BALANCE_OK;}

static Friend *find_friend(Vault*v,const char*user){for(size_t i=0;i<v->friend_count;++i)if(strcmp(v->friends[i].username,user)==0)return &v->friends[i];return NULL;}
static PeerConn *find_peer(NetContext*n,const char*tunnel_id){PeerConn*p=n->peers;while(p){if(p->alive&&strcmp(p->tunnel_id,tunnel_id)==0)return p;p=p->next;}return NULL;}
static void peer_add(NetContext*n,PeerConn*p){pthread_mutex_lock(&n->peers_mu);p->next=n->peers;n->peers=p;pthread_mutex_unlock(&n->peers_mu);}

static void remember_transfer(PeerConn*p,const char*transfer_id);

static int send_enc(PeerConn*p,uint8_t type,const uint8_t*payload,size_t payload_len){pthread_mutex_lock(&p->send_mu);uint8_t*b=NULL;size_t n=0;int rc=proto_encrypt_frame(type,p->tx_seq++,p->key,p->tx_nonce,NULL,0U,payload,payload_len,&b,&n);if(rc==BALANCE_OK)rc=send_frame(p->fd,b,n);if(rc==BALANCE_OK){atomic_fetch_add(&p->net->tx_bytes,(uint64_t)n+4ULL);atomic_fetch_add(&p->net->tx_frames,1ULL);}free(b);pthread_mutex_unlock(&p->send_mu);if(rc!=BALANCE_OK)p->alive=false;return rc;}

static int payload_tunnel(const char*tunnel_id,const uint8_t*extra,size_t extra_len,uint8_t**out,size_t*out_len){size_t tl=strlen(tunnel_id);if(tl==0||tl>=BALANCE_MAX_TUNNEL||extra_len>4096U)return BALANCE_ERR;uint8_t*b=malloc(2U+tl+extra_len);if(!b)return BALANCE_ERR;put16(b,(uint16_t)tl);memcpy(b+2,tunnel_id,tl);if(extra_len)memcpy(b+2+tl,extra,extra_len);*out=b;*out_len=2U+tl+extra_len;return BALANCE_OK;}
static int parse_tunnel(const uint8_t*p,size_t n,char*tunnel,size_t tunnel_sz,const uint8_t**extra,size_t*extra_len){if(n<2U)return BALANCE_ERR;uint16_t tl=get16(p);if(tl==0||tl>=tunnel_sz||2U+(size_t)tl>n)return BALANCE_ERR;memcpy(tunnel,p+2,tl);tunnel[tl]='\0';if(extra)*extra=p+2+tl;if(extra_len)*extra_len=n-2U-tl;return BALANCE_OK;}

static void *peer_thread(void*arg){PeerConn*p=arg;NetContext*n=p->net;uint8_t *b=NULL;size_t nbytes=0;for(;;){if(!p->alive)break;if(recv_frame(p->fd,&b,&nbytes)!=BALANCE_OK){free(b);b=NULL;break;}atomic_fetch_add(&n->rx_bytes,(uint64_t)nbytes+4ULL);atomic_fetch_add(&n->rx_frames,1ULL);ProtoFrame f={0};if(proto_decrypt_frame(p->key,p->rx_nonce,b,nbytes,NULL,0U,&f)!=BALANCE_OK){free(b);break;}free(b);b=NULL;if(f.seq!=p->rx_seq){proto_frame_free(&f);break;}p->rx_seq++;
        switch(f.type){
            case FRAME_TUNNEL_REQUEST:{char tid[BALANCE_MAX_TUNNEL];const uint8_t*e;size_t el;if(parse_tunnel(f.payload,f.payload_len,tid,sizeof(tid),&e,&el)!=BALANCE_OK){proto_frame_free(&f);p->alive=false;break;}if(el>0U){if(el<2U){proto_frame_free(&f);p->alive=false;break;}uint16_t ul=get16(e);if(ul==0U||ul>=BALANCE_MAX_USERNAME||2U+(size_t)ul!=el){proto_frame_free(&f);p->alive=false;break;}char from[BALANCE_MAX_USERNAME];memcpy(from,e+2,ul);from[ul]='\0';if(strcmp(from,p->username)!=0){proto_frame_free(&f);p->alive=false;break;}} Friend *trusted=find_friend(n->vault,p->username); if(!trusted||!trusted->key_pinned||CRYPTO_memcmp(trusted->ed_pub,p->ed_pub,32U)!=0){uint8_t *rej=NULL;size_t rj=0;if(payload_tunnel(tid,NULL,0U,&rej,&rj)==BALANCE_OK){(void)send_enc(p,FRAME_TUNNEL_REJECT,rej,rj);free(rej);}proto_frame_free(&f);p->alive=false;break;}copy_text(p->tunnel_id,sizeof(p->tunnel_id),tid);NetEvent ev={.type=NET_EV_TUNNEL_REQUEST};copy_text(ev.username,sizeof(ev.username),p->username);copy_text(ev.tunnel_id,sizeof(ev.tunnel_id),tid);memcpy(ev.ed_pub,p->ed_pub,32U);copy_text(ev.endpoint,sizeof(ev.endpoint),p->endpoint);emit(n,&ev);break;}
            case FRAME_TUNNEL_ACCEPT:{char tid[BALANCE_MAX_TUNNEL];if(parse_tunnel(f.payload,f.payload_len,tid,sizeof(tid),NULL,NULL)!=BALANCE_OK){proto_frame_free(&f);p->alive=false;break;}copy_text(p->tunnel_id,sizeof(p->tunnel_id),tid);p->tunnel_active=true;NetEvent ev={.type=NET_EV_TUNNEL_ACCEPTED};copy_text(ev.username,sizeof(ev.username),p->username);copy_text(ev.tunnel_id,sizeof(ev.tunnel_id),tid);emit(n,&ev);break;}
            case FRAME_TUNNEL_REJECT:{char tid[BALANCE_MAX_TUNNEL];const uint8_t*e;size_t el;if(parse_tunnel(f.payload,f.payload_len,tid,sizeof(tid),&e,&el)!=BALANCE_OK){proto_frame_free(&f);p->alive=false;break;}NetEvent ev={.type=NET_EV_TUNNEL_REJECTED};copy_text(ev.username,sizeof(ev.username),p->username);copy_text(ev.tunnel_id,sizeof(ev.tunnel_id),tid);if(el>0U&&el<sizeof(ev.reason)){memcpy(ev.reason,e,el);ev.reason[el]='\0';}emit(n,&ev);p->alive=false;break;}
            case FRAME_CHAT:{char tid[BALANCE_MAX_TUNNEL];const uint8_t*e;size_t el;if(parse_tunnel(f.payload,f.payload_len,tid,sizeof(tid),&e,&el)!=BALANCE_OK||el>=sizeof(((NetEvent*)0)->text)){proto_frame_free(&f);p->alive=false;break;}NetEvent ev={.type=NET_EV_MESSAGE};copy_text(ev.username,sizeof(ev.username),p->username);copy_text(ev.tunnel_id,sizeof(ev.tunnel_id),tid);memcpy(ev.text,e,el);ev.text[el]='\0';emit(n,&ev);break;}
            case FRAME_FILE_REQUEST:{char tid[BALANCE_MAX_TUNNEL];const uint8_t*e;size_t el;if(parse_tunnel(f.payload,f.payload_len,tid,sizeof(tid),&e,&el)!=BALANCE_OK||el<2U){proto_frame_free(&f);p->alive=false;break;}uint16_t idl=get16(e);if(idl==0||idl>64U||2U+idl+2U>el){proto_frame_free(&f);p->alive=false;break;}char xid[65];memcpy(xid,e+2,idl);xid[idl]='\0';size_t off=2U+idl;uint16_t fl=get16(e+off);off+=2U;if(fl>=sizeof(((NetEvent*)0)->filename)||off+fl+8U+32U>el){proto_frame_free(&f);p->alive=false;break;}NetEvent ev={.type=NET_EV_FILE_REQUEST};copy_text(ev.username,sizeof(ev.username),p->username);copy_text(ev.tunnel_id,sizeof(ev.tunnel_id),tid);copy_text(ev.transfer_id,sizeof(ev.transfer_id),xid);memcpy(ev.filename,e+off,fl);ev.filename[fl]='\0';off+=fl;ev.size=get64(e+off);off+=8U;memcpy(ev.digest,e+off,32U);remember_transfer(p,xid);emit(n,&ev);break;}
            case FRAME_FILE_ACCEPT:{if(f.payload_len>=2U){uint16_t l=get16(f.payload);if(l<=64U&&2U+l<=f.payload_len){NetEvent ev={.type=NET_EV_FILE_ACCEPTED};memcpy(ev.transfer_id,f.payload+2,l);ev.transfer_id[l]='\0';copy_text(ev.username,sizeof(ev.username),p->username);emit(n,&ev);}}break;}
            case FRAME_FILE_REJECT:{if(f.payload_len>=2U){uint16_t l=get16(f.payload);if(l<=64U&&2U+l<=f.payload_len){NetEvent ev={.type=NET_EV_FILE_REJECTED};memcpy(ev.transfer_id,f.payload+2,l);ev.transfer_id[l]='\0';size_t off=2U+l;if(off+2U<=f.payload_len){uint16_t rl=get16(f.payload+off);off+=2U;if(rl<sizeof(ev.reason)&&off+rl<=f.payload_len){memcpy(ev.reason,f.payload+off,rl);ev.reason[rl]='\0';}}copy_text(ev.username,sizeof(ev.username),p->username);emit(n,&ev);}}break;}
            case FRAME_FILE_CHUNK:{if(f.payload_len<2U+8U+4U){proto_frame_free(&f);p->alive=false;break;}uint16_t l=get16(f.payload);if(l>64U||2U+l+8U+4U>f.payload_len){proto_frame_free(&f);p->alive=false;break;}NetEvent ev={.type=NET_EV_FILE_CHUNK};memcpy(ev.transfer_id,f.payload+2,l);ev.transfer_id[l]='\0';ev.offset=get64(f.payload+2+l);uint32_t dl=get32(f.payload+2+l+8U);size_t off=2U+l+12U;if(dl>BALANCE_FILE_CHUNK||off+(size_t)dl!=f.payload_len){proto_frame_free(&f);p->alive=false;break;}ev.data=malloc(dl);if(!ev.data){proto_frame_free(&f);p->alive=false;break;}memcpy(ev.data,f.payload+off,dl);ev.data_len=dl;copy_text(ev.username,sizeof(ev.username),p->username);emit(n,&ev);free(ev.data);break;}
            case FRAME_FILE_DONE:{if(f.payload_len>=2U){uint16_t l=get16(f.payload);if(l<=64U&&2U+l<=f.payload_len){NetEvent ev={.type=NET_EV_FILE_DONE};memcpy(ev.transfer_id,f.payload+2,l);ev.transfer_id[l]='\0';copy_text(ev.username,sizeof(ev.username),p->username);emit(n,&ev);}}break;}
            case FRAME_FRIEND_REQUEST:{NetEvent ev={.type=NET_EV_FRIEND_REQUEST};copy_text(ev.username,sizeof(ev.username),p->username);copy_text(ev.endpoint,sizeof(ev.endpoint),p->endpoint);memcpy(ev.ed_pub,p->ed_pub,32U);emit(n,&ev);break;}
            case FRAME_FRIEND_ACCEPT:{NetEvent ev={.type=NET_EV_FRIEND_ACCEPTED};copy_text(ev.username,sizeof(ev.username),p->username);copy_text(ev.endpoint,sizeof(ev.endpoint),p->endpoint);memcpy(ev.ed_pub,p->ed_pub,32U);emit(n,&ev);break;}
            case FRAME_FRIEND_REJECT:{NetEvent ev={.type=NET_EV_FRIEND_REJECTED};copy_text(ev.username,sizeof(ev.username),p->username);emit(n,&ev);break;}
            case FRAME_PING:break;
            default:p->alive=false;break;
        }
        proto_frame_free(&f);if(!p->alive)break;
    }
    p->alive=false;shutdown(p->fd,SHUT_RDWR);close(p->fd);p->fd=-1;NetEvent ev={.type=NET_EV_DISCONNECTED};copy_text(ev.username,sizeof(ev.username),p->username);copy_text(ev.tunnel_id,sizeof(ev.tunnel_id),p->tunnel_id);emit(n,&ev);return NULL;
}

static int establish(NetContext*n,int fd,bool incoming,bool require_pinned,PeerConn**out){
    EVP_PKEY *eph=crypto_x25519_generate();if(!eph)return BALANCE_ERR;uint8_t *hb=NULL;size_t hn=0;if(build_hello(n->vault,eph,&hb,&hn)!=BALANCE_OK){EVP_PKEY_free(eph);return BALANCE_ERR;}uint8_t *frame=NULL;size_t fn=0;if(proto_build_plain(FRAME_HELLO,hb,hn,&frame,&fn)!=BALANCE_OK){free(hb);EVP_PKEY_free(eph);return BALANCE_ERR;}if(send_frame(fd,frame,fn)!=BALANCE_OK){free(frame);free(hb);EVP_PKEY_free(eph);return BALANCE_ERR;}free(frame);free(hb);uint8_t*rb=NULL;size_t rn=0;if(recv_frame(fd,&rb,&rn)!=BALANCE_OK){EVP_PKEY_free(eph);return BALANCE_ERR;}uint8_t rt;const uint8_t*rp;size_t rpl;if(proto_parse_plain(rb,rn,&rt,&rp,&rpl)!=BALANCE_OK||rt!=FRAME_HELLO){free(rb);EVP_PKEY_free(eph);return BALANCE_ERR;}Hello peerh;if(parse_hello(rp,rpl,&peerh)!=BALANCE_OK||verify_hello(&peerh)!=BALANCE_OK){free(rb);EVP_PKEY_free(eph);return BALANCE_ERR;}free(rb);
    if(require_pinned){Friend*f=find_friend(n->vault,peerh.username);if(!f||!f->key_pinned||CRYPTO_memcmp(f->ed_pub,peerh.ed_pub,32U)!=0){EVP_PKEY_free(eph);return BALANCE_ERR;}}
    uint8_t shared[32],k1[32],k2[32],n1[4],n2[4];if(crypto_x25519_derive(eph,peerh.eph_pub,shared)!=BALANCE_OK){EVP_PKEY_free(eph);return BALANCE_ERR;}
    /* Build the real local hello so transcript-derived keys include both identities. */
    uint8_t *lhb=NULL;size_t lhn=0;if(build_hello(n->vault,eph,&lhb,&lhn)!=BALANCE_OK){EVP_PKEY_free(eph);return BALANCE_ERR;}Hello localh;if(parse_hello(lhb,lhn,&localh)!=BALANCE_OK){free(lhb);EVP_PKEY_free(eph);return BALANCE_ERR;}free(lhb);
    if(derive_session(&localh,&peerh,shared,k1,k2,n1,n2)!=BALANCE_OK){EVP_PKEY_free(eph);return BALANCE_ERR;}
    PeerConn*p=calloc(1,sizeof(*p));if(!p){EVP_PKEY_free(eph);return BALANCE_ERR;}p->fd=fd;p->alive=true;p->incoming=incoming;p->net=n;pthread_mutex_init(&p->send_mu,NULL);copy_text(p->username,sizeof(p->username),peerh.username);memcpy(p->ed_pub,peerh.ed_pub,32U);p->tx_seq=0;p->rx_seq=0;
    bool local_low=memcmp(localh.ed_pub,peerh.ed_pub,32U)<0;const uint8_t*txk=local_low?k1:k2;const uint8_t*txn=local_low?n1:n2;const uint8_t*rxn=local_low?n2:n1;memcpy(p->key,txk,32U); /* single key is direction-specific; store send key only, receive key folded below is not supported by this struct */
    memcpy(p->tx_nonce,txn,4U);memcpy(p->rx_nonce,rxn,4U);memcpy(p->key,txk,32U);
    /* To avoid asymmetric key storage changes, k1/k2 are combined deterministically into one AEAD key; nonce prefixes provide direction separation. */
    uint8_t comb[64];memcpy(comb,k1,32U);memcpy(comb+32,k2,32U);uint8_t finalkey[32];crypto_sha256(comb,64U,finalkey);memcpy(p->key,finalkey,32U);secure_zero(shared,32U);secure_zero(comb,64U);secure_zero(finalkey,32U);EVP_PKEY_free(eph);*out=p;return BALANCE_OK;
}

static int connect_endpoint(const char*endpoint){char tmp[BALANCE_MAX_ENDPOINT];if(strlen(endpoint)>=sizeof(tmp))return -1;strcpy(tmp,endpoint);char*colon=strrchr(tmp,':');if(!colon)return -1;*colon='\0';const char*host=tmp;const char*port=colon+1;struct addrinfo hints={0},*res=NULL;hints.ai_socktype=SOCK_STREAM;hints.ai_family=AF_UNSPEC;if(getaddrinfo(host,port,&hints,&res)!=0)return -1;int fd=-1;for(struct addrinfo*r=res;r;r=r->ai_next){fd=socket(r->ai_family,r->ai_socktype,r->ai_protocol);if(fd<0)continue;struct timeval tv={5,0};setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(tv));if(connect(fd,r->ai_addr,r->ai_addrlen)==0)break;close(fd);fd=-1;}freeaddrinfo(res);if(fd>=0){int one=1;setsockopt(fd,SOL_SOCKET,SO_KEEPALIVE,&one,sizeof(one));setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,&one,sizeof(one));}return fd;}

static int track_worker(NetContext*n,pthread_t t){pthread_mutex_lock(&n->workers_mu);if(n->worker_count>=sizeof(n->workers)/sizeof(n->workers[0])){pthread_mutex_unlock(&n->workers_mu);return BALANCE_ERR;}n->workers[n->worker_count++]=t;pthread_mutex_unlock(&n->workers_mu);return BALANCE_OK;}

static void *listener_thread(void*arg){NetContext*n=arg;while(!n->stop){struct sockaddr_storage ss;socklen_t sl=sizeof(ss);int fd=accept(n->listen_fd,(struct sockaddr*)&ss,&sl);if(fd<0){if(errno==EINTR)continue;if(n->stop)break;struct timespec ts={0,10000000L};nanosleep(&ts,NULL);continue;}char host[128]={0},svc[32]={0};(void)getnameinfo((struct sockaddr*)&ss,sl,host,sizeof(host),svc,sizeof(svc),NI_NUMERICHOST|NI_NUMERICSERV);PeerConn*p=NULL;if(establish(n,fd,true,false,&p)!=BALANCE_OK){close(fd);continue;}if(p){snprintf(p->endpoint,sizeof(p->endpoint),"%s:%s",host,svc);peer_add(n,p);if(pthread_create(&p->thread,NULL,peer_thread,p)==0)p->thread_started=true;else{p->alive=false;close(p->fd);p->fd=-1;}} }
 return NULL;}

int net_init(NetContext **out,Vault*v,NetEventFn cb,void*opaque,uint16_t listen_port){if(!out||!v||!cb)return BALANCE_ERR;NetContext*n=calloc(1,sizeof(*n));if(!n)return BALANCE_ERR;n->vault=v;n->cb=cb;n->opaque=opaque;n->listen_fd=socket(AF_INET,SOCK_STREAM,0);if(n->listen_fd<0){free(n);return BALANCE_ERR;}int one=1;setsockopt(n->listen_fd,SOL_SOCKET,SO_REUSEADDR,&one,sizeof(one));struct sockaddr_in a={0};a.sin_family=AF_INET;a.sin_addr.s_addr=htonl(INADDR_ANY);a.sin_port=htons(listen_port);if(bind(n->listen_fd,(struct sockaddr*)&a,sizeof(a))!=0||listen(n->listen_fd,16)!=0){close(n->listen_fd);free(n);return BALANCE_ERR;}struct sockaddr_in actual={0};socklen_t al=sizeof(actual);getsockname(n->listen_fd,(struct sockaddr*)&actual,&al);n->listen_port=ntohs(actual.sin_port);pthread_mutex_init(&n->peers_mu,NULL);pthread_mutex_init(&n->workers_mu,NULL);pthread_mutex_init(&n->stats_mu,NULL);if(pthread_create(&n->listener,NULL,listener_thread,n)!=0){close(n->listen_fd);pthread_mutex_destroy(&n->workers_mu);pthread_mutex_destroy(&n->peers_mu);pthread_mutex_destroy(&n->stats_mu);free(n);return BALANCE_ERR;}return *out=n,BALANCE_OK;}

void net_stop(NetContext*n){
    if(!n)return;
    n->stop=true;
    shutdown(n->listen_fd,SHUT_RDWR);
    close(n->listen_fd);
    pthread_join(n->listener,NULL);
    pthread_mutex_lock(&n->workers_mu);
    size_t wc=n->worker_count;
    pthread_t workers[64];
    memcpy(workers,n->workers,wc*sizeof(*workers));
    pthread_mutex_unlock(&n->workers_mu);
    for(size_t i=0;i<wc;++i)pthread_join(workers[i],NULL);
    pthread_mutex_lock(&n->peers_mu);
    for(PeerConn*p=n->peers;p;p=p->next){p->alive=false;if(p->fd>=0)shutdown(p->fd,SHUT_RDWR);}
    pthread_mutex_unlock(&n->peers_mu);
    for(PeerConn*p=n->peers;p;p=p->next){if(p->thread_started)pthread_join(p->thread,NULL);else if(p->fd>=0)close(p->fd);}
    PeerConn *p2=n->peers;
    while(p2){PeerConn*next=p2->next;pthread_mutex_destroy(&p2->send_mu);free(p2);p2=next;}
    pthread_mutex_destroy(&n->workers_mu);
    pthread_mutex_destroy(&n->peers_mu);
    pthread_mutex_destroy(&n->stats_mu);
    free(n);
}
uint16_t net_listen_port(const NetContext*n){return n?n->listen_port:0;}

typedef struct { NetContext *n; Friend f; char tunnel_id[BALANCE_MAX_TUNNEL]; } TunnelArgs;
static void *tunnel_worker(void *arg){
    TunnelArgs *a=arg; NetContext*n=a->n;
    NetEvent sev={.type=NET_EV_STATUS}; snprintf(sev.text,sizeof(sev.text),"connect [--------------------] 0%%"); emit(n,&sev);
    int fd=connect_endpoint(a->f.endpoint);
    if(fd<0){snprintf(sev.text,sizeof(sev.text),"connect [##------------------] 0%% · %s offline",a->f.username);emit(n,&sev);free(a);return NULL;}
    snprintf(sev.text,sizeof(sev.text),"connect [##########----------] 50%% · authenticated handshake");emit(n,&sev);
    PeerConn*p=NULL;
    if(establish(n,fd,false,true,&p)!=BALANCE_OK){close(fd);snprintf(sev.text,sizeof(sev.text),"connect [################----] 80%% · identity verification failed");emit(n,&sev);free(a);return NULL;}
    p->incoming=false;snprintf(p->endpoint,sizeof(p->endpoint),"%s",a->f.endpoint);snprintf(p->tunnel_id,sizeof(p->tunnel_id),"%s",a->tunnel_id);peer_add(n,p);
    uint8_t extra[2+BALANCE_MAX_USERNAME];size_t un=strlen(n->vault->username);put16(extra,(uint16_t)un);memcpy(extra+2,n->vault->username,un);uint8_t*payload=NULL;size_t pl=0;
    if(payload_tunnel(a->tunnel_id,extra,2U+un,&payload,&pl)!=BALANCE_OK){p->alive=false;free(a);return NULL;}
    int rc=send_enc(p,FRAME_TUNNEL_REQUEST,payload,pl);free(payload);
    if(rc==BALANCE_OK){snprintf(sev.text,sizeof(sev.text),"connect [####################] 100%% · waiting for acceptance");emit(n,&sev);if(pthread_create(&p->thread,NULL,peer_thread,p)==0)p->thread_started=true;else{p->alive=false;close(p->fd);p->fd=-1;}}else{p->alive=false;}
    free(a);return NULL;
}
int net_tunnel(NetContext*n,const Friend*f,const char*tunnel_id){if(!n||!f||!tunnel_id)return BALANCE_ERR;TunnelArgs*a=calloc(1,sizeof(*a));if(!a)return BALANCE_ERR;a->n=n;a->f=*f;snprintf(a->tunnel_id,sizeof(a->tunnel_id),"%s",tunnel_id);pthread_t t;if(pthread_create(&t,NULL,tunnel_worker,a)!=0){free(a);return BALANCE_ERR;}if(track_worker(n,t)!=BALANCE_OK){pthread_join(t,NULL);return BALANCE_ERR;}return BALANCE_OK;}

int net_accept_tunnel(NetContext*n,const char*tunnel_id){if(!n||!tunnel_id)return BALANCE_ERR;pthread_mutex_lock(&n->peers_mu);PeerConn*p=find_peer(n,tunnel_id);pthread_mutex_unlock(&n->peers_mu);if(!p)return BALANCE_ERR;uint8_t*payload=NULL;size_t pl=0;if(payload_tunnel(tunnel_id,NULL,0U,&payload,&pl)!=BALANCE_OK)return BALANCE_ERR;p->tunnel_active=true;int rc=send_enc(p,FRAME_TUNNEL_ACCEPT,payload,pl);free(payload);return rc;}
int net_reject_tunnel(NetContext*n,const char*tunnel_id,const char*reason){if(!n||!tunnel_id)return BALANCE_ERR;pthread_mutex_lock(&n->peers_mu);PeerConn*p=find_peer(n,tunnel_id);pthread_mutex_unlock(&n->peers_mu);if(!p)return BALANCE_ERR;size_t rl=reason?strlen(reason):0;if(rl>200U)rl=200U;uint8_t ex[2+200];put16(ex,(uint16_t)rl);if(rl)memcpy(ex+2,reason,rl);uint8_t*payload=NULL;size_t pl=0;if(payload_tunnel(tunnel_id,ex,2U+rl,&payload,&pl)!=BALANCE_OK)return BALANCE_ERR;int rc=send_enc(p,FRAME_TUNNEL_REJECT,payload,pl);free(payload);p->alive=false;return rc;}

int net_send_chat(NetContext*n,const char*tunnel_id,const char*text){if(!n||!tunnel_id||!text)return BALANCE_ERR;pthread_mutex_lock(&n->peers_mu);PeerConn*p=find_peer(n,tunnel_id);pthread_mutex_unlock(&n->peers_mu);if(!p)return BALANCE_ERR;size_t tl=strlen(text);if(tl>=BALANCE_MAX_MESSAGE)return BALANCE_ERR;uint8_t*payload=NULL;size_t pl=0;if(payload_tunnel(tunnel_id,(const uint8_t*)text,tl,&payload,&pl)!=BALANCE_OK)return BALANCE_ERR;int rc=send_enc(p,FRAME_CHAT,payload,pl);free(payload);return rc;}

int net_send_file_request(NetContext*n,const char*tunnel_id,const char*transfer_id,const char*filename,uint64_t size,const uint8_t digest[32]){if(!n||!tunnel_id||!transfer_id||!filename||!digest||strlen(transfer_id)>64U||strlen(filename)>=256U)return BALANCE_ERR;pthread_mutex_lock(&n->peers_mu);PeerConn*p=find_peer(n,tunnel_id);pthread_mutex_unlock(&n->peers_mu);if(!p)return BALANCE_ERR;uint8_t ex[2+64+2+255+8+32];size_t x=0;size_t il=strlen(transfer_id),fl=strlen(filename);put16(ex+x,(uint16_t)il);x+=2;memcpy(ex+x,transfer_id,il);x+=il;put16(ex+x,(uint16_t)fl);x+=2;memcpy(ex+x,filename,fl);x+=fl;put64(ex+x,size);x+=8;memcpy(ex+x,digest,32);x+=32;uint8_t*payload=NULL;size_t pl=0;if(payload_tunnel(tunnel_id,ex,x,&payload,&pl)!=BALANCE_OK)return BALANCE_ERR;int rc=send_enc(p,FRAME_FILE_REQUEST,payload,pl);free(payload);if(rc==BALANCE_OK)remember_transfer(p,transfer_id);return rc;}

static PeerConn *find_by_transfer(NetContext*n,const char*transfer_id){
    pthread_mutex_lock(&n->peers_mu);
    for(PeerConn*p=n->peers;p;p=p->next){
        if(!p->alive)continue;
        for(size_t i=0;i<p->transfer_count;++i){if(strcmp(p->transfers[i],transfer_id)==0){pthread_mutex_unlock(&n->peers_mu);return p;}}
    }
    pthread_mutex_unlock(&n->peers_mu);
    return NULL;
}
static void remember_transfer(PeerConn*p,const char*transfer_id){
    if(!p||!transfer_id)return;
    for(size_t i=0;i<p->transfer_count;++i)if(strcmp(p->transfers[i],transfer_id)==0)return;
    if(p->transfer_count<16U){snprintf(p->transfers[p->transfer_count],sizeof(p->transfers[p->transfer_count]),"%s",transfer_id);p->transfer_count++;}
}
int net_accept_file(NetContext*n,const char*transfer_id){if(!n||!transfer_id)return BALANCE_ERR;PeerConn*p=find_by_transfer(n,transfer_id);if(!p)return BALANCE_ERR;size_t l=strlen(transfer_id);uint8_t ex[66];put16(ex,(uint16_t)l);memcpy(ex+2,transfer_id,l);return send_enc(p,FRAME_FILE_ACCEPT,ex,2U+l);}
int net_reject_file(NetContext*n,const char*transfer_id,const char*reason){if(!n||!transfer_id)return BALANCE_ERR;PeerConn*p=find_by_transfer(n,transfer_id);if(!p)return BALANCE_ERR;size_t l=strlen(transfer_id),rl=reason?strlen(reason):0;if(l>64U||rl>200U)return BALANCE_ERR;uint8_t ex[268];put16(ex,(uint16_t)l);memcpy(ex+2,transfer_id,l);put16(ex+2+l,(uint16_t)rl);if(rl)memcpy(ex+4+l,reason,rl);return send_enc(p,FRAME_FILE_REJECT,ex,4U+l+rl);}
int net_send_file_chunk(NetContext*n,const char*transfer_id,uint64_t offset,const uint8_t*data,size_t len){if(!n||!transfer_id||!data||len>BALANCE_FILE_CHUNK||strlen(transfer_id)>64U)return BALANCE_ERR;PeerConn*p=find_by_transfer(n,transfer_id);if(!p)return BALANCE_ERR;size_t l=strlen(transfer_id);uint8_t*ex=malloc(2U+l+8U+4U+len);if(!ex)return BALANCE_ERR;put16(ex,l);memcpy(ex+2,transfer_id,l);put64(ex+2+l,offset);put32(ex+10+l,(uint32_t)len);memcpy(ex+14+l,data,len);int rc=send_enc(p,FRAME_FILE_CHUNK,ex,14U+l+len);free(ex);return rc;}
int net_send_file_done(NetContext*n,const char*transfer_id){if(!n||!transfer_id)return BALANCE_ERR;PeerConn*p=find_by_transfer(n,transfer_id);if(!p)return BALANCE_ERR;size_t l=strlen(transfer_id);uint8_t ex[66];put16(ex,l);memcpy(ex+2,transfer_id,l);return send_enc(p,FRAME_FILE_DONE,ex,2U+l);}

static PeerConn *find_peer_user(NetContext*n,const char*user){PeerConn*p=n->peers;while(p){if(p->alive&&strcmp(p->username,user)==0)return p;p=p->next;}return NULL;}
int net_request_friend(NetContext*n,const Friend*f){if(!n||!f)return BALANCE_ERR;int fd=connect_endpoint(f->endpoint);if(fd<0)return BALANCE_ERR;PeerConn*p=NULL;if(establish(n,fd,false,false,&p)!=BALANCE_OK){close(fd);return BALANCE_ERR;}peer_add(n,p);int rc=send_enc(p,FRAME_FRIEND_REQUEST,NULL,0U);if(pthread_create(&p->thread,NULL,peer_thread,p)==0)p->thread_started=true;else{p->alive=false;close(p->fd);p->fd=-1;}return rc;}
int net_accept_friend(NetContext*n,const char*username){if(!n||!username)return BALANCE_ERR;PeerConn*p=find_peer_user(n,username);if(!p)return BALANCE_ERR;return send_enc(p,FRAME_FRIEND_ACCEPT,NULL,0U);}
int net_reject_friend(NetContext*n,const char*username){if(!n||!username)return BALANCE_ERR;PeerConn*p=find_peer_user(n,username);if(!p)return BALANCE_ERR;int rc=send_enc(p,FRAME_FRIEND_REJECT,NULL,0U);p->alive=false;return rc;}
int net_online_probe(NetContext*n,const Friend*f){if(!n||!f)return BALANCE_ERR;int fd=connect_endpoint(f->endpoint);if(fd<0)return BALANCE_ERR;PeerConn*p=NULL;int rc=establish(n,fd,false,true,&p);if(rc==BALANCE_OK){p->alive=false;close(p->fd);pthread_mutex_destroy(&p->send_mu);free(p);}else close(fd);return rc;}
int net_find_tunnel(NetContext*n,const char*tunnel_id,char*peer,size_t peer_sz){if(!n||!tunnel_id||!peer)return BALANCE_ERR;pthread_mutex_lock(&n->peers_mu);PeerConn*p=find_peer(n,tunnel_id);if(p)snprintf(peer,peer_sz,"%s",p->username);pthread_mutex_unlock(&n->peers_mu);return p?BALANCE_OK:BALANCE_ERR;}


int net_close_tunnel(NetContext*n,const char*tunnel_id){
    if(!n||!tunnel_id||!*tunnel_id) return BALANCE_ERR;
    pthread_mutex_lock(&n->peers_mu);
    PeerConn*p=find_peer(n,tunnel_id);
    if(!p){pthread_mutex_unlock(&n->peers_mu);return BALANCE_ERR;}
    p->alive=false;
    if(p->fd>=0) shutdown(p->fd,SHUT_RDWR);
    pthread_mutex_unlock(&n->peers_mu);
    return BALANCE_OK;
}

int net_get_stats(NetContext*n,NetStats*out){
    if(!n||!out) return BALANCE_ERR;
    uint64_t tx=atomic_load(&n->tx_bytes), rx=atomic_load(&n->rx_bytes);
    uint64_t txf=atomic_load(&n->tx_frames), rxf=atomic_load(&n->rx_frames);
    uint64_t now=now_millis();
    pthread_mutex_lock(&n->stats_mu);
    if(n->stats_last_ms==0){n->stats_last_ms=now;n->stats_last_tx_bytes=tx;n->stats_last_rx_bytes=rx;}
    uint64_t elapsed=now>n->stats_last_ms?now-n->stats_last_ms:0ULL;
    uint64_t txbps=0,rxbps=0;
    if(elapsed>=250ULL){txbps=((tx-n->stats_last_tx_bytes)*1000ULL)/elapsed;rxbps=((rx-n->stats_last_rx_bytes)*1000ULL)/elapsed;n->stats_last_ms=now;n->stats_last_tx_bytes=tx;n->stats_last_rx_bytes=rx;}
    pthread_mutex_unlock(&n->stats_mu);
    uint64_t peers=0,tunnels=0;
    pthread_mutex_lock(&n->peers_mu);
    for(PeerConn*p=n->peers;p;p=p->next){if(p->alive){peers++;if(p->tunnel_active)tunnels++;}}
    pthread_mutex_unlock(&n->peers_mu);
    memset(out,0,sizeof(*out));out->tx_bytes=tx;out->rx_bytes=rx;out->tx_frames=txf;out->rx_frames=rxf;out->tx_bytes_per_sec=txbps;out->rx_bytes_per_sec=rxbps;out->active_peers=peers;out->active_tunnels=tunnels;
    return BALANCE_OK;
}
