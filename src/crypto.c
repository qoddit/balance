#include "crypto.h"
#include <openssl/core_names.h>
#include <openssl/kdf.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/params.h>
#include <string.h>

int crypto_init(void) {
    return RAND_status() == 1 ? BALANCE_OK : BALANCE_ERR;
}

int crypto_random(void *out, size_t n) {
    if (out == NULL || n > INT_MAX) return BALANCE_ERR;
    return RAND_bytes((unsigned char *)out, (int)n) == 1 ? BALANCE_OK : BALANCE_ERR;
}

int crypto_scrypt(const char *password, const uint8_t salt[CRYPTO_SALT], uint8_t key[CRYPTO_KEY]) {
    if (password == NULL || salt == NULL || key == NULL) return BALANCE_ERR;
    EVP_KDF *kdf = EVP_KDF_fetch(NULL, "SCRYPT", NULL);
    if (kdf == NULL) return BALANCE_ERR;
    EVP_KDF_CTX *ctx = EVP_KDF_CTX_new(kdf);
    EVP_KDF_free(kdf);
    if (ctx == NULL) return BALANCE_ERR;
    const uint64_t n = 1ULL << 16;
    const uint64_t r = 8;
    const uint64_t p = 1;
    OSSL_PARAM params[] = {
        OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_PASSWORD, (void *)password, strlen(password)),
        OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SALT, (void *)salt, CRYPTO_SALT),
        OSSL_PARAM_construct_uint64(OSSL_KDF_PARAM_SCRYPT_N, (uint64_t *)&n),
        OSSL_PARAM_construct_uint64(OSSL_KDF_PARAM_SCRYPT_R, (uint64_t *)&r),
        OSSL_PARAM_construct_uint64(OSSL_KDF_PARAM_SCRYPT_P, (uint64_t *)&p),
        OSSL_PARAM_construct_end()
    };
    int rc = EVP_KDF_derive(ctx, key, CRYPTO_KEY, params) == 1 ? BALANCE_OK : BALANCE_ERR;
    EVP_KDF_CTX_free(ctx);
    return rc;
}

int crypto_aead_encrypt(const uint8_t key[CRYPTO_KEY], const uint8_t *aad, size_t aad_len,
                       const uint8_t *pt, size_t pt_len, uint8_t iv[CRYPTO_IV],
                       uint8_t *ct, uint8_t tag[CRYPTO_TAG]) {
    if (!key || !iv || !ct || !tag || (!pt && pt_len)) return BALANCE_ERR;
    if (crypto_random(iv, CRYPTO_IV) != BALANCE_OK) return BALANCE_ERR;
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return BALANCE_ERR;
    int len = 0, out_len = 0, ok = 0;
    do {
        if (EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, NULL, NULL) != 1) break;
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, CRYPTO_IV, NULL) != 1) break;
        if (EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv) != 1) break;
        if (aad_len && EVP_EncryptUpdate(ctx, NULL, &len, aad, (int)aad_len) != 1) break;
        len = 0;
        if (pt_len && EVP_EncryptUpdate(ctx, ct, &len, pt, (int)pt_len) != 1) break;
        out_len = len;
        if (EVP_EncryptFinal_ex(ctx, ct + out_len, &len) != 1) break;
        out_len += len;
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, CRYPTO_TAG, tag) != 1) break;
        ok = out_len == (int)pt_len;
    } while (0);
    EVP_CIPHER_CTX_free(ctx);
    return ok ? BALANCE_OK : BALANCE_ERR;
}

int crypto_aead_decrypt(const uint8_t key[CRYPTO_KEY], const uint8_t *aad, size_t aad_len,
                       const uint8_t *ct, size_t ct_len, const uint8_t iv[CRYPTO_IV],
                       const uint8_t tag[CRYPTO_TAG], uint8_t *pt) {
    if (!key || !iv || !tag || !pt || (!ct && ct_len)) return BALANCE_ERR;
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return BALANCE_ERR;
    int len = 0, out_len = 0, ok = 0;
    do {
        if (EVP_DecryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, NULL, NULL) != 1) break;
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, CRYPTO_IV, NULL) != 1) break;
        if (EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv) != 1) break;
        if (aad_len && EVP_DecryptUpdate(ctx, NULL, &len, aad, (int)aad_len) != 1) break;
        len = 0;
        if (ct_len && EVP_DecryptUpdate(ctx, pt, &len, ct, (int)ct_len) != 1) break;
        out_len = len;
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, CRYPTO_TAG, (void *)tag) != 1) break;
        if (EVP_DecryptFinal_ex(ctx, pt + out_len, &len) != 1) break;
        out_len += len;
        ok = out_len == (int)ct_len;
    } while (0);
    EVP_CIPHER_CTX_free(ctx);
    return ok ? BALANCE_OK : BALANCE_ERR;
}

int crypto_hkdf(const uint8_t *ikm, size_t ikm_len, const uint8_t *salt, size_t salt_len,
                const uint8_t *info, size_t info_len, uint8_t *out, size_t out_len) {
    EVP_KDF *kdf = EVP_KDF_fetch(NULL, "HKDF", NULL);
    if (!kdf) return BALANCE_ERR;
    EVP_KDF_CTX *ctx = EVP_KDF_CTX_new(kdf);
    EVP_KDF_free(kdf);
    if (!ctx) return BALANCE_ERR;
    const char *digest = "SHA256";
    OSSL_PARAM params[] = {
        OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_DIGEST, (char *)digest, 0),
        OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_KEY, (void *)ikm, ikm_len),
        OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SALT, (void *)salt, salt_len),
        OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_INFO, (void *)info, info_len),
        OSSL_PARAM_construct_end()
    };
    int rc = EVP_KDF_derive(ctx, out, out_len, params) == 1 ? BALANCE_OK : BALANCE_ERR;
    EVP_KDF_CTX_free(ctx);
    return rc;
}

int crypto_sha256(const uint8_t *data, size_t len, uint8_t out[32]) {
    if ((!data && len) || !out) return BALANCE_ERR;
    return SHA256(data, len, out) ? BALANCE_OK : BALANCE_ERR;
}

int crypto_hex(const uint8_t *data, size_t len, char *out, size_t out_sz) {
    static const char h[] = "0123456789abcdef";
    if (!data || !out || out_sz < len * 2U + 1U) return BALANCE_ERR;
    for (size_t i = 0; i < len; ++i) { out[i * 2U] = h[data[i] >> 4]; out[i * 2U + 1U] = h[data[i] & 0x0fU]; }
    out[len * 2U] = '\0';
    return BALANCE_OK;
}

static int hexval(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

int crypto_unhex(const char *hex, uint8_t *out, size_t out_len) {
    if (!hex || !out || strlen(hex) != out_len * 2U) return BALANCE_ERR;
    for (size_t i = 0; i < out_len; ++i) {
        int a = hexval(hex[i * 2U]), b = hexval(hex[i * 2U + 1U]);
        if (a < 0 || b < 0) return BALANCE_ERR;
        out[i] = (uint8_t)((a << 4) | b);
    }
    return BALANCE_OK;
}

EVP_PKEY *crypto_ed25519_generate(void) {
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, NULL);
    if (!ctx) return NULL;
    EVP_PKEY *p = NULL;
    if (EVP_PKEY_keygen_init(ctx) == 1 && EVP_PKEY_keygen(ctx, &p) == 1) { EVP_PKEY_CTX_free(ctx); return p; }
    EVP_PKEY_CTX_free(ctx); return NULL;
}

int crypto_ed25519_public(EVP_PKEY *pkey, uint8_t out[32]) {
    size_t n = 32U;
    return pkey && out && EVP_PKEY_get_raw_public_key(pkey, out, &n) == 1 && n == 32U ? BALANCE_OK : BALANCE_ERR;
}

int crypto_ed25519_sign(EVP_PKEY *pkey, const uint8_t *msg, size_t msg_len, uint8_t sig[64]) {
    if (!pkey || (!msg && msg_len) || !sig) return BALANCE_ERR;
    EVP_MD_CTX *ctx = EVP_MD_CTX_new(); if (!ctx) return BALANCE_ERR;
    size_t n = 64U;
    int ok = EVP_DigestSignInit(ctx, NULL, NULL, NULL, pkey) == 1 && EVP_DigestSign(ctx, sig, &n, msg, msg_len) == 1 && n == 64U;
    EVP_MD_CTX_free(ctx); return ok ? BALANCE_OK : BALANCE_ERR;
}

int crypto_ed25519_verify(EVP_PKEY *pkey, const uint8_t *msg, size_t msg_len, const uint8_t sig[64]) {
    if (!pkey || (!msg && msg_len) || !sig) return BALANCE_ERR;
    EVP_MD_CTX *ctx = EVP_MD_CTX_new(); if (!ctx) return BALANCE_ERR;
    int ok = EVP_DigestVerifyInit(ctx, NULL, NULL, NULL, pkey) == 1 && EVP_DigestVerify(ctx, sig, 64U, msg, msg_len) == 1;
    EVP_MD_CTX_free(ctx); return ok ? BALANCE_OK : BALANCE_ERR;
}

int crypto_pkey_private_der(EVP_PKEY *pkey, uint8_t **out, size_t *out_len) {
    if (!pkey || !out || !out_len) return BALANCE_ERR;
    int n = i2d_PrivateKey(pkey, NULL); if (n <= 0) return BALANCE_ERR;
    uint8_t *buf = OPENSSL_malloc((size_t)n); if (!buf) return BALANCE_ERR;
    unsigned char *p = buf; if (i2d_PrivateKey(pkey, &p) != n) { OPENSSL_free(buf); return BALANCE_ERR; }
    *out = buf; *out_len = (size_t)n; return BALANCE_OK;
}

EVP_PKEY *crypto_pkey_from_private_der(const uint8_t *der, size_t der_len) {
    if (!der || der_len > LONG_MAX) return NULL;
    const unsigned char *p = der;
    return d2i_AutoPrivateKey(NULL, &p, (long)der_len);
}

EVP_PKEY *crypto_x25519_generate(void) {
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);
    if (!ctx) return NULL;
    EVP_PKEY *p = NULL;
    if (EVP_PKEY_keygen_init(ctx) == 1 && EVP_PKEY_keygen(ctx, &p) == 1) { EVP_PKEY_CTX_free(ctx); return p; }
    EVP_PKEY_CTX_free(ctx); return NULL;
}

int crypto_x25519_public(EVP_PKEY *pkey, uint8_t out[32]) {
    size_t n = 32U;
    return pkey && out && EVP_PKEY_get_raw_public_key(pkey, out, &n) == 1 && n == 32U ? BALANCE_OK : BALANCE_ERR;
}

int crypto_x25519_derive(EVP_PKEY *local, const uint8_t peer_pub[32], uint8_t out[32]) {
    if (!local || !peer_pub || !out) return BALANCE_ERR;
    EVP_PKEY *peer = EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, NULL, peer_pub, 32U);
    if (!peer) return BALANCE_ERR;
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(local, NULL); if (!ctx) { EVP_PKEY_free(peer); return BALANCE_ERR; }
    size_t n = 32U; int ok = EVP_PKEY_derive_init(ctx) == 1 && EVP_PKEY_derive_set_peer(ctx, peer) == 1 && EVP_PKEY_derive(ctx, out, &n) == 1 && n == 32U;
    EVP_PKEY_CTX_free(ctx); EVP_PKEY_free(peer); return ok ? BALANCE_OK : BALANCE_ERR;
}
