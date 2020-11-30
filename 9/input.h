#ifndef _INPUT_H
#define _INPUT_H

typedef struct _CmdLineResult {
    int good;
    // required args
    unsigned char *gb_name;
    unsigned char key[16];
    int action;
    // all potential Arguments
    unsigned char *fname;
    unsigned char *lname;
    unsigned char *aname;
    int points;
    float weight;
    int weight_char_count;
    int grade;
    int order; // 1 for alphabetical, 2 for grade
} CmdLineResult;

// void parse_add_assignment(char **argv, int len, CmdLineResult *R);
// void parse_delete_assignment(char **argv, int len, CmdLineResult *R);
// void parse_add_or_delete_student(char **argv, int len, CmdLineResult *R);
// void parse_add_grade(char **argv, int len, CmdLineResult *R);
// 
// void parse_print_assignment(char **argv, int len, CmdLineResult *R);
// void parse_print_student(char **argv, int len, CmdLineResult *R);
// void parse_print_final(char **argv, int len, CmdLineResult *R);

CmdLineResult parse_cmdline(int argc, char *argv[]);

#endif
