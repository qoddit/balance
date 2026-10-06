#ifndef BALANCE_COMMANDS_H
#define BALANCE_COMMANDS_H

#include <stddef.h>
#include <stdbool.h>

typedef enum {
    BALANCE_SCOPE_HOME = 1,
    BALANCE_SCOPE_CHAT = 2,
    BALANCE_SCOPE_BOTH = 3
} BalanceCommandScope;

typedef struct {
    const char *name;
    const char *alias;
    const char *usage;
    const char *description;
    BalanceCommandScope scope;
} BalanceCommandSpec;

const BalanceCommandSpec *balance_command_specs(size_t *count);
const BalanceCommandSpec *balance_command_lookup(const char *name);
bool balance_command_is(const char *name);

#endif
