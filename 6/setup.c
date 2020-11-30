#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "data.h"

int create_blank_gradebook(Gradebook *g) {
  g->num_assignments = 0;
  g->num_students = 0;

  // Create blank student
  Student s;
  strcpy(s.firstname, "----");
  strcpy(s.lastname, "----");

  for (int i = 0; i < MAX_CLASS_SIZE; i++) {
    memcpy(&g->students[i], &s, sizeof(Student));
  }

  // Create blank assignment
  Assignment a;
  strcpy(a.name, "----");
  a.points = -1;
  a.weight = 0;
  for (int i = 0; i < MAX_CLASS_SIZE; i++) {
    Grade grade;
    memcpy(&grade.student, &s, sizeof(Student));
    grade.grade = -1;
    memcpy(&a.grades[i], &grade, sizeof(Grade));
  }


  for (int i = 0; i < MAX_NUM_ASSIGNMENTS; i++) {
    memcpy(&g->assignments[i], &a, sizeof(Assignment));
  }

  return 1;
}

int generate_rand_key(char* res) {
  char *options = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ123456789";
  size_t len = strlen(options);
  RAND_poll();

  unsigned char buf[4];
  int lookup;
  for (int i = 0; i < 16; i++) {
    if (!RAND_bytes(buf, sizeof(buf))) {
      Print("Error: Key generation failed.\n");
      return 0;
    }
    memcpy(&lookup, buf, 4);
    lookup = lookup % len;
    res[i] = options[lookup];
  }
  res[16] = '\0';

  return 1;
}

int main(int argc, char** argv) {
  int res = 1;
  unsigned char key[17];

  if(!generate_rand_key(key)) {
    Print("Error: Could not generate random key.\n");
    res = 0;
  }

  // Create the gradebook and optionally name the file
  FILE *gb;
  Gradebook *g = malloc(sizeof(Gradebook));
  create_blank_gradebook(g);

  if (argc <= 2 || strlen(argv[1]) > MAX_ARG_LEN || strncmp(argv[1], "-N", MAX_ARG_LEN) != 0) {
    Print("Command format incorrect.\n");
    res = 0;
  }
  if (res && (strlen(argv[2]) > MAX_ARG_LEN || !valid_string(argv[2], 1) || file_test(argv[2]))) {
    Print("Error: Poorly formatted arguments or gradebook already exists.\n");
    res = 0;
  }

  if(res && !write_gradebook(argv[2], key, g)) {
    Print("Error: could not write to gradebook.\n");
    res = 0;
  }


  free(g);
  remove("temp");
  if (!res) {
    printf("invalid\n");
    return 255;
  } else {
    printf("%s", key);
    return 0;
  }
}
