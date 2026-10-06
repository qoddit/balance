#ifndef BALANCE_UI_H
#define BALANCE_UI_H

#include "common.h"
#include "net.h"
#include "store.h"

#define UI_MAX_MESSAGES 2000
#define UI_MAX_ACTIONS 32
#define UI_MAX_TRANSFERS 64

typedef struct App App;

int ui_authenticate(Store *store, Vault *vault, uint8_t master_key[32]);
int ui_run(Store *store, Vault *vault, NetContext *net, uint8_t master_key[32]);
void ui_on_net_event(const NetEvent *ev, void *opaque);

#endif
