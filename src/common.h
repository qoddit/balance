#ifndef BALANCE_COMMON_H
#define BALANCE_COMMON_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>

#include "version.h"

#define BALANCE_VERSION_TEXT BALANCE_VERSION
#define BALANCE_MAX_USERNAME 64
#define BALANCE_MAX_ENDPOINT 256
#define BALANCE_MAX_TUNNEL 96
#define BALANCE_MAX_PATH 1024
#define BALANCE_MAX_MESSAGE 8192
#define BALANCE_MAX_FILE 1073741824ULL
#define BALANCE_FILE_CHUNK 65536U
#define BALANCE_MAX_FRAME (BALANCE_FILE_CHUNK + 8192U)
#define BALANCE_PRESENCE_PORT 33777
#define BALANCE_DEFAULT_LISTEN 0

#define BALANCE_OK 0
#define BALANCE_ERR (-1)

void secure_zero(void *p, size_t n);
uint64_t now_millis(void);
int parse_u64(const char *s, uint64_t *out);
void trim_inplace(char *s);
int sanitize_component(const char *src, char *dst, size_t dst_sz);
void format_speed(uint64_t bytes_per_sec, char *out, size_t out_sz);
void format_progress(uint64_t done, uint64_t total, char *out, size_t out_sz);

#endif
