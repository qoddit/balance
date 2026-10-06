#include "common.h"
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <openssl/crypto.h>

void secure_zero(void *p, size_t n) { if (p != NULL && n != 0) OPENSSL_cleanse(p, n); }

uint64_t now_millis(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) return 0;
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)(ts.tv_nsec / 1000000L);
}

int parse_u64(const char *s, uint64_t *out) {
    if (s == NULL || out == NULL || *s == '\0') return BALANCE_ERR;
    errno = 0;
    char *end = NULL;
    unsigned long long v = strtoull(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0') return BALANCE_ERR;
    *out = (uint64_t)v;
    return BALANCE_OK;
}

void trim_inplace(char *s) {
    if (s == NULL) return;
    size_t n = strlen(s);
    size_t a = 0;
    while (a < n && isspace((unsigned char)s[a])) a++;
    size_t b = n;
    while (b > a && isspace((unsigned char)s[b - 1U])) b--;
    if (a > 0U) memmove(s, s + a, b - a);
    s[b - a] = '\0';
}

int sanitize_component(const char *src, char *dst, size_t dst_sz) {
    if (src == NULL || dst == NULL || dst_sz == 0U) return BALANCE_ERR;
    size_t j = 0;
    for (size_t i = 0; src[i] != '\0'; ++i) {
        unsigned char c = (unsigned char)src[i];
        if (j + 1U >= dst_sz) return BALANCE_ERR;
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == ' ') dst[j++] = (char)c;
        else dst[j++] = '_';
    }
    dst[j] = '\0';
    return j == 0U ? BALANCE_ERR : BALANCE_OK;
}

void format_speed(uint64_t bps, char *out, size_t out_sz) {
    static const char *units[] = {"B/s", "KiB/s", "MiB/s", "GiB/s"};
    double v = (double)bps;
    size_t u = 0;
    while (v >= 1024.0 && u < 3U) { v /= 1024.0; ++u; }
    (void)snprintf(out, out_sz, "%.1f %s", v, units[u]);
}

void format_progress(uint64_t done, uint64_t total, char *out, size_t out_sz) {
    if (total == 0ULL) { (void)snprintf(out, out_sz, "[####################] 100.0%%"); return; }
    double pct = ((double)done * 100.0) / (double)total;
    if (pct > 100.0) pct = 100.0;
    size_t filled = (size_t)(pct / 5.0);
    if (filled > 20U) filled = 20U;
    char bar[21];
    for (size_t i = 0; i < 20U; ++i) bar[i] = i < filled ? '#' : '-';
    bar[20] = '\0';
    (void)snprintf(out, out_sz, "[%s] %5.1f%%", bar, pct);
}
