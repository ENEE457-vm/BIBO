#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <wordexp.h>
#include <ctype.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "data.h"


CmdLineResult parse_cmdline(int argc, char *argv[]) {
  CmdLineResult R = { 0 };
  FILE*file;
  /* static values are default to 0 */
  static int check_file;
  static int check_key;
  static int check_add;
  static int check_delete;
  static int addstudent;
  static int deletestudent;
  static int addgrade;
  static int assignment_name;
  static int check_points;
  static int check_weight;
  static int check_first_name;
  static int check_last_name;
  static int check_grade;
  int file_arg = 0;
  int opt_index,r = -1;
  int is_valid = 1;
  int opt = 0;
  int i = 0;
  int action = 0;
  int alpha, alnum;
  R.good = -1;

  static struct option options_long[] = {
    // gradebook filename
    {"N",     required_argument,  &check_file,    1},
    // key
    {"K",     required_argument,  &check_key,     1},
    // Add a new assignment
    {"AA",    no_argument,        &check_add,     1},
    // Delete an assignment for all students
    {"DA",    no_argument,        &check_delete,  1}, 
    // Add a new student
    {"AS",    no_argument,        &addstudent,    1},
    // Delete student
    {"DS",    no_argument,        &deletestudent, 1},
    // Add grade
    {"AG",    no_argument,        &addgrade,   1}, 
    //  - name of assignment (add-assignment, delete_assignment, add_grade)
    {"AN",    required_argument,  &assignment_name,  1},
    //  - assignment points
    {"P",     required_argument,  &check_points,  1}, 
    //  - assignment weight
    {"W",     required_argument,  &check_weight,  1},
    //  - first name (add-student, delete-student, add_grade)
    {"FN",     required_argument,  &check_first_name, 1},
    // - last name (add-student, delete-student, add_grade)
    {"LN",     required_argument,  &check_last_name,  1},
    // - grade number of points student received 
    {"G",       required_argument, &check_grade,      1},
    {NULL,    0,                  NULL,           0}
  };

    if(argc>=5) { 
        // gradebook name must be specified first with - N
        // a key must be specified second with -L
        // An action must be specified third with exactly one of {-AA, -DA, -AS, -DS, -AG}
        // action options {-AN, -FN, -LN, -P, -W, -G} may be specified in any order afterwards
        if ((strcmp(argv[1],"-N") == 0) && (strcmp(argv[3],"-K")==0)) {
          is_valid = 1;
        } else {
          is_valid = 0;
        }

        if ((strcmp(argv[5], "-AA") == 0) || (strcmp(argv[5],"-DA") == 0) ||
            (strcmp(argv[5], "-AS") == 0) || (strcmp(argv[5],"-DS") == 0) ||
            (strcmp(argv[5], "-AG") == 0)) {
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
            if (check_file && (strcmp(options_long[opt_index].name,"N") == 0)) {
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

            if(check_key) {
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
            // Add Assignment 
            if(check_add) {
            	R.check_add = check_add;
              // assignment name
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
              
              // assignment points -- non negative integer
              if (strcmp(options_long[opt_index].name, "P") == 0) {
                if (optarg != NULL) {
                  R.points = atoi(optarg);
                  if(R.points >= 0) {
                    //printf("Points: %d\n",R.points);
                  } else {
                    is_valid = 0;
                  }
                   
                } else {
                  is_valid = 0;
                  break;
                }
              }

              // assignment weight
              if (strcmp(options_long[opt_index].name, "W") == 0) {
                if (optarg != NULL) {
                  R.weight = atof(optarg);
                  if (R.weight >= 0 && R.weight <= 1) {
                    //printf("Weight: %f\n",R.weight);
                  } else {
                    is_valid = 0;
                  }
                } else {
                  is_valid = 0;
                  break;
                }
              }
            }

            // Delete Assignment
            if(check_delete) {
              R.check_delete = check_delete;
              // assignment name
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
            
            // Add Grade
            if(addgrade) {
                R.addgrade = addgrade;
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

                // assignment name
                if (strcmp(options_long[opt_index].name, "AN") == 0) {
                  if ((optarg != NULL) && (strlen(optarg) < 20)) {
                    alnum = 1;
                    // check if assignment name is alphanumeric
                    for (i = 0; optarg[i] != '\0'; i++) {
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

                // grade -- non negative interger
                if (strcmp(options_long[opt_index].name, "G") == 0) {
                  if (optarg != NULL){
                    R.grade = atoi(optarg);
                    if (R.grade >= 0) {
                      //printf("Grade: %d\n",R.grade);
                    } else {
                      is_valid = 0;
                    }
                  } else {
                    is_valid = 0;
                    break;
                  }
                }
            }

            if (addstudent || deletestudent) {
            	R.addstudent = addstudent;
            	R.deletestudent = deletestudent;
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
  // check to see if only action is specifed
  action = check_add + check_delete + addstudent + deletestudent + addgrade;
  // check validity of all commands and arguments
  if (is_valid && (action == 1)) {
  	if(check_add && assignment_name && check_points && check_weight) {
  		R.good = 1;
  	} else if (check_delete && assignment_name) {
  		R.good = 1;
  	} else if (addstudent && check_first_name && check_last_name) {
  		R.good = 1;
  	} else if (deletestudent && check_first_name && check_last_name) {
  		R.good = 1;
  	} else if (addgrade && check_first_name && check_last_name && assignment_name && check_grade) {
  		R.good = 1;
  	} else {
  		R.good = 0;
  	}
  } else {
    R.good = 0;
  }

  return R;
}


int add_Assignment(CmdLineResult* R, Gradebook *G) {
  char *assignment_name = R->test_name;
  int points = R->points;
  float weight = R->weight;
  int i = 0;
  int j = 0;
  int assignmentExists = 0;
  int isValid = 1;
  Student student;
  Assignment assignment;
  //printf("Add Assignment\n");

  int student_count = G->student_count;
  int assignment_count = G->assign_count;
  // check to see an assignment with this name already exists
  if(assignment_count > 0 && student_count > 0) {
    student = G->students[0];
    for(j = 0; j < assignment_count; j++) {
        assignment = student.assignments[j];
        // assignment found
        if(strcmp(assignment_name,assignment.name)==0) {
            assignmentExists = 1;
            isValid = 0;
            break;
        }
    }
  }
  // assignment has not been added yet
  if(assignmentExists == 0) {
    if ((G->total_weight + weight) <= 1) {
      for(i = 0; i < student_count; i++) {
        student = G->students[i];
        assignment = student.assignments[assignment_count];
        strcpy(assignment.name, assignment_name);
        // concatenate student first and last name for hash
        unsigned char src[40] = {"0"};
  		unsigned char dest[40] = {"0"};
  		unsigned char sha_hash[20];
        strcpy(src, student.firstname);
        strcpy(dest, student.lastname);
        strcat(dest,src);
        // compute the sha1 hash of this string
        calculate_sha(dest, strlen(dest), sha_hash);
        /*
        printf("SHA1 IN FUNC:");
        for (i = 0; i < sizeof(sha_hash); i++) {
          printf("%02x", sha_hash[i]);
        } 
        printf("\n");
        */
        memcpy(assignment.sha, sha_hash, sizeof(sha_hash)); 
        assignment.points = points;
        assignment.weight = weight;
        /*
        printf("Student Name: %s, %s\n", student.lastname, student.firstname);
        printf("Points: %d\n", assignment.points);
        printf("Weight: %f\n", assignment.weight);
        printf("Assignment Name: %s\n", assignment.name); */
        student.assignments[assignment_count] = assignment;
        G->students[i] = student;
      }
      G->total_weight = G->total_weight + weight;
      G->assign_count = assignment_count + 1;
      //printf("Total Weight: %f", G->total_weight);
      isValid = 1;
    } else {
      isValid = 0;
      // invalid - total weight with new assignment > 1
    }
  } else {
    // assignment already with exists with the name;
    isValid = 0;
  }
  return isValid;
}

int delete_Assignment(CmdLineResult* R, Gradebook *G) {
  char *assignment_name = R->test_name;
  int assignmentFound = 0;
  float assignment_weight;
  int isValid = 1;
  Student student;
  Assignment assignment;
  Assignment replaceAssignment;
  int i = 0;
  int j = 0;
  int index = 0;

  int student_count = G->student_count;
  int assign_count = G->assign_count;
  // check to see an assignment with this name already exists
  if(assign_count > 0 && student_count > 0) {
    student = G->students[0];
    for(j = 0; j < assign_count; j++) {
        assignment = student.assignments[j];
        // assignment found, will be deleted
        if(strcmp(assignment_name,assignment.name)==0) {
            assignmentFound = 1;
            index = j;
            assignment_weight = assignment.weight;
            break;
        }
    }
  }

  if (assignmentFound && index < assign_count) {
    assign_count--;
    // iterate through all students, and delete the assignment
    for(i = 0; i < student_count; i++) {
        student = G->students[i];
        // delete the assignment from the student
        for(j = index; j < assign_count; j++) {
          student.assignments[j] = student.assignments[j + 1]; 
        }
        // reset the previously last assignment in the array
        student.assignments[assign_count] = replaceAssignment;
        G->students[i] = student;
    }
    G->assign_count = assign_count;
    G->total_weight = (G->total_weight) - assignment_weight;
    //printf("New Total Weight of Assignments: %f\n", G->total_weight);
    isValid = 1;
  } else {
    // assignment not found
    isValid = 0;
  }
  return isValid;
}

int add_Student(CmdLineResult* R, Gradebook *G) {
  char * firstname = R->first_name;
  char * lastname = R->last_name;
  int studentExists = 0;
  Student student;
  int isValid = 1;
  int i = 0;
  //printf("Adding student\n");

  // keeps track of next available spot for student in Gradebook
  int student_count = G->student_count;

  // check if the student already exists
  for(i = 0; i < student_count; i++) {
    student = G->students[i];
     if((strcmp(student.firstname, firstname) == 0) && (strcmp(student.lastname, lastname) == 0)) {
      studentExists = 1;
      break;
    }
  }

  if(studentExists == 0) {
    student = G->students[student_count];
    strcpy(student.firstname, firstname);
    strcpy(student.lastname, lastname);
    G->students[student_count] = student;
    G->student_count = student_count + 1;
    isValid = 1;
  } else {
    // student already exists
    isValid = 0;
  }
  return isValid;
}

int delete_Student(CmdLineResult* R, Gradebook *G) {
  char * firstname = R->first_name;
  char * lastname = R->last_name;
  Student student;
  Student replaceStudent;
  Assignment assignment;
  int i = 0;
  int j = 0;
  int index = 0;
  int isValid = 1;

  //printf("Deleting student\n");
  int student_count = G->student_count;
  for (i = 0; i < student_count; i++) {
    student = G->students[i];
    // found the student to delete
    if((strcmp(student.firstname, firstname) == 0) && (strcmp(student.lastname, lastname) == 0)) {
      index = i;
      break;
    }
  }

  if (i < student_count) {
    student_count--;
    isValid = 1;
    // replace the deleted elements and shrink student array
    for (j = index; j < student_count; j++) {
        G->students[j] = G->students[j+1];
    }
    // reset the previous last element in the array
    replaceStudent.firstname[0] = '\0';
    replaceStudent.lastname[0]= '\0';
    for(j=0; j < 40; j++) {
      assignment = replaceStudent.assignments[j];
      assignment.name[0] = '\0';
      assignment.grade = 0;
      assignment.points = 0;
      replaceStudent.assignments[j] = assignment;
    }
    G->students[student_count] = replaceStudent;
    G->student_count = student_count;
  } else {
    // student not found in gradebook -> error
    isValid = 0;
  }
  return isValid;
}

int add_Grade(CmdLineResult* R, Gradebook *G) {
  char * firstname = R->first_name;
  char * lastname = R->last_name;
  char *assignment_name = R->test_name;
  int grade = R->grade;
  int studentFound = 0;
  int assignmentFound = 0;
  int i = 0;
  int j = 0;
  int isValid = 0;
  Student student;
  Assignment assignment;
  //printf("Adding Grade\n");

  int student_count = G->student_count;
  int assign_count = G->assign_count;
  for(i = 0; i < student_count; i++) {
    student = G->students[i];
    if((strcmp(student.firstname, firstname) == 0) && (strcmp(student.lastname, lastname) == 0)) {
      studentFound = 1;
      for(j = 0; j < assign_count; j++) {
        assignment = student.assignments[j];
        if(strcmp(assignment_name,assignment.name)==0) {
          // concatenate student first and last name for hash
          unsigned char src[40] = {"0"};
  		  unsigned char dest[40] = {"0"};
  		  unsigned char sha_hash[20];
          strcpy(src,student.firstname);
          strcpy(dest,student.lastname);
          strcat(dest,src);
          // compute the sha1 hash of this string
          calculate_sha(dest, strlen(dest), sha_hash);
          /*
          printf("SHA1 SAVED:");
          for (i = 0; i < sizeof(assignment.sha); i++) {
            printf("%02x", assignment.sha[i]);
          } 
          printf("\n");
          */
          // check if calculated hash equals the saved hash
          if(memcmp(assignment.sha,sha_hash, 20) == 0) {
            assignmentFound = 1;
            assignment.grade = grade;
            student.assignments[j] = assignment;
            G->students[i] = student; 
          } else {
            isValid = 0;
          }
        }
      }
    }
  }
  if(studentFound && assignmentFound) {
    isValid = 1; 
  } else {
    isValid = 0;
  }
  return isValid;
}

int main(int argc, char *argv[]) {
  CmdLineResult R;
  Gradebook G;
  FILE *fp;
  int isValid = 0;
  int i = 0, j = 0, z = 0;
  int num;
  unsigned char iv[16];
  unsigned char new_iv[16];
  unsigned char key[16];
  unsigned char ciphertext[425632];
  unsigned char decryptedtext[425612];
  unsigned char new_ciphertext[425616]; 
  unsigned char totalciphertext[425632];
  
  R = parse_cmdline(argc, argv);
  if(R.good == 1) {
    //printf("Successful commands\n");
    fp = fopen(R.filename, "r+");

    if(fp != NULL) {
      fread(ciphertext, sizeof(ciphertext), 1, fp);
      // iv is not encrypted
      memcpy(iv, ciphertext, sizeof(iv)); 

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
      int len = decrypt(ciphertext+16, sizeof(ciphertext)-16, key, iv, decryptedtext);
      //printf("Length of Decrypted text %d\n", len);
      memcpy(&G, decryptedtext, sizeof(decryptedtext));

      // generate a new iv
      RAND_bytes(new_iv,16);
      //printf("Read student count %d\n", G.student_count);
      if (R.check_add && G.assign_count < 39) {
      	fclose(fp);
        isValid = add_Assignment(&R,&G);
        if(isValid) {
          fp = fopen(R.filename, "w");
          memcpy(decryptedtext, &G, sizeof(Gradebook));
          // re-encrypt the data
          len = encrypt(decryptedtext, sizeof(decryptedtext), key, new_iv, new_ciphertext);
          memcpy(totalciphertext, new_iv, sizeof(new_iv));
          memcpy(totalciphertext+16, new_ciphertext, sizeof(new_ciphertext));
          fwrite(totalciphertext, sizeof(totalciphertext), 1, fp);
          fclose(fp);
        }
      } else if (R.check_delete) {
      	fclose(fp);
        isValid = delete_Assignment(&R,&G);
        if(isValid) {
          fp = fopen(R.filename, "w");
          memcpy(decryptedtext, &G, sizeof(Gradebook));
          // re-encrypt the data
          len = encrypt(decryptedtext, sizeof(decryptedtext), key, new_iv, new_ciphertext);
          memcpy(totalciphertext, new_iv, sizeof(new_iv));
          memcpy(totalciphertext+16, new_ciphertext, sizeof(new_ciphertext));
          fwrite(totalciphertext, sizeof(totalciphertext), 1, fp);
          fclose(fp);
        }
      } else if (R.addstudent && G.student_count < 199) {

      	fclose(fp);
        isValid = add_Student(&R,&G);
        if(isValid) {
          fp = fopen(R.filename, "w");
          memcpy(decryptedtext, &G, sizeof(Gradebook));
          // re-encrypt the data
          len = encrypt(decryptedtext, sizeof(decryptedtext), key, new_iv, new_ciphertext);
          memcpy(totalciphertext, new_iv, sizeof(new_iv));
          memcpy(totalciphertext+16, new_ciphertext, sizeof(new_ciphertext));
          fwrite(totalciphertext, sizeof(totalciphertext), 1, fp);
          /*
          if(fwrite != 0)   {
            printf("contents to file written successfully !\n"); 
          } else {
            printf("error writing file !\n");
          } */
          fclose(fp);
        }
      } else if (R.deletestudent) {
      	fclose(fp);
        isValid = delete_Student(&R,&G);
        if(isValid) {
          // student was deleted 
          fp = fopen(R.filename, "w");
          memcpy(decryptedtext, &G, sizeof(Gradebook));
          // re-encrypt the data
          len = encrypt(decryptedtext, sizeof(decryptedtext), key, new_iv, new_ciphertext);
          memcpy(totalciphertext, new_iv, sizeof(new_iv));
          memcpy(totalciphertext+16, new_ciphertext, sizeof(new_ciphertext));
          fwrite(totalciphertext, sizeof(totalciphertext), 1, fp);
          fclose(fp);
        }
      } else if (R.addgrade) {
      	fclose(fp);
        isValid = add_Grade(&R,&G);
        if(isValid) {
          fp = fopen(R.filename, "w");
          memcpy(decryptedtext, &G, sizeof(Gradebook));
          // re-encrypt the data
          len = encrypt(decryptedtext, sizeof(decryptedtext), key, new_iv, new_ciphertext);
          memcpy(totalciphertext, new_iv, sizeof(new_iv));
          memcpy(totalciphertext+16, new_ciphertext, sizeof(new_ciphertext));
          fwrite(totalciphertext, sizeof(totalciphertext), 1, fp);
          fclose(fp);
        }
      } else {
        fclose(fp);
      }
    } else {
    	// file does not exist in directory
    	printf("invalid\n");
    	return(255);
    }
  } else {
  	// set of options/values is not valid 
    printf("invalid\n");
    return(255);
  }
  // invalid error from not finding student or assignment in the gradebook
  if (isValid == 0) {
    printf("invalid\n");
    return(255);
  }
  
  return 0;
}
