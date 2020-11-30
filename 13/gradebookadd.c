#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <math.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/rand.h>
#include <openssl/aes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>   // stat
#include <stdbool.h>    // bool type

//Sanitize by parsing the command prompt inputs
//Checking for buffer overflows by checking the inputs if they are valid/if they cause an error by the edge cases

//NEED TO ENCRYPT AND DECRYPT WHENEVER WE OPEN THE FILE

typedef struct _Student
{
    char first_name[30];
    char last_name[30];
    int grade;
    float weight_grade;
    float final_grade;
} Student;

typedef struct _Assigment
{
    //char *name;
    char name[100];
    int points;
    float weight;
    Student students[200];
    int num_students;
    // int size;
} Assignment;

typedef struct _Gradebook
{
    Student roster[200];
    Assignment assignments[100];
    int num_students;
    int num_assignments;
}Gradebook;

typedef struct _File_info {
    char iv[16];
    char enc_file[60];
    char filename[60];
    int ciphertext_len;
}File_info;

typedef struct _CmdLineResult
{
    //TODO probably put more things here
    //int is_valid;
    char *filename;
    char *key;
    char *action;
    Assignment assignment;
    Student student;
} CmdLineResult;

void create_assignment(CmdLineResult *cmd,  int argc, char *argv[]);
void create_student(CmdLineResult *cmd, int argc, char *argv[]);
void add_grade(CmdLineResult *cmd, int argc, char *argv[]);
void store(CmdLineResult *cmd);
void display(CmdLineResult *cmd);
int find_assignment(Gradebook *g, Assignment *a);
int find_student(Gradebook *g, Student *s);
int decrypt (FILE *fp, FILE *f_dec, unsigned char *key, unsigned char *iv, int ciphertext_len);
File_info read_line(CmdLineResult *cmd);



void create_assignment(CmdLineResult *cmd,  int argc, char *argv[])
{
    Assignment a;
    a.num_students = 0;
    int i = 6;
    /*If the action is Add Assignment*/
    if (strcmp(cmd->action, "-AA") == 0) {
        while (i < argc)
        {
            if (strcmp(argv[i], "-AN") == 0)
            {
                int j;
                // Checking if the name is Alphanumeric
                for (j = 0; j < strlen(argv[i + 1]); j++)
                {
                    if (argv[i + 1] == NULL || isalnum(argv[i + 1][j]) == 0)
                    {
                        printf("\nAssingment Name not alphanumeric");
                        exit(255);
                    }
                }
                strcpy(cmd->assignment.name, argv[i + 1]);
            }else if (strcmp(argv[i], "-P") == 0) {
                int j;
                for (j = 0; j < strlen(argv[i + 1]); j++)
                {
                    if (argv[i + 1] == NULL || isdigit(argv[i + 1][j]) == 0)
                    {
                        printf("Not a Positive Integer");
                        exit(255);
                    }
                }
                int num = atoi(argv[i + 1]);
                if (num >= 0) {
                    cmd->assignment.points = num;
                }else {
                    printf("Not positive\n");
                    exit(255);
                }
            } else if (strcmp(argv[i], "-W") == 0) {
                int j;
                int flag = 0;
                for (j = 0; j < strlen(argv[i + 1]); j++)
                {
                    if (argv[i + 1][j] == '.')
                    {
                        if (flag == 0)
                        {
                            flag = 1;
                        }
                        else
                        {
                            printf("Float format error");
                            exit(255);
                        }
                    }
                    else if (argv[i + 1] == NULL || isdigit(argv[i + 1][j]) == 0)
                    {
                        printf("Invalid weight input");
                        exit(255);
                    }
                }
                char *p;
                float f = strtof(argv[i + 1], &p);
                if (f >= 0 && f<= 1) {
                    cmd->assignment.weight = f;
                }else {
                    printf("Not a valid weight\n");
                    exit(255);
                }
            }else {
                printf("Invalid \n");
                exit(255);
            }
            i += 2;
        }
    }else {
        while (i < argc) {
            if (strcmp(argv[i], "-AN") == 0)
            {
                int j;
                // Checking if the name is Alphanumeric
                for (j = 0; j < strlen(argv[i + 1]); j++)
                {
                    if (argv[i + 1] == NULL || isalnum(argv[i + 1][j]) == 0)
                    {
                        printf("\nAssingment Name not alphanumeric\n");
                        exit(255);
                    }
                }
                strcpy(cmd->assignment.name, argv[i + 1]);
            }else {
                printf("Invalid\n");
                exit(255);
            }
            i += 2;
        }
    }
}

void create_student(CmdLineResult *cmd, int argc, char *argv[])
{
    int i = 6;
    cmd->student.grade = 0;
    cmd->student.final_grade = 0;
    while (i < argc)
    {
        //printf("%s %d\n", argv[i], i);
       if (strcmp(argv[i], "-FN") == 0)
        {
            int j;
            for (j=0; j < strlen(argv[i + 1]); j ++) {
                if (argv[i + 1] == NULL || isalpha(argv[i + 1][j]) == 0) {
                    printf("\nFirst name not alphabetic");
                    exit(255);
                }
            }
            strcpy(cmd->student.first_name, argv[i + 1]);
        }
        else if (strcmp(argv[i], "-LN") == 0)
        {
            int j;
            for (j=0; j < strlen(argv[i + 1]); j ++) {
                if (argv[i + 1] == NULL || isalpha(argv[i + 1][j]) == 0) {
                    printf("\nLast name not alphabetic");
                    exit(255);
                }
            }
            strcpy(cmd->student.last_name, argv[i + 1]);
        }else {
            printf("Not a valid Option for Student");
            exit(255);
        }
        i = i + 2;
    }
}

void add_grade(CmdLineResult *cmd, int argc, char *argv[])
{
    int i = 6;
    while (i < argc)
    {
        if (strcmp(argv[i], "-FN") == 0)
        {
            int j;
            for (j=0; j < strlen(argv[i + 1]); j ++) {
                if (argv[i + 1] == NULL || isalpha(argv[i + 1][j]) == 0) {
                    printf("\nFirst name not alphabetic");
                    exit(255);
                }
            }
            strcpy(cmd->student.first_name, argv[i + 1]);
        }
        else if (strcmp(argv[i], "-LN") == 0)
        {
            int j;
            for (j=0; j < strlen(argv[i + 1]); j ++) {
                if (argv[i + 1] == NULL || isalpha(argv[i + 1][j]) == 0) {
                    printf("\nLast name not alphabetic");
                    exit(255);
                }
            }
            strcpy(cmd->student.last_name, argv[i + 1]);
        }
        else if (strcmp(argv[i], "-AN") == 0)
        {
            int j;
            // Checking if the name is Alphanumeric
            for (j = 0; j < strlen(argv[i + 1]); j++)
            {
                if (argv[i + 1] == NULL || isalnum(argv[i + 1][j]) == 0)
                {
                    printf("\nAssingment Name not alphanumeric");
                    exit(255);
                }
            }
            strcpy(cmd->assignment.name, argv[i + 1]);
        }
        else if (strcmp(argv[i], "-G") == 0)
        {
            int j;
            for (j = 0; j < strlen(argv[i + 1]); j++)
            {
                if (argv[i + 1] == NULL || isdigit(argv[i + 1][j]) == 0)
                {
                    printf("\nGrade not all integers");
                    exit(255);
                }
            }
            int num = atoi(argv[i + 1]);
            if (num >= 0)
            {
                cmd->student.grade = num;
            }
            else
            {
                exit(255);
            }
        }
        else
        {
            printf("input bad");
            exit(255);
        }
        i += 2;
    }
}

CmdLineResult parse_cmd(int argc, char *argv[])
{
    CmdLineResult R;

    int counter;
    //printf("Program Name Is: %s", argv[0]);
    if (argc == 1){
        printf("\nNo Extra Command Line Argument Passed Other Than Program Name");
        exit(255);
    }

    if (argc >= 2)
    {
        // printf("\nNumber Of Arguments Passed: %d", argc);
        // printf("\n----Following Are The Command Line Arguments Passed----\n");
        FILE *file;

        if (strcmp(argv[1], "-N") == 0)
        {

            file = fopen(argv[2], "r");
            if (file)
            {
                R.filename = malloc(strlen(argv[2]) + 1);
                strcpy(R.filename, argv[2]);
                fclose(file);
            }
            else
            {
                printf("File does not exist\n");
                exit(255);
            }
        }
        else
        {
            printf("\nInvalid Formatting");
            exit(255);
        }

        if (argc > 3 && strcmp(argv[3], "-K") == 0)
        {
            //Check if the given key matches with the expected key check if the file can be decrypted with the given key
            int j;
            for (j = 0; j < strlen(argv[4]); j++)
            {
                if (isxdigit(argv[4][j]) == 0)
                {
                    printf("\nnot a hex");
                    exit(255);
                }
            }
            R.key = malloc(strlen(argv[4]) + 1);
            strcpy(R.key, argv[4]);
        }
        else
        {
            printf("\nInvalid Formatting");
            exit(255);
        }

        if (argc > 5 && strcmp(argv[5], "-AA") == 0)
        {
            R.action = malloc(strlen(argv[5]) + 1);
            strcpy(R.action, argv[5]);
            //R.assignment = create_assignment(argc, argv, argv[5]);
            create_assignment(&R, argc, argv);
        }
        else if (argc > 5 && strcmp(argv[5], "-DA") == 0)
        {
            R.action = malloc(strlen(argv[5]) + 1);
            strcpy(R.action, argv[5]);
            //R.assignment = create_assignment(argc, argv, argv[5]);
            create_assignment(&R, argc, argv);
            //printf("Assignment to be deleted: %s", R.assignment.name);
        }
        else if (argc > 5 && strcmp(argv[5], "-AS") == 0 ) {
             R.action = malloc(strlen(argv[5]) + 1);
            strcpy(R.action, argv[5]);
            create_student(&R, argc, argv);
        } else if (argc > 5 && strcmp(argv[5], "-DS") == 0 ) {
            R.action = malloc(strlen(argv[5]) + 1);
            strcpy(R.action, argv[5]);
            create_student(&R, argc, argv);
        } else if (argc > 5 && strcmp(argv[5], "-AG") == 0 ) {
            R.action = malloc(strlen(argv[5]) + 1);
            strcpy(R.action, argv[5]);
            add_grade(&R, argc, argv);
        }
        else
        {
            printf("Invalid formatting");
            exit(255);
        }
    }
    return R;
}

void store(CmdLineResult *cmd) {
    //printf("\nStoring\n");
    
    // File_info f = read_line(cmd);
    // FILE *of, *f_dec;
    // of= fopen(f.enc_file, "rb");
    // f_dec = fopen(f.filename, "wb");
    // if (of == NULL || f_dec == NULL) {
    //     exit(255);
    // }
    // int plain_len = decrypt(of, f_dec, cmd->key, f.iv, f.ciphertext_len);

    FILE *fp;
    //CHANGE mytest to cmd->filename
    fp= fopen(cmd->filename, "rb");
    if (fp == NULL) {
        printf("File does not exist\n");
        exit(255);
    }

    Gradebook g;

    if (fread(&g, sizeof(g), 1, fp)) {
        //printf("Modifying\n");
        if (strcmp(cmd->action, "-AA") == 0) {
            if (find_assignment(&g, &cmd->assignment)) {
                printf("Assignment already exists");
                exit(255);        
            }
            g.assignments[g.num_assignments] = cmd->assignment;
            g.num_assignments ++;
        }else if (strcmp(cmd->action, "-DA") == 0) {
            int i = 0;
            while (i < g.num_assignments && strcmp(g.assignments[i].name, cmd->assignment.name) != 0) {
                //printf("%s %s\n", g.assignments[i].name, cmd->assignment.name);
                i ++;
            }
            if (i < g.num_assignments)
            {
                int j;
                for (j = i; j < g.num_assignments - 1; j++)
                {
                    g.assignments[j] = g.assignments[j + 1];
                }
                g.num_assignments --;
                //printf("Gradebook has %d assignments\n", g.num_assignments);
            }
            else
            {
                printf("Assignment does not exist");
                exit(255);
            }
        } else if (strcmp(cmd->action, "-AS") == 0) {
            // printf("Adding student\n");
            // printf("Before: %d\n", g.num_students);
            if (find_student(&g, &cmd->student)) {
                printf("Student already exists\n");
                exit(255);
            }
            g.roster[g.num_students] = cmd->student;
            g.num_students ++;

        } else if (strcmp(cmd->action, "-DS") == 0) {
            //printf("Deleting Student\n");
            int i = 0;
            while (i < g.num_students && strcmp(g.roster[i].first_name, cmd->student.first_name) != 0 && strcmp(g.roster[i].last_name, cmd->student.last_name) != 0) {
                i ++;
            }
            if (i < g.num_students) {
                int j;
                for (j = i; j < g.num_students - 1; j++)
                {
                    g.roster[j] = g.roster[j + 1];
                }
                g.num_students --;
                //printf("Gradebook has %d students\n", g.num_students);
            }else {
                printf("Student does not exist");
                exit(255);
            }
        } else if (strcmp(cmd->action, "-AG") == 0) {
            if (find_assignment(&g, &cmd->assignment) && find_student(&g, &cmd->student)) {
                int i = 0;
                while (i < g.num_assignments && strcmp(g.assignments[i].name, cmd->assignment.name) != 0)
                {
                    //printf("%s %s\n", g.assignments[i].name, cmd->assignment.name);
                    i++;
                }
                //printf("Assignment %s has %d students\n", g.assignments[i].name, g.assignments[i].num_students);
                int j = 0;
                int not_found = 1;
                while (j < g.assignments[i].num_students && not_found)
                {
                    //printf("%s %s %s %s\n", g.assignments[i].students[j].first_name, cmd->student.first_name,g.assignments[i].students[j].last_name, cmd->student.last_name);
                    if (strcmp(g.assignments[i].students[j].first_name, cmd->student.first_name) == 0 && strcmp(g.assignments[i].students[j].last_name, cmd->student.last_name) == 0) {
                        not_found = 0;
                    }else {
                        j ++;
                    }
                }
                //printf("Student %s has a grade %d\n", g.assignments[i].students[j].first_name, g.assignments[i].students[j].grade);
                if (j == g.assignments[i].num_students) {
                    g.assignments[i].students[j] = cmd->student;
                    g.assignments[i].students[j].weight_grade = g.assignments[i].weight * (g.assignments[i].students[j].grade / (float) g.assignments[i].points);
                    //roundf(g.assignments[i].weight * (g.assignments[i].students[j].grade / (float) g.assignments[i].points) * 100) / 100;
                    g.assignments[i].num_students ++;
                } else {
                    g.assignments[i].students[j].grade = cmd->student.grade;
                    g.assignments[i].students[j].weight_grade =  g.assignments[i].weight * (g.assignments[i].students[j].grade / (float) g.assignments[i].points);
                }
                //printf("Student %s now has a grade %d and weighted grade %f\n", g.assignments[i].students[j].first_name, g.assignments[i].students[j].grade, g.assignments[i].students[j].weight_grade);
            } else {
                //printf("Either student or assignment does not exist\n");
                exit(255);
            }
        }
        fclose(fp);
        
    }else {
        g.num_students = 0;
        g.num_assignments = 0;
        if (strcmp(cmd->action, "-AA") == 0) {
            g.assignments[0] = cmd->assignment;
            g.num_assignments = 1;
        }else if (strcmp(cmd->action, "-DA") == 0) {
            printf("Gradebook is empty");
            exit(255);
        } else if (strcmp(cmd->action, "-AS") == 0) {
            g.roster[0] = cmd->student;
            g.num_students ++;;
            //printf("%s %d\n", g.roster[0].first_name, g.num_students);
            
        } else if (strcmp(cmd->action, "-DS") == 0) {
            printf("Gradebook is empty");
            exit(255);
        } else if (strcmp(cmd->action, "-AG") == 0) {
            printf("Gradebook is empty");
            exit(255);
        } else {
            printf("Invalid Input\n");
            exit(255);
        }
        fclose(fp);
    }
    fp = fopen(cmd->filename, "wb");
    fwrite(&g, sizeof(g), 1, fp);
    fclose(fp);
}

int main(int argc, char *argv[])
{
    //char *temp = argv[2];
    CmdLineResult R = parse_cmd(argc, argv);
    //printf("Command Line Results: %s %s %s %s %s %d %s\n", R.filename, R.key, R.assignment.name, R.student.first_name, R.student.last_name, R.student.grade, R.action);
    store(&R);
    //display(&R);

    free(R.filename);
    free(R.key);
    R.filename = NULL;
    R.key = NULL;
    return 0;
}

int find_assignment(Gradebook *g, Assignment *a) {
    int i;
    for (i = 0; i < g->num_assignments; i ++) {
        if (strcmp(g->assignments[i].name, a->name) == 0) {
            return 1;
        }
    }
    return 0;
}

int find_student(Gradebook *g, Student *s) {
    int i;
    for (i = 0; i < g->num_students; i ++) {
        if (strcmp(g->roster[i].first_name, s->first_name) == 0 && strcmp(g->roster[i].last_name, s->last_name) == 0) {
            return 1;
        }
    }
    return 0;
}

File_info read_line(CmdLineResult *cmd) {
    FILE *fp;
    fp = fopen("fileinfo", "rb");
    File_info f;
    int found = 0;
    while (found == 0 && fread(&f, sizeof(f), 1, fp)) {
        if (strcmp(f.filename, cmd->filename) == 0) {
            found = 1;
        }
    }
    fclose(fp);
    if (found == 0) {
       exit(255);
    }
    return f;
}

int decrypt (FILE *fp, FILE *f_dec, unsigned char *key, unsigned char *iv, int ciphertext_len) {
    // fp = fopen("encrypt_blah", "rb");
    // f_dec = fopen("blah", "wb+");
    //Get file size
    fseek(fp, 0L, SEEK_END);
    int fsize = ftell(fp);
    //set back to normal
    fseek(fp, 0L, SEEK_SET);
    //printf("%d\n", fsize);

    int plaintext_len = 0; 
    int len = 0;
    unsigned char *plaintext = malloc(fsize); //plaintext
    unsigned char *ciphertext = malloc(fsize*2); //cipher
    fread(ciphertext,sizeof(char),ciphertext_len, fp);

    EVP_CIPHER_CTX *ctx;
    ctx = EVP_CIPHER_CTX_new();
    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv)) {
        printf("Error 1\n");
        exit(255);
    }
    if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len)) {
        printf("Error 2\n");
        exit(255);
    }
    plaintext_len = len;
    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) {
        printf("Error 3\n");
        exit(255);
    }
    plaintext_len += len;
    EVP_CIPHER_CTX_free(ctx);
    printf("finsiehd decrypting\n");
    fwrite(plaintext, sizeof(char), plaintext_len, f_dec);
    return plaintext_len;

}