#include "common.h"
#include "crypto.h"
#include "net.h"
#include "store.h"
#include "ui.h"
#include <stdio.h>
#include <string.h>

static void netcb(const NetEvent *ev, void *opaque) { ui_on_net_event(ev, opaque); }

int main(void) {
    if (crypto_init() != BALANCE_OK) { fprintf(stderr, "balance: cryptographic RNG unavailable\n"); return 1; }
    Store store;
    if (store_init(&store) != BALANCE_OK) { fprintf(stderr, "balance: local storage initialization failed\n"); return 1; }
    Vault vault; memset(&vault, 0, sizeof(vault));
    uint8_t master_key[32];
    if (ui_authenticate(&store, &vault, master_key) != BALANCE_OK) { secure_zero(master_key, sizeof(master_key)); return 1; }
    NetContext *net = NULL;
    if (net_init(&net, &vault, netcb, NULL, BALANCE_DEFAULT_LISTEN) != BALANCE_OK) {
        vault_free(&vault); secure_zero(master_key, sizeof(master_key)); fprintf(stderr, "balance: network initialization failed\n"); return 1;
    }
    snprintf(vault.endpoint, sizeof(vault.endpoint), "0.0.0.0:%u", (unsigned)net_listen_port(net));
    if (store_save_vault_key(&store, &vault, master_key) != BALANCE_OK) fprintf(stderr, "balance: warning: could not persist local endpoint\n");
    int rc = ui_run(&store, &vault, net, master_key);
    net_stop(net);
    vault_free(&vault);
    secure_zero(master_key, sizeof(master_key));
    return rc == BALANCE_OK ? 0 : 1;
}
