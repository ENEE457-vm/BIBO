#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "my_data.h"
#include "crypto.c"
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <ctype.h>
#define DEBUG

/* test whether the file exists */
int file_test(char* filename) 
{
	struct stat buffer;
	int exist=stat(filename, &buffer);
	if(exist==0)   //if it exists
		return 1;
	else 
		return 0;
}

unsigned char *mx_hmac_sha256(const void *key, int keylen,
                              const unsigned char *data, int datalen,
                              unsigned char *result, unsigned int *resultlen) {
    return HMAC(EVP_sha256(), key, keylen, data, datalen, result, resultlen);
}

int main(int argc, char** argv) 
{
	FILE *fp;
	int rc, size_fname, i, j, alnum=0;
	Decrypted_gradebook gradebook;
	Encrypted_gradebook e_gradebook;
	Decrypted_gradebook* ptr=&gradebook;
	Decrypted_gradebook d_gradebook;
	Decrypted_gradebook* ptr2=&d_gradebook;
	paddedDecrypted_gradebook p_gb = {0};
	paddedDecrypted_gradebook *p_gb_ptr=&p_gb;
	int encrypt_len=0, decrypt_len=0;
	unsigned char *result=NULL;
	unsigned int resultlen=-1, k;
	unsigned char hash_a[MAC_Size];
	unsigned char key[key_size]={0};
	strcpy(hash_a,"");
	if(argc!=3)
	{
		printf("invalid\n");
    	return(255);
  	}
  	if(strcmp(argv[1],"-N")!=0)   //checks for needed -N option
  	{
  		printf("invalid\n");
  		return(255);
  	}
 	else
 	{
 		size_fname=strlen(argv[2]);
	 	if(size_fname>=30)
	 	{
	 		printf("invalid\n");
	 		return(255);
	 	}
	 	for(i=0; i<size_fname; i++)   //checks for alphanumeric characters
		{
			alnum=isalnum(argv[2][i]);
			if(alnum==0)
			{
				if((argv[2][i]!='.')&&(argv[2][i]!='_'))    //accepts periods and underscores
				{
					printf("invalid\n");
					return(255);
				}
			}
		}
	 }
 	#ifdef DEBUG
		if(file_test(argv[2]))   //check if gradebook file already exists
		{
			printf("invalid\n");
			return(255);
		}
	    else
	    {
	    	printf("Created gradebook named %s\n", argv[2]);
	    } 
	#endif
	fp = fopen(argv[2], "w");
	if (fp == NULL)
	{
		#ifdef DEBUG
		    printf("setup: fopen() error could not create file\n");
		#endif
		    printf("invalid\n");
		    return(255);
	}
	strcpy(gradebook.g_name,"");
	for(i=0; i<max_students; i++)
	{
		strcpy(gradebook.s_array[i].first_name,"");
		strcpy(gradebook.s_array[i].last_name,"");
		for(j=0; j<max_assignments; j++)
		{
			gradebook.s_array[i].grades[j]=0;
		}
	}
	strcpy(e_gradebook.iv,"");  //initialize encrypted gradebook
	strcpy(e_gradebook.encrypted_data,"");
	strcpy(e_gradebook.mac,"");
	memcpy(gradebook.g_name,argv[2], strlen(argv[2])+1);
	rc=RAND_bytes(key,key_size);     //generate key
	rc=RAND_bytes(e_gradebook.iv,IV_Size);    //generate IV
  	printf("Key is: ");
  	for(i=0; i<key_size; i++)
  	{
  		printf("%02x", key[i]);    //print key for user
  	}
  	printf("\n");
  	SHA256(key,MAC_Size,hash_a);    //hash key to get mac tag
  	encrypt_len=encrypt((unsigned char*)&ptr->g_name, sizeof(paddedDecrypted_gradebook), key, e_gradebook.iv, e_gradebook.encrypted_data);
  	// get mac
  	HMAC(EVP_sha256(), hash_a, 32, (const unsigned char *)&e_gradebook.iv,(IV_Size + sizeof(paddedDecrypted_gradebook)), e_gradebook.mac, &resultlen);
  	decrypt_len = decrypt(e_gradebook.encrypted_data, encrypt_len, key, e_gradebook.iv, (unsigned char*)&p_gb_ptr->g_name);
  	fwrite(&e_gradebook, sizeof(struct Encrypted_gradebook), 1, fp);
  	fclose(fp);
  	return(0);

}