#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>
#include <ctype.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#define DEBUG
void handleErrors(void)
{
    ERR_print_errors_fp(stderr);
    abort();
}

int encryptKey(unsigned char *plaintext, int plaintext_len, unsigned char *key,
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


/* test whether the file exists */
int file_test(char* filename) {
  
  struct stat buffer;
  return (stat(filename, &buffer) == 0);
}

int is_alphanumeric(char* str) {
  for(int i = 0; i<strlen(str); i++){
    if(!(isalnum(str[i]) || str[i]=='.' || str[i]=='_')) {
      // printf("Not alphanumeric\n");
      return 0;
    }
  }
  return 1;
}

int main(int argc, char** argv) {
  FILE *fp;
  unsigned int seed = 0x00feeb00;
  char key [17];
  if (argc < 3) {
    printf("Error: Incorrect Number of Arguments\n");
    return(255);
  }
  if(strcmp(argv[1],"-N")) {
    printf("Error: Incorrect Option\n");
    return(255);
  }

  if (!file_test(argv[2])) {
   
    printf("Created File Named %s\n", argv[2]);
  } 
  else {
    printf("invalid\n");
    printf("File Already Exists!\n");
    return (255);
  }
  if(!is_alphanumeric(argv[2])){
    printf("invalid\n");
    return (255);
  }
  fp = fopen(argv[2], "w");

  if (fp == NULL){
    printf("setup: fopen() errors could not create file\n");
    printf("invalid\n");
    return(255);
  }

  while(strlen(key)!=16){
  char hex [9];

  RAND_bytes((unsigned char*)&seed, sizeof(seed));
  sprintf(hex, "%x", seed);
  for(int i = 0; i<8; i++) {
    key[i] = hex[i];
  }

  RAND_bytes((unsigned char*)&seed, sizeof(seed));
  sprintf(hex, "%x", seed);
  for(int i = 8; i<16; i++) {
    key[i] = hex[i-8];
  }
  key[16] = '\0';
}
/* add your code here */

  fclose(fp);

  printf("Key is: %s\n", key);


    /* A 128 bit IV */
    unsigned char *iv = (unsigned char *)"0000000000000000";

    unsigned char ciphertext[128];

    /* Buffer for the decrypted text */
    unsigned char decryptedtext[128];

    int decryptedtext_len, ciphertext_len;

    /* Encrypt the plaintext */
    ciphertext_len = encryptKey (key, strlen (key), key, iv,
                              ciphertext);
    char keyfileName[strlen(argv[2])+8+1];

    strcpy(keyfileName, "key-file");

    for(int i = 0; i<strlen(argv[2]); i++){
      keyfileName[i+8] = argv[2][i];
    }
    keyfileName[strlen(argv[2])+8] = '\0';
    // printf("%s\n", keyfileName);
    FILE* kfp = fopen(keyfileName, "w+");
    fputs(ciphertext, kfp);
    fclose(kfp);
    // printf("%s\n", ciphertext);

  return(0);

}
