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

#include "input.h"
#include "data.h"

//int do_batch(char *);

int check_filename(char *ch) {
    if (access(ch, F_OK) == -1) {
        return -1;
    }
    while (*ch != '\0') {
        if (!isalnum(*ch) && *ch != '.' && *ch != '_') {
            return -1;
        }
        ch++;
    }
    return 0;
}

int check_key(char *ch) {
    int key_length = 0;
    while (*ch != '\0') {
        key_length++;
        if (!isxdigit(*ch)) {
            return -1;
        }
        ch++;
    }
    if (key_length != 32) {
        return -1;
    }
    return 0;
}

void decode_hex_key(unsigned char *key_out, unsigned char *k, int k_len) {
    unsigned int msb, lsb;
    int i = 0;
    int j = 0;
    while (i < k_len) {
        msb = k[i] > '9' ? k[i] - 'a' + 10 : k[i] - '0';
        lsb = k[i+1] > '9' ? k[i+1] - 'a' + 10 : k[i+1] - '0';

        key_out[j] = (msb << 4) | lsb;
        i += 2;
        j++;
    }
}

int check_assignment_name(char *aname) {
    while (*aname != '\0') {
        if (!isalnum(*aname)) {
            return -1;
        }
        aname++;
    }
    return 0;
}

int check_points_val(char *points) {
    while (*points != '\0') {
        if (!isdigit(*points)) {
            return -1;
        }
        points++;
    }
    return 0;
}

int check_weight_val(char *weight) {
    int dec_count = 0;
    int char_count = 0;
    while (*weight != '\0') {
        if (!isdigit(*weight)) {
            if (*weight == '.' && dec_count == 0) {
                dec_count++;
            } else {
                return -1;
            }
        }
        char_count++;
        weight++;
    }
    return char_count;
}

int check_student_name(char *name) {
    while (*name != '\0') {
        if (!isalpha(*name)) {
            return -1;
        }
        name++;
    }
    return 0;
}

//////////////////////////////
/// gradebookadd functions ///
//////////////////////////////

void parse_add_assignment(char **argv, int len, CmdLineResult *R) {
    int has_name = 0;
    int has_points = 0;
    int has_weight = 0;
    int i = 0;
    while (i < len) {
        char *curr_arg = argv[i];
        if (strcmp(curr_arg, "-AN") == 0) {
            i++;
            if (i >= len) { printf("invalid\n"); exit(255); }
            char *ptr = argv[i];
            if (check_assignment_name(ptr) == -1) {
                printf("invalid\n");
                exit(255);
            }
            R->aname = (unsigned char *)realloc(R->aname, strlen(argv[i]) + 1);
            strcpy(R->aname, argv[i]);
            has_name = 1;
        } else if (strcmp(curr_arg, "-P") == 0) {
            i++;
            if (i >= len) { printf("invalid\n"); exit(255); }
            char *ptr = argv[i];
            if (check_points_val(ptr) == -1) {
                printf("invalid\n");
                exit(255);
            }
            R->points = atoi(argv[i]);
            has_points = 1;
        } else if (strcmp(curr_arg, "-W") == 0) {
            i++;
            if (i >= len) { printf("invalid\n"); exit(255); }
            char *ptr = argv[i];
            int wcc = check_weight_val(ptr);
            if (wcc == -1) {
                printf("invalid\n");
                exit(255);
            }
            R->weight_char_count = wcc;
            R->weight = strtof(argv[i], NULL);
            has_weight = 1;
        } else {
            printf("invalid\n");
            exit(255);
        }

        i++;
    }
    if (has_name && has_points && has_weight) {
        R->good = 0;
    }
}

void parse_delete_assignment(char **argv, int len, CmdLineResult *R) {
    int has_name = 0;
    int i = 0;
    while (i < len) {
        char *curr_arg = argv[i];
        if (strcmp(curr_arg, "-AN") == 0) {
            i++;
            if (i >= len) { printf("invalid\n"); exit(255); }
            char *ptr = argv[i];
            if (check_assignment_name(ptr) == -1) {
                printf("invalid\n");
                exit(255);
            }
            R->aname = (unsigned char *)realloc(R->aname, strlen(argv[i]) + 1);
            strcpy(R->aname, argv[i]);
            has_name = 1;
        } else {
            printf("invalid\n");
            exit(255);
        }
        i++;
    }
    if (has_name) {
        R->good = 0;
    }
}

void parse_add_or_delete_student(char **argv, int len, CmdLineResult *R) {
    int has_fname = 0;
    int has_lname = 0;
    int i = 0;
    while (i < len) {
        char *curr_arg = argv[i];
        if (strcmp(curr_arg, "-FN") == 0) {
            i++;
            if (i >= len) { printf("invalid\n"); exit(255); }
            char *ptr = argv[i];
            if (check_student_name(ptr) == -1) {
                printf("invalid\n");
                exit(255);
            }
            R->fname = (unsigned char *)realloc(R->fname, strlen(argv[i]) + 1);
            strcpy(R->fname, argv[i]);
            has_fname = 1;
        } else if (strcmp(curr_arg, "-LN") == 0) {
            i++;
            if (i >= len) { printf("invalid\n"); exit(255); }
            char *ptr = argv[i];
            if (check_student_name(ptr) == -1) {
                printf("invalid\n");
                exit(255);
            }
            R->lname = (unsigned char *)realloc(R->lname, strlen(argv[i]) + 1);
            strcpy(R->lname, argv[i]);
            has_lname = 1;
        } else {
            printf("invalid\n");
            exit(255);
        }
        i++;
    }
    if (has_fname && has_lname) {
        R->good = 0;
    }
}

void parse_add_grade(char **argv, int len, CmdLineResult *R) {
    int has_aname = 0;
    int has_fname = 0;
    int has_lname = 0;
    int has_grade = 0;
    int i = 0;
    while (i < len) {
        char *curr_arg = argv[i];
        if (strcmp(curr_arg, "-AN") == 0) {
            i++;
            if (i >= len) { printf("invalid\n"); exit(255); }
            char *ptr = argv[i];
            if (check_assignment_name(ptr) == -1) {
                printf("invalid\n");
                exit(255);
            }
            R->aname = (unsigned char *)realloc(R->aname, strlen(argv[i]) + 1);
            strcpy(R->aname, argv[i]);
            has_aname = 1;

        } else if (strcmp(curr_arg, "-FN") == 0) {
            i++;
            if (i >= len) { printf("invalid\n"); exit(255); }
            char *ptr = argv[i];
            if (check_student_name(ptr) == -1) {
                printf("invalid\n");
                exit(255);
            }
            R->fname = (unsigned char *)realloc(R->fname, strlen(argv[i]) + 1);
            strcpy(R->fname, argv[i]);
            has_fname = 1;

        } else if (strcmp(curr_arg, "-LN") == 0) {
            i++;
            if (i >= len) { printf("invalid\n"); exit(255); }
            char *ptr = argv[i];
            if (check_student_name(ptr) == -1) {
                printf("invalid\n");
                exit(255);
            }
            R->lname = (unsigned char *)realloc(R->lname, strlen(argv[i]) + 1);
            strcpy(R->lname, argv[i]);
            has_lname = 1;

        } else if (strcmp(curr_arg, "-G") == 0) {
            i++;
            if (i >= len) { printf("invalid\n"); exit(255); }
            char *ptr = argv[i];
            if (check_points_val(ptr) == -1) {
                printf("invalid\n");
                exit(255);
            }
            R->grade = atoi(argv[i]);
            has_grade = 1;

        } else {
            printf("invalid\n");
            exit(255);
        }
        i++;
    }
    if (has_aname && has_fname && has_lname && has_grade) {
        R->good = 0;
    }
}

//////////////////////////////////
/// gradebookdisplay functions ///
//////////////////////////////////

void parse_print_assignment(char **argv, int len, CmdLineResult *R) {
    int has_aname = 0;
    int has_order = 0;
    int i = 0;
    while (i < len) {
        char *curr_arg = argv[i];
        if (strcmp(curr_arg, "-AN") == 0) {
            i++;
            if (i >= len) { printf("invalid\n"); exit(255); }
            char *ptr = argv[i];
            if (check_assignment_name(ptr) == -1) {
                printf("invalid\n");
                exit(255);
            }
            R->aname = (unsigned char *)realloc(R->aname, strlen(argv[i]) + 1);
            strcpy(R->aname, argv[i]);
            has_aname = 1;

        } else if (strcmp(curr_arg, "-A") == 0) {
            if (has_order) {
                printf("invalid\n");
                exit(255);
            }
            R->order = 1;
            has_order = 1;

        } else if (strcmp(curr_arg, "-G") == 0) {
            if (has_order) {
                printf("invalid\n");
                exit(255);
            }
            R->order = 2;
            has_order = 1;

        } else {
            printf("invalid\n");
            exit(255);
        }
        i++;
    }
    if (has_aname && has_order) {
        R->good = 0;
    }
}

void parse_print_student(char **argv, int len, CmdLineResult *R) {
    int has_fname = 0;
    int has_lname = 0;
    int i = 0;
    while (i < len) {
        char *curr_arg = argv[i];
        if (strcmp(curr_arg, "-FN") == 0) {
            i++;
            if (i >= len) { printf("invalid\n"); exit(255); }
            char *ptr = argv[i];
            if (check_student_name(ptr) == -1) {
                printf("invalid\n");
                exit(255);
            }
            R->fname = (unsigned char *)realloc(R->fname, strlen(argv[i]) + 1);
            strcpy(R->fname, argv[i]);
            has_fname = 1;
        } else if (strcmp(curr_arg, "-LN") == 0) {
            i++;
            if (i >= len) { printf("invalid\n"); exit(255); }
            char *ptr = argv[i];
            if (check_student_name(ptr) == -1) {
                printf("invalid\n");
                exit(255);
            }
            R->lname = (unsigned char *)realloc(R->lname, strlen(argv[i]) + 1);
            strcpy(R->lname, argv[i]);
            has_lname = 1;
        } else {
            printf("invalid\n");
            exit(255);
        }
        i++;
    }

    if (has_fname && has_lname) {
        R->good = 0;
    }
}

void parse_print_final(char **argv, int len, CmdLineResult *R) {
    int has_order = 0;
    int i = 0;
    while (i < len) {
        char *curr_arg = argv[i];
        if (strcmp(curr_arg, "-A") == 0) {
            if (has_order) {
                printf("invalid\n");
                exit(255);
            }
            R->order = 1;
            has_order = 1;

        } else if (strcmp(curr_arg, "-G") == 0) {
            if (has_order) {
                printf("invalid\n");
                exit(255);
            }
            R->order = 2;
            has_order = 1;

        } else {
            printf("invalid\n");
            exit(255);
        }
        i++;
    }

    if (has_order) {
        R->good = 0;
    }
}

// main parse function

CmdLineResult parse_cmdline(int argc, char *argv[]) {
    CmdLineResult R = { 0 };
    R.good = -1;

    if(argc==1)
        printf("No Extra Command Line Argument Passed Other Than Program Name\n"); 
    if(argc>=2) {
        int counter = 1;
        //printf("Number Of Arguments Passed: %d\n", argc); 
        //printf("----Following Are The Command Line Arguments Passed----\n"); 
        // for (int i=0; i < argc; i++)
        //    printf("argv[%d]: %s\n", i, argv[i]); 

        // 1st argument must be -N
        if (strcmp(argv[counter], "-N") != 0) {
            printf("invalid\n");
            exit(255);
        }
        counter++;
        if (counter >= argc) { printf("invalid\n"); exit(255); }
        // assert gb name is valid
        // assert file exists
        // strcpy to R.gb_name
        char *ch = argv[counter];
        if (check_filename(ch) == -1) {
            printf("invalid filename\n");
            exit(255);
        }
        R.gb_name = (unsigned char *)malloc(strlen(argv[counter]) + 1);
        strcpy(R.gb_name, argv[counter]);
        counter++;
        if (counter >= argc) { printf("invalid\n"); exit(255); }

        // next argument must be -K
        if (strcmp(argv[counter], "-K") != 0) {
            printf("invalid\n");
            exit(255);
        }
        counter++;
        if (counter >= argc) { printf("invalid\n"); exit(255); }
        // assert key is hex
        // assert key length in chars is 32
        char *k = argv[counter];
        if (check_key(k) == -1) {
            printf("invalid key\n");
            exit(255);
        }
        // decode key to bytes
        k = argv[counter];
        decode_hex_key(R.key, k, 32);
        counter++;
        if (counter >= argc) { printf("invalid\n"); exit(255); }

        // next argument is an ActionType
        char *action = argv[counter];
        if (strcmp(action, "-AA") == 0) {
            R.action = add_assignment;
            if (argc < 12) { printf("invalid\n"); exit(255); }
            counter++;
            char **ptr = &argv[counter];
            parse_add_assignment(ptr, argc - counter, &R);

        } else if (strcmp(action, "-DA") == 0) {
            R.action = delete_assignment;
            if (argc < 8) { printf("invalid\n"); exit(255); }
            counter++;
            char **ptr = &argv[counter];
            parse_delete_assignment(ptr, argc - counter, &R);

        } else if (strcmp(action, "-AS") == 0) {
            R.action = add_student;
            if (argc < 10) { printf("invalid\n"); exit(255); }
            counter++;
            char **ptr = &argv[counter];
            parse_add_or_delete_student(ptr, argc - counter, &R);

        } else if (strcmp(action, "-DS") == 0) {
            R.action = delete_student;
            if (argc < 10) { printf("invalid\n"); exit(255); }
            counter++;
            char **ptr = &argv[counter];
            parse_add_or_delete_student(ptr, argc - counter, &R);

        } else if (strcmp(action, "-AG") == 0) {
            R.action = add_grade;
            if (argc < 14) { printf("invalid\n"); exit(255); }
            counter++;
            char **ptr = &argv[counter];
            parse_add_grade(ptr, argc - counter, &R);
            
        } else if (strcmp(action, "-PA") == 0) {
            R.action = print_assignment;
            if (argc < 9) { printf("invalid\n"); exit(255); }
            counter++;
            char **ptr = &argv[counter];
            parse_print_assignment(ptr, argc - counter, &R);

        } else if (strcmp(action, "-PS") == 0) {
            R.action = print_student;
            if (argc < 10) { printf("invalid\n"); exit(255); }
            counter++;
            char **ptr = &argv[counter];
            parse_print_student(ptr, argc - counter, &R);

        } else if (strcmp(action, "-PF") == 0) {
            R.action = print_final;
            if (argc < 7) { printf("invalid\n"); exit(255); }
            counter++;
            char **ptr = &argv[counter];
            parse_print_final(ptr, argc - counter, &R);

        } else {
            printf("invalid\n");
            exit(255);
        }
    }

    return R;
}
