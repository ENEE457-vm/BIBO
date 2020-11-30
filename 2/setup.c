#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <ctype.h>
#include "data.h"
#include "crypto.c"

#define DEBUG

/* test whether the file exists */

int main(int argc, char** argv) {
  FILE *fp;
  unsigned char key[32] ={0};

  if (argc != 3 || strcmp(argv[1], "-N") != 0) {
    printf("Usage: setup -N <gradebookName>\n");
    return(255);
  }

  // check if file already exists
  if ((fp = fopen(argv[2], "r")) != NULL) {
    // file already exists - return error
    printf("Invalid\n");
    return(255);
  }
  
  if (strlen(argv[2]) > 100) {
    printf("Invalid\n");
    return(255);
  } else {
    char * str = argv[2];
    int num_valid = 0;

    // make sure gradebook name only contains alphanumeric characters including underscores ("_") and periods
    // all other characters are considered invalid
    for (int j = 0; j < strlen(str); j++) {
        // checks for non alpha-numeric chars
        if (isalpha(str[j]) != 0) {
          num_valid++;
        } else if (isdigit(str[j]) != 0) {
          num_valid++;
        } else if (str[j] == '_') {
          num_valid++;
        } else if (str[j] == '.') {
          num_valid++;
        }
    }    

    if (num_valid != strlen(str)) {
      printf("Invalid\n");
      return(255);
    }

    // create new DecryptedGradebook struct
    PaddedDecryptedGradebook gb = {0};
    memcpy(gb.gradebook_name, argv[2], strlen(argv[2]));
    
    // generate random key and copy it into gradebook's key field
    int rc = RAND_bytes(key, 32);

    if(rc != 1) {
      printf("invalid - error\n");
      return(255);
    } else {
        // file doesn't exist - create it
        fp = fopen(argv[2], "w");
        printf("key is:\n");

        for (int i = 0; i < KEY_SIZE; i++) {
          printf("%02x", key[i]);
        } 

        printf("\n");

        // encrypt gradebook to file
        EncryptedGradebook encryptedgb = {0};
        RAND_bytes(encryptedgb.iv, 32);
        
        // generate Mac Key by taking the SHA1 hash of generated gb.key
        // Mac key used to generate tag <-- Mac(mac_key, ciphertext)
        unsigned char mac_key[KEY_SIZE];
        SHA256(key, KEY_SIZE, mac_key);

        PaddedDecryptedGradebook *gb_ptr = &gb;
        
        int encrypt_len = encrypt((unsigned char*) &gb_ptr->gradebook_name, sizeof(PaddedDecryptedGradebook), key, 
          encryptedgb.iv, encryptedgb.encrypted_data);

        unsigned int mac_tag_len = 0;
        // generate MAC
        HMAC(EVP_sha256(), mac_key, 32, (const unsigned char *)&encryptedgb.iv, 
            (IV_SIZE + sizeof(PaddedDecryptedGradebook)), encryptedgb.mac_tag, &mac_tag_len);

        fwrite (encryptedgb.iv, sizeof(EncryptedGradebook), 1, fp); 

        fflush(fp);
        fclose(fp);
        return(0);
    }
  } 
}
