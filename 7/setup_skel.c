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
#include <openssl/evp.h>
#include "data.h"

#define DEBUG

/* test whether the file exists */
int file_test(char* filename) {
  struct stat buffer;
  int res = stat(filename, &buffer);
  return (res == -1);
}

/*void handleErrors(void){    
	ERR_print_errors_fp(stderr);    
	abort();
}*/
/*
int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,            unsigned char *iv, unsigned char *plaintext){    
	EVP_CIPHER_CTX *ctx;    
	int len;    
	int plaintext_len;    /* Create and initialise the context */    
/*	if(!(ctx = EVP_CIPHER_CTX_new()))        
		handleErrors();    /*     * Initialise the decryption operation. IMPORTANT - ensure you use a key     * and IV size appropriate for your cipher     * In this example we are using 256 bit AES (i.e. a 256 bit key). The     * IV size for *most* modes is the same as the block size. For AES this     * is 128 bits     */    
/*	if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv))        
		handleErrors();    /*     * Provide the message to be decrypted, and obtain the plaintext output.     * EVP_DecryptUpdate can be called multiple times if necessary.     */    
/*	if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
		handleErrors();    
	plaintext_len = len;    
	/*     * Finalise the decryption. Further plaintext bytes may be written at     * this stage.     */    
/*	if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))        
		handleErrors();    
	plaintext_len += len;    /* Clean up */    
/*	EVP_CIPHER_CTX_free(ctx);    
	return plaintext_len;
}*/

int main(int argc, char** argv) {
  FILE *fp;
  unsigned char keys[17];
  unsigned char ivs[17];
  if (argc < 2) {
    printf("Usage: setup <logfile pathname>\n");
    return(255);
  }

  if (strlen(argv[2]) > 100){
	  printf("Invalid name\n");
          return(255);
  }

#ifdef DEBUG
  if (file_test(argv[2])){
    //printf("creating file named %s\n", argv[2]);
  }
  else {
    printf("invalid\n");
    return(255);
  }
#endif

  fp = fopen(argv[2], "w");
  if (fp == NULL){
#ifdef DEBUG
    printf("setup: fopen() error could not create file\n");
#endif
    printf("invalid\n");
    return(255);
  }
  Gradebook g;
  //strcpy(g.name, argv[2]);
  //g.name[101] = '\0';
/* add your code here */
  int i = 0;
  //for (i = 0; i < 32; i++){
  RAND_bytes(keys, sizeof(keys));
  RAND_bytes(ivs, sizeof(ivs));
  keys[16] = '\0';
  ivs[16] = '\0';
  unsigned char *iv = ivs;
  unsigned char *key = keys;
  unsigned char *plaintext, *ciphertext;
  unsigned char cipher[256];
  ciphertext = cipher;
  int plaintext_len, len, ciphertext_len;
  plaintext = keys;
//  EVP_CIPHER_CTX *ctx;
  plaintext_len = strlen((char *)plaintext);
  /*if(!(ctx = EVP_CIPHER_CTX_new())){
	  ERR_print_errors_fp(stderr);
	  abort();
  }
  if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv)){
	  ERR_print_errors_fp(stderr);
	  abort();
  }
  if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len)){
	  ERR_print_errors_fp(stderr);
	  abort();
  }
  ciphertext_len = len;
  if (1 != EVP_EncryptFinal_ex(ctx, ciphertext+len, &len)){
	  ERR_print_errors_fp(stderr);
	  abort();
  }
  ciphertext_len += len;
  
  EVP_CIPHER_CTX_free(ctx);*/
  //printf("Length: %d\n", ciphertext_len);
  for (i = 0; i<16; i++){
	  fprintf(fp, "%.2x", iv[i]);
  }
  fprintf(fp, "\n");
  for (i = 0; i< 16; i++){
          fprintf(fp, "%.2x", key[i]);
  }
  /*for (i = 0; i<ciphertext_len; i++){
	  fprintf(fp, "%.2x", ciphertext[i]);
  */
 fprintf(fp, "\n");
  fclose(fp);
  //printf("Name is : %s\n", g.name);
  printf("Key is: ");
  int length = strlen(key);
  for (i = 0; i<16; i++){
	  printf("%.2x",key[i]);
  }
  printf("\n");
  
  return(0);

}
