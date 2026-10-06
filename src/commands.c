#include "commands.h"
#include <string.h>

static const BalanceCommandSpec COMMANDS[] = {
    {"?",        "help",     "? [command]",                    "show command help",                         BALANCE_SCOPE_BOTH},
    {"version",  "ver",      "version",                        "show product version",                     BALANCE_SCOPE_BOTH},
    {"about",    NULL,        "about",                           "show product/security summary",             BALANCE_SCOPE_BOTH},
    {"status",   "st",       "status",                          "show session, tunnel and cloud status",     BALANCE_SCOPE_BOTH},
    {"fetch",    "dashboard", "fetch",                          "open the balance system fetch dashboard",     BALANCE_SCOPE_BOTH},
    {"whoami",   "me",       "whoami",                          "show the current username",                 BALANCE_SCOPE_BOTH},
    {"endpoint", "ep",       "endpoint",                        "show the local listening endpoint",         BALANCE_SCOPE_BOTH},
    {"key",      "fingerprint", "key",                          "show the local Ed25519 fingerprint",        BALANCE_SCOPE_BOTH},
    {"memory",   "mem",      "memory",                          "show current process memory usage",          BALANCE_SCOPE_BOTH},
    {"net",      "network",  "net",                            "show live network counters",                BALANCE_SCOPE_BOTH},
    {"time",     NULL,        "time",                            "show the current local time",               BALANCE_SCOPE_BOTH},
    {"friends",  "f",        "friends [online]",               "list friends or online friends",             BALANCE_SCOPE_HOME},
    {"online",   NULL,        "online",                          "probe all friends for reachability",         BALANCE_SCOPE_HOME},
    {"friend",   NULL,        "friend <name>",                  "show one friend and key details",            BALANCE_SCOPE_HOME},
    {"add",      NULL,        "add <name> <host:port>",          "create/send a friend request",               BALANCE_SCOPE_HOME},
    {"remove",   "rm",       "remove <name>",                   "remove a local friend record",               BALANCE_SCOPE_HOME},
    {"tunnel",   "connect",  "tunnel <friend>",                 "request an encrypted volatile tunnel",        BALANCE_SCOPE_HOME},
    {"chat",     "c",       "chat <friend>",                   "open an existing tunnel or request one",     BALANCE_SCOPE_BOTH},
    {"close",    "leave",   "close",                           "close the active volatile tunnel",           BALANCE_SCOPE_CHAT},
    {"accept",   "a",       "accept <id-or-name>",              "accept a pinned tunnel/file/friend request", BALANCE_SCOPE_BOTH},
    {"reject",   "r",       "reject <id-or-name> [reason]",     "reject a pending request",                   BALANCE_SCOPE_BOTH},
    {"ping",     NULL,       "ping <friend>",                    "measure peer reachability latency",           BALANCE_SCOPE_BOTH},
    {"history",  "hstry",   "history [clear|<friend>]",         "view or clear encrypted local history",      BALANCE_SCOPE_BOTH},
    {"search",   "find",    "search <text>",                    "search the current in-memory chat view",     BALANCE_SCOPE_BOTH},
    {"transfers", "xfer",   "transfers",                        "list active and recent file transfers",      BALANCE_SCOPE_BOTH},
    {"file",     "send",    "file <path>",                     "request a peer-to-peer file transfer",        BALANCE_SCOPE_CHAT},
    {"cloud",    NULL,       "cloud [on <url> [token]|off]",     "view or configure cloud persistence",        BALANCE_SCOPE_BOTH},
    {"sync",     "push",    "sync",                             "upload encrypted local history to cloud",     BALANCE_SCOPE_BOTH},
    {"pull",     NULL,       "pull",                             "download encrypted cloud history",            BALANCE_SCOPE_BOTH},
    {"config",   "cfg",     "config",                           "show local paths and non-secret settings",   BALANCE_SCOPE_BOTH},
    {"doctor",   "check",   "doctor",                           "run local health/security checks",            BALANCE_SCOPE_BOTH},
    {"theme",    NULL,       "theme <cyber|mono|amber|cycle>",   "change the TUI theme",                       BALANCE_SCOPE_BOTH},
    {"redraw",   "refresh", "redraw",                           "rebuild the TUI layout",                     BALANCE_SCOPE_BOTH},
    {"clear",    "cls",     "clear",                            "clear the current volatile view",            BALANCE_SCOPE_BOTH},
    {"quit",     "q",       "quit",                             "exit balance",                               BALANCE_SCOPE_BOTH},
    {"exit",     NULL,       "exit",                             "exit balance",                               BALANCE_SCOPE_BOTH},
};

const BalanceCommandSpec *balance_command_specs(size_t *count) {
    if (count) *count = sizeof(COMMANDS) / sizeof(COMMANDS[0]);
    return COMMANDS;
}

const BalanceCommandSpec *balance_command_lookup(const char *name) {
    if (!name || !*name) return NULL;
    for (size_t i = 0; i < sizeof(COMMANDS) / sizeof(COMMANDS[0]); ++i) {
        if (strcmp(COMMANDS[i].name, name) == 0) return &COMMANDS[i];
        if (COMMANDS[i].alias && strcmp(COMMANDS[i].alias, name) == 0) return &COMMANDS[i];
    }
    return NULL;
}

bool balance_command_is(const char *name) { return balance_command_lookup(name) != NULL; }
