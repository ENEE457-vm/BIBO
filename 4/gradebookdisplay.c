#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/queue.h>
#include <ctype.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "data.h"


int compare_total_grades(const void* a, const void* b) {
  return (((Student*)b)->total_grade) - (((Student*)a)->total_grade);
}

int compare_assignment_grades(const void* a, const void* b) {
  return (((Student*)b)->assignment_grade) - (((Student*)a)->assignment_grade);
}

int compare_names(const void* a, const void* b) { 
    int value = 0;
    // sort alphabetically by last name first
    value = strcmp(((Student*)a)->lastname, ((Student*)b)->lastname); 
    if (value == 0) {
      // if same last name, sort by first name next
      value = strcmp(((Student*)a)->firstname, ((Student*)b)->firstname); 
    }
    return value;
} 

/* For all students, print out each student grade for the assignment specified */
int print_Assignment(CmdLineResult *R, Gradebook *G) {
  char *assignment_name = R->test_name;
  Student student;
  Assignment assignment;
  int i = 0;
  int j = 0;
  int assignmentFound = 0;
  int isValid = 1;

  int student_count = G->student_count;
  int assign_count = G->assign_count;
  // check to see if this assignment exists in gradebook
  if(assign_count > 0 && student_count > 0) {
    student = G->students[0];
    for(j = 0; j < assign_count; j++) {
        assignment = student.assignments[j];
        if(strcmp(assignment_name,assignment.name)==0) {
            assignmentFound = 1;
            break;
        }
    }
  }

  if(assignmentFound) {
    // create a temporary list for sorting
    Student students[student_count];
    for(i=0; i < student_count; i++) {
      student = G->students[i];
      for(j = 0; j < assign_count; j++) {
        assignment = student.assignments[j];
        // found the assignment
        if(strcmp(assignment_name,assignment.name)==0) {
          student.assignment_grade = assignment.grade;
        }
      }
      // add to list for sorting
      students[i] = student;
    }

    // sort the list alpabetically by last name, first name
    if(R->alphabetical) {
      //printf("Sort alphabetically\n");
      qsort(students, student_count, sizeof(Student), compare_names);
    // sort the list numerically by final grade from highest to lowest
    } else if (R->grade_order) {
      //printf("Sort by final grade from highest to lowest\n");
      qsort(students, student_count, sizeof(Student), compare_assignment_grades);
    }

    for(i=0; i <student_count; i++) {
      printf("(%s, %s, %d)\n", students[i].lastname, students[i].firstname, students[i].assignment_grade);
    }
    isValid = 1;
  } else {
    // assignment was not found in gradebook
    isValid = 0;
  }
  return isValid;
}
/* For student specifed, print out their grade for all assignments */
int print_Student(CmdLineResult *R, Gradebook *G) {
  char *firstname = R->first_name;
  char *lastname = R->last_name;
  Student student;
  Assignment assignment;
  int i  = 0;
  int j = 0;
  int isValid = 0;
  int studentFound = 0;
  int index = 0;


  int student_count = G->student_count;
  int assign_count = G->assign_count;
  // check if the student already exists
  for(i = 0; i < student_count; i++) {
    student = G->students[i];
     if((strcmp(student.firstname, firstname) == 0) && (strcmp(student.lastname, lastname) == 0)) {
      studentFound = 1;
      index = i;
      break;
    }
  }

  if(studentFound) {
    student = G->students[index];
    if((strcmp(student.firstname, firstname) == 0) && (strcmp(student.lastname, lastname) == 0)) {
      //printf("Display Student\n");
      for(j = 0; j < assign_count; j++) {
          assignment = student.assignments[j];
          printf("(%s, %d)\n", assignment.name, assignment.grade);
      }
    }
    isValid = 1;
  } else {
    // no student with first and last name found in gradebook
    isValid = 0;
  }
  return isValid;
}

/* print final grades for each student */
void print_Final(CmdLineResult *R, Gradebook *G){
  Student student;
  Assignment assignment;
  int points = 0;
  float weight = 0;
  int grade = 0;
  float total_grade = 0;
  int i  = 0;
  int j = 0;
  
  
  int student_count = G->student_count;
  Student student_list[student_count];
  int assign_count = G->assign_count;
  //printf("Print Final Grades\n");
  for(i = 0; i < student_count; i++) {
    total_grade = 0;
    student = G->students[i];
    for(j = 0; j < assign_count; j++) {
        assignment = student.assignments[j];
        grade = assignment.grade;
        points = assignment.points;
        weight = assignment.weight;
        total_grade = total_grade + ((grade/ (float) points)* weight);
        student.total_grade = total_grade;
    }
    // add to list for sorting
    student_list[i] = student;
  }
  // sort the list alpabetically by last name, first name
  if(R->alphabetical) {
    //printf("Sort alphabetically\n");
    qsort(student_list, student_count, sizeof(Student), compare_names);
  // sort the list numerically by final grade from highest to lowest
  } else if (R->grade_order) {
    //printf("Sort by final grade from highest to lowest\n");
    qsort(student_list, student_count, sizeof(Student), compare_total_grades);
  }

  for(i = 0; i < student_count; i++) {
    printf("(%s, %s, %f)\n", student_list[i].lastname, student_list[i].firstname, student_list[i].total_grade);
  }
  return;
}


CmdLineResult parse_commands(int argc, char *argv[]) {
  CmdLineResult R;
  static int file_flag;
  static int key_flag;
  static int printassign;
  static int printstudent;
  static int printfinal;
  static int assign_name;
  static int alphabetical;
  static int grade_order;
  static int student_fn;
  static int student_ln;
  int is_valid = 1;
  int action = 0;
  int alpha, alnum;
  int opt_index;
  int opt;
  int i = 0;

  static struct option options_long[] = {
    // gradebook filename
    {"N",     required_argument,  &file_flag,      1},
    // key
    {"K",     required_argument,  &key_flag,       1},
    // print assignment - print the assignment for all students in the form (Last Name, First Name, Grade)
    {"PA",    no_argument,        &printassign,     1},
    // print student - print out all grades for one student in the the form (Assignment Name, Grade)
    {"PS",    no_argument,        &printstudent,    1},
    // print final - print final grades for all students
    {"PF",    no_argument,        &printfinal,    1}, 
    // assignment name
    {"AN",    required_argument,  &assign_name,     1},
    // alphabetical order - print the ordered by last name first, then fist name
    {"A",     no_argument,  &alphabetical,    1},
    // grade order from highest to lowest
    {"G",     no_argument,  &grade_order,     1},
    // first name
    {"FN",    required_argument,  &student_fn,      1},
    // last name
    {"LN",    required_argument,  &student_ln,      1}
  };
  
  //TODO Code this
  if(argc>=5) { 
      // A gradebook name must be specified first with -N 
      // A key must be specified second with -K
      // An action must be specified third with exactly one of {-PA, -PS, -PF}
      //  After the action option, other arguments from the set {-AN, -FN, -LN, -A, -G} may be specified in any order 
      //  and if the same argument is provided multiple times, the last value is accepted.
      if ((strcmp(argv[1],"-N") == 0) && (strcmp(argv[3],"-K")==0)) {
          is_valid = 1;
        } else {
          is_valid = 0;
      }

      if ((strcmp(argv[5], "-PA") == 0) || (strcmp(argv[5],"-PS") == 0) ||
            (strcmp(argv[5], "-PF") == 0)) {
          is_valid = 1;
        } else {
          is_valid = 0;
        }
        //printf("\nValid: %d\n", is_valid);
  } else {
    is_valid = 0;
  }

  while(((opt = getopt_long_only(argc, argv, "-:abc:d::", options_long, &opt_index)) != -1) && is_valid) {
        switch(opt) {
          case 0:
              /*
              printf("long option %s\n", options_long[opt_index].name);
              if (optarg) {
                printf(" with arg %s\n", optarg);
              } 
              */

              // filename
              if (file_flag && (strcmp(options_long[opt_index].name,"N") == 0)) {
                if ((optarg != NULL) && (strlen(optarg) < 20)) {
                  alnum = 1;
                  // check if assignment name is alphanumeric, has underscore or period
                  for(i = 0; optarg[i] != '\0'; i++) {
                      if (isalnum(optarg[i]) == 0) {
                          alnum = 0;
                          if ((optarg[i] == '_') || (optarg[i] == '.')) {
                            alnum = 1;
                          } else {
                            alnum = 0;
                            break;
                          }
                      }
                  }
                  if(alnum) {
                    strcpy(R.filename,optarg);
                  } else {
                    is_valid = 0;
                  }
                } else {
                  // no argument provided for file option - invalid
                  is_valid = 0;
                  break;
                }
              }

              if(key_flag) {
                // convert hex input string to 16 bytes
                if(strcmp(options_long[opt_index].name, "K") == 0) {
                  if ((optarg != NULL) && (strlen(optarg) == 32)) {
                    strncpy(R.key, optarg, 32);
                  } else {
                    is_valid = 0;
                    break;
                  }
                }
              }

              if(printassign) {
                // assignment name
                R.printassign = printassign;
                R.alphabetical = alphabetical;
                R.grade_order = grade_order;
                if (strcmp(options_long[opt_index].name, "AN") == 0) {
                  if ((optarg != NULL) && (strlen(optarg) < 20)) {
                    alnum = 1;
                    // check if assignment name is alphanumeric
                    for(i = 0; optarg[i] != '\0'; i++) {
                        if (isalnum(optarg[i]) == 0) {
                            alnum = 0;
                      }
                    }
                    if(alnum) {
                      strncpy(R.test_name,optarg, 20);
                      //printf("Assignment: %s\n",R.test_name);
                    } else {
                      is_valid = 0;
                      break;
                    }
                  } else {
                    is_valid = 0;
                    break;
                  }
                }
              }

              if(printstudent) {
                R.printstudent = printstudent;
                // check first name
                if(strcmp(options_long[opt_index].name, "FN") == 0) {
                  if ((optarg != NULL) && (strlen(optarg) < 20)) {
                    alpha = 1;
                     // check if first name is alphabetic 
                    for (i = 0; optarg[i] != '\0'; i++) {
                      if (isalpha(optarg[i]) == 0) {
                          alpha = 0;
                      }
                     }                   
                    if(alpha) {
                      strncpy(R.first_name,optarg, 20);
                      //printf("First Name: %s\n",R.first_name);
                    } else {
                      is_valid = 0;
                      break;
                    }
                  } else {
                    is_valid = 0;
                    break;
                  }
                }
                // check last name
                if(strcmp(options_long[opt_index].name, "LN") == 0) {
                  //printf("Size of last name:%d\n", strlen(optarg));
                  if ((optarg != NULL) && (strlen(optarg) < 20)) {
                    alpha = 1;
                     // check if last name is alphabetic 
                    for (i = 0; optarg[i] != '\0'; i++) {
                      if (isalpha(optarg[i]) == 0) {
                          alpha = 0;
                      }
                    }
                    if (alpha) {
                      strncpy(R.last_name,optarg, 20);
                      //printf("Last Name: %s\n",R.last_name);
                    } else {
                      is_valid = 0;
                      break;
                    }
                  } else {
                    is_valid = 0;
                    break;
                  }
                }
              }
              if (printfinal) {
                R.printfinal = printfinal;
                R.alphabetical = alphabetical;
                R.grade_order = grade_order;
              }
              break;
          case '?':
            //printf("invalid - unknown option");
            is_valid = 0;
            break;
          case ':':
              is_valid = 0;
              break;
          default:
              break;
        }
  }

  // check to see if an action provided is conflicting 
  action = printstudent + printassign + printfinal;
  // check validity of commands and arguments 
  if (is_valid) {
    int count = 0;

    if (printstudent && student_fn && student_ln) {
      R.good = 1;
  	} else if(printassign && assign_name) {
      count = 0;
      if(alphabetical) {
        count++;
      } 
      if(grade_order) {
        count++;
      }
      // exactly one of -A, -G should be specified. Otherwise, error occurs
      if (count == 1) {
        R.good = 1;
      } else {
        R.good = 0;
      }
  	} else if (printfinal) {
      count = 0;
      if(alphabetical) {
        count++;
      } 
      if(grade_order) {
        count++;
      }
      // exactly one of -A, -G should be specified
      if (count == 1) {
        R.good = 1;
      } else {
        R.good = 0;
      }
  	} else {
  		R.good = 0;
  	}
  } else {
    R.good = 0;
  }
  return R;
}

int main(int argc, char *argv[]) {
  char  *logpath = NULL;
  CmdLineResult R;
  Gradebook key_data;
  FILE *fp;
  int isValid = 0;
  int i = 0, j = 0, z = 0;
  int num;
  unsigned char iv[16];
  unsigned char key[16];
  unsigned char ciphertext[425632];
  unsigned char decryptedtext[425612];
  
  R = parse_commands(argc, argv);

  if(R.good == 1) {
    fp = fopen(R.filename, "r");
    if(fp != NULL) {
      fread(ciphertext, sizeof(ciphertext), 1, fp);
      // retrieving iv from read data - iv is not encrypted
      memcpy(iv, ciphertext, sizeof(iv)); 
      /*
      printf("IV:");
      for(i = 0; i < sizeof(iv); i++) {
        printf("%02x", iv[i]);
      }
      printf("\n");
      */

      // accessing key from input
      while(j < 32) {
        sscanf(&R.key[j], "%02x", &num);
        key[z] = (unsigned char) num;
        j = j + 2;
        z = z + 1;
      }
      /*
      printf("KEY:");
      for(int i = 0; i < sizeof(iv); i++) {
        printf("%02x", key[i]);
      }
      printf("\n");
      */

      // decrypt the data 
      int len = decrypt(ciphertext+16, 425616, key, iv, decryptedtext);
      //printf("Length of Decrypted text %d\n", len);
      memcpy(&key_data, decryptedtext, sizeof(decryptedtext));
      //printf("Successful commands\n");
      if (R.printassign) {
        isValid = print_Assignment(&R, &key_data);
      }else if (R.printfinal) {
        print_Final(&R, &key_data);
        isValid = 1;
      }else if (R.printstudent) {
        isValid = print_Student(&R, &key_data);
      }
      fclose(fp);
    } else {
      // filename does not exist in directory
      printf("invalid\n");
      return(255);
    }
    // invalid error - student name or assignment name not found in gradebook
    if(isValid == 0) {
      printf("invalid\n");
      return(255);
    }
  } else {
    // set of options entered is invalid
    printf("invalid\n");
    return(255);
  }
  return 0;

}
