#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "data.h"

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/comp.h>


void encrypt(char *filename, unsigned char *key, unsigned char *iv) {
	char buffer[2000]="";
	char iv_hex[200]="";

	for(int i=0; i<KEY_LEN; ++i) {
		char hex[20];
		sprintf(hex, "%02x", iv[i]);
		strcat(iv_hex, hex);
	} 

	// embed the iv to the first 16 bytes of the cipher
	strcat(buffer, iv_hex);

	// read the entire file into another buffer
	FILE *infile = fopen(filename, "r");
	char *plaintext;
	long numbytes;

	if (infile == NULL) {
		printf("invalid\n");
		return;
	}

	fseek(infile, 0L, SEEK_END);
	numbytes = ftell(infile);

	fseek(infile, 0L, SEEK_SET);

	plaintext = (char*)calloc(numbytes, sizeof(char)); 

	if(plaintext == NULL)
		return;

	fread(plaintext, sizeof(char), numbytes, infile);
	fclose(infile);

	printf("%s\n", plaintext);

	// perform encryption on the plaintext
	unsigned char outbuf[1024 + EVP_MAX_BLOCK_LENGTH];
	int outlen, tmplen;

	EVP_CIPHER_CTX *ctx;
	ctx = EVP_CIPHER_CTX_new();

	EVP_EncryptInit_ex(ctx, EVP_aes_128_ofb(), NULL, key, iv);

	if (!EVP_EncryptUpdate(ctx, outbuf, &outlen, plaintext, strlen(plaintext))) {
		EVP_CIPHER_CTX_free(ctx);
		return 0;
	}

	if (!EVP_EncryptFinal_ex(ctx, outbuf + outlen, &tmplen)) {
			EVP_CIPHER_CTX_free(ctx);
			return 0;
	}		

	// obtain the final length of the cipher
	outlen += tmplen;

	EVP_CIPHER_CTX_free(ctx);

	// Format the cipher appropriately to compare with the given cipher
	int j;

	/* since the program returns 32 raw bytes to outbuf,
	we will have to turn them into 64 displayable bytes */
	// malloc buf_str as the string to compare and a pointer to that string
	char *buf_str = (char *) malloc(2 * outlen + 1);
	char *buf_ptr = buf_str;

	// for every byte in outbuf, write it to buf_str as the form 2 hex bits
	for (j = 0; j < outlen; j++) {
		buf_ptr += sprintf(buf_ptr, "%02x", outbuf[j]);
	}

	// append the terminating character at the end of buf_str
	*(buf_ptr+1) = "\0";

	// write the ciphertext to a file
	strcat(buffer, buf_str);
	FILE *outfile = fopen("mygradebook_cipher", "w");
	fprintf(outfile, "%s\n", buffer);

}

static unsigned char gethex(const char *s, char **endptr) {
  assert(s);
  while (isspace(*s)) s++;
  assert(*s);
  return strtoul(s, endptr, 16);
}

unsigned char *convert(const char *s, int *length) {
  unsigned char *answer = malloc((strlen(s) + 1) / 3);
  unsigned char *p;
  for (p = answer; *s; p++)
    *p = gethex(s, (char **)&s);
  *length = p - answer;
  return answer;
}

void decrypt(char *filename, unsigned char *key) {

	// read the entire file into another buffer
	FILE *infile = fopen(filename, "r");
	char *ciphertext;
	long numbytes;

	if (infile == NULL) {
		printf("invalid\n");
		return;
	}

	fseek(infile, 0L, SEEK_END);
	numbytes = ftell(infile);

	fseek(infile, 0L, SEEK_SET);

	ciphertext = (char*)calloc(numbytes, sizeof(char)); 

	if(ciphertext == NULL)
		return;

	fread(ciphertext, sizeof(char), numbytes, infile);
	fclose(infile);



	// extract the IV from the ciphertext
	unsigned char *iv = (unsigned char *) malloc(sizeof(unsigned char) * KEY_LEN);
	size_t i, n = strlen(ciphertext) / 2;
	char iv_hex[32];
	strncpy(iv_hex, ciphertext, 32);
	char *pos = iv_hex;
  

  for (size_t count = 0; count < sizeof(iv)/sizeof(*iv); count++) {
      sscanf(pos, "%2hhx", &iv[count]);
      pos += 2;
  }

  // getting the actual cipher text
  pos = ciphertext;
  pos += 32;

  unsigned char inbuf[1024 + EVP_MAX_BLOCK_LENGTH];
  for (size_t count = 0; count < sizeof(outbuf)/sizeof(*inbuf); count++) {
      sscanf(pos, "%2hhx", &inbuf[count]);
      pos += 2;
  }

}

int main() {
	FILE *fp;
	unsigned char *key = (unsigned char *) malloc(sizeof(unsigned char) * KEY_LEN);
	unsigned char *iv = (unsigned char *) malloc(sizeof(unsigned char) * KEY_LEN);
	fp = fopen("/dev/urandom", "r");
	fread(key, sizeof(unsigned char) * KEY_LEN, 1, fp);
	fread(iv, sizeof(unsigned char) * KEY_LEN, 1, fp);
	fclose(fp);

	decrypt("mygradebook_cipher", key);
}