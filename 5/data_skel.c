#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data.h"

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/comp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>


//Copied handleErrors, encrypt and decrypt from crypto_sample.c
void handleErrors(void)
{
    ERR_print_errors_fp(stderr);
    abort();
}

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
            unsigned char *iv, unsigned char *ciphertext)
{
    ERR_load_crypto_strings();
    EVP_CIPHER_CTX *ctx;
    int len,ciphertext_len;

    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv))
        handleErrors();
    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        handleErrors();
    ciphertext_len = len;
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        handleErrors();
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
            unsigned char *iv, unsigned char *plaintext)
{
    ERR_load_crypto_strings();
    EVP_CIPHER_CTX *ctx;

    int len, plaintext_len;

    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv))
        handleErrors();
    EVP_CIPHER_CTX_set_padding(ctx, 0);
    if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        handleErrors();
    plaintext_len = len;
    if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
        handleErrors();
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    return plaintext_len;
}

//Prints gradebook R to a buffer B
void print_Gradebook(Gradebook *R, Buffer *B) {
  //Gradebooks max size, initialized to all 0's
  unsigned char p[4101100] = {0};
  /*
  gradebook format the will be printed to the buffer:
      Assignment 1    Assignment 2     ...
      (p1, w1)        (p2,w2)          ...
  s1  g11             g12              ...
  s2  g21             g22              ...
  .   .               .                ...
  .   .               .                ...
  .   .               .                ...
  sn  gn1             gn2              ...    
  */
  strcat(p, "\t\t");
  int i, j, z;
  for(i = 0; i < R->num_assignments; i++){
      strcat(p, R->assignments[i].name);
      strcat(p, "\t\t");       
  }
  strcat(p, "\n\t\t");
  for(i = 0; i < R->num_assignments; i++){
    strcat(p, "(");
    unsigned char tot_points[5];
    sprintf(tot_points, "%d", R->assignments[i].total);
    strcat(p, tot_points);
    strcat(p, ", ");      
    unsigned char weight[314];
    sprintf(weight, "%.2f", R->assignments[i].weight);
    strcat(p, weight);
    strcat(p, ")\t");
  }
  strcat(p, "\n");
  for(i = 0; i < R->num_students; i++){
    strcat(p, R->students_first[i]);
    strcat(p, " ");
    strcat(p, R->students_last[i]);
    strcat(p, "\t");
    for(j = 0; j < R->num_assignments; j++){
      int has_grade = 0;
      for(z = 0; z < R->num_students; z++){
        if(strcmp(R->assignments[j].students_first[z], R->students_first[i]) == 0 && strcmp(R->assignments[j].students_last[z], R->students_last[i]) == 0){
          has_grade = 1;
          unsigned char points[4];
          sprintf(points, "%d", R->assignments[j].points[z]);
          strcat(p, points);
          break;
        }
      }
      if(has_grade == 0){
        strcat(p, "-1");
      }
      strcat(p, "\t\t");
    }
    strcat(p, "\n");  
  }

  strcpy(B->Buf, p);
  B->Length = strlen(p);
  return;
}

//Parse a passed in gradebook string
int parse_gradebook_str(unsigned char gradebook_str[4101100], Gradebook *gb){
  //Tokenize string based on tabs
  unsigned char *tok = strtok(gradebook_str, "\t");
  int new_line_ctr = 0, assignment_ctr = 0, points_weight_ctr = 0, student_ctr = 0, student_grade_ctr = 0;
  //Initialize everything to 0
  gb->num_assignments = 0;
  gb->num_students = 0;
  unsigned char first[100] = {0};
  unsigned char last[100] = {0};
  //Go through all the tokens
  while(tok != NULL){
    //Case where we have a new line
    if(strstr(tok, "\n") != NULL){
      //If we have \n*student first name* *student last name*
      if(sscanf(tok, "\n%s %s", first, last) == 2){
        //add the students and increment total number of students
        strcpy(gb->students_first[student_ctr], first);
        strcpy(gb->students_last[student_ctr], last);
        gb->num_students++;
        student_ctr++;
        student_grade_ctr = 0;
      }
      //keep track of how many \n's we've seen
      new_line_ctr++;
    }
    else{
      //If we're on the first line we will have assignments
      if(new_line_ctr == 0){
        //get the name and add it to the array, incrementing the total number of assignments
        strcpy(gb->assignments[assignment_ctr].name, tok);
        gb->num_assignments++;
        assignment_ctr++;
      }
      //Second line has weights and points for assignments
      else if(new_line_ctr == 1){
        int points;
        float weight;
        int valid = sscanf(tok, "(%d, %f)", &points, &weight);

        if(valid == 2){
          gb->assignments[points_weight_ctr].total = points;
          gb->assignments[points_weight_ctr].weight = weight;
          points_weight_ctr++;
        }
      }
      //Rest of the lines have students and their grades, if a students doesn't have a grade I put -1 to signify that
      else{
        int grade = atoi(tok);
        if(grade != -1){
          gb->assignments[student_grade_ctr].points[new_line_ctr-2] = grade;
          strcpy(gb->assignments[student_grade_ctr].students_first[new_line_ctr-2], first);
          strcpy(gb->assignments[student_grade_ctr].students_last[new_line_ctr-2], last);
          gb->assignments[student_grade_ctr].num_students++;
          student_grade_ctr++;
        }
      }
    }
    //Go to next token
    tok = strtok(NULL, "\t");
  }  
  return 0;
}

//Given a encrypted file populate a gradebook
int read_Gradebook_from_path(unsigned char *path, unsigned char *key, Gradebook *outbuf) {
  //Zero out variables
  static unsigned char plaintext[4101100+32] = {0};
  static unsigned char ciphertext[4101100+16+32] = {0};
  unsigned char iv[16] = {0};
  //size of ciphertext
  int size;
  unsigned char hash_enc[SHA_DIGEST_LENGTH] = {0};

  FILE *f = fopen(path, "rb");
  //First 16 bytes are the IV
  fread(iv, 16, 1, f);
  //Next 4 bytes are the size of the ciphertext + 20 bytes for the SHA1 hashed key
  fread(&size, sizeof(int), 1, f);
  //Get the ciphertext
  fread(ciphertext, size-SHA_DIGEST_LENGTH, 1, f);
  //Get the hashed key
  fread(hash_enc, SHA_DIGEST_LENGTH, 1, f);
  fclose(f);

  //SHA1 the passed in key
  unsigned char hash[SHA_DIGEST_LENGTH] = {0};
  SHA1(key, strlen(key), hash);

  //Make sure the 2 keys are equivalent
  //Ensures that the current user is legitmate
  //Ensures that the ciphertext hasn't been tampered with
  if(memcmp(hash_enc, hash, SHA_DIGEST_LENGTH) != 0){
    printf("invalid\n");
    exit(255);
  }

  //Case where we have an empty gradebook
  if(*(ciphertext) == '\0'){
    outbuf->num_students = 0;
    outbuf->num_assignments = 0;
    return 0;
  }

  //If we don't have an empty gradebook, decrypt the ciphertext
  decrypt(ciphertext, size-SHA_DIGEST_LENGTH, key, iv, plaintext);

  unsigned char *charset = "\n\t -().,0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYS\0";
  int i, padding_flag = -1;
  //Go through the decrypted text and check if any characters were changed to things outside of the charset
  for(i = strlen(plaintext) - 1; i >= 0; i--){
    //disregard padding at the end of the plaintext
    if(plaintext[i] == '\n')
      padding_flag = 0;
    if(strchr(charset, plaintext[i]) == NULL && padding_flag == 0){
      printf("invalid\n");
      exit(255);
    }
  }
  if(padding_flag == -1){
    printf("invalid\n");
    exit(255);
  }

  //populate the data structure with the plaintext
  int valid = parse_gradebook_str(plaintext, outbuf);
  return valid;
}

//Writes a gradebook's string to a path
void write_to_path(char *path, Buffer *B, unsigned char *key_data) {
  unsigned char iv[16] = {0};
  static unsigned char ciphertext[4101100+16+32] = {0};
  static unsigned char final_str[16+4101100+32] = {0};
  //16 random bytes for the IV
  RAND_bytes(iv, 16);
  //Encrpyt the plaintext
  int c = encrypt(B->Buf, B->Length, key_data, iv, ciphertext);
  //Copy the IV to the string that will be written to the file
  memcpy(final_str, iv, 16);
  //Copy the ciphertext length + 20 bytes for the hashed key in the 16th index
  final_str[16] = c+SHA_DIGEST_LENGTH;
  //Copy the ciphertext itself 
  memcpy(final_str+16+sizeof(int), ciphertext, c);
  
  unsigned char hash[SHA_DIGEST_LENGTH] = {0};
  SHA1(key_data, strlen(key_data), hash);
  //Copy the hashed key after the ciphertext
  memcpy(final_str+16+sizeof(int)+c, hash, SHA_DIGEST_LENGTH);

  FILE *f = fopen(path, "wb");
  //Write final_str to the file (contains an IV, size of ciphertext+20, ciphertext itself and hashed key)
  fwrite(final_str, 1, 16 + sizeof(int) + c + SHA_DIGEST_LENGTH, f);
  fclose(f);
  return;
}
