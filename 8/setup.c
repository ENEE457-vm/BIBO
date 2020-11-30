#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>
#include "gradebookobjects.h"

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#define DEBUG

/* test whether the file exists */
int file_test(char* filename) {
  struct stat buffer;
  return (stat(filename, &buffer) == 0);
}

/* return whether the input only contains alphanumeric characters, _, and . */
int is_valid_filename(char* filename) {
  if (strlen(filename) > 50) {
    return 0;
  }
  for (int i = 0; i < strlen(filename); i++) {
    if (!isalnum(filename[i]) && filename[i] != '_' && filename[i] != '.')
      return 0;
  }
  return 1;
} 

/* crypto functions */
void handleErrors(void)
{
    ERR_print_errors_fp(stderr);
    abort();
}

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
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv))
        handleErrors();

    /*
     * Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary.
     */
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


/* ------------- HELPER FUNCTIONS END ------------- */


int main(int argc, char** argv) {
  FILE *fp;

  /* Not enough arguments were provided */
  if (argc < 3) {
    printf("Usage: setup <logfile pathname>\n");
    return(255);
  }

  if (file_test(argv[2])) {
    /* testing read from encrypted file here 
    unsigned char read_ciphertext[sizeof(Gradebook) + 32];
    unsigned char readIV[16];
    int c_len;
    unsigned char read_tag[16];
    FILE *file = fopen(argv[2], "rb");
    if (file != NULL) {
      fread(&read_ciphertext, sizeof(read_ciphertext), 1, file);
      fread(&readIV, sizeof(readIV), 1, file);
      fread(&read_tag, sizeof(read_tag), 1, file);
      fread(&c_len, sizeof(int), 1, file);
      fclose(file);
    }
    unsigned char key[16] = {0xC5,0xAB,0xD7,0x9F,0xCD,0xD3,0x0B,0x21,0xEB,0x3F,0x29,0x9C,0x0F,0x64,0x8B,0x39};
    unsigned char hmac_key[16] = {0x44,0x8C,0xFF,0xB3,0x51,0xBA,0xD5,0x39,0xA5,0x04,0x75,0x6D,0x96,0xCC,0xEE,0xDB};
    unsigned char *computed_tag = NULL;
    unsigned int computed_result_len = -1;
    computed_tag = HMAC(EVP_md5(), hmac_key, sizeof(hmac_key), read_ciphertext, c_len, computed_tag, &computed_result_len);
    if (memcmp(computed_tag, read_tag, 16) == 0) {
      printf("Tag is the same\n");
    } else {
      printf("uh oh\n");
    }

    Gradebook result;
    int decryptedtext_len = decrypt(read_ciphertext, c_len, key, readIV, result.name);
    printf("%s\n", result.name);
    printf("%s\n", result.students[0].first_name);
    /* ending read from encrypted file here */

    /* A gradebook with the provided name is already in the directory */
    printf("invalid\n");
    return(255);
  } else if (!is_valid_filename(argv[2])) {
    /* The provided filename was not alphanumeric */
    printf("invalid\n");
    return(255);
  } else if (strcmp(argv[0], "setup") != 0 || strcmp(argv[1], "-N") != 0) {
    /* The arguments were not provided in the proper order */
    printf("invalid\n");
    return(255);
  } else {
    /* The provided arguments were valid. Create a new file. */
    fp = fopen(argv[2], "w");

    /* Check if the file creation was successful */
    if (fp == NULL){
      #ifdef DEBUG
          printf("setup: fopen() error could not create file\n");
      #endif
      printf("invalid\n");
      return(255);
    }

    /* Notify the user a new gradebook was created */
    #ifdef DEBUG
      if (file_test(argv[2]))
        printf("created file named %s\n", argv[2]);
    #endif

    /* Create new gradebook corresponding to the filename */
    Gradebook new_gb;
    strcpy(new_gb.name, argv[2]);
    new_gb.num_assignments = 0;
    new_gb.num_students = 0;

    /* Generate random 128 bit IV  */
    unsigned char iv_buffer[16]; /* 128 bit IV -> 16 byte IV */
    int iv_rc = RAND_bytes(iv_buffer, sizeof(iv_buffer));
    unsigned long iv_err = ERR_get_error();
    if(iv_rc != 1) {
        printf("invalid\n");
        return(255);
    }

    /* Generate random 128 bit key */
    unsigned char buffer[16]; /* 128 bit key -> 16 byte key */
    int rc = RAND_bytes(buffer, sizeof(buffer));
    unsigned long err = ERR_get_error();
    if(rc != 1) {
        printf("invalid\n");
        return(255);
    }
    printf("Key is: ");
    for (int i = 0; i < 16; i++) {
        printf("%02X", buffer[i]);
    }

    /* Generate random 128 bit HMAC key */
    unsigned char HMAC_buffer[16]; /* 128 bit key -> 16 byte key */
    int HMAC_rc = RAND_bytes(HMAC_buffer, sizeof(HMAC_buffer));
    unsigned long HMAC_err = ERR_get_error();
    if(HMAC_rc != 1) {
        printf("invalid\n");
        return(255);
    }
    for (int i = 0; i < 16; i++) {
        printf("%02X", HMAC_buffer[i]);
    }
    printf("\n");

    /* Encrypt the gradebook */
    unsigned char ciphertext[sizeof(Gradebook) + 32];
    int ciphertext_len = encrypt(new_gb.name, sizeof(Gradebook), buffer, iv_buffer, ciphertext);

    /* Produce tag with HMAC */
    unsigned char *tag = NULL;
    unsigned int result_len = -1;
    tag = HMAC(EVP_md5(), HMAC_buffer, sizeof(HMAC_buffer), ciphertext, ciphertext_len, tag, &result_len);

    /* Write the tag to a unsigned char array with fixed size */
    unsigned char actual_tag[16];
    for (int i = 0; i < result_len; i++) {
        actual_tag[i] = tag[i];
    }

    /* Write the ciphertext to the gradebook file */
    fwrite(&ciphertext, sizeof(ciphertext), 1, fp);

    /* Write the IV to the gradebook file */
    fwrite(&iv_buffer, sizeof(iv_buffer), 1, fp); 

    /* Write the tag to the gradebook file */
    fwrite(&actual_tag, sizeof(actual_tag), 1, fp);

    /* Write the ciphertext length to the gradebook file */
    fwrite(&ciphertext_len, sizeof(int), 1, fp);

    fclose(fp);

    return(0);
  }

}
