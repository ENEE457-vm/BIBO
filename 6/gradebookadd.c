#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>
#include <float.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/rand.h>

#include "data.h"
#include "crypto.h"


enum AddOption {AN, FN, LN, P, W, G, ER};


enum ActionType parse_action(char *act) {
  enum ActionType action;
  if (strcmp(act, "-AA") == 0) {
    action = AA;
  } else if (strcmp(act, "-DA") == 0) {
    action = DA;
  } else if (strcmp(act, "-AS") == 0) {
    action = AS;
  } else if (strcmp(act, "-DS") == 0) {
    action = DS;
  } else if (strcmp(act, "-AG") == 0) {
    action = AG;
  } else {
    action = ERR;
  }
  return action;
}

enum AddOption parse_option(char *opt) {
  enum AddOption option;
  if (strcmp(opt, "-AN") == 0) {
    option = AN;
  } else if (strcmp(opt, "-P") == 0) {
    option = P;
  } else if (strcmp(opt, "-W") == 0) {
    option = W;
  } else if (strcmp(opt, "-FN") == 0) {
    option = FN;
  } else if (strcmp(opt, "-LN") == 0) {
    option = LN;
  } else if (strcmp(opt, "-G") == 0) {
    option = G;
  } else {
    option = ER;
  }
  return option;
}

float gradebook_weight(Gradebook *g) {
  float w = 0;
  for (int i = 0; i < g->num_assignments; i++) {
    w += g->assignments[i].weight;
  }
  return w;
}

int add_assignment(char *gradebook, unsigned char *key, char *name, int points, float weight) {
  // Get gradebook
  Gradebook *g = malloc(sizeof(Gradebook));
  if (!retrieve_gradebook(gradebook, key, g)) {
    Print("Erorr: Could not retrieve gradebook.\n");
    free(g);
    return 0;
  }

  // Check we have room for assignment
  if (g->num_assignments >= MAX_NUM_ASSIGNMENTS) {
    printf("Cannot add assignment, number of assignments in the gradebook is at max capacity.\n");
    free(g);
    return 0;
  }

  // Make sure that assignment doesn't exist
  if (-1 != get_assignment_position(g, name)) {
    Print("Error: Assignment already exists.\n");
    free(g);
    return 0;
  }

  // Make sure weight doesn't exceed 1
  if (weight + gradebook_weight(g) > 1) {
    Print("Error: That weight would cause the gradebook weight to exceed 1.\n");
    free(g);
    return 0;
  }

  // Create new assignment
  Assignment assign;
  strcpy(assign.name, name);
  assign.points = points;
  assign.weight = weight;
  for (int i = 0; i < g->num_students; i++) {
    Grade grade;
    memcpy(&grade.student, &g->students[i], sizeof(Student));
    grade.grade = -1;
    memcpy(&assign.grades[i], &grade, sizeof(Grade));
  }

  // Add assignment to gradebook
  if(NULL == memcpy(&g->assignments[g->num_assignments++], &assign, sizeof(Assignment))) {
    Print("Error: Could not write to gradebook struct.\n");
    free(g);
    return 0;
  }

  // Write new gradebook to file.
  if(!write_gradebook(gradebook, key, g)) {
    Print("Error: Could not write to gradebook file.\n");
    free(g);
    return 0;
  }
  
  free(g);
  return 1;
}

int delete_assignment(char *gradebook, unsigned char *key, char *name) {
  // Get gradebook
  Gradebook *g = malloc(sizeof(Gradebook));
  if (!retrieve_gradebook(gradebook, key, g)) {
    Print("Erorr: Could not retrieve gradebook.\n");
    return 0;
  }

  // Make sure that assignment exists
  int assign_pos = get_assignment_position(g, name);
  if (-1 == assign_pos) {
    Print("Error: Assignment doesn't exist.\n");
    return 0;
  }

  // Go into assignments array in gradebook and remove it
  // then shift over everything that was to the right
  for (int i = assign_pos; i + 1 < g->num_assignments; i++) {
    memcpy(&g->assignments[i], &g->assignments[i+1], sizeof(Assignment));
  }
  g->num_assignments--;

  // Write new gradebook to file.
  if(!write_gradebook(gradebook, key, g)) {
    Print("Error: Could not write to gradebook file.\n");
    return 0;
  }

  return 1;
}

int add_student(char *gradebook, unsigned char *key, char *firstname, char *lastname) {
  // Get gradebook
  Gradebook *g = malloc(sizeof(Gradebook));
  if (!retrieve_gradebook(gradebook, key, g)) {
    Print("Erorr: Could not retrieve gradebook.\n");
    free(g);
    return 0;
  }

  if (g->num_students >= MAX_CLASS_SIZE) {
    printf("Cannot add student, number of students in the gradebook is at max capacity.\n");
    free(g);
    return 0;
  }

  if (-1 != get_student_position(g, firstname, lastname)) {
    Print("Error: Student already exists.\n");
    free(g);
    return 0;
  }

  // Create new student
  Student new_student;
  strcpy(new_student.firstname, firstname);
  strcpy(new_student.lastname, lastname);

  // Add student to gradebook
  if(NULL == memcpy(&g->students[g->num_students], &new_student, sizeof(Student))) {
    Print("Error: Could not write to gradebook struct.\n");
    free(g);
    return 0;
  }

  // Attach student to all assignments in that position
  for (int i = 0; i < g->num_assignments; i++) {
    memcpy(&g->assignments[i].grades[g->num_students].student, &new_student, sizeof(Student));
    g->assignments[i].grades[g->num_students].grade = -1;
  }

  g->num_students++;

  // Write new gradebook to file.
  if(!write_gradebook(gradebook, key, g)) {
    Print("Error: Could not write to gradebook file.\n");
    free(g);
    return 0;
  }

  free(g);
  return 1;
}

int delete_student(char *gradebook, unsigned char *key, char *firstname, char *lastname) {
  // Get gradebook
  Gradebook *g = malloc(sizeof(Gradebook));
  if (!retrieve_gradebook(gradebook, key, g)) {
    Print("Erorr: Could not retrieve gradebook.\n");
    return 0;
  }

  // Make sure that student exists
  int student_pos = get_student_position(g, firstname, lastname);
  if (-1 == student_pos) {
    Print("Error: Student doesn't exist.\n");
    return 0;
  }

  // Remove all traces of student.
  for (int i = student_pos; i + 1 < g->num_students; i++) {
    memcpy(&g->students[i], &g->students[i + 1], sizeof(Student));
    for (int j = 0; j < g->num_assignments; j++) {
      memcpy(&g->assignments[j].grades[i], &g->assignments[j].grades[i + 1], sizeof(Grade));
    }
  }
  g->num_students--;

  // Write new gradebook to file.
  if(!write_gradebook(gradebook, key, g)) {
    Print("Error: Could not write to gradebook file.\n");
    return 0;
  }
}

int add_grade(char *gradebook, unsigned char *key, char *firstname, char *lastname, char* assignment_name, int grade) {
  // Get gradebook
  Gradebook *g = malloc(sizeof(Gradebook));
  if (!retrieve_gradebook(gradebook, key, g)) {
    Print("Erorr: Could not retrieve gradebook.\n");
    return 0;
  }

  // Get assignment and student positions
  int student_pos = get_student_position(g, firstname, lastname);
  int assignment_pos = get_assignment_position(g, assignment_name);
  if (student_pos == -1 || assignment_pos == -1) {
    Print("Error: Assignment or student does not exist.\n");
    return 0;
  }

  // Check that grade < points
  if (grade > g->assignments[assignment_pos].points) {
    Print("Error: Grade is too high for this assignment.\n");
    return 0;
  }

  // Add grade to assignment for that student
  g->assignments[assignment_pos].grades[student_pos].grade = grade;

  // Write new gradebook to file.
  if(!write_gradebook(gradebook, key, g)) {
    Print("Error: Could not write to gradebook file.\n");
    return 0;
  }

  return 1;
}

int process_action(CmdLineResult *R, int argc, char *argv[]) {
  enum ActionType action;
  if (argc >= 7) {
    action = parse_action(argv[5]);
    R->action = action;
  } else {
    action = ERR;
  }

  int i;
  char options[6][MAX_ARG_LEN];
  char seen_options[6] = { 0 };

  // Parse the options
  for (i = 7; i < argc && argc % 2 == 0; i += 2) {
    enum AddOption o = parse_option(argv[i - 1]);
    if (o == ER) {
      Print("Error: invalid option when parsing.");
      return 0;
    } else {
      strcpy(options[(int) o], argv[i]);
      seen_options[(int) o] = 1;
    }
  }

  // Check for malformed inputs
  char *endptr;
  errno = 0;
  if (seen_options[(int) AN]) {
    if (!valid_string(options[(int) AN], 1)) {
      Print("Bad assignment name.\n");
      return 0;
    }
  }
  if (seen_options[(int) FN]) {
    if (!valid_string(options[(int) FN], 0)) {
      Print("Bad first name.\n");
      return 0;
    }
  }
  if (seen_options[(int) LN]) {
    if (!valid_string(options[(int) LN], 0)) {
      Print("Bad last name.\n");
      return 0;
    }
  }
  if (seen_options[(int) P]) {
    long p = strtol(options[(int) P], &endptr, 10);
    if (options[(int) P] == endptr || '\0' != *endptr || ((LONG_MIN == p || LONG_MAX == p) && ERANGE == errno) || p > INT_MAX || p < INT_MIN || p < 0) {
      Print("Bad points.\n");
      return 0;
    }
  }
  if (seen_options[(int) W]) {
    float w = strtof(options[(int) W], &endptr);
    if (options[(int) W] == endptr || '\0' != *endptr || ((FLT_MIN == w || FLT_MAX == w) && ERANGE == errno) || w < 0 || w > 1) {
      Print("Bad weight.\n");
      return 0;
    }
  }
  if (seen_options[(int) G]) {
    long g = strtol(options[(int) G], &endptr, 10);
    if (options[(int) G] == endptr || '\0' != *endptr || ((LONG_MIN == g || LONG_MAX == g) && ERANGE == errno) || g > INT_MAX || g < INT_MIN || g < 0) {
      Print("Bad grade.\n");
      return 0;
    }
  }

  // Make sure that an action has all the options it requires and those options satisfy the existance requirements
  if (action == AA && seen_options[(int) AN] && !seen_options[(int) FN] && !seen_options[(int) LN] && seen_options[(int) P] && seen_options[(int) W] && !seen_options[(int) G]) { // Add assignment
    float w = strtof(options[(int) W], &endptr);
    
    // Copy data to struct.
    strcpy(R->assignmentname, options[(int) AN]);
    R->points = (int) strtol(options[(int) P], &endptr, 10);
    R->weight = w;
  } else if (action == DA && seen_options[(int) AN] && !seen_options[(int) FN] && !seen_options[(int) LN] && !seen_options[(int) P] && !seen_options[(int) W] && !seen_options[(int) G]) { // Delete assignment
    // Copy data to struct.
    strcpy(R->assignmentname, options[(int) AN]);
  } else if (action == AS && !seen_options[(int) AN] && seen_options[(int) FN] && seen_options[(int) LN] && !seen_options[(int) P] && !seen_options[(int) W] && !seen_options[(int) G]) { // Add student
    // Copy data to struct.
    strcpy(R->firstname, options[(int) FN]);
    strcpy(R->lastname, options[(int) LN]);
  } else if (action == DS && !seen_options[(int) AN] && seen_options[(int) FN] && seen_options[(int) LN] && !seen_options[(int) P] && !seen_options[(int) W] && !seen_options[(int) G]) { // Delete student
    // Copy data to struct.
    strcpy(R->firstname, options[(int) FN]);
    strcpy(R->lastname, options[(int) LN]);
  } else if (action == AG && seen_options[(int) AN] && seen_options[(int) FN] && seen_options[(int) LN] && !seen_options[(int) P] && !seen_options[(int) W] && seen_options[(int) G]) { // Add grade
    int g = (int) strtol(options[(int) G], &endptr, 10);

    // Copy data to struct.
    strcpy(R->assignmentname, options[(int) AN]);
    strcpy(R->firstname, options[(int) FN]);
    strcpy(R->lastname, options[(int) LN]);
    R->grade = g;
  } else {
    Print("Error: Could not match action.\n");
    return 0;
  }

  return 1;
}

int parse_cmdline(CmdLineResult *R, int argc, char *argv[]) {

  if (!validate_beginning(R, argc, argv)) {
    Print("Error: beginning invalid.\n");
    return 0;
  }

  // Parse and process the action arguments for this gradebook
  if(!process_action(R, argc, argv)) {
    Print("Error: Was not able to process action arguments.\n");
    return 0;
  }

  return 1;
}

int main(int argc, char *argv[]) {
  int res = 1;

  CmdLineResult *R = malloc(sizeof(CmdLineResult));
  if (!parse_cmdline(R, argc, argv)) {
    res = 0;
  } else {
    if (R->action == AA) {
      if (!add_assignment(R->filename, R->key, R->assignmentname, R->points, R->weight)) {
        Print("Error: Could not add assignment.\n");
        res = 0;
      }
    } else if (R->action == DA) {
      if (!delete_assignment(R->filename, R->key, R->assignmentname)) {
        Print("Error: Could not delete assignment.\n");
        res = 0;
      }
    } else if (R->action == AS) {
      if (!add_student(R->filename, R->key, R->firstname, R->lastname)) {
        Print("Error: Could not add student.\n");
        res = 0;
      }
    } else if (R->action == DS) {
      if (!delete_student(R->filename, R->key, R->firstname, R->lastname)) {
        Print("Error: Could not delete student.\n");
        res = 0;
      }
    } else if (R->action == AG) {
      if (!add_grade(R->filename, R->key, R->firstname, R->lastname, R->assignmentname, R->grade)) {
        Print("Error: Could not add grade.\n");
        res = 0;
      }
    } else {
      res = 0;
    }
  }
  
  free(R);
  remove("temp");
  if (!res) {
    printf("invalid\n");
    return 255;
  } else {
    return 0;
  }
}
