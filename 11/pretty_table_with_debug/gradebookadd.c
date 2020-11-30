#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>
#include <stdbool.h>
#include <ctype.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "data.h"


// Stores command line inputs converted into useful data structures.
typedef struct {
  ActionTypeAdd action;
  char assignmentname[MAX_NAME_BUFFER_LEN];
  int assignment_points;
  float assignment_weight;
  char firstname[MAX_NAME_BUFFER_LEN];
  char lastname[MAX_NAME_BUFFER_LEN];
  int assignment_grade;
} ExecutionData;


// Stores raw command line inputs
typedef struct {
  char filename[MAX_NAME_BUFFER_LEN];
  char key[KEY_SIZE * 2 + 1];
  ActionTypeAdd action;
  char assignmentname[MAX_NAME_BUFFER_LEN];
  char assignment_points[MAX_NAME_BUFFER_LEN];
  char assignment_weight[MAX_NAME_BUFFER_LEN];
  char firstname[MAX_NAME_BUFFER_LEN];
  char lastname[MAX_NAME_BUFFER_LEN];
  char assignment_grade[MAX_NAME_BUFFER_LEN];
} CmdLineResult;


// Parse command line inputs into a structure and sanitize/whitelist inputs.
CmdLineResult parse_cmdline(int argc, char *argv[]) {
  CmdLineResult R = {0};
  int count = 0;
  int i = 0;
  int j = 0;
  bool unique_args[4] = {false};

  // Blacklist invalid argument count
  if (argc < 8) {
    #ifdef DEBUG
      printf("Not enough arguments.\n");
    #endif
    printf("invalid\n");
    exit(INVALID);
  }
  if (argc > MAX_CMD_LINE_ARGS) {
    #ifdef DEBUG
      printf("Too many arguments.\n");
    #endif
    printf("invalid\n");
    exit(INVALID);
  }
  if (argc % 2 == 1) {
    #ifdef DEBUG
      printf("Number of arguments must be even.\n");
    #endif
    printf("invalid\n");
    exit(INVALID);
  }

  // Blacklist invalid string lengths.
  if ((strnlen(argv[0], MAX_NAME_INPUT_LEN + 1) >= MAX_NAME_INPUT_LEN + 1) ||
      (strnlen(argv[1], 3) != 2) ||
      (strnlen(argv[2], MAX_NAME_INPUT_LEN + 1) >= MAX_NAME_INPUT_LEN + 1) ||
      (strnlen(argv[3], 3) != 2) ||
      (strnlen(argv[4], KEY_SIZE * 2 + 1) != KEY_SIZE * 2) ||
      (strnlen(argv[5], 4) != 3)) {
    #ifdef DEBUG
      printf("Invalid argument lengths [0 to 5].\n"); 
    #endif
    printf("invalid\n");
    exit(INVALID);
  }

  for (i = 6; i < argc; i += 2) {
    if (strnlen(argv[i], 4) >= 4 || strnlen(argv[i], 4) <= 1) {
      #ifdef DEBUG
        printf("Invalid flag lengths [6+].\n"); 
      #endif
      printf("invalid\n");
      exit(INVALID);
    }
  }

  for (i = 7; i < argc; i += 2) {
    if (strnlen(argv[i], MAX_NAME_INPUT_LEN + 1) >= MAX_NAME_INPUT_LEN + 1) {
      #ifdef DEBUG
        printf("Invalid argument lengths [7+].\n"); 
      #endif
      printf("invalid\n");
      exit(INVALID);
    }
  }

  // Validate each input for the first 5 arguments
  if (strncmp(argv[1], "-N", 3)) {
    #ifdef DEBUG
      printf("Invalid argument [1].\n"); 
    #endif
    printf("invalid\n");
    exit(INVALID);
  }

  for (i = 0; i < strnlen(argv[2], MAX_NAME_INPUT_LEN); i++) {
    if (!(isalnum(argv[2][i]) || argv[2][i] == '.' || argv[2][i] == '_')) {
      #ifdef DEBUG
        printf("Invalid argument [2].\n"); 
      #endif
      printf("invalid\n");
      exit(INVALID);
    }
  }

  if (strncmp(argv[3], "-K", 3)) {
    #ifdef DEBUG
      printf("Invalid argument [3].\n"); 
    #endif
    printf("invalid\n");
    exit(INVALID);
  }

  for (i = 0; i < strnlen(argv[4], KEY_SIZE * 2); i++) {
    if (!isxdigit(argv[4][i])) {
      #ifdef DEBUG
        printf("Invalid argument [4].\n"); 
      #endif
      printf("invalid\n");
      exit(INVALID);
    }
  }

  // Populate CmdLineResult structure
  strncpy(R.filename, argv[2], MAX_NAME_INPUT_LEN);

  strncpy(R.key, argv[4], KEY_SIZE * 2);

  if (!strncmp(argv[5], "-AA", 4)) {
    for (i = 6; i < argc; i += 2) {
      if (!strncmp(argv[i], "-AN", 4)) {
        for (j = 0; j < strnlen(argv[i+1], MAX_NAME_INPUT_LEN); j++) {
          if (!(isalnum(argv[i+1][j]))) {
            #ifdef DEBUG
              printf("Invalid argument [%d].\n", i+1); 
            #endif
            printf("invalid\n");
            exit(INVALID);
          }
        }
        unique_args[0] = true;
        strncpy(R.assignmentname, argv[i+1], MAX_NAME_INPUT_LEN);
      } else if (!strncmp(argv[i], "-P", 3)) {
        for (j = 0; j < strnlen(argv[i+1], MAX_NAME_INPUT_LEN); j++) {
          if (!(isdigit(argv[i+1][j]))) {
            #ifdef DEBUG
              printf("Invalid argument [%d].\n", i+1); 
            #endif
            printf("invalid\n");
            exit(INVALID);
          }
        }
        unique_args[1] = true;
        strncpy(R.assignment_points, argv[i+1], MAX_NAME_INPUT_LEN);
      } else if (!strncmp(argv[i], "-W", 3)) {
        count = 0;
        for (j = 0; j < strnlen(argv[i+1], MAX_NAME_INPUT_LEN); j++) {
          if (!(isdigit(argv[i+1][j]) || argv[i+1][j] == '.')) {
            #ifdef DEBUG
              printf("Invalid argument [%d].\n", i+1); 
            #endif
            printf("invalid\n");
            exit(INVALID);
          }
          if (argv[i+1][j] == '.') {
            count++;
          }
          if (count >= 2) {
            #ifdef DEBUG
              printf("Invalid argument [%d].\n", i+1); 
            #endif
            printf("invalid\n");
            exit(INVALID);
          }
        }
        unique_args[2] = true;
        strncpy(R.assignment_weight, argv[i+1], MAX_NAME_INPUT_LEN);
      } else {
        #ifdef DEBUG
          printf("Invalid argument [%d].\n", i); 
        #endif
        printf("invalid\n");
        exit(INVALID);
      }
    }
    if (unique_args[0] == false || unique_args[1] == false || unique_args[2] == false) {
      #ifdef DEBUG
        printf("Not enough arguments for -AA.\n"); 
      #endif
      printf("invalid\n");
      exit(INVALID);
    }
    R.action = add_assignment;

  } else if (!strncmp(argv[5], "-DA", 4)) {
    for (i = 6; i < argc; i += 2) {
      if (!strncmp(argv[i], "-AN", 4)) {
        for (j = 0; j < strnlen(argv[i+1], MAX_NAME_INPUT_LEN); j++) {
          if (!(isalnum(argv[i+1][j]))) {
            #ifdef DEBUG
              printf("Invalid argument [%d].\n", i+1); 
            #endif
            printf("invalid\n");
            exit(INVALID);
          }
        }
        unique_args[0] = true;
        strncpy(R.assignmentname, argv[i+1], MAX_NAME_INPUT_LEN);
      } else {
        #ifdef DEBUG
          printf("Invalid argument [%d].\n", i); 
        #endif
        printf("invalid\n");
        exit(INVALID);
      }
    }
    if (unique_args[0] == false) {
      #ifdef DEBUG
        printf("Not enough arguments for -DA.\n"); 
      #endif
      printf("invalid\n");
      exit(INVALID);
    }
    R.action = delete_assignment;

  } else if (!strncmp(argv[5], "-AS", 4)) {
    for (i = 6; i < argc; i += 2) {
      if (!strncmp(argv[i], "-FN", 4)) {
        for (j = 0; j < strnlen(argv[i+1], MAX_NAME_INPUT_LEN); j++) {
          if (!(isalpha(argv[i+1][j]))) {
            #ifdef DEBUG
              printf("Invalid argument [%d].\n", i+1); 
            #endif
            printf("invalid\n");
            exit(INVALID);
          }
        }
        unique_args[0] = true;
        strncpy(R.firstname, argv[i+1], MAX_NAME_INPUT_LEN);
      } else if (!strncmp(argv[i], "-LN", 4)) {
        for (j = 0; j < strnlen(argv[i+1], MAX_NAME_INPUT_LEN); j++) {
          if (!(isalpha(argv[i+1][j]))) {
            #ifdef DEBUG
              printf("Invalid argument [%d].\n", i+1); 
            #endif
            printf("invalid\n");
            exit(INVALID);
          }
        }
        unique_args[1] = true;
        strncpy(R.lastname, argv[i+1], MAX_NAME_INPUT_LEN);
      } else {
        #ifdef DEBUG
          printf("Invalid argument [%d].\n", i); 
        #endif
        printf("invalid\n");
        exit(INVALID);
      }
    }
    if (unique_args[0] == false || unique_args[1] == false) {
      #ifdef DEBUG
        printf("Not enough arguments for -AS.\n"); 
      #endif
      printf("invalid\n");
      exit(INVALID);
    }
    R.action = add_student;

  } else if (!strncmp(argv[5], "-DS", 4)) {
    for (i = 6; i < argc; i += 2) {
      if (!strncmp(argv[i], "-FN", 4)) {
        for (j = 0; j < strnlen(argv[i+1], MAX_NAME_INPUT_LEN); j++) {
          if (!(isalpha(argv[i+1][j]))) {
            #ifdef DEBUG
              printf("Invalid argument [%d].\n", i+1); 
            #endif
            printf("invalid\n");
            exit(INVALID);
          }
        }
        unique_args[0] = true;
        strncpy(R.firstname, argv[i+1], MAX_NAME_INPUT_LEN);
      } else if (!strncmp(argv[i], "-LN", 4)) {
        for (j = 0; j < strnlen(argv[i+1], MAX_NAME_INPUT_LEN); j++) {
          if (!(isalpha(argv[i+1][j]))) {
            #ifdef DEBUG
              printf("Invalid argument [%d].\n", i+1); 
            #endif
            printf("invalid\n");
            exit(INVALID);
          }
        }
        unique_args[1] = true;
        strncpy(R.lastname, argv[i+1], MAX_NAME_INPUT_LEN);
      } else {
        #ifdef DEBUG
          printf("Invalid argument [%d].\n", i); 
        #endif
        printf("invalid\n");
        exit(INVALID);
      }
    }
    if (unique_args[0] == false || unique_args[1] == false) {
      #ifdef DEBUG
        printf("Not enough arguments for -DS.\n"); 
      #endif
      printf("invalid\n");
      exit(INVALID);
    }
    R.action = delete_student;

  } else if (!strncmp(argv[5], "-AG", 4)) {
    for (i = 6; i < argc; i += 2) {
      if (!strncmp(argv[i], "-FN", 4)) {
        for (j = 0; j < strnlen(argv[i+1], MAX_NAME_INPUT_LEN); j++) {
          if (!(isalpha(argv[i+1][j]))) {
            #ifdef DEBUG
              printf("Invalid argument [%d].\n", i+1); 
            #endif
            printf("invalid\n");
            exit(INVALID);
          }
        }
        unique_args[0] = true;
        strncpy(R.firstname, argv[i+1], MAX_NAME_INPUT_LEN);
      } else if (!strncmp(argv[i], "-LN", 4)) {
        for (j = 0; j < strnlen(argv[i+1], MAX_NAME_INPUT_LEN); j++) {
          if (!(isalpha(argv[i+1][j]))) {
            #ifdef DEBUG
              printf("Invalid argument [%d].\n", i+1); 
            #endif
            printf("invalid\n");
            exit(INVALID);
          }
        }
        unique_args[1] = true;
        strncpy(R.lastname, argv[i+1], MAX_NAME_INPUT_LEN);
      } else if (!strncmp(argv[i], "-AN", 4)) {
        for (j = 0; j < strnlen(argv[i+1], MAX_NAME_INPUT_LEN); j++) {
          if (!(isalnum(argv[i+1][j]))) {
            #ifdef DEBUG
              printf("Invalid argument [%d].\n", i+1); 
            #endif
            printf("invalid\n");
            exit(INVALID);
          }
        }
        unique_args[2] = true;
        strncpy(R.assignmentname, argv[i+1], MAX_NAME_INPUT_LEN);
      } else if (!strncmp(argv[i], "-G", 3)) {
        for (j = 0; j < strnlen(argv[i+1], MAX_NAME_INPUT_LEN); j++) {
          if (!(isdigit(argv[i+1][j]))) {
            #ifdef DEBUG
              printf("Invalid argument [%d].\n", i+1); 
            #endif
            printf("invalid\n");
            exit(INVALID);
          }
        }
        unique_args[3] = true;
        strncpy(R.assignment_grade, argv[i+1], MAX_NAME_INPUT_LEN);
      } else {
        #ifdef DEBUG
          printf("Invalid argument [%d].\n", i); 
        #endif
        printf("invalid\n");
        exit(INVALID);
      }
    }
    if (unique_args[0] == false || unique_args[1] == false ||
        unique_args[2] == false || unique_args[3] == false) {
      #ifdef DEBUG
        printf("Not enough arguments for -DS.\n"); 
      #endif
      printf("invalid\n");
      exit(INVALID);
    }
    R.action = add_grade;

  } else {
    #ifdef DEBUG
      printf("Invalid argument [5].\n"); 
    #endif
    printf("invalid\n");
    exit(INVALID);
  }

  return R;
}


// Convert raw characters to floats and int.
ExecutionData convert_cmdline(const CmdLineResult R) {
  ExecutionData E = {0};

  E.action = R.action;

  strncpy(E.assignmentname, R.assignmentname, MAX_NAME_INPUT_LEN);

  if (strnlen(R.assignment_points, MAX_INT_DIGITS + 1) >= MAX_INT_DIGITS + 1) {
    #ifdef DEBUG
      printf("Please keep assignment points < 10^%d.\n", MAX_INT_DIGITS); 
    #endif
    printf("invalid\n");
    exit(INVALID);
  }
  E.assignment_points = (int)strtol(R.assignment_points, NULL, 10);
  if (E.action == add_assignment && E.assignment_points == 0) {
    #ifdef DEBUG
      printf("Assignment points must be greater than 0.\n"); 
    #endif
    printf("invalid\n");
    exit(INVALID);
  }

  E.assignment_weight = strtof(R.assignment_weight, NULL);
  if (E.assignment_weight > 1.0000000001) {
    #ifdef DEBUG
      printf("Please keep assignment weight < 1.\n"); 
    #endif
    printf("invalid\n");
    exit(INVALID);
  }

  strncpy(E.firstname, R.firstname, MAX_NAME_INPUT_LEN);

  strncpy(E.lastname, R.lastname, MAX_NAME_INPUT_LEN);

  if (strnlen(R.assignment_grade, MAX_INT_DIGITS + 1) >= MAX_INT_DIGITS + 1) {
    #ifdef DEBUG
      printf("Please keep assignment grade < 10^%d.\n", MAX_INT_DIGITS); 
    #endif
    printf("invalid\n");
    exit(INVALID);
  }
  E.assignment_grade = (int)strtol(R.assignment_grade, NULL, 10);

  return E;
}


int modify_gradebook(ExecutionData E, DecryptedGradebook* gradebook){
  
  int i = 0;
  int index = 0;
  int index2 = 0;
  float total_weight = 0.0;

  if (E.action == add_assignment) {
    // Check that there's room in the gradebook.
    if (gradebook -> num_assignments >= MAX_ASSIGNMENTS) {
      #ifdef DEBUG
        printf("Gradebook full. There are already %d assignments.\n", MAX_ASSIGNMENTS); 
      #endif
      printf("invalid\n");
      exit(INVALID);
    }
    // Loop through all assignments.
    total_weight = E.assignment_weight;
    index = -1;
    for (i = 0; i < MAX_ASSIGNMENTS; i++) {
      // Find the index in which we place the new assignment into.
      if (index == -1 && gradebook -> assignment_index_filled[i] == false) {
        index = i;
      }
      // Calculate total weight of all assignments.
      total_weight = total_weight + gradebook -> assignments[i].weight;
      if (total_weight > 1.0000000001) {
        #ifdef DEBUG
          printf("Total weight of all assignments will exceed 1.\n"); 
        #endif
        printf("invalid\n");
        exit(INVALID);
      }
      // Check for pre-existing assignments with the same name.
      if (strncmp(E.assignmentname, gradebook -> assignments[i].assignmentname,
        MAX_NAME_INPUT_LEN) == 0) {
        #ifdef DEBUG
          printf("Gradebook already contains assignment with the same name.\n"); 
        #endif
        printf("invalid\n");
        exit(INVALID);
      }
    }
    // Update gradebook.
    gradebook -> num_assignments++;
    gradebook -> assignment_index_filled[index] = true;
    strncpy(gradebook -> assignments[index].assignmentname, E.assignmentname, MAX_NAME_INPUT_LEN);
    gradebook -> assignments[index].maxpoints = E.assignment_points;
    gradebook -> assignments[index].weight = E.assignment_weight;
    // Initialize/clear student grades to 0 for this assignment.
    for (i = 0; i < MAX_STUDENTS; i++) {
      gradebook -> students[i].grades[index] = 0;
    }

  } else if (E.action == delete_assignment) {
    // Check if there's any assignments left to delete.
    if (gradebook -> num_assignments <= 0) {
      #ifdef DEBUG
        printf("Gradebook does not contain any assignments.\n"); 
      #endif
      printf("invalid\n");
      exit(INVALID);
    }
    // Search for assignment
    index = -1;
    for (i = 0; i < MAX_ASSIGNMENTS; i++) {
      // Find the index of the assignment we'd like to delete.
      if (strncmp(E.assignmentname, gradebook -> assignments[i].assignmentname,
        MAX_NAME_INPUT_LEN) == 0) {
        index = i;
      }
    }
    // Exit if assignmentname does not exist.
    if (index == -1) {
      #ifdef DEBUG
        printf("The assignment you'd like to delete does not exist.\n"); 
      #endif
      printf("invalid\n");
      exit(INVALID);
    }
    // Delete assignment from gradebook.
    gradebook -> num_assignments--;
    gradebook -> assignment_index_filled[index] = false;
    memset(gradebook -> assignments[index].assignmentname, '\0', MAX_NAME_BUFFER_LEN);
    gradebook -> assignments[index].maxpoints = 0;
    gradebook -> assignments[index].weight = 0.0;
    // Clear student grades for this assignment.
    for (i = 0; i < MAX_STUDENTS; i++) {
      gradebook -> students[i].grades[index] = 0;
    }

  } else if (E.action == add_student) {
    // Check that there's room in the gradebook.
    if (gradebook -> num_students >= MAX_STUDENTS) {
      #ifdef DEBUG
        printf("Gradebook full. There are already %d students.\n", MAX_STUDENTS); 
      #endif
      printf("invalid\n");
      exit(INVALID);
    }
    // Loop through all students.
    index = -1;
    for (i = 0; i < MAX_STUDENTS; i++) {
      // Find the index in which we place the new assignment into.
      if (index == -1 && gradebook -> student_index_filled[i] == false) {
        index = i;
      }
      // Check for pre-existing  students with the same name.
      if (strncmp(E.firstname, gradebook -> students[i].firstname, MAX_NAME_INPUT_LEN) == 0 &&
          strncmp(E.lastname, gradebook -> students[i].lastname, MAX_NAME_INPUT_LEN) == 0) {
        #ifdef DEBUG
          printf("Gradebook already contains student with the same name.\n"); 
        #endif
        printf("invalid\n");
        exit(INVALID);
      }
    }
    // Update gradebook.
    gradebook -> num_students++;
    gradebook -> student_index_filled[index] = true;
    strncpy(gradebook -> students[index].firstname, E.firstname, MAX_NAME_INPUT_LEN);
    strncpy(gradebook -> students[index].lastname, E.lastname, MAX_NAME_INPUT_LEN);
    // Initialize/clear student grades to 0.
    for (i = 0; i < MAX_ASSIGNMENTS; i++) {
      gradebook -> students[index].grades[i] = 0;
    }
    
  } else if (E.action == delete_student) {
    // Check if there's any students left to delete.
    if (gradebook -> num_students <= 0) {
      #ifdef DEBUG
        printf("Gradebook does not contain any students.\n"); 
      #endif
      printf("invalid\n");
      exit(INVALID);
    }
    // Search for student
    index = -1;
    for (i = 0; i < MAX_STUDENTS; i++) {
      // Find the index of the student we'd like to delete.
      if (strncmp(E.firstname, gradebook -> students[i].firstname, MAX_NAME_INPUT_LEN) == 0 &&
          strncmp(E.lastname, gradebook -> students[i].lastname, MAX_NAME_INPUT_LEN) == 0) {
        index = i;
      }
    }
    // Exit if student does not exist.
    if (index == -1) {
      #ifdef DEBUG
        printf("The student you'd like to delete does not exist.\n"); 
      #endif
      printf("invalid\n");
      exit(INVALID);
    }
    // Delete student from gradebook.
    gradebook -> num_students--;
    gradebook -> student_index_filled[index] = false;
    memset(gradebook -> students[index].firstname, '\0', MAX_NAME_BUFFER_LEN);
    memset(gradebook -> students[index].lastname, '\0', MAX_NAME_BUFFER_LEN);
    // Clear student grades.
    for (i = 0; i < MAX_ASSIGNMENTS; i++) {
      gradebook -> students[index].grades[i] = 0;
    }
    
  } else if (E.action == add_grade) {
    // Check if there's any students.
    if (gradebook -> num_students <= 0) {
      #ifdef DEBUG
        printf("Gradebook does not contain any students.\n"); 
      #endif
      printf("invalid\n");
      exit(INVALID);
    }
    // Search for student.
    index = -1;
    for (i = 0; i < MAX_STUDENTS; i++) {
      // Find the index of the student we'd like to grade.
      if (strncmp(E.firstname, gradebook -> students[i].firstname, MAX_NAME_INPUT_LEN) == 0 &&
          strncmp(E.lastname, gradebook -> students[i].lastname, MAX_NAME_INPUT_LEN) == 0) {
        index = i;
      }
    }
    // Exit if student does not exist.
    if (index == -1) {
      #ifdef DEBUG
        printf("The student does not exist.\n"); 
      #endif
      printf("invalid\n");
      exit(INVALID);
    }
    // Check if there's any assignments.
    if (gradebook -> num_assignments <= 0) {
      #ifdef DEBUG
        printf("Gradebook does not contain any assignments.\n"); 
      #endif
      printf("invalid\n");
      exit(INVALID);
    }
    // Search for assignment.
    index2 = -1;
    for (i = 0; i < MAX_ASSIGNMENTS; i++) {
      // Find the index of the assignment we'd like to grade.
      if (strncmp(E.assignmentname, gradebook -> assignments[i].assignmentname,
        MAX_NAME_INPUT_LEN) == 0) {
        index2 = i;
      }
    }
    // Exit if assignmentname does not exist.
    if (index2 == -1) {
      #ifdef DEBUG
        printf("The assignment does not exist.\n"); 
      #endif
      printf("invalid\n");
      exit(INVALID);
    }
    // Update grades
    gradebook -> students[index].grades[index2] = E.assignment_grade;

  } else {
    #ifdef DEBUG
      printf("Failed to process action command.\n"); 
    #endif
    printf("invalid\n");
    exit(INVALID);
  }

  return SUCCESS;
}


int main(int argc, char *argv[]) {
  
  CmdLineResult R;
  ExecutionData E;
  unsigned char key[KEY_SIZE] = {0};
  DecryptedGradebook gradebook = {0};
  EncryptedGradebook encrypted_gradebook = {0};
  FILE *infile;
  FILE *outfile;

  int ret = 0;
  char * pos;
  size_t count = 0;

  R = parse_cmdline(argc, argv);

  E = convert_cmdline(R);

  // Read from file
  infile = fopen(R.filename, "r");
  if (infile){
    ret = fread(encrypted_gradebook.iv, 1, sizeof(EncryptedGradebook) + 1, infile);
    fflush(infile);
    fclose(infile);

    // If length of file is not equal to the entire EncryptedGradebook structure, return invalid.
    if (ret != sizeof(EncryptedGradebook)) {
      #ifdef DEBUG
        printf("Invalid file length.\n"); 
      #endif
      printf("invalid\n");
      exit(INVALID);
    }
    
    // Create key from chars
    pos = R.key;
    for (count = 0; count < KEY_SIZE; count++){
      sscanf(pos, "%2hhx", &key[count]);
      pos += 2 * sizeof(char);
    }

    // Verify and decrypt
    ret = verify_and_dec(&gradebook, &encrypted_gradebook, key, sizeof(key));
    if (ret == INVALID) {
      #ifdef DEBUG
        printf("Failed to authenticate file.\n"); 
      #endif
      printf("invalid\n");
      exit(INVALID);
    }
  } else {
    #ifdef DEBUG
      printf("Gradebook file does not exist.\n"); 
    #endif
    printf("invalid\n");
    exit(INVALID);
  }

  modify_gradebook(E, &gradebook);

  // Encrypt and authenticate
  ret = enc_and_auth(&gradebook, &encrypted_gradebook, key, sizeof(key));
  if (ret != SUCCESS) {
    #ifdef DEBUG
      printf("Encryption failed.\n");
    #endif
    return INVALID;
  }

  // Open file
  outfile = fopen(R.filename, "w");
  if (outfile == NULL){
    #ifdef DEBUG
      printf("setup: fopen() error could not create file\n");
    #endif
    printf("invalid\n");
    return INVALID;
  }

  // Write encrypted_gradebook to file.
  fwrite(encrypted_gradebook.iv, 1, sizeof(EncryptedGradebook), outfile);
  fflush(outfile);
  fclose(outfile);

  return SUCCESS;
}