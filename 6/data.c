#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "data.h"
#include "crypto.h"

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/comp.h>

int get_student_position(Gradebook *g, char *firstname, char *lastname) {
  Student s;
  for (int i = 0; i < g->num_students; i++) {
    s = g->students[i];
    if (strcmp(s.firstname, firstname) == 0 && strcmp(s.lastname, lastname) == 0) {
      return i;
    }
  }

  return -1;
}

int get_assignment_position(Gradebook *g, char *name) {
  Assignment a;
  for (int i = 0; i < g->num_assignments; i++) {
    a = g->assignments[i];
    if (strcmp(a.name, name) == 0) {
      return i;
    }
  }
  
  return -1;
}

int retrieve_gradebook(char *gradebook, unsigned char *key, Gradebook *g) {
  int res = 1;

  if(!decrypt_file(gradebook, "temp", key)) {
    Print("Error: encountered issue when trying to decrypt file.\n");
    res = 0;
  }

  FILE *infile;
  infile = fopen("temp", "r");
  if (res && infile == NULL) {
    Print("Error: cannot open decrypted file.\n");
    res = 0;
  }

  if (res) {
    fread(g, sizeof(Gradebook), 1, infile);
  }

  remove("temp");
  fclose(infile);
  return res;
}

int write_gradebook(char *gradebook, unsigned char *key, Gradebook *g) {
  int res = 1;

  // Create temp file to write into 
  FILE *outfile = fopen("temp", "w");
  if (outfile == NULL) {
    Print("Error: Could not create temp file.\n");
    res = 0;
  }

  // Write into temp file
  if(res && !fwrite(g, sizeof(Gradebook), 1, outfile)) {
    Print("Error: Could not write to file. \n");
    res = 0;
  }

  fclose(outfile);
  
  // Encrypt into gradebook file
  if(res && !encrypt_file("temp", gradebook, key)) {
    Print("Error: Could not encrypt file.\n");
    res = 0;
  }

  remove("temp");
  return res;
}

int validate_beginning(CmdLineResult *R, int argc, char *argv[]) {
  // Check to make sure all arguments are valid.
  for (int i = 1; i < argc; i++) {
    if (strlen(argv[i]) > MAX_ARG_LEN || (i != 4 && !valid_string(argv[i], 2))) {
      Print("Error: bad argument.\n");
      return 0;
    }
  }

  if(argc >= 5 && argc <= 25) {
    // Check N and K are in correct spots
    if (strcmp(argv[1],"-N") != 0 || strcmp(argv[3],"-K") != 0) {
      Print("Error: Bad options.\n");
      return 0;
    }

    // Get filename from args
    if (!file_test(argv[2]) || !valid_string(argv[2], 1)) { // Check that file exists with good name
      Print("Error: filename is bad or doesn't exist\n");
      return 0;
    } else { // Get filename from cmd line
      strcpy(R->filename, argv[2]);
    }

    // Get key from args
    if (strlen(argv[4]) != KEY_LEN - 1) { // Check key is valid length
      Print("Error: bad key\n");
      return 0;
    } else { // Get key value from cmd line
      strcpy(R->key, argv[4]);
    }
    return 1;
  } else {
    Print("Error: Not enough or too many arguments.\n");
    return 0;
  }
}

// Checks to see if the passed in string is alphanumeric with 
// the exception of periods and underscores.
// Modes: 0 -> just alphabet chars 1 -> purely alphanumeric, 2 -> alpanum w/ . and _ -> 3 alphanum with . _ and - (general args)
int valid_string(char* str, int mode) {
  int i;
  for (i = 0; i < strlen(str); i++) {
    char c = *(str + i);
    if (mode == 0) {
      if (!isalnum((int) c))
        return 0;
    } else if (mode == 1) {
      if (!isalnum((int) c) && c != '.' && c != '_')
        return 0;
    } else if (mode == 2) {
      if (!isalnum((int) c) && c != '.' && c != '_' && c != '-')
        return 0;
    }
  }
  return 1;
}

/* test whether the file exists */
int file_test(char* filename) {
  struct stat buffer;
  return (stat(filename, &buffer) == 0);
}
