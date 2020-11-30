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
#include "gradebookobjects.h"

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

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

/* return whether the input is only digits */
int is_valid_number(char* num) {
  for (int i = 0; i < strlen(num); i++) {
    if (!isdigit(num[i]))
      return 0;
  }
  return 1;
}

/* return whether the input is only digits and one decimal */
int is_valid_double(char* num) {
  int decimal_points = 0;
  for (int i = 0; i < strlen(num); i++) {
    if (!isdigit(num[i]) && num[i] != '.')
      return 0;
    if (num[i] == '.')
      decimal_points = decimal_points + 1;
  }
  if (decimal_points > 1) {
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

  int flag_AN = 0, flag_FN = 0, flag_LN = 0, flag_P = 0, flag_W = 0, flag_G = 0;
  unsigned char *input_AN = NULL, *input_FN = NULL, *input_LN = NULL, *input_P = NULL, *input_W = NULL, *input_G = NULL;
  int P, G;
  double W;

  Gradebook gb;
  unsigned char readIV[16];
 
  /* Get required fields first */
  if (argc >= 6) {
    if (strcmp(argv[1], "-N") == 0 && strcmp(argv[3], "-K") == 0 && (strcmp(argv[5], "-AA") == 0
                                                                      || strcmp(argv[5], "-DA") == 0
                                                                      || strcmp(argv[5], "-AS") == 0
                                                                      || strcmp(argv[5], "-DS") == 0
                                                                      || strcmp(argv[5], "-AG") == 0)) {
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
            printf("invalid\n");
            return(255);
          }
        } else if (strcmp(argv[i], "-P") == 0) {
          flag_P = 1;
          if (argc > i + 1) {
            input_P = argv[i + 1];
            i = i + 1;
          } else {
            printf("invalid\n");
            return(255);
          }         
        } else if (strcmp(argv[i], "-W") == 0) {
          flag_W = 1;
          if (argc > i + 1) {
            input_W = argv[i + 1];
            i = i + 1;
          } else {
            printf("invalid\n");
            return(255);
          }       
        } else if (strcmp(argv[i], "-G") == 0) {
          flag_G = 1;
          if (argc > i + 1) {
            input_G = argv[i + 1];
            i = i + 1;
          } else {
            printf("invalid\n");
            return(255);
          }       
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
    printf("Usage: gradebookadd <logfile pathname> <key> <action>\n");
    return(255);
  }

  
  /* Perform action */
  if (strcmp(action, "-AA") == 0) {
    /* validate required options */
    if (flag_AN && flag_P && flag_W && input_AN != NULL && input_P != NULL && input_W != NULL && is_valid_assignment_name(input_AN) && is_valid_number(input_P) && is_valid_double(input_W)) {
      P = atoi(input_P);
      if (P < 0) {
        printf("invalid\n");
        return(255);
      }
      char *extra;
      W = strtod(input_W, &extra);
      if (W < 0 || W > 1) {
        printf("invalid\n");
        return(255);
      }
    } else {
      printf("invalid\n");
      return(255);
    }

    /* perform action */
    /* Add assignment to assignments array */
    double total_weight = W;
    for (int i = 0; i < gb.num_assignments; i++) {
      if (strcmp(input_AN, gb.assignments[i].name) == 0) {
        printf("invalid\n");
        return(255);
      }
      total_weight = total_weight + gb.assignments[i].weight;
    }
    if (gb.num_assignments < 100 && total_weight <= 1.0) {
      Assignment new_assignment;
      strcpy(new_assignment.name, input_AN);
      new_assignment.points = P;
      new_assignment.weight = W;
      new_assignment.grade = 0;
      gb.assignments[gb.num_assignments] = new_assignment;
      gb.num_assignments = gb.num_assignments + 1;
    } else {
      printf("invalid: you cannot add any more assignments or total weight is > 1\n");
      return(255);
    }

    /* Add assignment to each students assignment list */
    for (int i = 0; i < gb.num_students; i++) {
      Assignment new_assignment;
      strcpy(new_assignment.name, input_AN);
      new_assignment.points = P;
      new_assignment.weight = W;
      new_assignment.grade = 0;
      if (gb.students[i].num_assignments < 100) {
        gb.students[i].assignments[gb.students[i].num_assignments] = new_assignment;
        gb.students[i].num_assignments = gb.students[i].num_assignments + 1;
      }
    }

  } else if (strcmp(action, "-DA") == 0) {
    /* validate required options */
    if (flag_AN && input_AN != NULL && is_valid_assignment_name(input_AN)) {
      /* Perform action */
      /* Delete assignment from assignments array */
      int deleted = 0;
      int pos = 0;
      for (int i = 0; i < gb.num_assignments; i++) {
        if (strcmp(gb.assignments[i].name, input_AN) == 0) {
          /* We found the assignment to be deleted */
          pos = i;
          deleted = 1;
        }
      }
      /* If assignment with provided name was not in gradebook, return invalid*/
      if (!deleted) {
        printf("invalid\n");
        return(255);
      }
      for (int i = pos; i < gb.num_assignments - 1; i++) {
        gb.assignments[i] = gb.assignments[i + 1];
      }
      gb.num_assignments = gb.num_assignments - 1;

      /* Delete assignment from each students assignment array */
      for (int i = 0; i < gb.num_students; i++) {
        int deleted2 = 0;
        int pos2 = 0;
        for (int j = 0; j < gb.students[i].num_assignments; j++) {
          if (strcmp(gb.students[i].assignments[j].name, input_AN) == 0) {
            /* We found the assignment to be deleted */
            pos2 = j;
            deleted2 = 1;
          }
        }
        if (deleted2) {
          for (int j = pos2; j < gb.students[i].num_assignments - 1; j++) {
            gb.students[i].assignments[j] = gb.students[i].assignments[j + 1];
          }
          gb.students[i].num_assignments = gb.students[i].num_assignments - 1;
        }
      }
    } else {
      printf("invalid\n");
      return(255);
    }
  } else if(strcmp(action, "-AS") == 0) {
    /* validate required options */
    if (flag_FN && flag_LN && input_FN != NULL && input_LN != NULL && is_valid_firstlast_name(input_FN) && is_valid_firstlast_name(input_LN)) {
      /* Perform action */
      for (int i = 0; i < gb.num_students; i++) {
        if (strcmp(gb.students[i].first_name, input_FN) == 0 && strcmp(gb.students[i].last_name, input_LN) == 0) {
          /* A student with the same first and last name already exists */
          printf("invalid\n");
          return (255);
        }
      }
      if (gb.num_students < 500) {
        /* Add student to student array */
        Student new_stu;
        strcpy(new_stu.first_name, input_FN);
        strcpy(new_stu.last_name, input_LN);
        new_stu.num_assignments = gb.num_assignments;
        for (int i = 0; i < gb.num_assignments; i++) {
          Assignment new_assignment;
          strcpy(new_assignment.name, gb.assignments[i].name);
          new_assignment.points = gb.assignments[i].points;
          new_assignment.weight = gb.assignments[i].weight;
          new_assignment.grade = 0;
          new_stu.assignments[i] = new_assignment;
        }
        gb.students[gb.num_students] = new_stu;
        gb.num_students = gb.num_students + 1;
      }
    } else {
      printf("invalid\n");
      return(255);
    }
  } else if (strcmp(action, "-DS") == 0) {
    /* validate required options */
    if (flag_FN && flag_LN && input_FN != NULL && input_LN != NULL && is_valid_firstlast_name(input_FN) && is_valid_firstlast_name(input_LN)) {
      /* Perform action */
      int deleted = 0;
      int pos = 0;
      for (int i = 0; i < gb.num_students; i++) {
        if (strcmp(gb.students[i].first_name, input_FN) == 0 && strcmp(gb.students[i].last_name, input_LN) == 0) {
          deleted = 1;
          pos = i;
        }
      }
      /* Student with provided name is not in gradebook */
      if (!deleted) {
        printf("invalid\n");
        return (255);
      }
      /* Delete student from gradebook list */
      for (int i = pos; i < gb.num_students - 1; i++) {
        gb.students[i] = gb.students[i + 1];
      }
      gb.num_students = gb.num_students - 1;
    } else {
      printf("invalid\n");
      return(255);
    }
  } else if (strcmp(action, "-AG") == 0) {
    /* validate required options */
    if (flag_FN && flag_LN && flag_AN && flag_G 
          && input_FN != NULL && input_LN != NULL && input_AN != NULL && input_G != NULL 
          && is_valid_firstlast_name(input_FN) && is_valid_firstlast_name(input_LN) && is_valid_assignment_name(input_AN) && is_valid_number(input_G)) {
      G = atoi(input_G);
      if (G < 0) {
        printf("invalid\n");
        return(255);
      }
    } else {
      printf("invalid\n");
      return(255);
    }
    /* Perform action */
    int found = 0;
    for (int i = 0; i < gb.num_students; i++) {
      if (strcmp(gb.students[i].first_name, input_FN) == 0 && strcmp(gb.students[i].last_name, input_LN) == 0) {
        found = 1;
        int found_assignment = 0;
        for (int j = 0; j < gb.students[i].num_assignments; j++) {
          if (strcmp(gb.students[i].assignments[j].name, input_AN) == 0) {
            /* Found assignment */
            found_assignment = 1;
            gb.students[i].assignments[j].grade = G;
          }
        }
        if (!found_assignment) {
          printf("invalid\n");
          return (255);
        }
      }
    }
    /* Student with provided name is not in gradebook */
    if (!found) {
      printf("invalid\n");
      return (255);
    }
  } else {
    printf("invalid\n");
    return(255);
  }


  fp = fopen(gradebook_name, "w");

  /* Generate random 128 bit IV  */
  unsigned char iv_buffer[16]; /* 128 bit IV -> 16 byte IV */
  int iv_rc = RAND_bytes(iv_buffer, sizeof(iv_buffer));
  unsigned long iv_err = ERR_get_error();
  if(iv_rc != 1) {
      printf("invalid\n");
      return(255);
  }

  /* Encrypt the gradebook */
  unsigned char ciphertext[sizeof(Gradebook) + 32];
  int ciphertext_len = encrypt(gb.name, sizeof(Gradebook), decrypt_key_hex_cpy, iv_buffer, ciphertext);

  /* Produce tag with HMAC */
  unsigned char *tag = NULL;
  unsigned int result_len = -1;
  tag = HMAC(EVP_md5(), hmac_key_hex, sizeof(hmac_key_hex), ciphertext, ciphertext_len, tag, &result_len);

  /* Write the tag to a unsigned char array with fixed size */
  unsigned char actual_tag[16];
  for (int i = 0; i < result_len; i++) {
      actual_tag[i] = tag[i];
  }

  /* Write the ciphertext to the gradebook file */
  fwrite(&ciphertext, sizeof(ciphertext), 1, fp);

  /* Write the IV to the gradebook file */
  fwrite(&iv_buffer, sizeof(iv_buffer), 1, fp); 

  /* Write the tag to the gradebook file */
  fwrite(&actual_tag, sizeof(actual_tag), 1, fp);

  /* Write the ciphertext length to the gradebook file */
  fwrite(&ciphertext_len, sizeof(int), 1, fp);

  fclose(fp);

  return 0;
}