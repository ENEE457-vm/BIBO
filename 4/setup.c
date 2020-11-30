#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include "data.h"
#define DEBUG

/* test whether the file exists */
int file_test(char* filename) {
  struct stat buffer;
  return (stat(filename, &buffer) == 0);
}

int main(int argc, char** argv) {
  FILE *fp;
  unsigned char plaintext[425612] = {"0"};
  unsigned char ciphertext[425616] = {"0"};
  unsigned char totaltext[425632] = {"0"};
  unsigned char key[16];
  unsigned char iv[16];
  int i = 0, j = 0;
  Gradebook gradebook;
  Student student;
  Assignment assignment;
  
  if (argc < 2) {
    printf("Usage: setup <logfile pathname>\n");
    printf("invalid\n");
    return(255);
  }

  if (strcmp(argv[1],"-N") != 0) {
     printf("invalid\n");
    return(255);
  }

  fp = fopen(argv[2], "w");
  if (fp == NULL){
#ifdef DEBUG
    printf("setup: fopen() error could not create file\n");
#endif
    printf("invalid\n");
    return(255);
  }

#ifdef DEBUG
  if (file_test(argv[2]))
    //printf("created file named %s\n", argv[2]);
#endif

/* add your code here */

  // initalizing the gradebook
  for(i = 0; i < 200; i++) {
    student = gradebook.students[i];
    student.firstname[0] = '\0';
    student.lastname[0]= '\0';
    for(j=0; j < 40; j++) {
      assignment = student.assignments[j];
      assignment.name[0] = '\0';
      assignment.grade = 0;
      assignment.points = 0;
      student.assignments[j] = assignment;
    }
    gradebook.students[i] = student;
  }
  gradebook.student_count = 0;
  gradebook.assign_count = 0;
  gradebook.total_weight = 0;
  RAND_bytes(key,16);
  RAND_bytes(iv,16);
  /*
  printf("IV:");
  for(int i = 0; i < sizeof(iv); i++) {
        printf("%02x", iv[i]);
  }
  printf("\n");
  */
  // printf("Gradebook size:%d", sizeof(Gradebook));
  memcpy(plaintext, &gradebook, sizeof(Gradebook));
  int len = encrypt(plaintext, sizeof(plaintext), key, iv, ciphertext);
  // concatenate iv with ciphertext of gradebook after it and place into larger buffer
  memcpy(totaltext, iv, sizeof(iv));
  memcpy(totaltext+16, ciphertext, sizeof(ciphertext));
  //printf("Length of ciphertext: %d\n", len);
  // write buffer to file 
  fwrite(totaltext, sizeof(totaltext), 1, fp);
  fclose(fp);
  printf("Key is:");
  for(int i = 0; i < sizeof(key); i++) {
        printf("%02x", key[i]);
  }
  printf("\n");

  return(0);

}
