#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/conf.h>
#include <openssl/evp.h>

//#define DEBUG

/* test whether the file exists */
int file_test(char* filename) {
  struct stat buffer;
  return (stat(filename, &buffer) == 0);
}

void handleErrors(void)
{
    ERR_print_errors_fp(stderr);
    abort();
}

int get_mac(unsigned char *ciphertext, int ciphertext_len, unsigned char *key, unsigned char *outputMac){
 	EVP_CIPHER_CTX *ctx;

    int len;
  	unsigned char iv[16]={0};
  	int total_len;
    int mac_len =16;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv))
        handleErrors();

    /*
     * Provide the ciphertext to MAC, and obtain the encrypted output.
     * EVP_EncryptUpdate can be called multiple times if necessary
     */
    if(1 != EVP_EncryptUpdate(ctx, outputMac, &len, ciphertext, ciphertext_len))
        handleErrors();
    total_len = len;

    /*
     * Finalise the encryption. Further MAC bytes may be written at
     * this stage.
     */
    if(1 != EVP_EncryptFinal_ex(ctx, outputMac + len, &len))
        handleErrors();
    total_len += len;

    /* The Final MAC is the last 128 bits (16 chars)*/

    memmove(outputMac,outputMac+total_len-mac_len,mac_len);

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

	return mac_len;
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

int main(int argc, char** argv) {
  FILE *fp;
  unsigned char key[16] ={0};
  unsigned char *plaintext =(unsigned char *) "numStudents 0 numAssignments 0 ";
  unsigned char cipher[128+16];	// 16 byte IV concatenated with the ciphertext
  unsigned char *iv=cipher;
  int iv_len = 16;
  unsigned char *ciphertext = cipher+iv_len;
  int cipher_len;
  unsigned char mac[128];	// needs to be 128, but only use first 16 bytes
  int mac_len =16;
  struct stat buffer;
  
  // Check usage
  if (argc < 3) {
    //printf("Usage: setup -N <logfile pathname>\n");
    printf("invalid\n");
    return(255);
  }
  if (argc>3 || strcmp("-N",argv[1])!=0){
    //printf("Usage: setup -N <logfile pathname>\n");
    printf("invalid\n");
    return(255);  	
  }

  // If File Exists, invalid
  if(stat(argv[2], &buffer) == 0){
    printf("invalid\n");
    return (255);
  }

  fp = fopen(argv[2], "w");
  if (fp == NULL){
	#ifdef DEBUG
    	printf("setup: fopen() error could not create file\n");
	#endif
    printf("invalid\n");
    return(255);
  }

  #ifdef DEBUG
  	if (file_test(argv[2]))
   	 printf("created file named %s\n", argv[2]);
  #endif

  // Generate the cryptographically secure random key and IV
  if (!RAND_bytes(key, sizeof key)) {
    printf("key generation error: could not generate a key\n");
    return (0);
  }
  if (!RAND_bytes(cipher, iv_len*8)) {
    printf("IV generation error: could not generate an IV\n");
  }

  // Encrypt the plaintext
  cipher_len = encrypt (plaintext, strlen ((char *)plaintext), key, iv, ciphertext); // iv, ciphertext
  #ifdef DEBUG
  	printf("cipher len: %d\n",cipher_len);
  	printf("iv and cipher:\n");
  	BIO_dump_fp (stdout, (const char *)cipher, cipher_len+iv_len);
  #endif

  // MAC the IV and ciphertext
  mac_len = get_mac(cipher, iv_len+cipher_len, key, mac);
  #ifdef DEBUG
  	printf("mac:\n");
  	BIO_dump_fp (stdout, (const char *)mac, mac_len);
  #endif

  // Print MAC to file
  for(int i=0;i<mac_len;i++){
  	fprintf(fp,"%c",mac[i]);  	
  }
  // Print IV and ciphertext to file
  for (int i=0; i<cipher_len+iv_len;i++){
  	  fprintf(fp,"%c",cipher[i]);
  }
  fclose(fp);

// print the key to stdout
  #ifdef DEBUG
	  printf("key:\n");
  #endif
  for (int i = 0; i <sizeof(key); i++){
    printf("%02x", key[i]);
  }
  printf("\n");

  return(0);

}
