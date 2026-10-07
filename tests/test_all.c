#if defined(__APPLE__)
#ifndef _DARWIN_C_SOURCE
#define _DARWIN_C_SOURCE 1
#endif
#endif
#include "common.h"
#include "crypto.h"
#include "proto.h"
#include "store.h"
#include "net.h"
#include "cloud.h"
#include "commands.h"
#include "version.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>


static void test_commands(void){
    size_t count=0; const BalanceCommandSpec *specs=balance_command_specs(&count);
    assert(specs && count>=28U);
    assert(strcmp(BALANCE_VERSION,"beta-0.3.3")==0);
    assert(balance_command_lookup("?")!=NULL);
    assert(balance_command_lookup("help")!=NULL);
    assert(balance_command_lookup("fetch")==NULL);
    assert(balance_command_lookup("dashboard")==NULL);
    assert(balance_command_lookup("logo")==NULL);
    assert(balance_command_lookup("fingerprint")!=NULL);
    assert(balance_command_lookup("xfer")!=NULL);
    assert(balance_command_lookup("does-not-exist")==NULL);
}

static void test_crypto(void){
    uint8_t key[32],key2[32],salt[16],iv[12],tag[16],ct[64],pt[64],out[32];
    assert(crypto_random(salt,16)==0);assert(crypto_scrypt("correct horse battery",salt,key)==0);assert(crypto_scrypt("correct horse battery",salt,key2)==0);assert(memcmp(key,key2,32)==0);
    const char *msg="balance crypto test";assert(crypto_aead_encrypt(key,(const uint8_t*)"aad",3,(const uint8_t*)msg,strlen(msg),iv,ct,tag)==0);assert(crypto_aead_decrypt(key,(const uint8_t*)"aad",3,ct,strlen(msg),iv,tag,pt)==0);assert(memcmp(pt,msg,strlen(msg))==0);tag[0]^=1;assert(crypto_aead_decrypt(key,(const uint8_t*)"aad",3,ct,strlen(msg),iv,tag,pt)!=0);tag[0]^=1;
    EVP_PKEY *ed=crypto_ed25519_generate();assert(ed);assert(crypto_ed25519_public(ed,out)==0);uint8_t sig[64];assert(crypto_ed25519_sign(ed,(const uint8_t*)msg,strlen(msg),sig)==0);assert(crypto_ed25519_verify(ed,(const uint8_t*)msg,strlen(msg),sig)==0);sig[0]^=1;assert(crypto_ed25519_verify(ed,(const uint8_t*)msg,strlen(msg),sig)!=0);sig[0]^=1;
    EVP_PKEY *a=crypto_x25519_generate(),*b=crypto_x25519_generate();assert(a&&b);uint8_t ap[32],bp[32],sa[32],sb[32];assert(crypto_x25519_public(a,ap)==0&&crypto_x25519_public(b,bp)==0);assert(crypto_x25519_derive(a,bp,sa)==0&&crypto_x25519_derive(b,ap,sb)==0);assert(memcmp(sa,sb,32)==0);EVP_PKEY_free(ed);EVP_PKEY_free(a);EVP_PKEY_free(b);secure_zero(key,32);secure_zero(key2,32);secure_zero(sa,32);secure_zero(sb,32);
}

static void test_store(void){
    char tmp[]="/tmp/balance-test-home-XXXXXX"; assert(mkdtemp(tmp)); assert(setenv("HOME",tmp,1)==0);
    Store st; assert(store_init(&st)==0); assert(!store_exists(&st));
    Vault v; memset(&v,0,sizeof(v)); assert(store_register(&st,"alice","a-very-strong-test-password",0,&v)==0); assert(store_exists(&st));
    uint8_t master[32],salt[16]; assert(store_master_key(&st,"a-very-strong-test-password",master,salt)==0);
    assert(store_login(&st,"wrong-password",&v)!=0); vault_free(&v); memset(&v,0,sizeof(v)); assert(store_login(&st,"a-very-strong-test-password",&v)==0); assert(strcmp(v.username,"alice")==0);
    v.friend_count=1; snprintf(v.friends[0].username,sizeof(v.friends[0].username),"bob"); snprintf(v.friends[0].endpoint,sizeof(v.friends[0].endpoint),"127.0.0.1:9999"); memset(v.friends[0].ed_pub,7,32); v.friends[0].key_pinned=true; assert(store_save_vault(&st,&v,"a-very-strong-test-password")==0);
    HistoryEntry e={.ts=1234,.local=true}; snprintf(e.chat,sizeof(e.chat),"alice <-> bob"); snprintf(e.text,sizeof(e.text),"secret message"); assert(store_append_history(&st,master,&e)==0); HistoryEntry out[4];size_t count=0;assert(store_load_history(&st,master,out,4,&count)==0&&count==1);assert(strcmp(out[0].text,"secret message")==0);
    vault_free(&v); secure_zero(master,32); char cmd[512]; snprintf(cmd,sizeof(cmd),"rm -rf %s",tmp); system(cmd);
}

static void test_proto(void){
    uint8_t key[32]={0},np[4]={1,2,3,4};uint8_t *enc=NULL;size_t en=0;const char*msg="hello";assert(proto_encrypt_frame(FRAME_CHAT,7,key,np,NULL,0,(const uint8_t*)msg,strlen(msg),&enc,&en)==0);ProtoFrame f={0};assert(proto_decrypt_frame(key,np,enc,en,NULL,0,&f)==0);assert(f.seq==7&&f.type==FRAME_CHAT&&f.payload_len==strlen(msg)&&memcmp(f.payload,msg,strlen(msg))==0);proto_frame_free(&f);enc[1]^=1;assert(proto_decrypt_frame(key,np,enc,en,NULL,0,&f)!=0);free(enc);
    enc=NULL; en=0; assert(proto_encrypt_frame(FRAME_FRIEND_REQUEST,8,key,np,NULL,0,NULL,0,&enc,&en)==0); assert(proto_decrypt_frame(key,np,enc,en,NULL,0,&f)==0); assert(f.seq==8&&f.type==FRAME_FRIEND_REQUEST&&f.payload_len==0); proto_frame_free(&f); free(enc);
}

typedef struct {
    pthread_mutex_t mu;
    pthread_cond_t cv;
    bool tunnel_request;
    bool tunnel_accepted;
    bool chat_received;
    bool file_request;
    bool file_accepted;
    bool file_done;
    bool friend_request;
    bool friend_accepted;
    char tunnel_id[BALANCE_MAX_TUNNEL];
    char transfer_id[65];
    uint8_t file_buf[100000];
    size_t file_len;
    bool file_bad_order;
} NetTestState;

typedef struct { NetContext *net; NetTestState *state; } NetNode;

static void net_test_cb(const NetEvent *ev, void *opaque) {
    NetNode *node = opaque; NetTestState *st = node->state;
    pthread_mutex_lock(&st->mu);
    if (ev->type == NET_EV_TUNNEL_REQUEST && node->net) {
        snprintf(st->tunnel_id, sizeof(st->tunnel_id), "%s", ev->tunnel_id);
        st->tunnel_request = true;
        pthread_mutex_unlock(&st->mu);
        (void)net_accept_tunnel(node->net, ev->tunnel_id);
        return;
    }
    if (ev->type == NET_EV_TUNNEL_ACCEPTED) st->tunnel_accepted = true;
    else if (ev->type == NET_EV_MESSAGE && strcmp(ev->text, "hello over p2p") == 0) st->chat_received = true;
    else if (ev->type == NET_EV_FILE_REQUEST && node->net) {
        snprintf(st->transfer_id, sizeof(st->transfer_id), "%s", ev->transfer_id);
        st->file_request = true;
        pthread_mutex_unlock(&st->mu);
        (void)net_accept_file(node->net, ev->transfer_id);
        return;
    } else if (ev->type == NET_EV_FILE_ACCEPTED) st->file_accepted = true;
    else if (ev->type == NET_EV_FILE_CHUNK) {
        if (ev->offset != st->file_len || st->file_len + ev->data_len > sizeof(st->file_buf)) st->file_bad_order = true;
        else { memcpy(st->file_buf + st->file_len, ev->data, ev->data_len); st->file_len += ev->data_len; }
    } else if (ev->type == NET_EV_FILE_DONE) st->file_done = true;
    else if (ev->type == NET_EV_FRIEND_REQUEST && node->net) { st->friend_request = true; pthread_mutex_unlock(&st->mu); (void)net_accept_friend(node->net, ev->username); return; }
    else if (ev->type == NET_EV_FRIEND_ACCEPTED) st->friend_accepted = true;
    pthread_cond_broadcast(&st->cv);
    pthread_mutex_unlock(&st->mu);
}

static int wait_flag(NetTestState *st, volatile bool *flag, uint64_t timeout_ms) {
    (void)flag;
    struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += (time_t)(timeout_ms / 1000ULL);
    ts.tv_nsec += (long)((timeout_ms % 1000ULL) * 1000000ULL);
    if (ts.tv_nsec >= 1000000000L) { ts.tv_sec++; ts.tv_nsec -= 1000000000L; }
    return pthread_cond_timedwait(&st->cv, &st->mu, &ts);
}

static bool wait_condition(NetTestState *st, bool (*pred)(const NetTestState *), uint64_t timeout_ms) {
    pthread_mutex_lock(&st->mu);
    uint64_t start = now_millis();
    while (!pred(st)) {
        uint64_t elapsed = now_millis() - start;
        if (elapsed >= timeout_ms) { pthread_mutex_unlock(&st->mu); return false; }
        uint64_t remain = timeout_ms - elapsed;
        (void)wait_flag(st, NULL, remain > 250ULL ? 250ULL : remain);
    }
    pthread_mutex_unlock(&st->mu); return true;
}

static bool pred_tunnel_accept(const NetTestState *s){ return s->tunnel_accepted; }
static bool pred_chat(const NetTestState *s){ return s->chat_received; }
static bool pred_file_accept(const NetTestState *s){ return s->file_accepted; }
static bool pred_file_done(const NetTestState *s){ return s->file_done; }
static bool pred_friend_accepted(const NetTestState *s){ return s->friend_accepted; }

static void vault_test_identity(Vault *v, const char *name) {
    memset(v, 0, sizeof(*v)); snprintf(v->username, sizeof(v->username), "%s", name);
    v->ed_private = crypto_ed25519_generate(); assert(v->ed_private); assert(crypto_ed25519_public(v->ed_private, v->ed_pub) == 0);
}

static void test_net_integration(void) {
    NetTestState sa = {0}, sb = {0}; pthread_mutex_init(&sa.mu, NULL); pthread_cond_init(&sa.cv, NULL); pthread_mutex_init(&sb.mu, NULL); pthread_cond_init(&sb.cv, NULL);
    Vault a, b; vault_test_identity(&a, "alice"); vault_test_identity(&b, "bob");
    NetNode na = {.net=NULL, .state=&sa}, nb = {.net=NULL, .state=&sb}; NetContext *net_a=NULL, *net_b=NULL;
    assert(net_init(&net_a, &a, net_test_cb, &na, 0) == 0); na.net=net_a;
    assert(net_init(&net_b, &b, net_test_cb, &nb, 0) == 0); nb.net=net_b;
    a.friend_count=1; snprintf(a.friends[0].username,sizeof(a.friends[0].username),"bob"); snprintf(a.friends[0].endpoint,sizeof(a.friends[0].endpoint),"127.0.0.1:%u",(unsigned)net_listen_port(net_b)); memcpy(a.friends[0].ed_pub,b.ed_pub,32); a.friends[0].key_pinned=true;
    b.friend_count=1; snprintf(b.friends[0].username,sizeof(b.friends[0].username),"alice"); snprintf(b.friends[0].endpoint,sizeof(b.friends[0].endpoint),"127.0.0.1:%u",(unsigned)net_listen_port(net_a)); memcpy(b.friends[0].ed_pub,a.ed_pub,32); b.friends[0].key_pinned=true;
    a.endpoint[0]=b.endpoint[0]='\0';
    char tunnel[]="vlt-test-connection"; assert(net_tunnel(net_a,&a.friends[0],tunnel)==0); assert(wait_condition(&sa,pred_tunnel_accept,5000));
    assert(net_send_chat(net_a,tunnel,"hello over p2p")==0); assert(wait_condition(&sb,pred_chat,3000));
    uint8_t payload[100000]; for(size_t i=0;i<sizeof(payload);++i) payload[i]=(uint8_t)(i*31U+i/257U); uint8_t digest[32]; assert(crypto_sha256(payload,sizeof(payload),digest)==0);
    assert(net_send_file_request(net_a,tunnel,"transfer-integration","sample.bin",sizeof(payload),digest)==0); assert(wait_condition(&sa,pred_file_accept,3000));
    for(size_t off=0;off<sizeof(payload);){size_t n=sizeof(payload)-off; if(n>BALANCE_FILE_CHUNK)n=BALANCE_FILE_CHUNK; assert(net_send_file_chunk(net_a,"transfer-integration",off,payload+off,n)==0); off+=n;}
    assert(net_send_file_done(net_a,"transfer-integration")==0); assert(wait_condition(&sb,pred_file_done,5000));
    pthread_mutex_lock(&sb.mu); assert(!sb.file_bad_order && sb.file_len==sizeof(payload) && memcmp(sb.file_buf,payload,sizeof(payload))==0); pthread_mutex_unlock(&sb.mu);

    NetTestState sc = {0}; pthread_mutex_init(&sc.mu, NULL); pthread_cond_init(&sc.cv, NULL);
    Vault c; vault_test_identity(&c, "carol"); NetNode nc = {.net=NULL, .state=&sc}; NetContext *net_c=NULL;
    assert(net_init(&net_c, &c, net_test_cb, &nc, 0) == 0); nc.net=net_c;
    Friend target={0}; snprintf(target.username,sizeof(target.username),"alice"); snprintf(target.endpoint,sizeof(target.endpoint),"127.0.0.1:%u",(unsigned)net_listen_port(net_a)); memcpy(target.ed_pub,a.ed_pub,32); target.key_pinned=false;
    int frc=net_request_friend(net_c,&target); assert(frc==0); assert(wait_condition(&sc,pred_friend_accepted,3000));
    net_stop(net_c); vault_free(&c); pthread_cond_destroy(&sc.cv); pthread_mutex_destroy(&sc.mu);

    net_stop(net_a); net_stop(net_b); vault_free(&a); vault_free(&b); pthread_cond_destroy(&sa.cv); pthread_mutex_destroy(&sa.mu); pthread_cond_destroy(&sb.cv); pthread_mutex_destroy(&sb.mu);
}

static void test_cloud(void) {
    const char *url = getenv("BALANCE_CLOUD_TEST_URL");
    if (!url || !*url) return;
    char tmp[] = "/tmp/balance-cloud-test-home-XXXXXX"; assert(mkdtemp(tmp)); assert(setenv("HOME", tmp, 1) == 0);
    Store st; assert(store_init(&st) == 0);
    Vault v; memset(&v, 0, sizeof(v));
    const char *pw = "cloud-test-password-strong";
    assert(store_register(&st, "cloudalice", pw, 0, &v) == 0);
    uint8_t key[32], salt[16]; assert(store_master_key(&st, pw, key, salt) == 0);
    v.cloud_enabled = true; snprintf(v.cloud_url, sizeof(v.cloud_url), "%s", url); const char *tok=getenv("BALANCE_CLOUD_TEST_TOKEN"); if(tok) snprintf(v.cloud_token,sizeof(v.cloud_token),"%s",tok);
    snprintf(v.cloud_object_id, sizeof(v.cloud_object_id), "cloud-test-object-001");
    assert(cloud_url_is_allowed(v.cloud_url));
    HistoryEntry e={.ts=42,.local=true}; snprintf(e.chat,sizeof(e.chat),"cloudalice <-> bob"); snprintf(e.text,sizeof(e.text),"encrypted cloud history");
    assert(store_append_history(&st,key,&e)==0); assert(cloud_put_history(&v,&st)==0);
    char hist_path[BALANCE_MAX_PATH]; snprintf(hist_path,sizeof(hist_path),"%s",st.history_path); assert(unlink(hist_path)==0);
    assert(cloud_get_history(&v,&st)==0);
    HistoryEntry out[4]; size_t n=0; assert(store_load_history(&st,key,out,4,&n)==0 && n==1); assert(strcmp(out[0].text,"encrypted cloud history")==0);
    vault_free(&v); secure_zero(key,sizeof(key)); char cmd[512]; snprintf(cmd,sizeof(cmd),"rm -rf '%s'",tmp); system(cmd);
}

int main(void){assert(crypto_init()==0);test_commands();test_crypto();test_store();test_proto();test_net_integration();test_cloud();puts("balance-tests: OK");return 0;}
