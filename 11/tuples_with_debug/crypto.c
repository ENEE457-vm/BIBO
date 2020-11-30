#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/pem.h>
#include <openssl/hmac.h>
#include <string.h>
#include <stdbool.h>

#include <limits.h>

#include "data.h"

void handleErrors(void)
{
    ERR_print_errors_fp(stderr);
    abort();
}

// From https://wiki.openssl.org/index.php/EVP_Symmetric_Encryption_and_Decryption
int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
            unsigned char *iv, unsigned char *ciphertext)
{
    EVP_CIPHER_CTX *ctx;

    int len;

    int ciphertext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    /*
     * Initialise the encryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        handleErrors();

    // Disable padding since I will handle it myself.
    if(1 != EVP_CIPHER_CTX_set_padding(ctx, 0))
        handleErrors();

    /*
     * Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can be called multiple times if necessary
     */
    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        handleErrors();
    ciphertext_len = len;

    /*
     * Finalise the encryption. Further ciphertext bytes may be written at
     * this stage.
     */
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        handleErrors();
    ciphertext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

// From https://wiki.openssl.org/index.php/EVP_Symmetric_Encryption_and_Decryption
int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
            unsigned char *iv, unsigned char *plaintext)
{
    EVP_CIPHER_CTX *ctx;

    int len;

    int plaintext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    /*
     * Initialise the decryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        handleErrors();

    // Disable padding since I will handle it myself.
    if(1 != EVP_CIPHER_CTX_set_padding(ctx, 0))
        handleErrors();

    /*
     * Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary.
     */
    // This one bugged out
    if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        handleErrors();
    plaintext_len = len;

    /*
     * Finalise the decryption. Further plaintext bytes may be written at
     * this stage.
     */
    if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
        handleErrors();
    plaintext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return plaintext_len;
}


// From https://wiki.openssl.org/index.php/EVP_Message_Digests
void sha256(const unsigned char *message, size_t message_len, unsigned char *digest,
            unsigned int *digest_len){
  EVP_MD_CTX *mdctx;

  if((mdctx = EVP_MD_CTX_create()) == NULL)
    handleErrors();

  if(1 != EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL))
    handleErrors();

  if(1 != EVP_DigestUpdate(mdctx, message, message_len))
    handleErrors();
/*
  if((*digest = (unsigned char *)OPENSSL_malloc(EVP_MD_size(EVP_sha256()))) == NULL)
    handleErrors();
*/
  if(1 != EVP_DigestFinal_ex(mdctx, digest, digest_len))
    handleErrors();
    
  EVP_MD_CTX_destroy(mdctx);
}


// From https://wiki.openssl.org/index.php/Random_Numbers
int generate_random_number(unsigned char* val, size_t val_len){
  int rc = RAND_bytes(val, val_len);
  unsigned long err = ERR_get_error();
  if(rc != 1) {
    // Failure
    return FAILURE;
  }
  return SUCCESS;
}


int enc_and_auth(const DecryptedGradebook* gradebook, EncryptedGradebook* encrypted_gradebook,
                 unsigned char* enc_key, size_t enc_key_size){

  unsigned char mac_key[KEY_SIZE] = {0};
  unsigned int mac_key_length = 0;
  unsigned int mac_length = 0;
  unsigned char iv[IV_SIZE] = {0};

  int ret = 0;

  // Create a MAC key corresponding to the ENC key.
  sha256(enc_key, enc_key_size, mac_key, &mac_key_length);

  // Create a random IV.
  ret = generate_random_number(iv, IV_SIZE);
  if (ret != SUCCESS) {
    return FAILURE;
  }
  
  // Create ciphertext in encrypted_gradebook.
  memcpy(encrypted_gradebook->iv, iv, IV_SIZE);
  encrypt((unsigned char *)&gradebook -> num_students, sizeof(DecryptedGradebook),
    enc_key, iv, encrypted_gradebook -> encrypted_data);

  // Create a MAC tag in encrypted_gradebook.
  HMAC(EVP_sha256(), mac_key, mac_key_length, (const unsigned char *)&encrypted_gradebook -> iv,
    (IV_SIZE + sizeof(DecryptedGradebook)), encrypted_gradebook->mac, &mac_length);

  return SUCCESS;
}


int verify_and_dec(const DecryptedGradebook* gradebook, EncryptedGradebook* encrypted_gradebook,
                   unsigned char* enc_key, size_t enc_key_size){

  unsigned char mac_key[KEY_SIZE] = {0};
  unsigned int mac_key_length = 0;
  unsigned int mac_length = 0;
  unsigned char new_mac[MAC_SIZE] = {0};
  int verify = 0;

  // Create a MAC key corresponding to the ENC key.
  sha256(enc_key, enc_key_size, mac_key, &mac_key_length);

  // Create a new MAC tag based on encrypted_gradebook's ciphertext.
  HMAC(EVP_sha256(), mac_key, mac_key_length, (const unsigned char *)&encrypted_gradebook -> iv,
    (IV_SIZE + sizeof(DecryptedGradebook)), 
    new_mac, &mac_length);

  // Check to make sure that the new MAC equals the old MAC tag.
  verify = memcmp(new_mac, encrypted_gradebook -> mac, MAC_SIZE);
  if (verify != SUCCESS) {
    return INVALID;
  }

  // Decrypt encrypted_gradebook.
  decrypt(encrypted_gradebook -> encrypted_data, sizeof(DecryptedGradebook), enc_key,
    encrypted_gradebook -> iv, (unsigned char *)&gradebook -> num_students);

  return SUCCESS;
}