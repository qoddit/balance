#if defined(__APPLE__)
#ifndef _DARWIN_C_SOURCE
#define _DARWIN_C_SOURCE 1
#endif
#endif
#include "store.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

static int ensure_dir(const char *path) {
    if (mkdir(path, 0700) == 0) return BALANCE_OK;
    if (errno == EEXIST) return BALANCE_OK;
    return BALANCE_ERR;
}

static int write_all(int fd, const void *buf, size_t n) {
    const uint8_t *p=(const uint8_t *)buf;
    while(n){ ssize_t w=write(fd,p,n); if(w<=0)return BALANCE_ERR; p+=(size_t)w; n-=(size_t)w; }
    return BALANCE_OK;
}
static int read_all(int fd, void *buf, size_t n) {
    uint8_t *p=(uint8_t *)buf;
    while(n){ ssize_t r=read(fd,p,n); if(r<=0)return BALANCE_ERR; p+=(size_t)r; n-=(size_t)r; }
    return BALANCE_OK;
}
static void put16(uint8_t*p,uint16_t x){p[0]=(uint8_t)(x>>8);p[1]=(uint8_t)x;}
static uint16_t get16(const uint8_t*p){return (uint16_t)(((uint16_t)p[0]<<8)|p[1]);}
static void put32(uint8_t*p,uint32_t x){p[0]=(uint8_t)(x>>24);p[1]=(uint8_t)(x>>16);p[2]=(uint8_t)(x>>8);p[3]=(uint8_t)x;}
static uint32_t get32(const uint8_t*p){return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];}
static void put64(uint8_t*p,uint64_t x){for(int i=0;i<8;++i)p[7-i]=(uint8_t)(x>>(i*8));}
static uint64_t get64(const uint8_t*p){uint64_t x=0;for(int i=0;i<8;++i)x=(x<<8)|p[i];return x;}

int store_init(Store *s){
    if(!s) return BALANCE_ERR;
    memset(s,0,sizeof(*s));
    const char *home=getenv("HOME"); if(!home) return BALANCE_ERR;
#if defined(__APPLE__)
    char base[BALANCE_MAX_PATH], app[BALANCE_MAX_PATH];
    if(snprintf(base,sizeof(base),"%s/Library",home)<=0) return BALANCE_ERR;
    if(ensure_dir(base)!=BALANCE_OK) return BALANCE_ERR;
    if(snprintf(app,sizeof(app),"%s/Application Support",base)<=0) return BALANCE_ERR;
    if(ensure_dir(app)!=BALANCE_OK) return BALANCE_ERR;
    if(snprintf(s->root,sizeof(s->root),"%s/balance",app)<=0) return BALANCE_ERR;
#else
    char base[BALANCE_MAX_PATH], share[BALANCE_MAX_PATH];
    if(snprintf(base,sizeof(base),"%s/.local",home)<=0) return BALANCE_ERR;
    if(ensure_dir(base)!=BALANCE_OK) return BALANCE_ERR;
    if(snprintf(share,sizeof(share),"%s/share",base)<=0) return BALANCE_ERR;
    if(ensure_dir(share)!=BALANCE_OK) return BALANCE_ERR;
    if(snprintf(s->root,sizeof(s->root),"%s/balance",share)<=0) return BALANCE_ERR;
#endif
    if(ensure_dir(s->root)!=BALANCE_OK) return BALANCE_ERR;
    (void)snprintf(s->vault_path,sizeof(s->vault_path),"%s/vault.enc",s->root);
    (void)snprintf(s->history_path,sizeof(s->history_path),"%s/history.enc",s->root);
    chmod(s->root,0700);
    return BALANCE_OK;
}

int store_exists(const Store *s){ if(!s)return 0; return access(s->vault_path,F_OK)==0; }

static int vault_plain_encode(const Vault *v,uint8_t **out,size_t *out_len){
    if(!v||!out||!out_len||strlen(v->username)>=BALANCE_MAX_USERNAME||strlen(v->endpoint)>=BALANCE_MAX_ENDPOINT||v->friend_count>MAX_FRIENDS)return BALANCE_ERR;
    uint8_t *buf=NULL; size_t cap=1024U, len=0; buf=malloc(cap); if(!buf)return BALANCE_ERR;
#define NEED(N) do { size_t need=(N); if(len+need>cap){size_t nc=cap; while(nc<len+need)nc*=2U; uint8_t *nb=realloc(buf,nc); if(!nb){free(buf);return BALANCE_ERR;} buf=nb;cap=nc;} }while(0)
#define APPBYTES(P,N) do{NEED(N);memcpy(buf+len,(P),(N));len+=(N);}while(0)
#define APP16(V) do{uint8_t t[2];put16(t,(uint16_t)(V));APPBYTES(t,2U);}while(0)
#define APP32(V) do{uint8_t t[4];put32(t,(uint32_t)(V));APPBYTES(t,4U);}while(0)
    APP32(2U); APP16(strlen(v->username)); APPBYTES(v->username,strlen(v->username));
    APP16(strlen(v->endpoint)); APPBYTES(v->endpoint,strlen(v->endpoint)); APPBYTES(v->ed_pub,32U);
    uint8_t *priv=NULL; size_t priv_len=0; if(crypto_pkey_private_der(v->ed_private,&priv,&priv_len)!=BALANCE_OK){free(buf);return BALANCE_ERR;}
    if(priv_len>UINT32_MAX){OPENSSL_free(priv);free(buf);return BALANCE_ERR;} APP32(priv_len); APPBYTES(priv,priv_len); OPENSSL_free(priv);
    APP16((uint16_t)v->friend_count);
    for(size_t i=0;i<v->friend_count;++i){const Friend*f=&v->friends[i]; APP16(strlen(f->username));APPBYTES(f->username,strlen(f->username));APP16(strlen(f->endpoint));APPBYTES(f->endpoint,strlen(f->endpoint));APPBYTES(f->ed_pub,32U);uint8_t p=(uint8_t)(f->key_pinned?1:0);APPBYTES(&p,1U);}
    uint8_t c=(uint8_t)(v->cloud_enabled?1:0);APPBYTES(&c,1U);APP16(strlen(v->cloud_url));APPBYTES(v->cloud_url,strlen(v->cloud_url));APP16(strlen(v->cloud_token));APPBYTES(v->cloud_token,strlen(v->cloud_token)); APP16(strlen(v->cloud_object_id)); APPBYTES(v->cloud_object_id,strlen(v->cloud_object_id));
#undef NEED
#undef APPBYTES
#undef APP16
#undef APP32
    *out=buf;*out_len=len;return BALANCE_OK;
}

static int get_bytes(const uint8_t **p,size_t *rem,void *out,size_t n){if(*rem<n)return BALANCE_ERR;memcpy(out,*p,n);*p+=n;*rem-=n;return BALANCE_OK;}
static int get_string(const uint8_t **p,size_t *rem,char *out,size_t out_sz){if(*rem<2U)return BALANCE_ERR;uint16_t n=get16(*p);*p+=2;*rem-=2;if((size_t)n>=out_sz||*rem<(size_t)n)return BALANCE_ERR;memcpy(out,*p,n);out[n]='\0';*p+=n;*rem-=n;return BALANCE_OK;}

static int vault_plain_decode(const uint8_t *p, size_t n, Vault *v) {
    memset(v, 0, sizeof(*v));
    if (n < 6U) return BALANCE_ERR;
    size_t rem = n;
    uint32_t version = get32(p); p += 4U; rem -= 4U;
    if (version != 2U) return BALANCE_ERR;
    if (get_string(&p, &rem, v->username, sizeof(v->username)) != BALANCE_OK ||
        get_string(&p, &rem, v->endpoint, sizeof(v->endpoint)) != BALANCE_OK ||
        get_bytes(&p, &rem, v->ed_pub, 32U) != BALANCE_OK) return BALANCE_ERR;
    if (rem < 4U) return BALANCE_ERR;
    uint32_t priv_len = get32(p); p += 4U; rem -= 4U;
    if (priv_len > rem || priv_len > 65536U) return BALANCE_ERR;
    v->ed_private = crypto_pkey_from_private_der(p, priv_len);
    if (!v->ed_private) return BALANCE_ERR;
    p += priv_len; rem -= priv_len;
    if (rem < 2U) return BALANCE_ERR;
    uint16_t fc = get16(p); p += 2U; rem -= 2U;
    if (fc > MAX_FRIENDS) return BALANCE_ERR;
    v->friend_count = fc;
    for (size_t i = 0; i < v->friend_count; ++i) {
        Friend *f = &v->friends[i];
        if (get_string(&p, &rem, f->username, sizeof(f->username)) != BALANCE_OK ||
            get_string(&p, &rem, f->endpoint, sizeof(f->endpoint)) != BALANCE_OK ||
            get_bytes(&p, &rem, f->ed_pub, 32U) != BALANCE_OK) { vault_free(v); return BALANCE_ERR; }
        if (rem < 1U) { vault_free(v); return BALANCE_ERR; }
        f->key_pinned = p[0] != 0; p++; rem--;
    }
    if (rem < 1U) { vault_free(v); return BALANCE_ERR; }
    v->cloud_enabled = p[0] != 0; p++; rem--;
    if (get_string(&p, &rem, v->cloud_url, sizeof(v->cloud_url)) != BALANCE_OK ||
        get_string(&p, &rem, v->cloud_token, sizeof(v->cloud_token)) != BALANCE_OK ||
        get_string(&p, &rem, v->cloud_object_id, sizeof(v->cloud_object_id)) != BALANCE_OK) { vault_free(v); return BALANCE_ERR; }
    return BALANCE_OK;
}

static int encrypt_blob(const uint8_t key[32],const char magic[6],const uint8_t *pt,size_t pt_len,const char *path){
    uint8_t iv[12],tag[16];uint8_t *ct=malloc(pt_len);if(!ct)return BALANCE_ERR;uint8_t *data=(uint8_t *)magic;int rc=crypto_aead_encrypt(key,data,6U,pt,pt_len,iv,ct,tag);if(rc!=BALANCE_OK){free(ct);return rc;}
    size_t total=6U+1U+12U+4U+pt_len+16U;uint8_t *file=malloc(total);if(!file){free(ct);return BALANCE_ERR;}memcpy(file,magic,6U);file[6]=1;memcpy(file+7,iv,12U);put32(file+19,(uint32_t)pt_len);memcpy(file+23,ct,pt_len);memcpy(file+23+pt_len,tag,16U);
    char tmp[BALANCE_MAX_PATH];snprintf(tmp,sizeof(tmp),"%s.tmp.%ld",path,(long)getpid());int fd=open(tmp,O_WRONLY|O_CREAT|O_TRUNC,0600);if(fd<0){free(ct);free(file);return BALANCE_ERR;}int ok=write_all(fd,file,total)==BALANCE_OK && fsync(fd)==0;close(fd);if(ok)ok=(rename(tmp,path)==0);else unlink(tmp);chmod(path,0600);free(ct);free(file);return ok?BALANCE_OK:BALANCE_ERR;
}

static int decrypt_blob(const uint8_t key[32], const char magic[6], const char *path, uint8_t **pt, size_t *pt_len) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return BALANCE_ERR;
    struct stat st;
    if (fstat(fd, &st) != 0 || st.st_size < 39 || (uint64_t)st.st_size > 100ULL * 1024ULL * 1024ULL) { close(fd); return BALANCE_ERR; }
    size_t n = (size_t)st.st_size;
    uint8_t *b = malloc(n);
    if (!b) { close(fd); return BALANCE_ERR; }
    int ok = read_all(fd, b, n) == BALANCE_OK;
    close(fd);
    if (!ok || memcmp(b, magic, 6U) != 0 || b[6] != 1U) { free(b); return BALANCE_ERR; }
    uint32_t l = get32(b + 19U);
    if ((size_t)l + 39U != n || l > 90U * 1024U * 1024U) { free(b); return BALANCE_ERR; }
    uint8_t *p = malloc((size_t)l + 1U);
    if (!p) { free(b); return BALANCE_ERR; }
    ok = crypto_aead_decrypt(key, b, 6U, b + 23U, l, b + 7U, b + 23U + l, p) == BALANCE_OK;
    if (!ok) { free(b); free(p); return BALANCE_ERR; }
    free(b); *pt = p; *pt_len = l; return BALANCE_OK;
}

int store_master_key(const Store *s, const char *password, uint8_t key[32], uint8_t salt[16]) {
    if (!s || !password || !key || !salt) return BALANCE_ERR;
    int fd = open(s->vault_path, O_RDONLY);
    if (fd < 0) return BALANCE_ERR;
    uint8_t hdr[23];
    int ok = read_all(fd, hdr, sizeof(hdr)) == BALANCE_OK;
    close(fd);
    if (!ok || memcmp(hdr, "BALV2", 5U) != 0 || hdr[5] != 1U) return BALANCE_ERR;
    memcpy(salt, hdr + 6U, 16U);
    return crypto_scrypt(password, salt, key);
}

int store_register(Store *s,const char *username,const char *password,uint16_t listen_port,Vault *out){
    if(!s||!username||!password||!out||strlen(username)==0||strlen(username)>=BALANCE_MAX_USERNAME||strlen(password)<10U||store_exists(s))return BALANCE_ERR;
    Vault v;memset(&v,0,sizeof(v));strncpy(v.username,username,sizeof(v.username)-1U);snprintf(v.endpoint,sizeof(v.endpoint),"127.0.0.1:%u",(unsigned)listen_port);v.ed_private=crypto_ed25519_generate();if(!v.ed_private)return BALANCE_ERR;if(crypto_ed25519_public(v.ed_private,v.ed_pub)!=BALANCE_OK){vault_free(&v);return BALANCE_ERR;}
    uint8_t salt[16],key[32];if(crypto_random(salt,16U)!=BALANCE_OK||crypto_scrypt(password,salt,key)!=BALANCE_OK){vault_free(&v);return BALANCE_ERR;}uint8_t *plain=NULL;size_t plain_len=0;if(vault_plain_encode(&v,&plain,&plain_len)!=BALANCE_OK){vault_free(&v);secure_zero(key,32U);return BALANCE_ERR;}uint8_t iv[12],tag[16];uint8_t *ct=malloc(plain_len);if(!ct){free(plain);vault_free(&v);secure_zero(key,32U);return BALANCE_ERR;}crypto_random(iv,12U);int rc=crypto_aead_encrypt(key,(const uint8_t *)"BALV2",5U,plain,plain_len,iv,ct,tag);if(rc!=BALANCE_OK){free(plain);free(ct);vault_free(&v);secure_zero(key,32U);return BALANCE_ERR;}
    size_t total=5U+1U+16U+12U+4U+plain_len+16U;uint8_t *file=malloc(total);if(!file){free(plain);free(ct);vault_free(&v);secure_zero(key,32U);return BALANCE_ERR;}memcpy(file,"BALV2",5U);file[5]=1;memcpy(file+6,salt,16U);memcpy(file+22,iv,12U);put32(file+34,(uint32_t)plain_len);memcpy(file+38,ct,plain_len);memcpy(file+38+plain_len,tag,16U);
    int fd=open(s->vault_path,O_WRONLY|O_CREAT|O_EXCL,0600);if(fd<0){free(file);free(plain);free(ct);vault_free(&v);secure_zero(key,32U);return BALANCE_ERR;}int ok=write_all(fd,file,total)==BALANCE_OK&&fsync(fd)==0;close(fd);chmod(s->vault_path,0600);free(file);free(plain);free(ct);secure_zero(key,32U);if(!ok){unlink(s->vault_path);vault_free(&v);return BALANCE_ERR;}*out=v;return BALANCE_OK;
}

int store_login(Store *s,const char *password,Vault *out){
    if(!s||!password||!out||!store_exists(s))return BALANCE_ERR;
    uint8_t key[32],salt[16];
    if(store_master_key(s,password,key,salt)!=BALANCE_OK)return BALANCE_ERR;
    int fd=open(s->vault_path,O_RDONLY);if(fd<0){secure_zero(key,32U);return BALANCE_ERR;}struct stat st;if(fstat(fd,&st)!=0||st.st_size<54){close(fd);secure_zero(key,32U);return BALANCE_ERR;}size_t n=(size_t)st.st_size;uint8_t *b=malloc(n);if(!b){close(fd);secure_zero(key,32U);return BALANCE_ERR;}int ok=read_all(fd,b,n)==BALANCE_OK;close(fd);if(!ok||memcmp(b,"BALV2",5)!=0||b[5]!=1){free(b);secure_zero(key,32U);return BALANCE_ERR;}uint32_t l=get32(b+34);if((size_t)l+54U!=n){free(b);secure_zero(key,32U);return BALANCE_ERR;}uint8_t *plain=malloc(l);if(!plain){free(b);secure_zero(key,32U);return BALANCE_ERR;}ok=crypto_aead_decrypt(key,(const uint8_t *)"BALV2",5U,b+38,l,b+22,b+38+l,plain)==BALANCE_OK;free(b);secure_zero(key,32U);if(!ok){free(plain);return BALANCE_ERR;}ok=vault_plain_decode(plain,l,out)==BALANCE_OK;secure_zero(plain,l);free(plain);if(!ok)return BALANCE_ERR;return BALANCE_OK;
}

void vault_free(Vault *v){if(!v)return;if(v->ed_private)EVP_PKEY_free(v->ed_private);secure_zero(v,sizeof(*v));}

int store_save_vault_key(Store *s,const Vault *v,const uint8_t master_key[32]){
    if(!s||!v||!master_key)return BALANCE_ERR;
    uint8_t *plain=NULL; size_t plain_len=0;
    if(vault_plain_encode(v,&plain,&plain_len)!=BALANCE_OK)return BALANCE_ERR;
    int lockfd=open(s->vault_path,O_RDWR); if(lockfd<0){free(plain);return BALANCE_ERR;}
    flock(lockfd,LOCK_EX);
    uint8_t iv[12],tag[16]; uint8_t *ct=malloc(plain_len); if(!ct){flock(lockfd,LOCK_UN);close(lockfd);free(plain);return BALANCE_ERR;}
    int ok=crypto_random(iv,12U)==BALANCE_OK && crypto_aead_encrypt(master_key,(const uint8_t *)"BALV2",5U,plain,plain_len,iv,ct,tag)==BALANCE_OK;
    size_t total=5U+1U+16U+12U+4U+plain_len+16U; uint8_t *file=NULL;
    if(ok){file=malloc(total);if(!file)ok=0;}
    uint8_t salt[16];
    if(ok){int fd=open(s->vault_path,O_RDONLY);if(fd<0||lseek(fd,6,SEEK_SET)!=(off_t)6||read_all(fd,salt,16U)!=BALANCE_OK)ok=0;if(fd>=0)close(fd);}
    if(ok){memcpy(file,"BALV2",5U);file[5]=1;memcpy(file+6,salt,16U);memcpy(file+22,iv,12U);put32(file+34,(uint32_t)plain_len);memcpy(file+38,ct,plain_len);memcpy(file+38+plain_len,tag,16U);char tmp[BALANCE_MAX_PATH];snprintf(tmp,sizeof(tmp),"%s.tmp.%ld",s->vault_path,(long)getpid());int fd=open(tmp,O_WRONLY|O_CREAT|O_TRUNC,0600);ok=fd>=0&&write_all(fd,file,total)==BALANCE_OK&&fd>=0&&fsync(fd)==0;if(fd>=0)close(fd);if(ok)ok=rename(tmp,s->vault_path)==0;else unlink(tmp);chmod(s->vault_path,0600);}
    free(file);free(ct);flock(lockfd,LOCK_UN);close(lockfd);secure_zero(plain,plain_len);free(plain);return ok?BALANCE_OK:BALANCE_ERR;
}

int store_save_vault(Store *s,const Vault *v,const char *password){
    if(!s||!v||!password)return BALANCE_ERR;
    uint8_t key[32],salt[16];
    if(store_master_key(s,password,key,salt)!=BALANCE_OK)return BALANCE_ERR;
    int rc=store_save_vault_key(s,v,key);secure_zero(key,32U);return rc;
}

static int history_plain_encode(const HistoryEntry *es,size_t n,uint8_t **out,size_t *out_len){
    size_t cap=1024U,len=0;uint8_t*b=malloc(cap);if(!b)return BALANCE_ERR;
#define NEED(N) do{size_t need=(N);if(len+need>cap){size_t nc=cap;while(nc<len+need)nc*=2U;uint8_t*nb=realloc(b,nc);if(!nb){free(b);return BALANCE_ERR;}b=nb;cap=nc;}}while(0)
    NEED(4U);put32(b+len,(uint32_t)n);len+=4U;
    for(size_t i=0;i<n;++i){size_t cl=strlen(es[i].chat),tl=strlen(es[i].text);if(cl>=128U||tl>UINT32_MAX){free(b);return BALANCE_ERR;}NEED(8U+1U+2U+cl+4U+tl);put64(b+len,es[i].ts);len+=8U;b[len++]=(uint8_t)(es[i].local?1:0);put16(b+len,(uint16_t)cl);len+=2U;memcpy(b+len,es[i].chat,cl);len+=cl;put32(b+len,(uint32_t)tl);len+=4U;memcpy(b+len,es[i].text,tl);len+=tl;}
#undef NEED
    *out=b;*out_len=len;return BALANCE_OK;
}

static int history_plain_decode(const uint8_t *p,size_t n,HistoryEntry *out,size_t cap,size_t*count){size_t rem=n;if(rem<4U)return BALANCE_ERR;uint32_t total=get32(p);p+=4;rem-=4;size_t take=total<cap?total:cap;HistoryEntry *tmp=calloc(total?total:1U,sizeof(*tmp));if(!tmp && total)return BALANCE_ERR;for(uint32_t i=0;i<total;++i){if(rem<11U){free(tmp);return BALANCE_ERR;}tmp[i].ts=get64(p);p+=8;tmp[i].local=p[0]!=0;p++;rem--;uint16_t cl=get16(p);p+=2;rem-=2;if(cl>=128U||rem<cl){free(tmp);return BALANCE_ERR;}memcpy(tmp[i].chat,p,cl);tmp[i].chat[cl]='\0';p+=cl;rem-=cl;if(rem<4U){free(tmp);return BALANCE_ERR;}uint32_t tl=get32(p);p+=4;rem-=4;if(tl>=BALANCE_MAX_MESSAGE||rem<tl){free(tmp);return BALANCE_ERR;}memcpy(tmp[i].text,p,tl);tmp[i].text[tl]='\0';p+=tl;rem-=tl;}if(take)memcpy(out,tmp+total-take,take*sizeof(*out));free(tmp);*count=take;return BALANCE_OK;}

int store_append_history(const Store *s,const uint8_t master_key[32],const HistoryEntry *e){
    if(!s||!master_key||!e)return BALANCE_ERR;
    HistoryEntry *arr=calloc(MAX_HISTORY,sizeof(*arr));
    if(!arr)return BALANCE_ERR;
    size_t n=0;
    if(access(s->history_path,F_OK)==0){if(store_load_history(s,master_key,arr,MAX_HISTORY,&n)!=BALANCE_OK){free(arr);return BALANCE_ERR;}}
    if(n<MAX_HISTORY)arr[n++]=*e;else{memmove(arr,arr+1U,(MAX_HISTORY-1U)*sizeof(*arr));arr[MAX_HISTORY-1U]=*e;}
    uint8_t *plain=NULL;size_t plain_len=0;if(history_plain_encode(arr,n,&plain,&plain_len)!=BALANCE_OK){free(arr);return BALANCE_ERR;}int rc=encrypt_blob(master_key,"BALH2",plain,plain_len,s->history_path);secure_zero(plain,plain_len);free(plain);free(arr);return rc;
}

int store_load_history(const Store *s,const uint8_t master_key[32],HistoryEntry *out,size_t cap,size_t *count){if(!s||!master_key||!out||!count)return BALANCE_ERR;if(access(s->history_path,F_OK)!=0){*count=0;return BALANCE_OK;}uint8_t *plain=NULL;size_t n=0;if(decrypt_blob(master_key,"BALH2",s->history_path,&plain,&n)!=BALANCE_OK)return BALANCE_ERR;int rc=history_plain_decode(plain,n,out,cap,count);secure_zero(plain,n);free(plain);return rc;}

int store_cloud_sync(const Vault *v,const Store *s,const uint8_t master_key[32]){(void)v;(void)s;(void)master_key;return BALANCE_OK;}int store_clear_history(const Store *s){
    if(!s) return BALANCE_ERR;
    if(unlink(s->history_path)==0 || errno==ENOENT) return BALANCE_OK;
    return BALANCE_ERR;
}


