#ifndef BALANCE_NET_H
#define BALANCE_NET_H

#include "common.h"
#include "crypto.h"
#include "proto.h"
#include "store.h"

#define MAX_CONNECTIONS 64

typedef enum {
    NET_EV_STATUS,
    NET_EV_TUNNEL_REQUEST,
    NET_EV_TUNNEL_ACCEPTED,
    NET_EV_TUNNEL_REJECTED,
    NET_EV_MESSAGE,
    NET_EV_FILE_REQUEST,
    NET_EV_FILE_ACCEPTED,
    NET_EV_FILE_REJECTED,
    NET_EV_FILE_CHUNK,
    NET_EV_FILE_DONE,
    NET_EV_DISCONNECTED,
    NET_EV_FRIEND_REQUEST,
    NET_EV_FRIEND_ACCEPTED,
    NET_EV_FRIEND_REJECTED
} NetEventType;

typedef struct {
    NetEventType type;
    char username[BALANCE_MAX_USERNAME];
    char tunnel_id[BALANCE_MAX_TUNNEL];
    char transfer_id[65];
    char text[BALANCE_MAX_MESSAGE];
    char filename[256];
    char endpoint[BALANCE_MAX_ENDPOINT];
    char reason[256];
    uint64_t size;
    uint64_t offset;
    uint8_t digest[32];
    uint8_t ed_pub[32];
    uint8_t *data;
    size_t data_len;
} NetEvent;

typedef void (*NetEventFn)(const NetEvent *ev, void *opaque);


typedef struct {
    uint64_t tx_bytes;
    uint64_t rx_bytes;
    uint64_t tx_frames;
    uint64_t rx_frames;
    uint64_t tx_bytes_per_sec;
    uint64_t rx_bytes_per_sec;
    uint64_t active_peers;
    uint64_t active_tunnels;
} NetStats;

typedef struct NetContext NetContext;

int net_init(NetContext **out, Vault *vault, NetEventFn cb, void *opaque, uint16_t listen_port);
void net_stop(NetContext *n);
uint16_t net_listen_port(const NetContext *n);
int net_tunnel(NetContext *n, const Friend *f, const char *tunnel_id);
int net_accept_tunnel(NetContext *n, const char *tunnel_id);
int net_reject_tunnel(NetContext *n, const char *tunnel_id, const char *reason);
int net_send_chat(NetContext *n, const char *tunnel_id, const char *text);
int net_send_file_request(NetContext *n, const char *tunnel_id, const char *transfer_id, const char *filename, uint64_t size, const uint8_t digest[32]);
int net_accept_file(NetContext *n, const char *transfer_id);
int net_reject_file(NetContext *n, const char *transfer_id, const char *reason);
int net_send_file_chunk(NetContext *n, const char *transfer_id, uint64_t offset, const uint8_t *data, size_t len);
int net_send_file_done(NetContext *n, const char *transfer_id);
int net_request_friend(NetContext *n, const Friend *f);
int net_accept_friend(NetContext *n, const char *username);
int net_reject_friend(NetContext *n, const char *username);
int net_online_probe(NetContext *n, const Friend *f);
int net_find_tunnel(NetContext *n, const char *tunnel_id, char *peer, size_t peer_sz);
int net_close_tunnel(NetContext *n, const char *tunnel_id);
int net_get_stats(NetContext *n, NetStats *out);

#endif
