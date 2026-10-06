#ifndef BALANCE_PROTO_H
#define BALANCE_PROTO_H

#include "common.h"
#include "crypto.h"

#define FRAME_HELLO 1
#define FRAME_TUNNEL_REQUEST 2
#define FRAME_TUNNEL_ACCEPT 3
#define FRAME_TUNNEL_REJECT 4
#define FRAME_CHAT 5
#define FRAME_FILE_REQUEST 6
#define FRAME_FILE_ACCEPT 7
#define FRAME_FILE_REJECT 8
#define FRAME_FILE_CHUNK 9
#define FRAME_FILE_DONE 10
#define FRAME_FRIEND_REQUEST 11
#define FRAME_FRIEND_ACCEPT 12
#define FRAME_FRIEND_REJECT 13
#define FRAME_PING 14

#define FRAME_ENCRYPTED_FLAG 0x80

typedef struct {
    uint8_t type;
    uint64_t seq;
    uint8_t *payload;
    size_t payload_len;
} ProtoFrame;

int proto_build_plain(uint8_t type, const uint8_t *payload, size_t payload_len, uint8_t **out, size_t *out_len);
int proto_parse_plain(const uint8_t *buf, size_t len, uint8_t *type, const uint8_t **payload, size_t *payload_len);
int proto_encrypt_frame(uint8_t type, uint64_t seq, const uint8_t key[32], const uint8_t nonce_prefix[4],
                        const uint8_t *aad_extra, size_t aad_extra_len, const uint8_t *pt, size_t pt_len,
                        uint8_t **out, size_t *out_len);
int proto_decrypt_frame(const uint8_t key[32], const uint8_t nonce_prefix[4],
                        const uint8_t *buf, size_t len, const uint8_t *aad_extra, size_t aad_extra_len,
                        ProtoFrame *out);
void proto_frame_free(ProtoFrame *f);

#endif
