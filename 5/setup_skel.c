#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include "data.h"

#define DEBUG

/* test whether the file exists */
int file_test(char* filename) {
  struct stat buffer;
  return (stat(filename, &buffer) == 0);
}

int main(int argc, char** argv) {
  //Make sure 3 arguments are given (executable, -N and name of gradebook)
  if (argc != 3) {
    printf("Usage: setup <logfile pathname>\n");
    return(255);
  }
  if(file_test(argv[2]) != 0 || strcmp(argv[1], "-N") != 0) {
    printf("invalid\n");
    return(255);
  }
  if(strstr(argv[2], ";") != 0){
    printf("invalid\n");
    return(255);
  }

  if(strlen(argv[2]) > 100){
    printf("invalid (gradebook length > 100 characters)\n");
    return(255);
  }
  //string to be outputted to the file
  unsigned char out[16+64+sizeof(int)];

  //IV used for decryption later on
  unsigned char iv[16];
  //Copy 16 random bytes into IV and copy to output string
  RAND_bytes(iv, 16);
  memcpy(out, iv, 16);

  //Generate 32 random bytes that will be the key
  unsigned char key_hex[33];
  RAND_bytes(key_hex, 32);
  key_hex[32] = '\0';

  unsigned char key_str[32] = {0};
  int i;
  //Prints out the 32 bytes to a string
  for(i = 0; i < 32; i++){
    sprintf(key_str+i, "%x", key_hex[i]);
  }
  key_str[31] = '\0';
  printf("Key is: %s\n", key_str);

  //Hash the secret key
  unsigned char hash[SHA_DIGEST_LENGTH] = {0};
  SHA1(key_str, strlen(key_str), hash);
  int s = SHA_DIGEST_LENGTH;
  //copy the size of the hash (20 bytes) to the output string
  memcpy(out+16, &s, sizeof(int));
  //copy the hashed key itself to the file
  memcpy(out+16+sizeof(int), hash, SHA_DIGEST_LENGTH);
  
  //Open new gradebook file
  FILE *fp = fopen(argv[2], "w");

  if (fp == NULL){
    #ifdef DEBUG
        printf("setup: fopen() error could not create file\n");
    #endif
    printf("invalid\n");
    return(255);
  }
  //Write the IV, size of hashed key and hashed key to the file
  fwrite(out, 1, 16+sizeof(int)+SHA_DIGEST_LENGTH, fp);
  fclose(fp);

  return(0);

}
