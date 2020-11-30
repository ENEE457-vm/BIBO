#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "data.h"

enum DisplayOption {AN, FN, LN, A, G, ER};

void swap(Grade *a, Grade *b) { 
    Grade temp = *a; 
    *a = *b; 
    *b = temp; 
}

void swap_final(Final_Grade *a, Final_Grade *b) { 
    Final_Grade temp = *a; 
    *a = *b; 
    *b = temp; 
}

void sort_grades(Grade *grades, int n, int order) {
  for (int i = 0; i < n - 1; i++) {
    for (int j = 0; j < n - i - 1; j++) {
      if (order) {
        if (grades[j].grade < grades[j+1].grade)
          swap(&grades[j], &grades[j+1]);
      } else {
        if (strcmp(grades[j].student.lastname, grades[j+1].student.lastname) > 0) {
          swap(&grades[j], &grades[j+1]);
        } else if (strcmp(grades[j].student.lastname, grades[j+1].student.lastname) == 0) {
          if (strcmp(grades[j].student.firstname, grades[j+1].student.firstname) > 0)
            swap(&grades[j], &grades[j+1]);
        }
      }
    }
  }
}

void sort_final_grades(Final_Grade *grades, int n, int order) {
  for (int i = 0; i < n - 1; i++) {
    for (int j = 0; j < n - i - 1; j++) {
      if (order) {
        if (grades[j].grade < grades[j+1].grade)
          swap_final(&grades[j], &grades[j+1]);
      } else {
        if (strcmp(grades[j].student.lastname, grades[j+1].student.lastname) > 0) {
          swap_final(&grades[j], &grades[j+1]);
        } else if (strcmp(grades[j].student.lastname, grades[j+1].student.lastname) == 0) {
          if (strcmp(grades[j].student.firstname, grades[j+1].student.firstname) > 0)
            swap_final(&grades[j], &grades[j+1]);
        }
      }
    }
  }
}

int print_assignment(char *gradebook, char *key, char *name, int order) {
  // Get gradebook
  Gradebook *g = malloc(sizeof(Gradebook));
  if (!retrieve_gradebook(gradebook, key, g)) {
    Print("Erorr: Could not retrieve gradebook.\n");
    free(g);
    return 0;
  }
  
  // Look for assignment in gradebook
  int pos = get_assignment_position(g, name);
  if (-1 == pos) {
    Print("Error: Assignment doesn't exist.\n");
    free(g);
    return 0;
  }

  // Get list of grades for that assignment
  Grade grades[g->num_students];
  memcpy(&grades, &g->assignments[pos].grades, sizeof(Grade)*g->num_students);
  
  sort_grades(grades, g->num_students, order);

  int grade;
  for (int i = 0; i < g->num_students; i++) {
    grade = grades[i].grade;
    if (grade != -1) {
      printf("(%s, %s, %d)\n", grades[i].student.lastname, grades[i].student.firstname, grade);
    } else {
      printf("(%s, %s, N/A)\n", grades[i].student.lastname, grades[i].student.firstname);
    }
  }
  
  free(g);
  return 1;
}

int print_student(char *gradebook, char *key, char *firstname, char *lastname) {
  // Get gradebook
  Gradebook *g = malloc(sizeof(Gradebook));
  if (!retrieve_gradebook(gradebook, key, g)) {
    Print("Erorr: Could not retrieve gradebook.\n");
    free(g);
    return 0;
  }

  // Look for student in gradebook
  int pos = get_student_position(g, firstname, lastname);
  if (-1 == pos) {
    Print("Error: Student doesn't exist.\n");
    free(g);
    return 0;
  }

  // Print out student's grades
  Grade grade;
  for (int i = 0; i < g->num_assignments; i++) {
    grade = g->assignments[i].grades[pos];
    if (grade.grade != -1) {
      printf("(%s, %d)\n", g->assignments[i].name, grade.grade);
    } else {
      printf("(%s, N/A)\n", g->assignments[i].name);
    }
  }

  free(g);
  return 1;
}

int print_final(char *gradebook, char *key, int order) {
  // Get gradebook
  Gradebook *g = malloc(sizeof(Gradebook));
  if (!retrieve_gradebook(gradebook, key, g)) {
    Print("Error: Could not retrieve gradebook.\n");
    free(g);
    return 0;
  }

  // Calculate final grade for each student
  Final_Grade *final_grades = calloc(g-> num_students, sizeof(Final_Grade));
  for (int s_pos = 0; s_pos < g->num_students; s_pos++) {
    for (int a_pos = 0; a_pos < g->num_assignments; a_pos++) {
      float grade = g->assignments[a_pos].grades[s_pos].grade;
      if (grade > 0) {
        float fraction = (grade / g->assignments[a_pos].points);
        final_grades[s_pos].grade += fraction * g->assignments[a_pos].weight;
      }
    }
    strcpy(final_grades[s_pos].student.firstname, g->students[s_pos].firstname);
    strcpy(final_grades[s_pos].student.lastname, g->students[s_pos].lastname);
  }
  
  sort_final_grades(final_grades, g->num_students, order);

  // Print final grades
  float grade;
  for (int i = 0; i < g->num_students; i++) {
    grade = final_grades[i].grade;
    printf("(%s, %s, %.4f)\n", final_grades[i].student.lastname, final_grades[i].student.firstname, grade);
  }

  free(g);
  free(final_grades);
  return 1;
}

enum ActionType parse_action(char *act) {
  enum ActionType action;
  if (strcmp(act, "-PA") == 0) {
    action = PA;
  } else if (strcmp(act, "-PS") == 0) {
    action = PS;
  } else if (strcmp(act, "-PF") == 0) {
    action = PF;
  } else {
    action = ERR;
  }
  return action;
}

enum DisplayOption parse_option(char *opt) {
  enum DisplayOption option;
  if (strcmp(opt, "-AN") == 0) {
    option = AN;
  } else if (strcmp(opt, "-FN") == 0) {
    option = FN;
  } else if (strcmp(opt, "-LN") == 0) {
    option = LN;
  } else if (strcmp(opt, "-G") == 0) {
    option = G;
  } else if (strcmp(opt, "-A") == 0) {
    option = A;
  } else {
    option = ER;
  }
  return option;
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

  // Parse the options (not gauranteed to have pairs)
  int option_required = 1;
  enum DisplayOption prev = ER;
  for (i = 6; i < argc; i++) {
    enum DisplayOption o = parse_option(argv[i]);

    if (o == ER && option_required) {
      Print("Error: invalid option when parsing.\n");
      return 0;
    } else {
      seen_options[(int) o] = 1;
      if (o == ER && (prev == AN || prev == FN || prev == LN)) {
        strcpy(options[(int) prev], argv[i]);
        option_required = 1;
      } else if (o == AN || o == FN || o == LN) {
        option_required = 0;
      }
    }

    prev = o;
  }

  // Check to see inputs are not malformed
  if (!option_required) {
    Print("Ended with an option that needed a parameter.\n");
    return 0;
  }
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

  // Make sure that an action has all the options it requires and those options satisfy the existance requirements
  if (action == PA && seen_options[(int) AN] && !seen_options[(int) FN] && !seen_options[(int) LN] && seen_options[(int) G] + seen_options[(int) A] == 1) { // Print assignment
    // Copy data to struct
    strcpy(R->assignmentname, options[(int) AN]);
    R->order = seen_options[(int) G];
  } else if (action == PS && !seen_options[(int) AN] && seen_options[(int) FN] && seen_options[(int) LN] && seen_options[(int) G] + seen_options[(int) A] == 0) { // Print Student
    // Copy data to struct
    strcpy(R->firstname, options[(int) FN]);
    strcpy(R->lastname, options[(int) LN]);
  } else if (action == PF && !seen_options[(int) AN] && !seen_options[(int) FN] && !seen_options[(int) LN] && seen_options[(int) G] + seen_options[(int) A] == 1) { // Print Final
    R->order = seen_options[(int) G];
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
    Print("Error: Could not correctly parse the command line.\n");
    res = 0;
  } else {
    if (R->action == PA) {
      if (!print_assignment(R->filename, R->key, R->assignmentname, R->order)) {
        Print("Error: Could not print assignment.\n");
        res = 0;
      }
    } else if (R->action == PS) {
      if (!print_student(R->filename, R->key, R->firstname, R->lastname)) {
        Print("Error: Could not print student.\n");
        res = 0;
      }
    } else if (R->action == PF) {
      if (!print_final(R->filename, R->key, R->order)) {
        Print("Error: Could not print final.\n");
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
