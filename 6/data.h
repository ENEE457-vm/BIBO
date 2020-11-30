#define KEY_LEN 17
#define MAX_ARG_LEN 40
#define MAX_CLASS_SIZE 400
#define MAX_NUM_ASSIGNMENTS 200

//#define _DEBUG
#ifdef _DEBUG
#define Print(s)  printf("%s", s)
#else
#define Print(s)  
#endif

enum ActionType {
  AA,
  DA,
  AS,
  DS,
  AG,
  PA,
  PS,
  PF,
  ERR
};

typedef struct _CmdLineResult {
  unsigned char key[KEY_LEN];
  char filename[MAX_ARG_LEN];
  enum ActionType action;
  char firstname[MAX_ARG_LEN];
  char lastname[MAX_ARG_LEN];
  char assignmentname[MAX_ARG_LEN];
  int grade;
  int points;
  float weight;
  int order; // 0 = A , 1 = G
} CmdLineResult;

typedef struct _Student {
  char firstname[MAX_ARG_LEN];
  char lastname[MAX_ARG_LEN];
} Student;

typedef struct _Grade {
  Student student;
  int grade;
} Grade;

typedef struct _Final_Grade {
  Student student;
  float grade;
} Final_Grade;

typedef struct _Assignment {
  char name[MAX_ARG_LEN];
  Grade grades[MAX_CLASS_SIZE];
  int points;
  float weight;
} Assignment;

typedef struct _Gradebook {
  Student students[MAX_CLASS_SIZE];
  int num_students;

  Assignment assignments[MAX_NUM_ASSIGNMENTS];
  int num_assignments;
} Gradebook;

int get_student_position(Gradebook *g, char *firstname, char *lastname);
int get_assignment_position(Gradebook *g, char *name);

int retrieve_gradebook(char *gradebook, unsigned char *key, Gradebook *g);
int write_gradebook(char *gradebook, unsigned char *key, Gradebook *g);
int validate_beginning(CmdLineResult *R, int argc, char *argv[]);

int valid_string(char* str, int mode);
int file_test(char* filename);
