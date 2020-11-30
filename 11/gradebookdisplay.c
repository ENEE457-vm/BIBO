#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <stdbool.h>
#include <ctype.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "data.h"


// Tuples for output
typedef struct {
  char lastname[MAX_NAME_BUFFER_LEN];
  char firstname[MAX_NAME_BUFFER_LEN];
  int grade;
} PATuple;

typedef struct {
  char assignmentname[MAX_NAME_BUFFER_LEN];
  int grade;
  int max_grade;
} PSTuple;

typedef struct {
  char lastname[MAX_NAME_BUFFER_LEN];
  char firstname[MAX_NAME_BUFFER_LEN];
  float raw_score;
} PFTuple;


// Stores command line inputs.
typedef struct {
  char filename[MAX_NAME_BUFFER_LEN];
  char key[KEY_SIZE * 2 + 1];
  ActionTypeDisplay action;
  char assignmentname[MAX_NAME_BUFFER_LEN];
  char firstname[MAX_NAME_BUFFER_LEN];
  char lastname[MAX_NAME_BUFFER_LEN];
  bool alpha_order;
  bool grade_order;
} ExecutionData;


ExecutionData parse_cmdline(int argc, char *argv[]) {
  ExecutionData E = {0};
  int count = 0;
  int i = 0;
  int j = 0;
  bool unique_args[2] = {false};

  // Blacklist invalid argument count
  if (argc < 6) {
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

  for (i = 6; i < argc; i++) {
    if (strnlen(argv[i], MAX_NAME_INPUT_LEN + 1) >= MAX_NAME_INPUT_LEN + 1) {
      #ifdef DEBUG
        printf("Invalid argument lengths [6+].\n"); 
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

  // Populate ExecutionData structure
  strncpy(E.filename, argv[2], MAX_NAME_INPUT_LEN);

  strncpy(E.key, argv[4], KEY_SIZE * 2);


  if (!strncmp(argv[5], "-PA", 4)) {
    for (i = 6; i < argc; i++) {
      if (!strncmp(argv[i], "-AN", 4)) {
        if (i+1 >= argc) {
          #ifdef DEBUG
            printf("No argument [%d].\n", i+1); 
          #endif
          printf("invalid\n");
          exit(INVALID);
        }
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
        strncpy(E.assignmentname, argv[i+1], MAX_NAME_INPUT_LEN);
        i++;
      } else if (!strncmp(argv[i], "-A", 3)) {
        E.alpha_order = true;
      } else if (!strncmp(argv[i], "-G", 3)) {
        E.grade_order = true;
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
        printf("Not enough arguments for -PA.\n"); 
      #endif
      printf("invalid\n");
      exit(INVALID);
    }
    if (E.alpha_order == true && E.grade_order == true) {
      #ifdef DEBUG
        printf("Please input either alphabetical order or grade order, not both.\n"); 
      #endif
      printf("invalid\n");
      exit(INVALID);
    }
    E.action = print_assignment;

  } else if (!strncmp(argv[5], "-PS", 4)) {
    for (i = 6; i < argc; i += 2) {
      if (!strncmp(argv[i], "-FN", 4)) {
        if (i+1 >= argc) {
          #ifdef DEBUG
            printf("No argument [%d].\n", i+1); 
          #endif
          printf("invalid\n");
          exit(INVALID);
        }
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
        strncpy(E.firstname, argv[i+1], MAX_NAME_INPUT_LEN);
      } else if (!strncmp(argv[i], "-LN", 4)) {
        if (i+1 >= argc) {
          #ifdef DEBUG
            printf("No argument [%d].\n", i+1); 
          #endif
          printf("invalid\n");
          exit(INVALID);
        }
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
        strncpy(E.lastname, argv[i+1], MAX_NAME_INPUT_LEN);
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
        printf("Not enough arguments for -PA.\n"); 
      #endif
      printf("invalid\n");
      exit(INVALID);
    }
    E.action = print_student;

  } else if (!strncmp(argv[5], "-PF", 4)) {
    for (i = 6; i < argc; i++) {
      if (!strncmp(argv[i], "-A", 3)) {
        E.alpha_order = true;
      } else if (!strncmp(argv[i], "-G", 3)) {
        E.grade_order = true;
      } else {
        #ifdef DEBUG
          printf("Invalid argument [%d].\n", i); 
        #endif
        printf("invalid\n");
        exit(INVALID);
      }
    }
    if (E.alpha_order == true && E.grade_order == true) {
      #ifdef DEBUG
        printf("Please input either alphabetical order or grade order, not both.\n"); 
      #endif
      printf("invalid\n");
      exit(INVALID);
    }
    E.action = print_final;

  } else {
    #ifdef DEBUG
      printf("Invalid argument [5].\n"); 
    #endif
    printf("invalid\n");
    exit(INVALID);
  }

  return E;
}


// Helper function to compare two tuples.
int pa_compare_alpha(PATuple a, PATuple b){
  int cmp_first = strncmp(a.firstname, b.firstname, MAX_NAME_INPUT_LEN);
  int cmp_last = strncmp(a.lastname, b.lastname, MAX_NAME_INPUT_LEN);
  if (cmp_last != 0){
    return cmp_last;
  } else {
    return cmp_first;
  }
}


// Helper function to compare two tuples.
int pf_compare_alpha(PFTuple a, PFTuple b){
  int cmp_first = strncmp(a.firstname, b.firstname, MAX_NAME_INPUT_LEN);
  int cmp_last = strncmp(a.lastname, b.lastname, MAX_NAME_INPUT_LEN);
  if (cmp_last != 0){
    return cmp_last;
  } else {
    return cmp_first;
  }
}


// Print assignment data.
int p_assignment(ExecutionData E, DecryptedGradebook* gradebook, int index){

  PATuple tupl[MAX_STUDENTS] = {0};
  int tupl_index = 0;
  int max_firstname_len = strnlen("First Name", MAX_NAME_INPUT_LEN);
  int max_lastname_len = strnlen("Last Name", MAX_NAME_INPUT_LEN);
  int i = 0;
  int j = 0;
  PATuple temp;

  for (i = 0; i < MAX_STUDENTS; i++) {
    if (gradebook -> student_index_filled[i]) {
      strncpy(tupl[tupl_index].firstname, gradebook -> students[i].firstname, MAX_NAME_INPUT_LEN);
      strncpy(tupl[tupl_index].lastname, gradebook -> students[i].lastname, MAX_NAME_INPUT_LEN);
      tupl[tupl_index].grade = gradebook -> students[i].grades[index];
      if (max_firstname_len < strnlen(tupl[tupl_index].firstname, MAX_NAME_INPUT_LEN)) {
        max_firstname_len = strnlen(tupl[tupl_index].firstname, MAX_NAME_INPUT_LEN);
      }
      if (max_lastname_len < strnlen(tupl[tupl_index].lastname, MAX_NAME_INPUT_LEN)) {
        max_lastname_len = strnlen(tupl[tupl_index].lastname, MAX_NAME_INPUT_LEN);
      }
      tupl_index++;
    }
  }

  if (E.alpha_order == true) {
    for (i = 0; i < tupl_index - 1; i++) {
      for (j = 0; j < tupl_index - i - 1; j++) {
        if (pa_compare_alpha(tupl[j], tupl[j+1]) > 0) {
          memcpy(&temp, &tupl[j], sizeof(PATuple));
          memcpy(&tupl[j], &tupl[j + 1], sizeof(PATuple));
          memcpy(&tupl[j + 1], &temp, sizeof(PATuple));
        }
      }
    }
  } else if (E.grade_order == true) {
    for (i = 0; i < tupl_index - 1; i++) {
      for (j = 0; j < tupl_index - i - 1; j++) {
        if (tupl[j].grade < tupl[j+1].grade) {
          memcpy(&temp, &tupl[j], sizeof(PATuple));
          memcpy(&tupl[j], &tupl[j + 1], sizeof(PATuple));
          memcpy(&tupl[j + 1], &temp, sizeof(PATuple));
        }
      }
    }
  }

  // Print tuples
  for (i = 0; i < tupl_index; i++) {
    printf("(%s, %s, %d)\n",
      tupl[i].lastname,
      tupl[i].firstname,
      tupl[i].grade);
  }
}


// Print student data.
int p_student(ExecutionData E, DecryptedGradebook* gradebook, int index){

  PSTuple tupl[MAX_ASSIGNMENTS] = {0};
  int tupl_index = 0;
  int max_assignmentname_len = strnlen("Assignment Name", MAX_NAME_INPUT_LEN);
  int i = 0;

  for (i = 0; i < MAX_ASSIGNMENTS; i++) {
    if (gradebook -> assignment_index_filled[i]) {
      strncpy(tupl[tupl_index].assignmentname, gradebook -> assignments[i].assignmentname,
              MAX_NAME_INPUT_LEN);
      tupl[tupl_index].grade = gradebook -> students[index].grades[i];
      tupl[tupl_index].max_grade = gradebook -> assignments[i].maxpoints;
      if (max_assignmentname_len < strnlen(tupl[tupl_index].assignmentname, MAX_NAME_INPUT_LEN)) {
        max_assignmentname_len = strnlen(tupl[tupl_index].assignmentname, MAX_NAME_INPUT_LEN);
      }
      tupl_index++;
    }
  }

  // Print tuples
  for (i = 0; i < tupl_index; i++) {
    printf("(%s, %d)\n",
      tupl[i].assignmentname,
      tupl[i].grade);
  }
}


// Print final grades.
int p_final(ExecutionData E, DecryptedGradebook* gradebook){

  PFTuple tupl[MAX_STUDENTS] = {0};
  int tupl_index = 0;
  float grade = 0.0;
  float max_grade = 0.0;
  float fraction = 0.0;
  float total_weight = 0.0;
  int max_firstname_len = strnlen("First Name", MAX_NAME_INPUT_LEN);
  int max_lastname_len = strnlen("Last Name", MAX_NAME_INPUT_LEN);
  int i = 0;
  int j = 0;
  PFTuple temp;

  for (i = 0; i < MAX_ASSIGNMENTS; i++) {
    if (gradebook -> assignment_index_filled[i]) {
      total_weight += gradebook -> assignments[i].weight;
    }
  }

  for (i = 0; i < MAX_STUDENTS; i++) {
    if (gradebook -> student_index_filled[i]) {
      strncpy(tupl[tupl_index].firstname, gradebook -> students[i].firstname, MAX_NAME_INPUT_LEN);
      strncpy(tupl[tupl_index].lastname, gradebook -> students[i].lastname, MAX_NAME_INPUT_LEN);
      for (j = 0; j < MAX_ASSIGNMENTS; j++) {
        if (gradebook -> assignment_index_filled[j]){
          grade = (float)gradebook -> students[i].grades[j];
          max_grade = (float)gradebook -> assignments[j].maxpoints;
          fraction = grade / max_grade;
          tupl[tupl_index].raw_score += fraction * gradebook -> assignments[j].weight;
        }
      }
      if (max_firstname_len < strnlen(tupl[tupl_index].firstname, MAX_NAME_INPUT_LEN)) {
        max_firstname_len = strnlen(tupl[tupl_index].firstname, MAX_NAME_INPUT_LEN);
      }
      if (max_lastname_len < strnlen(tupl[tupl_index].lastname, MAX_NAME_INPUT_LEN)) {
        max_lastname_len = strnlen(tupl[tupl_index].lastname, MAX_NAME_INPUT_LEN);
      }
      tupl_index++;
    }
  }

  if (E.alpha_order == true) {
    for (i = 0; i < tupl_index - 1; i++) {
      for (j = 0; j < tupl_index - i - 1; j++) {
        if (pf_compare_alpha(tupl[j], tupl[j+1]) > 0) {
          memcpy(&temp, &tupl[j], sizeof(PFTuple));
          memcpy(&tupl[j], &tupl[j + 1], sizeof(PFTuple));
          memcpy(&tupl[j + 1], &temp, sizeof(PFTuple));
        }
      }
    }
  } else if (E.grade_order == true) {
    for (i = 0; i < tupl_index - 1; i++) {
      for (j = 0; j < tupl_index - i - 1; j++) {
        if (tupl[j].raw_score < tupl[j+1].raw_score) {
          memcpy(&temp, &tupl[j], sizeof(PFTuple));
          memcpy(&tupl[j], &tupl[j + 1], sizeof(PFTuple));
          memcpy(&tupl[j + 1], &temp, sizeof(PFTuple));
        }
      }
    }
  }

  // Print tuples
  for (i = 0; i < tupl_index; i++) {
    printf("(%s, %s, %g)\n",
      tupl[i].lastname,
      tupl[i].firstname,
      tupl[i].raw_score);
  }
}


int display_gradebook(ExecutionData E, DecryptedGradebook* gradebook){

  int ret = 0;
  int index = 0;
  int i = 0;

  if (E.action == print_assignment) {
    // Check if there's any assignments.
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
      // Find the index of the assignment.
      if (strncmp(E.assignmentname, gradebook -> assignments[i].assignmentname,
        MAX_NAME_INPUT_LEN) == 0) {
        index = i;
      }
    }
    // Exit if assignmentname does not exist.
    if (index == -1) {
      #ifdef DEBUG
        printf("The assignment does not exist.\n"); 
      #endif
      printf("invalid\n");
      exit(INVALID);
    }
    ret = p_assignment(E, gradebook, index);

  } else if (E.action == print_student) {
    // Check if there's any students.
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
      // Find the index of the student.
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
    ret = p_student(E, gradebook, index);

  } else if (E.action == print_final) {
    // Check if there's any assignments.
    if (gradebook -> num_assignments <= 0) {
      #ifdef DEBUG
        printf("Gradebook does not contain any assignments.\n"); 
      #endif
      printf("invalid\n");
      exit(INVALID);
    }
    // Check if there's any students.
    if (gradebook -> num_students <= 0) {
      #ifdef DEBUG
        printf("Gradebook does not contain any students.\n"); 
      #endif
      printf("invalid\n");
      exit(INVALID);
    }
    ret = p_final(E, gradebook);

  } else {
    #ifdef DEBUG
      printf("Failed to process action command.\n"); 
    #endif
    printf("invalid\n");
    exit(INVALID);
  }

  return ret;
}


int main(int argc, char *argv[]) {
  
  ExecutionData E;
  unsigned char key[KEY_SIZE] = {0};
  DecryptedGradebook gradebook = {0};
  EncryptedGradebook encrypted_gradebook = {0};
  FILE *infile;

  int ret = 0;
  char * pos;
  size_t count = 0;

  E = parse_cmdline(argc, argv);

  // Read from file
  infile = fopen(E.filename, "r");
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
    pos = E.key;
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

  display_gradebook(E, &gradebook);

  return SUCCESS;
}
