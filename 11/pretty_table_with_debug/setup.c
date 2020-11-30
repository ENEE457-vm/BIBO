#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>
#include <ctype.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "data.h"


// Tests whether filename exists.
int file_test(char* filename) {
  struct stat buffer;
  return (stat(filename, &buffer) == 0);
}


int main(int argc, char** argv) {
  FILE *fp;

  unsigned char key[KEY_SIZE] = {0};
  DecryptedGradebook gradebook = {0};
  EncryptedGradebook encrypted_gradebook = {0};

  int i = 0;
	int ret = 0;

	// Test for 3 arguments
  if (argc != 3) {
    #ifdef DEBUG
    	printf("Usage: setup -N <logfile pathname>\n");
    #endif
    printf("invalid\n");
    return INVALID;
  }

  // Blacklist invalid string lengths.
  if ((strnlen(argv[0], MAX_NAME_INPUT_LEN + 1) >= MAX_NAME_INPUT_LEN + 1) ||
      (strnlen(argv[1], 3) != 2) ||
      (strnlen(argv[2], MAX_NAME_INPUT_LEN + 1) >= MAX_NAME_INPUT_LEN + 1)) {
    #ifdef DEBUG
      printf("Invalid argument lengths [0 to 2].\n"); 
    #endif
    printf("invalid\n");
    exit(INVALID);
  }

  // Validate each input
  if (strncmp(argv[1], "-N", 3)) {
    #ifdef DEBUG
      printf("Invalid argument [1].\n"); 
    #endif
    printf("invalid\n");
    exit(INVALID);
  }

  for (i = 0; i < strnlen(argv[2], MAX_NAME_INPUT_LEN); i++) {
    if (!(isalnum(argv[2][i]) || argv[2][i] == '.' || argv[2][i] == '_')) {
      #ifdef DEBUG
        printf("Invalid argument [2].\n"); 
      #endif
      printf("invalid\n");
      exit(INVALID);
    }
  }

  // Test for pre-existing file
  ret = file_test(argv[2]);
  if (ret != SUCCESS) {
  	#ifdef DEBUG
    	printf("setup: File %s already exists.\n", argv[2]);
		#endif
    printf("invalid\n");
  	return INVALID;
  }

	#ifdef DEBUG
	  if (file_test(argv[2]))
	    printf("Created file named %s\n", argv[2]);
	#endif

	// Generate random key
  ret = generate_random_number(key, sizeof(key));
  if (ret != SUCCESS) {
		#ifdef DEBUG
			printf("Random number generator failed.\n");
		#endif
		return INVALID;
  }

  // Encrypt and authenticate
	ret = enc_and_auth(&gradebook, &encrypted_gradebook, key, sizeof(key));
  if (ret != SUCCESS) {
		#ifdef DEBUG
			printf("Encryption failed.\n");
		#endif
		return INVALID;
  }

  // Open file
  fp = fopen(argv[2], "w");
  if (fp == NULL){
		#ifdef DEBUG
			printf("setup: fopen() error could not create file\n");
		#endif
    printf("invalid\n");
    return INVALID;
  }

  // Write encrypted_gradebook to file.
  fwrite(encrypted_gradebook.iv, 1, sizeof(EncryptedGradebook), fp);
  fflush(fp);
  fclose(fp);

  // Print key.
  printf("Key is: ");
  for (i = 0; i < KEY_SIZE; i++) {
    printf("%02x", key[i]);
  }
  printf("\n");

  return SUCCESS;
}