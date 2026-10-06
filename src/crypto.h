#ifndef BALANCE_CRYPTO_H
#define BALANCE_CRYPTO_H

#include "common.h"
#include <openssl/evp.h>

#define CRYPTO_KEY 32U
#define CRYPTO_IV 12U
#define CRYPTO_TAG 16U
#define CRYPTO_SALT 16U

int crypto_init(void);
int crypto_random(void *out, size_t n);
int crypto_scrypt(const char *password, const uint8_t salt[CRYPTO_SALT], uint8_t key[CRYPTO_KEY]);
int crypto_aead_encrypt(const uint8_t key[CRYPTO_KEY], const uint8_t *aad, size_t aad_len,
                       const uint8_t *pt, size_t pt_len, uint8_t iv[CRYPTO_IV],
                       uint8_t *ct, uint8_t tag[CRYPTO_TAG]);
int crypto_aead_decrypt(const uint8_t key[CRYPTO_KEY], const uint8_t *aad, size_t aad_len,
                       const uint8_t *ct, size_t ct_len, const uint8_t iv[CRYPTO_IV],
                       const uint8_t tag[CRYPTO_TAG], uint8_t *pt);
int crypto_hkdf(const uint8_t *ikm, size_t ikm_len, const uint8_t *salt, size_t salt_len,
                const uint8_t *info, size_t info_len, uint8_t *out, size_t out_len);
int crypto_sha256(const uint8_t *data, size_t len, uint8_t out[32]);
int crypto_hex(const uint8_t *data, size_t len, char *out, size_t out_sz);
int crypto_unhex(const char *hex, uint8_t *out, size_t out_len);

EVP_PKEY *crypto_ed25519_generate(void);
int crypto_ed25519_public(EVP_PKEY *pkey, uint8_t out[32]);
int crypto_ed25519_sign(EVP_PKEY *pkey, const uint8_t *msg, size_t msg_len, uint8_t sig[64]);
int crypto_ed25519_verify(EVP_PKEY *pkey, const uint8_t *msg, size_t msg_len, const uint8_t sig[64]);
int crypto_pkey_private_der(EVP_PKEY *pkey, uint8_t **out, size_t *out_len);
EVP_PKEY *crypto_pkey_from_private_der(const uint8_t *der, size_t der_len);
EVP_PKEY *crypto_x25519_generate(void);
int crypto_x25519_public(EVP_PKEY *pkey, uint8_t out[32]);
int crypto_x25519_derive(EVP_PKEY *local, const uint8_t peer_pub[32], uint8_t out[32]);

#endif
