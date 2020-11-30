#ifndef _CRYPTO_H
#define _CRYPTO_H

void handleErrors(void);
int get_mac(unsigned char *ciphertext, int ciphertext_len, unsigned char *key, unsigned char *outputMac);
int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
            unsigned char *iv, unsigned char *plaintext);
int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
            unsigned char *iv, unsigned char *ciphertext);
#endif