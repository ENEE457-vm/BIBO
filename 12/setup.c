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
#include <stdbool.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "data.h"

#define DEBUG

int main(int argc, char** argv) {
	EncryptedGradebook enc_gradebook = {0};
	DecryptedGradebook dec_gradebook = {0};
	FILE *fp;
	unsigned char key[KEY_SIZE];
	unsigned char iv[IV_SIZE];
	unsigned char tag[MAC_SIZE];
	int i, ret;

	// blacklisting unwanted arguments
	if (argc < 3 || argc > 3) {
		#ifdef DEBUG
		printf("Setup Usage: setup option gradebook_name\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}

	// validating input option
	if (strncmp(argv[1], "-N", 3)) {
		#ifdef DEBUG
    printf("Second argument must be -N\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}

	// checking that gradebook name is valid
  for (i=0; i < strnlen(argv[2], MAX_USER_INPUT_LEN); i++) {
  	if (!(isalnum(argv[2][i]) || argv[2][i] == '.' || argv[2][i] == '_')) {
  		#ifdef DEBUG
			printf("Invalid filename\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
  	}
  	// checking existence of gradebook
  	if (!access(argv[2], F_OK)) {
			#ifdef DEBUG
			printf("Gradebook already exists\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
		}
  }

	// generate random key
	ret = RAND_bytes(key, sizeof(key));
	if (ret == -1) {
		#ifdef DEBUG
		printf("Random key generation failed\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}

	// print key to stdout
	printf("Key is: ");
	for (i=0; i < KEY_SIZE; i++) {
		printf("%02X", key[i]);
	}
	printf("\n");

	// generate random IV
	ret = RAND_bytes(iv, sizeof(iv));
	if (ret == -1) {
		#ifdef DEBUG
		printf("Random IV generation failed\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}

	// encrypt
	memcpy(enc_gradebook.iv, iv, sizeof(iv));
	ret = encrypt((unsigned char *)&dec_gradebook.num_assignments, sizeof(DecryptedGradebook), key, iv, (unsigned char *)&enc_gradebook.encrypted_data);
	if (ret == -1) {
		#ifdef DEBUG
		printf("Encryption failed\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}
	
	// compute the message authentication code
	unsigned char* mac_res; 
	mac_res = HMAC(EVP_md5(), key, sizeof(key), (unsigned char *)&enc_gradebook.iv, sizeof(EncryptedGradebookSize), tag, NULL);

	// store tag in encrypted gradebook
	memcpy(enc_gradebook.tag, tag, sizeof(tag));
	
	// open encrypted gradebook
	fp = fopen(argv[2], "w");
	if (fp == NULL) {
		#ifdef DEBUG
		printf("Could not create file\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}
	
	// write encrypted gradebook to file
	fwrite(enc_gradebook.iv, 1, sizeof(EncryptedGradebook), fp);
	fflush(fp);
  fclose(fp);
	return 0;
}

