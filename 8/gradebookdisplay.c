#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <ctype.h>
#include "gradebookobjects.h"

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

int get_grade(Student stu, char *input_AN) {
  for (int i = 0; i < stu.num_assignments; i++) {
    if (strcmp(stu.assignments[i].name, input_AN) == 0) {
      return stu.assignments[i].grade;
    }
  }
  return -1;
}

double get_final_grade(Student stu) {
  double result = 0;
  for (int i = 0; i < stu.num_assignments; i++) {
    result = result + (((float)stu.assignments[i].grade/(float)stu.assignments[i].points) * stu.assignments[i].weight);
  }
  return result;
}

int cmpstr(void const *a, void const *b) { 
    char const *aa = (char const *)a;
    char const *bb = (char const *)b;

    return strcmp(aa, bb);
}

/* test whether the file exists */
int file_test(char* filename) {
  struct stat buffer;
  return (stat(filename, &buffer) == 0);
}

/* return whether the input only contains alphanumeric characters, _, and . */
int is_valid_filename(char* filename) {
  if (strlen(filename) > 50) {
    return 0;
  }
  for (int i = 0; i < strlen(filename); i++) {
    if (!isalnum(filename[i]) && filename[i] != '_' && filename[i] != '.')
      return 0;
  }
  return 1;
} 

/* return whether the input only contains alphanumeric characters */
int is_valid_assignment_name(char* name) {
  if (strlen(name) > 50) {
    return 0;
  }
  for (int i = 0; i < strlen(name); i++) {
    if (!isalnum(name[i]))
      return 0;
  }
  return 1;
}

/* return whether the input only contains alphabetic characters */
int is_valid_firstlast_name(char* name) {
  if (strlen(name) > 50) {
    return 0;
  }
  for (int i = 0; i < strlen(name); i++) {
    if (!isalpha(name[i]))
      return 0;
  }
  return 1;
}

/* crypto functions */
void handleErrors(void)
{
    printf("invalid\n");
}

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
            unsigned char *iv, unsigned char *ciphertext)
{
    EVP_CIPHER_CTX *ctx;

    int len;

    int ciphertext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new())) {
        handleErrors();
        return (255);
    }

    /*
     * Initialise the encryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv)) {
      handleErrors();
      return (255);
    }

    /*
     * Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can be called multiple times if necessary
     */
    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len)) {
      handleErrors();
      return (255);
    }
    ciphertext_len = len;

    /*
     * Finalise the encryption. Further ciphertext bytes may be written at
     * this stage.
     */
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) {
      handleErrors();
      return (255);
    }
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
    if(!(ctx = EVP_CIPHER_CTX_new())) {
      handleErrors();
      return (255);
    }

    /*
     * Initialise the decryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv)) {
      handleErrors();
      return (255);
    }

    /*
     * Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary.
     */
    if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len)) {
      handleErrors();
      return (255);
    }
    plaintext_len = len;

    /*
     * Finalise the decryption. Further plaintext bytes may be written at
     * this stage.
     */
    if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) {
      handleErrors();
      return (255);
    }
    plaintext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return plaintext_len;
}


/* ------------- HELPER FUNCTIONS END ------------- */


int main(int argc, char *argv[]) {
  
  FILE *fp;
  unsigned char *gradebook_name;
  unsigned char *input_key;
  unsigned char key[64];
  unsigned char decrypt_key[32];
  unsigned char hmac_key[32];
  unsigned char decrypt_key_hex[16];
  unsigned char decrypt_key_hex_cpy[16];
  unsigned char hmac_key_hex[16];
  unsigned char *action;

  int flag_AN = 0, flag_FN = 0, flag_LN = 0, flag_A = 0, flag_G = 0;
  unsigned char *input_AN = NULL, *input_FN = NULL, *input_LN = NULL;

  Gradebook gb;
  unsigned char readIV[16];

  /* Get required fields first */
  if (argc >= 6) {
    if (strcmp(argv[1], "-N") == 0 && strcmp(argv[3], "-K") == 0 && (strcmp(argv[5], "-PA") == 0
                                                                      || strcmp(argv[5], "-PS") == 0
                                                                      || strcmp(argv[5], "-PF") == 0)) {
      /* verify the provided gradebook name is valid */
      gradebook_name = argv[2];
      if (!is_valid_filename(gradebook_name)) {
        printf("invalid\n");
        return(255);
      }

      /* verify the provided key is the correct length */
      input_key = argv[4];
      memcpy(key, input_key, 64);
      if (sizeof(key) != 64) {
        printf("invalid\n");
        return(255);
      }

      /* divide the provided key into two keys: decrypt & hmac */
      for (int i = 0; i < 32; i++) {
        decrypt_key[i] = key[i];
        hmac_key[i] = key[i + 32];
      }

      /* convert keys into proper format */
      unsigned char *pos_decrypt = decrypt_key;
      unsigned char *pos_hmac = hmac_key;
      for (int i = 0; i < 32; i++) {
        sscanf(pos_decrypt, "%2hhx", &decrypt_key_hex[i]);
        sscanf(pos_hmac, "%2hhx", &hmac_key_hex[i]);
        pos_decrypt = pos_decrypt + 2;
        pos_hmac = pos_hmac + 2;
      }

      /* Find and decrypt gradebook */
      if (file_test(gradebook_name)) {
        unsigned char read_ciphertext[sizeof(Gradebook) + 32];

        int c_len;
        unsigned char read_tag[16];
        fp = fopen(gradebook_name, "rb");
        if (fp != NULL) {
          fread(&read_ciphertext, sizeof(read_ciphertext), 1, fp);
          fread(&readIV, sizeof(readIV), 1, fp);
          fread(&read_tag, sizeof(read_tag), 1, fp);
          fread(&c_len, sizeof(int), 1, fp);
          fclose(fp);
        }
        unsigned char *computed_tag = NULL;
        unsigned int computed_result_len = -1;
        computed_tag = HMAC(EVP_md5(), hmac_key_hex, sizeof(hmac_key_hex), read_ciphertext, c_len, computed_tag, &computed_result_len);
        if (memcmp(computed_tag, read_tag, 16) != 0) {
          printf("invalid\n");
          return(255);
        } 
        memcpy(decrypt_key_hex_cpy, decrypt_key_hex, 16);
        int decryptedtext_len = decrypt(read_ciphertext, c_len, decrypt_key_hex, readIV, gb.name);
        if (decryptedtext_len == 255) {
          return (255);
        }
      } else {
        printf("invalid\n");
        return(255);
      }

      action = argv[5];

      /* Process the additional arguments */
      for (int i = 6; i < argc; i++) {
        if (strcmp(argv[i], "-AN") == 0) {
          flag_AN = 1;
          if (argc > i + 1) {
            input_AN = argv[i + 1];
            i = i + 1;
          } else {
            printf("invalid\n");
            return(255);
          }
        } else if (strcmp(argv[i], "-FN") == 0) {
          flag_FN = 1;
          if (argc > i + 1) {
            input_FN = argv[i + 1];
            i = i + 1;
          } else {
            printf("invalid\n");
            return(255);
          }
        } else if (strcmp(argv[i], "-LN") == 0) {
          flag_LN = 1;
          if (argc > i + 1) {
            input_LN = argv[i + 1];
            i = i + 1;
          } else {
            printf("invalid1\n");
            return(255);
          }
        } else if (strcmp(argv[i], "-A") == 0) {
          flag_A = 1;
        } else if (strcmp(argv[i], "-G") == 0) {
          flag_G = 1;
        } else {
          printf("invalid\n");
          return(255);
        }
      }
    } else {
      printf("invalid\n");
      return(255);
    }
  } else {
    printf("Usage: gradebookdisplay <logfile pathname> <key> <action>\n");
    return(255);
  }

  /* Perform action */
  if (strcmp(action, "-PA") == 0) {
    /* validate required options */
    if ((flag_A && flag_G) || (!flag_A && !flag_G)) {
      printf("invalid\n");
      return(255);
    }
    if (flag_AN && input_AN != NULL && is_valid_assignment_name(input_AN)) {
      
    } else {
      printf("invalid\n");
      return(255);
    }

    /* perform action */
    /* Check that an assignment with the given name exists */
    int found = 0;
    for (int i = 0; i < gb.num_assignments; i++) {
      if (strcmp(gb.assignments[i].name, input_AN) == 0) {
        found = 1;
      }
    }
    if (!found) {
      printf("invalid\n");
      return(255);
    }

    Student students[gb.num_students];

    /* Copy gradebooks students to a different array to sort */
    for (int i = 0; i < gb.num_students; i++) {
      students[i] = gb.students[i];
    }
    /* Sort alphabetically */
    if (flag_A) {
      for (int i = 1; i < gb.num_students; i++) {
        for (int j = 0; j < gb.num_students - i; j++) {
          if (strcmp(students[j].last_name, students[j + 1].last_name) > 0) {
            Student temp = students[j];
            students[j] = students[j + 1];
            students[j + 1] = temp;
          } else if (strcmp(students[j].last_name, students[j + 1].last_name) == 0) {
            if (strcmp(students[j].first_name, students[j + 1].first_name) > 0) {
              Student temp = students[j];
              students[j] = students[j + 1];
              students[j + 1] = temp;
            } 
          }
        } 
      }
    }
    /* Sort by grade */
    if (flag_G) {
      for (int i = 1; i < gb.num_students; i++) {
        for (int j = 0; j < gb.num_students - i; j++) {
          if (get_grade(students[j], input_AN) < get_grade(students[j + 1], input_AN)) {
            Student temp = students[j];
            students[j] = students[j + 1];
            students[j + 1] = temp;
          } 
        } 
      }
    }

    /* Print each student */
    for (int i = 0; i < gb.num_students; i++) {
      for (int j = 0; j < students[i].num_assignments; j++) {
        /* Find correct assignment to print */
        if (strcmp(input_AN, students[i].assignments[j].name) == 0) {
          printf("(%s, %s, %d)", students[i].last_name, students[i].first_name, students[i].assignments[j].grade);
          printf("\n");
        }
      }
    }
     
  } else if (strcmp(action, "-PS") == 0) {
    /* validate required options */
    if (flag_FN && flag_LN && input_FN != NULL && input_LN != NULL && is_valid_firstlast_name(input_FN) && is_valid_firstlast_name(input_LN)) {
      int found = 0;
      for (int i = 0; i < gb.num_students; i++) {
        if (strcmp(input_FN, gb.students[i].first_name) == 0 && strcmp(input_LN, gb.students[i].last_name) == 0) {
          found = 1;
          for (int j = 0; j < gb.students[i].num_assignments; j++) {
            printf("(%s, %d)\n", gb.students[i].assignments[j].name, gb.students[i].assignments[j].grade);
          }
        }
      }
      if (!found) {
        printf("invalid\n");
        return(255);
      }
    } else {
      printf("invalid\n");
      return(255);
    }


  } else if(strcmp(action, "-PF") == 0) {
    /* validate required options */
    if ((flag_A && flag_G) || (!flag_A && !flag_G)) {
      printf("invalid\n");
      return(255);
    } 
    
    /* perform action */
    Student students[gb.num_students];

    /* Copy gradebooks students to a different array to sort */
    for (int i = 0; i < gb.num_students; i++) {
      students[i] = gb.students[i];
    }
    /* Sort alphabetically */
    if (flag_A) {
      for (int i = 1; i < gb.num_students; i++) {
        for (int j = 0; j < gb.num_students - i; j++) {
          if (strcmp(students[j].last_name, students[j + 1].last_name) > 0) {
            Student temp = students[j];
            students[j] = students[j + 1];
            students[j + 1] = temp;
          } else if (strcmp(students[j].last_name, students[j + 1].last_name) == 0) {
            if (strcmp(students[j].first_name, students[j + 1].first_name) > 0) {
              Student temp = students[j];
              students[j] = students[j + 1];
              students[j + 1] = temp;
            } 
          }
        } 
      }
    }
    /* Sort by Grade */
    if (flag_G) {
      for (int i = 1; i < gb.num_students; i++) {
        for (int j = 0; j < gb.num_students - i; j++) {
          if (get_final_grade(students[j]) < get_final_grade(students[j + 1])) {
            Student temp = students[j];
            students[j] = students[j + 1];
            students[j + 1] = temp;
          } 
        } 
      }
    }

    /* Print each student */
    for (int i = 0; i < gb.num_students; i++) {
      printf("(%s, %s, %lf)", students[i].last_name, students[i].first_name, get_final_grade(students[i]));
      printf("\n");
    }

  } else {
    printf("invalid\n");
    return(255);
  }

  return 0;
}
