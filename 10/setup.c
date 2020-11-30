#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <unistd.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

struct arguments {
	char * n; //name
};

void handleErrors(void)
{
	ERR_print_errors_fp(stderr);
	abort();
}

int validate_name(char * name) {
	for (int i = 0; i < strlen(name); i++) {
		if (!(isalnum(name[i]) || name[i] == '_' || name[i] == '.')) {
			return -1;
		}
	}
	return 0;
}

int gcm_encrypt(unsigned char *plaintext, int plaintext_len,
				unsigned char *key,
				unsigned char *iv, int iv_len,
				unsigned char *ciphertext,
				unsigned char *tag)
{
	EVP_CIPHER_CTX *ctx;
	int len;
	int ciphertext_len;


	/* Create and initialise the context */
	if(!(ctx = EVP_CIPHER_CTX_new()))
		handleErrors();

	/* Initialise the encryption operation. */
	if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL))
		handleErrors();

	/*
	 * Set IV length if default 12 bytes (96 bits) is not appropriate
	 */
	if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL))
		handleErrors();

	/* Initialise key and IV */
	if(1 != EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv))
		handleErrors();

	/*
	 * Provide the message to be encrypted, and obtain the encrypted output.
	 * EVP_EncryptUpdate can be called multiple times if necessary
	 */
	if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
		handleErrors();
	ciphertext_len = len;

	/*
	 * Finalise the encryption. Normally ciphertext bytes may be written at
	 * this stage, but this does not occur in GCM mode
	 */
	if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
		handleErrors();
	ciphertext_len += len;

	/* Get the tag */
	if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag))
		handleErrors();

	/* Clean up */
	EVP_CIPHER_CTX_free(ctx);

	return ciphertext_len;
}

int gcm_decrypt(unsigned char *ciphertext, int ciphertext_len,
				unsigned char *tag,
				unsigned char *key,
				unsigned char *iv, int iv_len,
				unsigned char *plaintext)
{
	EVP_CIPHER_CTX *ctx;
	int len;
	int plaintext_len;
	int ret;

	/* Create and initialise the context */
	if(!(ctx = EVP_CIPHER_CTX_new()))
		handleErrors();

	/* Initialise the decryption operation. */
	if(!EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL))
		handleErrors();

	/* Set IV length. Not necessary if this is 12 bytes (96 bits) */
	if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL))
		handleErrors();

	/* Initialise key and IV */
	if(!EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv))
		handleErrors();

	/*
	 * Provide the message to be decrypted, and obtain the plaintext output.
	 * EVP_DecryptUpdate can be called multiple times if necessary
	 */
	if(!EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
		handleErrors();
	plaintext_len = len;

	/* Set expected tag value. Works in OpenSSL 1.0.1d and later */
	if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag))
		handleErrors();

	/*
	 * Finalise the decryption. A positive return value indicates success,
	 * anything else is a failure - the plaintext is not trustworthy.
	 */
	ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);

	/* Clean up */
	EVP_CIPHER_CTX_free(ctx);

	if(ret > 0) {
		/* Success */
		plaintext_len += len;
		return plaintext_len;
	} else {
		/* Verify failed */
		return -1;
	}
}

int main(int argc, char *argv[]) {
	int c;
	struct arguments args;
	args.n = NULL;

	static struct option long_options[] = {
		{"N", required_argument, 0, 'a'},
		{0, 0, 0, 0},
	};

	int option_index = 0;
	while((c = getopt_long_only(argc, argv, "a:", long_options, &option_index)) != -1) {
		switch(c) {
			case 'a':
				if (optarg) {
					if (validate_name(optarg) == 0) {
						args.n = malloc(strlen(optarg));
						strcpy(args.n, optarg);
					} else {
						printf("invalid name\n");
						exit(-1);
					}
				} else {
					printf("option requires an argument\n");
					exit(-1);
				}
				break;
			default:
				printf("invalid input\n");
				exit(-1);
				break;
		}
	}

	if (!args.n) {
		printf("invalid\n");
		exit(255);

	}
	if (access(args.n, F_OK) != -1) {
		 printf("invalid\n");
		 exit(255);
	}


	int num_students = 1;
	int num_assignments = 0;
	double tot_weight = 0.0;
	char * first = "xxx";
	int flen = strlen(first) + 1;
	char * last = "xxx";
	int llen = strlen(last) + 1;
	double grade = -1.0;

	int size = sizeof(int) + sizeof(int) + sizeof(double) + strlen(first) + strlen(last) + 2 + sizeof(int) + sizeof(int) + sizeof(double);
	uint8_t buf[size];
	uint8_t * ptr = buf;
	memcpy(ptr, &num_students, sizeof(int));
	ptr += sizeof(int);
	memcpy(ptr, &num_assignments, sizeof(int));
	ptr += sizeof(int);
	memcpy(ptr, &tot_weight, sizeof(double));
	ptr += sizeof(double);
	memcpy(ptr, &flen, sizeof(int));
	ptr += sizeof(int);
	memcpy(ptr, first, strlen(first) + 1);
	ptr += strlen(first) + 1;
	memcpy(ptr, &llen, sizeof(int));
	ptr += sizeof(int);
	memcpy(ptr, last, strlen(last) + 1);
	ptr += strlen(last) + 1;
	memcpy(ptr, &grade, sizeof(double));


	unsigned char ciphertext[size];
	FILE * randfp;
	randfp = fopen("/dev/urandom", "r");
	unsigned char tag[16];
	unsigned char iv[16];
	unsigned char key[32];
	fread(&key, 32, 1, randfp);
	fread(&iv, 16, 1, randfp);
	fclose(randfp);
	int cipher_size = gcm_encrypt(buf, size, key, iv, 16, ciphertext, tag);
	if (cipher_size == 0) {
		free(args.n);
		exit(255);
	}
	char final[32 + cipher_size];
	char * fptr = final;
	memcpy(fptr, iv, 16);
	fptr += 16;
	memcpy(fptr, tag, 16);
	fptr += 16;
	memcpy(fptr, ciphertext, cipher_size);

	FILE *fp;
	fp = fopen(args.n, "w");
	fwrite(final, cipher_size + 32, 1, fp);

	fclose(fp);
	for (int i = 0; i < 32; i++) {
		printf("%02x", key[i]);
	}
	printf("\n");
	free(args.n);
	return 0;
}