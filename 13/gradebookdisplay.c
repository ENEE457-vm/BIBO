#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>

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
} Gradebook;

typedef struct _Student_Display
{
    char first_name[30];
    char last_name[30];
} Student_Display;

typedef struct _Assigment_Display
{
    //char *name;
    char name[100];
    int alpha;
    int grade;
    // int size;
} Assignment_Display;

// typedef struct _Display
// {
//     //char *name;
//     char name[100];
//     int alpha;
//     int grade;
//     // int size;
// } Display;

typedef struct _CmdLineResult
{
    //TODO probably put more things here
    char *filename;
    char *key;
    char *action;
    Assignment_Display ad;
    Student_Display sd;
} CmdLineResult;

void parse_assignment(CmdLineResult *cmd, int argc, char *argv[])
{
    int i = 6;
    cmd->ad.grade = 0;
    cmd->ad.alpha = 0;
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
                    printf("Assingment Name not alphanumeric\n");
                    exit(255);
                }
            }
            strcpy(cmd->ad.name, argv[i + 1]);
            i += 2;
        }
        else if (strcmp(argv[i], "-A") == 0)
        {
            if (cmd->ad.grade == 1)
            {
                printf("Only one -G or -A specificer is allowed\n");
                exit(255);
            }
            cmd->ad.alpha = 1;
            i++;
        }
        else if (strcmp(argv[i], "-G") == 0)
        {
            if (cmd->ad.alpha == 1)
            {
                printf("Only one -G or -A specificer is allowed\n");
                exit(255);
            }
            cmd->ad.grade = 1;
            i++;
        }
        else
        {
            printf("Invalid Inputs\n");
            exit(255);
        }
    }
}

void parse_student(CmdLineResult *cmd, int argc, char *argv[])
{
    int i = 6;
    while (i < argc)
    {
        if (strcmp(argv[i], "-FN") == 0)
        {
            int j;
            for (j = 0; j < strlen(argv[i + 1]); j++)
            {
                if (argv[i + 1] == NULL || isalpha(argv[i + 1][j]) == 0)
                {
                    printf("\nFirst name not alphabetic");
                    exit(255);
                }
            }
            strcpy(cmd->sd.first_name, argv[i + 1]);
        }
        else if (strcmp(argv[i], "-LN") == 0)
        {
            int j;
            for (j = 0; j < strlen(argv[i + 1]); j++)
            {
                if (argv[i + 1] == NULL || isalpha(argv[i + 1][j]) == 0)
                {
                    printf("\nLast name not alphabetic");
                    exit(255);
                }
            }
            strcpy(cmd->sd.last_name, argv[i + 1]);
        }
        else
        {
            printf("Not a valid Option for Student");
            exit(255);
        }
        i += 2;
    }
}

void pf(CmdLineResult *cmd, int argc, char *argv[])
{
    int i = 6;
    cmd->ad.grade = 0;
    cmd->ad.alpha = 0;
    while (i < argc)
    {
        if (strcmp(argv[i], "-A") == 0)
        {
            if (cmd->ad.grade == 1)
            {
                printf("Only one -G or -A specificer is allowed\n");
                exit(255);
            }
            cmd->ad.alpha = 1;
            i++;
        }
        else if (strcmp(argv[i], "-G") == 0)
        {
            if (cmd->ad.alpha == 1)
            {
                printf("Only one -G or -A specificer is allowed\n");
                exit(255);
            }
            cmd->ad.grade = 1;
            i++;
        }
        else
        {
            printf("Invalid Inputs\n");
            exit(255);
        }
    }
}

CmdLineResult parse(int argc, char *argv[])
{
    CmdLineResult R;
    if (argc == 1){
        printf("No Extra Command Line Argument Passed Other Than Program Name\n");
        exit(255);
    }
    if (argc >= 2)
    {
        if (strcmp(argv[1], "-N") == 0)
        {
            FILE *file;
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
            printf("Invalid Formatting\n");
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
                    printf("Key value not a hex\n");
                    exit(255);
                }
            }
            // if(strlen(argv[4]) != 32) {
            //     printf("Key not long enough\n");
            //     exit(255);
            // }
            R.key = malloc(strlen(argv[4]) + 1);
            strcpy(R.key, argv[4]);
        }
        else
        {
            printf("\nInvalid Formatting");
            exit(255);
        }

        if ((argc > 5 && strcmp(argv[5], "-PA") == 0))
        {
            R.action = malloc(strlen(argv[5]) + 1);
            strcpy(R.action, argv[5]);
            parse_assignment(&R, argc, argv);
        }
        else if ((argc > 5 && strcmp(argv[5], "-PS") == 0))
        {
            R.action = malloc(strlen(argv[5]) + 1);
            strcpy(R.action, argv[5]);
            parse_student(&R, argc, argv);
        }
        else if ((argc > 5 && strcmp(argv[5], "-PF") == 0))
        {
            R.action = malloc(strlen(argv[5]) + 1);
            strcpy(R.action, argv[5]);
            pf(&R, argc, argv);
        }
        else
        {
            printf("Invalid Input\n");
            exit(255);
        }
    }

    return R;
}

void display(CmdLineResult *cmd)
{
    FILE *fp;
    fp = fopen(cmd->filename, "rb");
    if (fp == NULL) {
        printf("Can't open the file\n");
        exit(255);
    }
    Gradebook g;
    if (fread(&g, sizeof(g), 1, fp))
    {
        if (strcmp(cmd->action, "-PA") == 0)
        {
            int i = 0;
            while (i < g.num_assignments && strcmp(g.assignments[i].name, cmd->ad.name) != 0) 
            {
                //printf("%s\n", g.assignments[i].name);
                i++;
            }
            if (i == g.num_assignments)
            {
                printf("Assignment does not exist\n");
                exit(255);
            }
            
            Student s_arr[g.assignments[i].num_students];
            int j;
            for (j = 0; j < g.assignments[i].num_students; j ++) {
                s_arr[j] = g.assignments[i].students[j];
            }

            if (cmd->ad.alpha) {
                int x;
                for (x = g.assignments[i].num_students - 1; x >= 0; x --) {
                    int y;
                    for (y = 0; y < x; y ++) {
                        if (strcmp(s_arr[y].last_name, s_arr[y + 1].last_name) > 0) {
                            Student temp = s_arr[y];
                            s_arr[y] = s_arr[y + 1];
                            s_arr[y + 1] = temp;
                        }else if (strcmp(s_arr[y].last_name, s_arr[y + 1].last_name) == 0) {
                            if (strcmp(s_arr[y].first_name, s_arr[y + 1].first_name) > 0) {
                                Student temp = s_arr[y];
                                s_arr[y] = s_arr[y + 1];
                                s_arr[y + 1] = temp;
                            }
                        }
                    }
                }
                for (x = 0; x < g.assignments[i].num_students; x ++) {
                    printf("(%s, %s, %d)\n", s_arr[x].last_name, s_arr[x].first_name, s_arr[x].grade);
                }
            }else if (cmd->ad.grade) {
                int x;
                for (x = g.assignments[i].num_students - 1; x >= 0; x --) {
                    int y;
                    for (y = 0; y < x; y ++) {
                        if (s_arr[y].grade < s_arr[y + 1].grade) {
                            Student temp = s_arr[y];
                            s_arr[y] = s_arr[y + 1];
                            s_arr[y + 1] = temp;
                        }
                    }
                }
                for (x = 0; x < g.assignments[i].num_students; x ++) {
                    printf("(%s, %s, %d)\n", s_arr[x].last_name, s_arr[x].first_name, s_arr[x].grade);
                }
            }else {
                printf("error, none of -G or -A was specified");
                exit(255);
            }
        }
        else if (strcmp(cmd->action, "-PS") == 0)
        {
            int i;
            for (i = 0; i < g.num_assignments; i ++) {
                int j;
                for (j = 0; j < g.assignments[i].num_students; j ++) {
                    Student temp = g.assignments[i].students[j];
                    if (strcmp(temp.first_name, cmd->sd.first_name) == 0 && strcmp(temp.last_name, cmd->sd.last_name) == 0) {
                        printf("(%s, %d)\n", g.assignments[i].name, temp.grade);
                    }
                }
            }
        }
        else if (strcmp(cmd->action, "-PF") == 0)
        {
            //printf("Printing final grade\n");
            Student s_arr[g.num_students];
            int i;
            for (i = 0; i < g.num_students; i ++) {
                s_arr[i] = g.roster[i];
                s_arr[i].final_grade = 0;
                
                int j;
                for (j= 0; j < g.num_assignments; j ++) {
                    int k;
                    
                    for (k = 0; k < g.assignments[j].num_students; k ++) {
                        Student temp = g.assignments[j].students[k]; 
                        //printf("%s %s %f\n", temp.first_name, temp.last_name, temp.weight_grade);
                        if (strcmp(temp.first_name, s_arr[i].first_name) == 0 && strcmp(temp.last_name, s_arr[i].last_name) == 0) {
                            s_arr[i].final_grade = temp.weight_grade + s_arr[i].final_grade;
                        }
                    }
                }
                //printf("%s %f\n", s_arr[i].first_name, s_arr[i].final_grade);
            }

            if (cmd->ad.alpha) {
                //printf("Print by name\n");
                int j;
                for (j = g.num_students - 1; j >= 0; j --) {
                    int k;
                    for (k = 0; k < j; k ++) {
                        if (strcmp(s_arr[k].last_name, s_arr[k + 1].last_name) > 0) {
                            Student temp = s_arr[k];
                            s_arr[k] = s_arr[k + 1];
                            s_arr[k + 1] = temp;
                        }else if (strcmp(s_arr[k].last_name, s_arr[k + 1].last_name) == 0) {
                            if (strcmp(s_arr[k].first_name, s_arr[k + 1].first_name) > 0) {
                                Student temp = s_arr[k];
                                s_arr[k] = s_arr[k + 1];
                                s_arr[k + 1] = temp;
                            }
                        }
                    }
                }
            } else if (cmd->ad.grade) {
                //printf("Sort by grade\n");
                int j;
                for (j = g.num_students - 1; j >= 0; j --) {
                    int k;
                    for (k = 0; k < j; k ++) {
                        if (s_arr[k].final_grade < s_arr[k + 1].final_grade ) {
                            Student temp = s_arr[k];
                            s_arr[k] = s_arr[k + 1];
                            s_arr[k + 1] = temp;
                        }
                    }
                }
            }

            for (i = 0; i < g.num_students; i ++) {
                printf("(%s, %s, %f)\n", s_arr[i].last_name, s_arr[i].first_name, s_arr[i].final_grade);
            }
        }
        else
        {
            printf("Invalid action\n");
            exit(255);
        }
    }
    else
    {
        printf("gradebook is empty");
        exit(255);
    }
    fclose(fp);
}

int main(int argc, char *argv[])
{
    CmdLineResult R;
    R = parse(argc, argv);
    display(&R);
    free(R.action);
}

int find_assignment(Gradebook *g, char *name)
{
    int i;
    for (i = 0; i < g->num_assignments; i++)
    {
        if (strcmp(g->assignments[i].name, name) == 0)
        {
            return 1;
        }
    }
    return 0;
}