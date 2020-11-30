#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/rand.h>
#include <openssl/aes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>   // stat
#include <stdbool.h>    // bool type

bool file_exists (char *filename) {
  struct stat   buffer;   
  return (stat (filename, &buffer) == 0);
}

typedef struct _File_info {
    char iv[16];
    char enc_file[60];
    char filename[60];
    int ciphertext_len;
}File_info;

void handleErrors(void)
{
    ERR_print_errors_fp(stderr);
    abort();
}

int encrypt(FILE *fp, FILE *f_enc, char *key, char *iv, char *enc_file) {
	f_enc = fopen(enc_file, "wb+");
	//Get file size
    fseek(fp, 0L, SEEK_END);
    int fsize = ftell(fp);
    //set back to normal
    fseek(fp, 0L, SEEK_SET);
    //printf("%d\n", fsize);

    int outLen1 = 0; int outLen2 = 0;
    unsigned char *indata = malloc(fsize); //plaintext
    unsigned char *outdata = malloc(fsize * 2); //cipher
    fread(indata,sizeof(char),fsize, fp);
    int ciphertext_len;
    

    //Set up encryption
    EVP_CIPHER_CTX ctx;
    if (1 != EVP_EncryptInit(&ctx, EVP_aes_128_cbc(), key, iv)) {
    	exit(255);
    }
    if (1 != EVP_EncryptUpdate(&ctx, outdata, &outLen1, indata, fsize) ){
    	exit(255);
    }
    if (1 != EVP_EncryptFinal(&ctx,outdata + outLen1,&outLen2)) {
    	exit(255);
    }
    fwrite(outdata,sizeof(char),outLen1 + outLen2,f_enc); 
    fclose(fp);
    fclose(f_enc);
    return outLen1 + outLen2;
}

int main(int argc, char* argv[]) {
	 if (argc != 3){
        printf("Invalid Input %d\n", argc);
        exit(255);
     }  

    if (file_exists(argv[2])){
        printf("File already exists\n");
        exit(255);
    }
    else{
        //printf("%s does not exist\n", argv[1]);
        printf("Creating: %s\n", argv[2]);
        FILE *of, *fp, *fe;
        of = fopen(argv[2], "w+");
        fclose(of);

        unsigned char key[32];
		RAND_bytes(key, 32);
		unsigned char iv[16];
		RAND_bytes(iv, 16);

        int i;
        printf("Key value: ");
        for (i = 0; i < 32; i ++) {
            printf("%02x", key[i]);
        }
        printf("\n");

  //       of = fopen("fileinfo", "a+");
  //       char enc_file[60] = {"encrypt_"};
		// strcat(enc_file, argv[2]);
		// fprintf(of, "%s %s ", argv[2], enc_file);
		
		// for (i = 0; i < 16; i ++) {
		// 	fprintf(of, "%02x", iv[i]);
		// }
		// fprintf(of, "%s\n", "");
		// fclose(of);
		

  //       File_info f;
  //       char enc_file[60] = {"encrypt_"};
  //       strcat(enc_file, argv[2]);
  //       strcpy(f.filename, argv[2]);
  //       strcpy(f.enc_file, enc_file);

		// fp = fopen(argv[2], "rb");
		// int ciph_len = encrypt(fp, fe, key, iv, enc_file);

  //       of = fopen("fileinfo", "a+");
  //       f.ciphertext_len = ciph_len;
  //       fwrite(&f, sizeof(f), 1, of);
  //       fclose(of);
		//printf("finished\n");
    }
    return 0;

}