
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "data.h"

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/evp.h>

/* 
	Test whether the file exists.
	Return: 
		true if exist a file with filename 
		false otherwise
*/
int fileExists(char* filename) {
	struct stat buffer;
	return (stat(filename, &buffer) == 0);
}






int main(int argc, char** argv) {
	
	// to create and output 
	FILE *fp;
	char *filename = NULL;

	// to parse cmd argument
	int option;
	int nflag = 0;

	// if there are not enough arguments, return error code
	if (argc < 2) {
		printf("Example Usage: setup -N [gradebookName]\n");
		return(255);
	}

	/* PARSING THE CMD ARGUMENTS */

	while ( (option = getopt(argc, argv, "N:")) != -1 ) {
		switch (option) {

			case 'N':
				nflag = 1;
				filename = optarg;
				break;

			case '?':
				if (optopt == 'N') {
					fprintf (stderr, "Option -%c needs argument: [gradebookName]\n", optopt);
				} else {
					fprintf(stderr, "Unknown option -%c\n", optopt);
				}
				break;

			default:
				fprintf(stderr, "getopt");
		}
	}

	// check if file already existed
	if (!fileExists(filename) && nflag == 1) {

		fp = fopen(filename, "w");
	
	} else if ( fileExists(filename) ){

		printf("Invalid\n");
		return(255);

	}

	// check if create file successful.
	if (fp == NULL) {
		printf("Invalid\n");
		return(255);
	}

	fclose(fp);

	/* GENERATING THE SECURED KEY */

	unsigned char *key = (unsigned char *) malloc(sizeof(unsigned char) * KEY_LEN);
	fp = fopen("/dev/urandom", "r");
	fread(key, sizeof(unsigned char) * KEY_LEN, 1, fp);
	fclose(fp);

	// printing the key
	printf("\nPlease store the key locally for further usage.\n");
	printf("Key is: "); for(int i=0; i<KEY_LEN; ++i) { printf("%02x", key[i]); } printf("\n\n");


	return(0);

}







/* SCRATCH CODE */
// /* Generate a random string to act as a password to generate key and iv */
// void rand_str(char *dest, size_t length) {
// 	srand(time(0));
//     char charset[] = "0123456789"
//                      "abcdefghijklmnopqrstuvwxyz"
//                      "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

//     while (length-- > 0) {
//         size_t index = (double) rand() / RAND_MAX * (sizeof charset - 1);
//         *dest++ = charset[index];
//     }
//     *dest = '\0';
// }


// // generating a random password to generate the key and IV with
// rand_str(pass, sizeof(pass) - 1);
// /* Using openssl to generate key from the randomly generated password above */
// const EVP_CIPHER *cipher;
//    const EVP_MD *dgst = NULL;
//    unsigned char key[EVP_MAX_KEY_LENGTH], iv[EVP_MAX_IV_LENGTH];
//    const char *password = pass;
//    const unsigned char *salt = NULL;
//    int i;
//    OpenSSL_add_all_algorithms();
//    cipher = EVP_get_cipherbyname("aes-128-cbc");
//    if(!cipher) { fprintf(stderr, "no such cipher\n"); return 1; }
//    dgst=EVP_get_digestbyname("md5");
//    if(!dgst) { fprintf(stderr, "no such digest\n"); return 1; }

//    if(!EVP_BytesToKey(cipher, dgst, salt,
//        (unsigned char *) password,
//        strlen(password), 1, key, iv))
//    {
//        fprintf(stderr, "EVP_BytesToKey failed\n");
//        return 1;
//    }

//    printf("Key: "); for(i=0; i<cipher->key_len; ++i) { printf("%02x", key[i]); } printf("\n");
//    printf("IV: "); for(i=0; i<cipher->iv_len; ++i) { printf("%02x", iv[i]); } printf("\n");

// printf("Key is: %s\n", pass);