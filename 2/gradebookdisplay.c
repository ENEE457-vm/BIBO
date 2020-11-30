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
#include <openssl/sha.h>
#include <ctype.h>
#include "crypto.c"
#include "data.h"

typedef struct _CmdLineResult {
    int good;
    char action[4];
    unsigned char key[KEY_SIZE];

    char gradebook_name[MAX_NAME_LEN];
    char assignment_name[MAX_NAME_LEN];
    char first_name[MAX_NAME_LEN];
    char last_name[MAX_NAME_LEN];

    //print by last name or grades
    char print_order[3];
} CmdLineResult;

void print_parsed_inputs(CmdLineResult R) {
    printf("\nR struct is: \n");
    printf("action is %s\n", R.action);
    printf("key is \n");
    printf("0x");
    for(size_t count = 0; count < sizeof(R.key)/sizeof(*(R.key)); count++) {
        printf("%02x", R.key[count]);
    }
    printf("\n");

    printf("gradebook name is %s\n", R.gradebook_name);
    printf("assignment_name is %s\n", R.assignment_name);
    printf("first_name is %s\n", R.first_name);
    printf("last_name is %s\n", R.last_name);

    printf("print order is %s\n", R.print_order);
}

int checkIfMode(unsigned char* str) {
  if (!strcmp(str, "-PA") || !strcmp(str, "-PS") || !strcmp(str, "-PF")) {
      return 1;
  } else {
      return 0;
  }
}

void print_gradebook(PaddedDecryptedGradebook gb) {
    printf("gradebook name is: %s\n", gb.gradebook_name);

    printf("num students is: %d\n", gb.num_students);
    printf("Current students:\n");
    for (int i = 0; i < gb.num_students; i++) {
        printf("Student %d FN: '%s' and LN: '%s' \n", i+1, gb.students[i].first_name, gb.students[i].last_name);
        
        printf("Grades for this student\n");
        for (int j = 0; j < gb.num_assignments; j++) {
            printf("Assignment %d name '%s' grade: %d\n", j, gb.assignments[j].assignment_name, gb.students[i].grades[j]);
        }

    }
    printf("\n");
   
    printf("num assignments is: %d\n", gb.num_assignments);
    printf("Current assignments:\n");
    for (int i = 0; i < gb.num_assignments; i++) {
        printf("Assignment %d Assignment name: '%s' maxPts: %d and weight: %f \n", i+1, gb.assignments[i].assignment_name, 
                gb.assignments[i].maxPts, gb.assignments[i].weight);
    }

    printf("total assignment weight is %f\n", gb.total_assignment_weight);
    printf("\n");
}

CmdLineResult parse_cmdline(int argc, char *argv[]) {
    CmdLineResult R = {0};
    R.good = 0;

    int opt,r = -1;

    if(argc <= 6) {
        R.good = 255;
    } else { 
        int sum = 0;

        // check if only got 1 action request
        for(int i = 0; i < argc; i++) {
            if (strcmp("-PA", argv[i]) == 0) {
                strcpy(R.action, "-PA");
            }
            if (strcmp("-PS", argv[i]) == 0) {
                strcpy(R.action, "-PS");
            }
            if (strcmp("-PF", argv[i]) == 0) {
                strcpy(R.action, "-PF");
            }
           
            
            sum += checkIfMode(argv[i]);
        } 

        // check proper order of first 5 args, if we only got 1 action requested and if we gto even # args to parse
        if (strcmp("-N", argv[1]) != 0 || strcmp("-K", argv[3]) != 0 || sum != 1 || strcmp(argv[5], R.action) != 0){
          R.good = 255;
        } else {
            // convert key to hex format and store it in R.bytearray
            // char hexstring = argv[4];
            const char *pos = argv[4]; //hexstring;

            for(size_t count = 0; count < sizeof(R.key)/sizeof(*(R.key)); count++) {
                sscanf(pos, "%2hhx", &R.key[count]);
                pos += 2; 
            }

            // getting gradebook
            strcpy(R.gradebook_name, argv[2]);

            // 1. PRINT ASSIGNMENT ________________________________________
            if (!strcmp("-PA", R.action)) {
                // num_AorG != 1 --> checks if get more than one -A or -G
                int num_AorG = 0;

                for(int i = 6; i < argc; i++) {
          
                    // Assignment Name
                    if (strcmp("-AN", argv[i]) == 0) {
                        // if got -AN but no thing after
                        if (i+1 == argc) {
                            R.good = 255;
                            break;
                        }

                        char * str = argv[i+1];

                        if (strlen(str) < MAX_NAME_LEN) {
                            for (int j = 0; j < strlen(str); j++) {
                            // checks for non alpha-numeric chars
                                if (isalpha(str[j]) == 0 && isdigit(str[j]) == 0 && isspace(str[j]) == 0) {
                                    R.good = 255;
                                    break;
                                }
                            }

                            strcpy(R.assignment_name, argv[i+1]);
                            i++;

                        } else {
                            R.good = 255;
                            break;
                        }

                    } else if (strcmp("-A", argv[i]) == 0) {
                            num_AorG += 1;
                            strcpy(R.print_order, "-A");

                    } else if (strcmp("-G", argv[i]) == 0) {
                            num_AorG += 1;
                            strcpy(R.print_order, "-G");
                     // other error
                    } else {
                      R.good = 255;
                      break;
                    }
                }

                if (num_AorG != 1) {
                    R.good = 255;
                }

            // 2. PRINT STUDENT ___________________________________________
            } else if (!strcmp("-PS", R.action)) {
                for(int i = 6; i < argc; i+=2) {
                    if (strcmp("-FN", argv[i]) == 0) {
                        if (i+1 == argc) {
                            R.good = 255;
                            break;
                        }

                        char * str = argv[i+1];

                        if (strlen(str) < MAX_NAME_LEN) {
                            for (int j = 0; j < strlen(str); j++) {
                                // checks for non alpha-numeric chars
                                if (isalpha(str[j]) == 0 && isspace(str[j]) == 0) {
                                    R.good = 255;
                                    break;
                                }
                            }

                            strcpy(R.first_name, argv[i+1]);
                        } else {
                            R.good = 255;
                            break;
                        }

                    } else if (strcmp("-LN", argv[i]) == 0) {
                        if (i+1 == argc) {
                            R.good = 255;
                            break;
                        }
                        char * str = argv[i+1];

                        if (strlen(str) < MAX_NAME_LEN) {
                            for (int j = 0; j < strlen(str); j++) {
                                // checks for non alpha-numeric chars
                                if (isalpha(str[j]) == 0 && isspace(str[j]) == 0) {
                                  R.good = 255;
                                  break;
                                }
                            }

                            strcpy(R.last_name, argv[i+1]);
                        } else {
                            R.good = 255;
                            break;
                        }

                    // error
                    } else {
                        R.good = 255;
                        break;
                    }
                }

            // PRINT FINAL GRADES __________________________________________
            } else if (!strcmp("-PF", R.action)) {
                int num_AorG = 0;
                for(int i = 6; i < argc; i++) {

                    if (strcmp("-A", argv[i]) == 0) {
                        num_AorG += 1;
                        strcpy(R.print_order, "-A");

                    } else if (strcmp("-G", argv[i]) == 0) {
                        num_AorG += 1;
                        strcpy(R.print_order, "-G");

                    // other error
                    } else {
                      R.good = 255;
                      break;
                    }
                }

                if (num_AorG != 1) {
                    R.good = 255;
                }

            } else {
              R.good = 255;
            }
        }
    }
    return R;
}

// prints all students' grades for specified assignment: (Last Name, First Name, Grade) 
// sorted alphabetically by Lastname/firstname or by highest to lowest grade
int print_Assignment(CmdLineResult R, PaddedDecryptedGradebook *gb) {
    int a = -1;

    // check if assignment name exists
    for (int i = 0; i < gb->num_assignments; i++) {
        if (strcmp(gb->assignments[i].assignment_name, R.assignment_name) == 0) {
            a = i;
        }
    }

    if (a == -1) {
      // assignment doesn't exist! 
      return 255;
    } else {
        // made copy of gradebook - used to change for sorting 
        PaddedDecryptedGradebook temp; 
        strcpy(temp.gradebook_name, gb->gradebook_name);
        memcpy(&temp.num_students, &gb->num_students, sizeof(int));
        memcpy(&temp.students, &gb->students, sizeof(Student) * 1000);
        memcpy(&temp.num_assignments, &gb->num_assignments, sizeof(int));
        memcpy(&temp.total_assignment_weight, &gb->total_assignment_weight, sizeof(float));
        memcpy(&temp.assignments, &gb->assignments, sizeof(Assignment) * 100);

        // print alphabetically by Lastname/firstname
        // ** check if last names match!
        if (strcmp(R.print_order, "-A") == 0) {
            for (int i = 1; i < temp.num_students; i++) {
                for (int j = 0; j < temp.num_students - 1; j++) {
                    // if mutiple people have same last name 
                    if (strcmp(temp.students[j].last_name, temp.students[j+1].last_name) == 0) {
                        if (strcmp(temp.students[j].first_name, temp.students[j+1].first_name) > 0) {
                          Student st = temp.students[j];
                          temp.students[j] = temp.students[j+1];
                          temp.students[j+1] = st;
                        }

                    } else if (strcmp(temp.students[j].last_name, temp.students[j+1].last_name) > 0) {
                        Student st = temp.students[j];
                        temp.students[j] = temp.students[j+1];
                        temp.students[j+1] = st;
                    }
                }
            }       

          // print by highest to lowest grade
        } else if (strcmp(R.print_order, "-G") == 0) {
            for (int i = 1; i < temp.num_students; i++) {
                for (int j = 0; j < temp.num_students - 1; j++) {
                    if (temp.students[j].grades[a] < temp.students[j+1].grades[a]) {
                        Student st = temp.students[j];
                        temp.students[j] = temp.students[j+1];
                        temp.students[j+1] = st;
                    }
                }
            }
        } else {
            return 255;
        }

        // print results:
        for (int i = 0; i < temp.num_students; i++) {
            printf("(%s, %s, %d)\n", temp.students[i].last_name, temp.students[i].first_name, temp.students[i].grades[a]);
        }

        return 1;
    }
}

int print_Student(CmdLineResult R, PaddedDecryptedGradebook *gb) {
    int s = -1;
    // check if student name  exists
    for (int i = 0; i < gb->num_students; i++) {
        if (strcmp(gb->students[i].first_name, R.first_name) == 0 && strcmp(gb->students[i].last_name, R.last_name) == 0) {
            s = i;
        }
    }

    if (s == -1) {
        // student not found
        return 255;
    } else {
        // print out all students' grades in format: (Assignment Name, Grade)
        for (int i = 0; i < gb->num_assignments; i++) {
            printf("(%s, %d)\n", gb->assignments[i].assignment_name, gb->students[s].grades[i]);
        }

        return 1;
    }

}

// prints all students' final grades: (Last Name, First Name, Grade) 
// sorted alphabetically by Lastname/firstname or by highest to lowest grade
int print_Final(CmdLineResult R, PaddedDecryptedGradebook *gb){

    // compute final grades for each student: sum of [(weight) * (grade)] of each assignment
    for (int i = 0; i < gb->num_students; i++) {
        float final = 0.0;

        for (int j = 0; j < gb->num_assignments; j++) {
          final += (((float)gb->students[i].grades[j])/(float)(gb->assignments[j].maxPts) * gb->assignments[j].weight);
        }

        gb->students[i].final_grade = final;
    }

    // made copy of gradebook - used to change for sorting 
    PaddedDecryptedGradebook temp; 
    strcpy(temp.gradebook_name, gb->gradebook_name);
    memcpy(&temp.num_students, &gb->num_students, sizeof(int));
    memcpy(&temp.students, &gb->students, sizeof(Student) * 1000);
    memcpy(&temp.num_assignments, &gb->num_assignments, sizeof(int));
    memcpy(&temp.total_assignment_weight, &gb->total_assignment_weight, sizeof(float));
    memcpy(&temp.assignments, &gb->assignments, sizeof(Assignment) * 100);

    // print alphabetically by Lastname/firstname
    if (strcmp(R.print_order, "-A") == 0) {
        for (int i = 1; i < temp.num_students; i++) {
            for (int j = 0; j < temp.num_students - 1; j++) {
                // if mutiple people have same last name 
                if (strcmp(temp.students[j].last_name, temp.students[j+1].last_name) == 0) {
                    if (strcmp(temp.students[j].first_name, temp.students[j+1].first_name) > 0) {
                        Student st = temp.students[j];
                        temp.students[j] = temp.students[j+1];
                        temp.students[j+1] = st;
                    }

                } else if (strcmp(temp.students[j].last_name, temp.students[j+1].last_name) > 0) {
                    Student st = temp.students[j];
                    temp.students[j] = temp.students[j+1];
                    temp.students[j+1] = st;
                }
            }
        }    

    // print by highest to lowest grade
    } else if (strcmp(R.print_order, "-G") == 0) {
        for (int i = 1; i < temp.num_students; i++) {
            for (int j = 0; j < temp.num_students - 1; j++) {
                if (temp.students[j].final_grade < temp.students[j+1].final_grade) {
                    Student st = temp.students[j];
                    temp.students[j] = temp.students[j+1];
                    temp.students[j+1] = st;
                }
            }
        }
    } else {
        return 255;
    }

    //print results
    for (int i = 0; i < temp.num_students; i++) {
        printf("(%s, %s, %f)\n", temp.students[i].last_name, temp.students[i].first_name, temp.students[i].final_grade);
    }

    return 1;
}

int main(int argc, char *argv[]) {
    int r = 0;
    CmdLineResult R;

    R = parse_cmdline(argc, argv);
    // print_parsed_inputs(R);

    // R.good = 0 --> cmd line was valid
    if(R.good != 0) {
        printf("Invalid\n");
        return(255);
 
    } else {
        FILE *fp;
        fp = fopen(argv[2], "r");

        if (fp == NULL) {
            printf("Invalid\n");
            return(255);
        }

        EncryptedGradebook e = {0};
        fread(&e, sizeof(EncryptedGradebook), 1, fp);

        fflush(fp);
        fclose (fp); 

        unsigned char mac_key[KEY_SIZE];
        SHA256(R.key, KEY_SIZE, mac_key);

        unsigned int mac_tag_len = 0;
        unsigned char mac_tag[MAC_SIZE];

        // generate MAC
        HMAC(EVP_sha256(), mac_key, 32, (const unsigned char *)&e.iv, 
            (IV_SIZE + sizeof(PaddedDecryptedGradebook)), mac_tag, &mac_tag_len);

        // make sure no unwanted changes were made 
        if (memcmp(mac_tag, e.mac_tag, 32) != 0) {
            // error if tags don't match
            printf("Invalid \n");
            return(255);
        } else {

            // tags match! Now decrypt 
            PaddedDecryptedGradebook gb = {0};
            PaddedDecryptedGradebook *gb_ptr = &gb;

            decrypt (e.encrypted_data, sizeof(PaddedDecryptedGradebook), R.key, e.iv, (unsigned char*) &gb_ptr->gradebook_name);

            if (strcmp(R.action, "-PA") == 0) {
                r = print_Assignment(R, &gb);

            } else if (strcmp(R.action, "-PS") == 0) {
                r = print_Student(R, &gb);

            } else if (strcmp(R.action, "-PF") == 0) {
                r = print_Final(R, &gb);
            } 

            if (r == 255) {
                printf("Invalid\n");
                return(255);
            } else {
                return r; 
            }
        }
    } 
}
