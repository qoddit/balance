#ifndef BALANCE_CLOUD_H
#define BALANCE_CLOUD_H

#include "common.h"
#include "store.h"

int cloud_put_history(const Vault *v, const Store *s);
int cloud_get_history(const Vault *v, const Store *s);
int cloud_url_is_allowed(const char *url);

#endif
