#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data.h"

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/comp.h>
#include <openssl/sha.h>

/* Generates a 160-bit (20 Byte) hash value for each assignment using SHA1 Hash */
/* Modifies the openssl example from  https://www.openssl.org/docs/man1.0.2/man3/EVP_DigestInit.html to use the sha1() method */
void calculate_sha(unsigned char *input_data, int data_size, unsigned char *sha1_data) {
    EVP_MD_CTX *ctx; //context
    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len;
    int i = 0;

    //calc SHA1
    ctx = EVP_MD_CTX_create(); 
    if (0 == EVP_DigestInit_ex(ctx, EVP_sha1(), NULL))  // setup context for SHA1 hash
    {
        printf("Failed to initialize SHA1 hash\n");
        exit(1);
    }
    if (0 == EVP_DigestUpdate(ctx, input_data, data_size)) // hashes the data into context
    {
        printf("SHA1 hash failed\n");
        exit(1);
    }
    // finalize the hash
    EVP_DigestFinal_ex(ctx, md_value, &md_len); 
    /*
    printf("SHA1 CALC:");
    for (i = 0; i < md_len; i++)
    {
        printf("%02x", md_value[i]);
    }
    printf("\n");
    */
    memcpy(sha1_data,md_value,20);
    EVP_MD_CTX_destroy(ctx); 
}


void handleErrors(void)
{
    ERR_print_errors_fp(stderr);
    abort();
}

/* Encryption provided by the sample code */
int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
            unsigned char *iv, unsigned char *ciphertext){
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
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv))
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

/* Decryption provided from the sample code */
int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
            unsigned char *iv, unsigned char *plaintext)
{
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();


    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv))
        handleErrors();

    if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        handleErrors();
    plaintext_len = len;


    if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
        handleErrors();
    plaintext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return plaintext_len;
}

