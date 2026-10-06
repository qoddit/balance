#include "proto.h"
#include <stdlib.h>
#include <string.h>

static void put32(uint8_t *p, uint32_t x) { p[0]=(uint8_t)(x>>24); p[1]=(uint8_t)(x>>16); p[2]=(uint8_t)(x>>8); p[3]=(uint8_t)x; }
static uint32_t get32(const uint8_t *p) { return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3]; }
static void put64(uint8_t *p, uint64_t x) { for (int i=0;i<8;++i) p[7-i]=(uint8_t)(x>>(i*8)); }
static uint64_t get64(const uint8_t *p) { uint64_t x=0; for(int i=0;i<8;++i) x=(x<<8)|p[i]; return x; }

int proto_build_plain(uint8_t type, const uint8_t *payload, size_t payload_len, uint8_t **out, size_t *out_len) {
    if (!out || !out_len || payload_len > BALANCE_MAX_FRAME) return BALANCE_ERR;
    size_t n = 5U + payload_len;
    uint8_t *b = malloc(n); if (!b) return BALANCE_ERR;
    b[0] = type; put32(b+1, (uint32_t)payload_len); if (payload_len) memcpy(b+5,payload,payload_len);
    *out=b; *out_len=n; return BALANCE_OK;
}

int proto_parse_plain(const uint8_t *buf, size_t len, uint8_t *type, const uint8_t **payload, size_t *payload_len) {
    if (!buf || len < 5U || !type || !payload || !payload_len) return BALANCE_ERR;
    uint32_t n = get32(buf+1); if (n > BALANCE_MAX_FRAME || (size_t)n + 5U != len) return BALANCE_ERR;
    *type=buf[0]; *payload=buf+5; *payload_len=n; return BALANCE_OK;
}

int proto_encrypt_frame(uint8_t type, uint64_t seq, const uint8_t key[32], const uint8_t nonce_prefix[4],
                        const uint8_t *aad_extra, size_t aad_extra_len, const uint8_t *pt, size_t pt_len,
                        uint8_t **out, size_t *out_len) {
    (void)nonce_prefix;
    if (!key || !out || !out_len || pt_len > BALANCE_MAX_FRAME || aad_extra_len > 1024U) return BALANCE_ERR;
    uint8_t aad[1+8+1024];
    aad[0]=type; put64(aad+1,seq); if(aad_extra_len) memcpy(aad+9,aad_extra,aad_extra_len);
    uint8_t iv[12];
    uint8_t *ct=malloc(pt_len+16U); if (!ct) return BALANCE_ERR;
    uint8_t tag[16];
    if(crypto_aead_encrypt(key,aad,9U+aad_extra_len,pt,pt_len,iv,ct,tag)!=BALANCE_OK){free(ct);return BALANCE_ERR;}
    size_t n=1U+8U+12U+pt_len+16U;
    uint8_t *b=malloc(n); if(!b){free(ct);return BALANCE_ERR;}
    b[0]=(uint8_t)(type|FRAME_ENCRYPTED_FLAG); put64(b+1,seq); memcpy(b+9,iv,12U); if(pt_len)memcpy(b+21,ct,pt_len); memcpy(b+21+pt_len,tag,16U);
    free(ct); *out=b; *out_len=n; return BALANCE_OK;
}

int proto_decrypt_frame(const uint8_t key[32], const uint8_t nonce_prefix[4],
                        const uint8_t *buf, size_t len, const uint8_t *aad_extra, size_t aad_extra_len,
                        ProtoFrame *out) {
    (void)nonce_prefix;
    if (!key||!buf||!out||len<1U+8U+12U+16U||aad_extra_len>1024U) return BALANCE_ERR;
    uint8_t type=buf[0]; if((type&FRAME_ENCRYPTED_FLAG)==0U)return BALANCE_ERR; type&=(uint8_t)~FRAME_ENCRYPTED_FLAG;
    uint64_t seq=get64(buf+1); const uint8_t *iv=buf+9; size_t ct_len=len-21U-16U; const uint8_t *ct=buf+21; const uint8_t *tag=buf+21+ct_len;
    uint8_t aad[1+8+1024]; aad[0]=type; put64(aad+1,seq); if(aad_extra_len)memcpy(aad+9,aad_extra,aad_extra_len);
    uint8_t *pt=malloc(ct_len+1U); if(!pt)return BALANCE_ERR;
    if(crypto_aead_decrypt(key,aad,9U+aad_extra_len,ct,ct_len,iv,tag,pt)!=BALANCE_OK){free(pt);return BALANCE_ERR;}
    out->type=type; out->seq=seq; out->payload=pt; out->payload_len=ct_len; return BALANCE_OK;
}

void proto_frame_free(ProtoFrame *f){ if(f){ free(f->payload); f->payload=NULL; f->payload_len=0; }}
