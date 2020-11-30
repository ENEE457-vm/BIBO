#ifndef GRADEBOOK_CRYPTO_H
#define GRADEBOOK_CRYPTO_H
#include "gradebook.h"

#define KEY_SIZE 32
#define IV_SIZE  16
#define TAG_SIZE 16

int parse_key(const char *key_str, uint8_t *key_val);

int gradebook_encrypt(gradebook_t *gradebook, uint8_t *key, uint8_t *iv,
                      int iv_len, uint8_t *ciphertext, uint8_t *tag);

int gradebook_decrypt(uint8_t *ciphertext, uint8_t *key, uint8_t *iv,
                      int iv_len, uint8_t *plaintext, uint8_t *tag);
#endif
