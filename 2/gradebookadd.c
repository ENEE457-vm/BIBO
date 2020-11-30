#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <ctype.h>
#include "data.h"
#include "crypto.c"

typedef struct _CmdLineResult {
    int good;
    char action[4];
    unsigned char key[KEY_SIZE];

    char gradebook_name[MAX_NAME_LEN];
    char assignment_name[MAX_NAME_LEN];
    char first_name[MAX_NAME_LEN];
    char last_name[MAX_NAME_LEN];

    int grade;
    int maxPts;
    float weight;

} CmdLineResult;

int checkIfMode(unsigned char* str) {
  if (!strcmp(str, "-AA") || !strcmp(str, "-DA") || !strcmp(str, "-AS") || !strcmp(str, "-DS") || !strcmp(str, "-AG")) {
      return 1;
  } else {
      return 0;
  }
}

int checkIfFLoat(unsigned char* str) {
    int num_digits = 0;
    int num_periods = 0;
    for (int i = 0; i < strlen(str); i++) {
        if (isdigit(str[i]) != 0) {
            num_digits++;
        } else if (str[i] == '.') {
            num_periods++;
        }
    }

    if ((num_periods + num_digits) == strlen(str) && num_periods == 1) {
        // is a float
        return 1;
    } else {
        return 0;
    }
}

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

    printf("grade is %d\n", R.grade);
    printf("maxpts is %d\n", R.maxPts);
    printf("weight is %f\n", R.weight);
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
            if (strcmp("-AA", argv[i]) == 0) {
                strcpy(R.action, "-AA");
            }
            if (strcmp("-DA", argv[i]) == 0) {
                strcpy(R.action, "-DA");
            }
            if (strcmp("-AS", argv[i]) == 0) {
                strcpy(R.action, "-AS");
            }
            if (strcmp("-DS", argv[i]) == 0) {
                strcpy(R.action, "-DS");
            }
            if (strcmp("-AG", argv[i]) == 0) {
                strcpy(R.action, "-AG");
            }
            sum += checkIfMode(argv[i]);
        } 

        // check proper order of first 5 args, if we only got 1 action requested and if we gto even # args to parse
        if (strcmp("-N", argv[1]) != 0 || strcmp("-K", argv[3]) != 0 || sum != 1 || strcmp(argv[5], R.action) != 0 
              || (argc %  2) != 0) {
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
            
            // 1. ADD ASSIGNMENT __________________________________________________________
            if (!strcmp("-AA", R.action)) {
                for(int i = 6; i < argc; i+=2) {

                    // Assignment Name
                    if (strcmp("-AN", argv[i]) == 0) {
                        char * str = argv[i+1];

                        // check if length is within bounds
                        if (strlen(str) < MAX_NAME_LEN) {
                            for (int j = 0; j < strlen(str); j++) {
                                // checks for non alpha-numeric chars
                                if (isalpha(str[j]) == 0 && isdigit(str[j]) == 0 && isspace(str[j]) == 0) {
                                    R.good = 255;
                                    break;
                                }
                            }

                            strcpy(R.assignment_name, argv[i+1]);

                        } else {
                            R.good = 255;
                            break;
                        }

                    // Points
                    } else if (!strcmp("-P", argv[i])) {
                        char * str = argv[i+1];
                        int temp_pts = atoi(argv[i+1]);

                        // check if non-negative integer
                        if (temp_pts >= 0)
                            R.maxPts = temp_pts;
                        else {
                            R.good = 255;
                            break;
                        }

                    // Weight
                    } else if (!strcmp("-W", argv[i])) {
                        char * str = argv[i+1];
                        if (checkIfFLoat(str) == 1) {
                            // is a float
                            R.weight = atof(argv[i+1]);
                        } else {
                            R.good = 255;
                            break;
                        }

                    // other error
                    } else {
                        R.good = 255;
                        break;
                    }
                }

            // 2. DELETE ASSIGNMENT __________________________________________________________
            } else if (!strcmp("-DA", R.action)) {
                for(int i = 6; i < argc; i+=2) {

                    // Assignment Name
                    if (strcmp("-AN", argv[i]) == 0) {
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

            // 3/4. ADD/DELETE STUDENT ________________________________________________________________
            } else if (!strcmp("-AS", R.action) || !strcmp("-DS", R.action)) {
                for(int i = 6; i < argc; i+=2) {

                    if (strcmp("-FN", argv[i]) == 0) {
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

            // 5. ADD Grade ____________________________________________________________________
            } else if (!strcmp("-AG", R.action)) {
                for(int i = 6; i < argc; i+=2) {

                    if (strcmp("-AN", argv[i]) == 0) {
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
                        } else {
                            R.good = 255;
                            break;
                        }

                    } else if (strcmp("-FN", argv[i]) == 0) {
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
                        char * str = argv[i+1];

                        if (strlen(str) < MAX_NAME_LEN) {
                            for (int j = 0; j < strlen(str); j++) {
                              // checks for non alpha-numeric chars
                                if (isalpha(str[j]) == 0 && isspace(str[j]) == 0) {
                                    R.good = 255;
                                    break;
                                }
                            

                                strcpy(R.last_name, argv[i+1]);
                            }
                        } else {
                            R.good = 255;
                            break;
                        }


                    } else if (strcmp("-G", argv[i]) == 0) {
                        char * str = argv[i+1];
                        for (int j = 0; j < strlen(str); j++) {
                          // checks for non alpha-numeric chars
                            if (isdigit(str[j]) == 0) {
                                R.good = 255;
                                break;
                            }
                        }

                        R.grade = atoi(argv[i+1]);

                    // error
                    } else {
                        R.good = 255;
                        break;
                    }
                }

            } else {
                R.good = 255;
            }

        }
    } 

    return R;
}

// ADD ASSIGNMENT
int add_assignment(CmdLineResult R, PaddedDecryptedGradebook *gb) {
    if (gb->num_assignments < MAX_ASSIGNMENTS) {

        // check if assignment name already exists
        for (int i = 0; i < gb->num_assignments; i++) {
            if (strcmp(gb->assignments[i].assignment_name, R.assignment_name) == 0) {
                return 255;
            }
        }

        // check if sum of the weights of the new assignment and of all curr assignments adds up to more than 1 
        if (R.weight + gb->total_assignment_weight > 1) {
            // return error code
            return 255;
        } else {

            // printf("before adding Assignment\n");
            // add assignment
            int num_a = gb->num_assignments;
            strcpy(gb->assignments[num_a].assignment_name, R.assignment_name);
            gb->assignments[num_a].maxPts = R.maxPts;
            gb->assignments[num_a].weight = R.weight;

            // initialze all students' grade = 0 for that added assignment
            for (int i = 0; i < gb->num_students; i++) {
                gb->students[i].grades[num_a] = 0;
            }

            // update total weight of all assignments
            gb->total_assignment_weight += R.weight;            

            // increment num assignments by 1
            gb->num_assignments++;
        }

        return 1;
    } else {
        return 255;
    }
}

// DELETE ASSIGNMENT
int delete_assignment(CmdLineResult R, PaddedDecryptedGradebook *gb) {
    int a = -1;

    // check if assignment name already exists
    for (int i = 0; i < gb->num_assignments; i++) {
        if (strcmp(gb->assignments[i].assignment_name, R.assignment_name) == 0) {
            a = i;
        }
    }

    if (a == -1) {
        // assignment doesn't exist!
        return 255;
    } else {
        int num_a = gb->num_assignments;

        // update total weight of assignments before shifting/deleting assignment from array of assignments
        gb->total_assignment_weight -= gb->assignments[a].weight;

        // shift all assignments down by one
        for (int i = a; i <num_a - 1; i++) {
            gb->assignments[i] = gb->assignments[i+1];
        }

        // delete last assignment
        strcpy(gb->assignments[num_a - 1].assignment_name, "");
        gb->assignments[num_a - 1].maxPts = 0;
        gb->assignments[num_a - 1].weight = 0.0;

        // shift all students grades down by one
        for (int i = 0; i < gb->num_students; i++) {
            for (int j = a; j < num_a - 1; j++) {
                gb->students[i].grades[num_a] = gb->students[i].grades[num_a + 1];
            }
            gb->students[i].grades[num_a - 1] = 0;
        }

        // decrement num assignments by 1
        gb->num_assignments--;

        return 1;
    }
}

// ADD STUDENT
int add_student(CmdLineResult R, PaddedDecryptedGradebook *gb) {
    if (gb->num_students < MAX_STUDENTS) {

        // check if student name already exists
        for (int i = 0; i < gb->num_students; i++) {
            if (strcmp(gb->students[i].first_name, R.first_name) == 0 && strcmp(gb->students[i].last_name, R.last_name) == 0) {
                return 255;
            }
        }

        // add student
        int s = gb->num_students;
        strcpy(gb->students[s].first_name, R.first_name);
        strcpy(gb->students[s].last_name, R.last_name);

        for (int i = 0; i < MAX_ASSIGNMENTS; i++) {
            gb->students[s].grades[i] = 0;
        }

        // increment num students by 1
        gb->num_students++;
        return 1;

    } else {
        return 255;
    }
}

// DELETE STUDENT
int delete_student(CmdLineResult R, PaddedDecryptedGradebook *gb) {
    int a = -1;

    // check if student name already exists
    for (int i = 0; i < gb->num_students; i++) {
        if (strcmp(gb->students[i].first_name, R.first_name) == 0 && strcmp(gb->students[i].last_name, R.last_name) == 0) {
            a = i;
        }
    }

    if (a == -1) {
        // student doesn't exist!
        return 255;
    } else {
        int num_s = gb->num_students;

        // shift all students down by one
        for (int i = a; i < num_s - 1; i++) {
            gb->students[i] = gb->students[i+1];
        }

        // delete last student copy
        strcpy(gb->students[num_s - 1].first_name, "");
        strcpy(gb->students[num_s - 1].last_name, "");
        gb->students[num_s - 1].final_grade = 0.0;

        for (int i = a; i < MAX_ASSIGNMENTS; i++) {
            gb->students[num_s - 1].grades[i] = 0;
        }

        // decrement num assignments by 1
        gb->num_students--;

        return 1;
    }
}

// ADD GRADE
int add_grade(CmdLineResult R, PaddedDecryptedGradebook *gb) {
    int s = -1;
    int a = -1;

    // check if student name already exists
    for (int i = 0; i < gb->num_students; i++) {
        if (strcmp(gb->students[i].first_name, R.first_name) == 0 && strcmp(gb->students[i].last_name, R.last_name) == 0) {
            s = i;
        }
    }

    if (s == -1) {
        return 255;
    } else {
        // check if assignment name already exists
        for (int i = 0; i < gb->num_assignments; i++) {
            if (strcmp(gb->assignments[i].assignment_name, R.assignment_name) == 0) {
                a = i;
            }
        }

        if (a == -1) {
             return 255;
        } else {
            // update grade
            gb->students[s].grades[a] = R.grade;
            return 1;
        }
    }
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
            printf("Invalid\n");
            return(255);
        } else {

            // tags match! Now decrypt 
            PaddedDecryptedGradebook gb = {0};
            PaddedDecryptedGradebook *gb_ptr = &gb;

            decrypt (e.encrypted_data, sizeof(PaddedDecryptedGradebook), R.key, e.iv, (unsigned char*) &gb_ptr->gradebook_name);

            if (strcmp(R.action, "-AA") == 0) {
                r = add_assignment(R, &gb);

            } else if (strcmp(R.action, "-DA") == 0) {
                r = delete_assignment(R, &gb);

            } else if (strcmp(R.action, "-AS") == 0) {
                r = add_student(R, &gb);

            } else if (strcmp(R.action, "-DS") == 0) {
                r = delete_student(R, &gb);

            } else if (strcmp(R.action, "-AG") == 0) {
                r = add_grade(R, &gb);
            }

            // Now write results back to file
            if (r == 255) {
                printf("Invalid\n");
                return(255);
            } else {
                // print_gradebook(gb);
                fp = fopen(argv[2], "w");

                // generate new IV
                RAND_bytes(e.iv, 32);

                int encrypt_len = encrypt((unsigned char*) &gb_ptr->gradebook_name, sizeof(PaddedDecryptedGradebook), R.key, 
                    e.iv, e.encrypted_data);

                unsigned int mac_tag_len = 0;

                // generate MAC
                HMAC(EVP_sha256(), mac_key, 32, (const unsigned char *)&e.iv, 
                    (IV_SIZE + sizeof(PaddedDecryptedGradebook)), e.mac_tag, &mac_tag_len);

                fwrite (e.iv, sizeof(EncryptedGradebook), 1, fp); 
                fflush(fp);
                fclose (fp);
            }
        }
    } 
  return r;
}