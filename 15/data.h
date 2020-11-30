#ifndef _BUFFR_H
#define _BUFFR_H

#define maxLength 128

#define numActionsAdd 5
#define numActionsDisplay 3
#define numTotalActionOptions 7
#define maxSpecifiedActionOptions 4
#define keyLength 16
#define macLength 16
#define ivLength 16

#define numActionsAA 3
#define numActionsDA 1
#define numActionsAS 2
#define numActionsDS 2
#define numActionsAG 4

#define numActionsPA 3
#define numActionsPS 2
#define numActionsPF 2

// PA, PF can only specify one of (-A, -G)
#define totalActionsPA 2
#define totalActionsPF 1

typedef struct _Buffer {
  unsigned char *Buf;
  unsigned long Length;
} Buffer;

typedef enum _ActionType {
  add_assignment,
  delete_assignment,
  add_student,
  delete_student,
  add_grade
} ActionType;

enum validity{
  invalid =-1,
  valid
};

typedef enum _ProgramType {
	addType,
	displayType
} ProgramType;

typedef struct _Assignment {
  char name[maxLength+1];
  int points;
  float weight;
  struct _Assignment *next;
} Assignment;

typedef struct _Student {
  char first_name[maxLength+1];
  char last_name[maxLength+1];
  int *grade;
  float final_grade;
  struct _Student *next;
} Student;

typedef struct _Gradebook {
  int numAssignments;
  int numStudents;
  Assignment *A;
  Student *S;
} Gradebook;

typedef struct _CmdLineResult {
  //TODO probably put more things here
  char    action[4];
  char    actionOption[maxSpecifiedActionOptions][4];
  char    actionOptionSpecifier[maxSpecifiedActionOptions][maxLength+1];  // +1 for NULL terminator
  char    name[maxLength+1];  // +1 for NULL terminator
  char    key[maxLength+1]; // +1 for NULL terminator
  int     numActionOptions;
  int     good;
} CmdLineResult;


enum actionOptionType{
  fileNameOption,
  keyOption,
  studentNameOption,
  assignmentNameOption,
  assignmentPointsOption,
  assignmentWeightOption,
  assignmentGradeOption,
  alphabetOrderOption,
  gradeOrderOption
};

typedef enum _sortType{
	alphabetOrder,
	gradeOrder
} sortType;

int parse_name_and_key(CmdLineResult *R, char *argv[]);
int check_valid_specifier(char *str,int optionType);
CmdLineResult parse_cmdline(int argc, char *argv[], ProgramType progType);
int parse_action(CmdLineResult *R, char *argv[]);
int parse_action_option(CmdLineResult *R, char *argv[],int argc);
int check_valid_action_option(CmdLineResult R);
int check_valid_action_option_specifier(CmdLineResult *R,char *current_action_str, int current_action_num,char *argv[], int current_arg);
void printCmdLineResults(CmdLineResult R);

#endif
