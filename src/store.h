#ifndef BALANCE_STORE_H
#define BALANCE_STORE_H

#include "common.h"
#include "crypto.h"

#define MAX_FRIENDS 256
#define MAX_HISTORY 1000

typedef struct {
    char username[BALANCE_MAX_USERNAME];
    char endpoint[BALANCE_MAX_ENDPOINT];
    uint8_t ed_pub[32];
    bool key_pinned;
} Friend;

typedef struct {
    char username[BALANCE_MAX_USERNAME];
    char endpoint[BALANCE_MAX_ENDPOINT];
    uint8_t ed_pub[32];
    EVP_PKEY *ed_private;
    Friend friends[MAX_FRIENDS];
    size_t friend_count;
    bool cloud_enabled;
    char cloud_url[512];
    char cloud_token[512];
    char cloud_object_id[65];
} Vault;

typedef struct {
    uint64_t ts;
    char chat[128];
    bool local;
    char text[BALANCE_MAX_MESSAGE];
} HistoryEntry;

typedef struct {
    char root[BALANCE_MAX_PATH];
    char vault_path[BALANCE_MAX_PATH];
    char history_path[BALANCE_MAX_PATH];
} Store;

int store_init(Store *s);
int store_exists(const Store *s);
int store_register(Store *s, const char *username, const char *password, uint16_t listen_port, Vault *out);
int store_login(Store *s, const char *password, Vault *out);
void vault_free(Vault *v);
int store_save_vault(Store *s, const Vault *v, const char *password);
int store_save_vault_key(Store *s, const Vault *v, const uint8_t master_key[32]);
int store_append_history(const Store *s, const uint8_t master_key[32], const HistoryEntry *e);
int store_load_history(const Store *s, const uint8_t master_key[32], HistoryEntry *out, size_t cap, size_t *count);
int store_clear_history(const Store *s);
int store_master_key(const Store *s, const char *password, uint8_t key[32], uint8_t salt[16]);
int store_cloud_sync(const Vault *v, const Store *s, const uint8_t master_key[32]);

#endif
