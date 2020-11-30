#ifndef _BUFFR_H
#define _BUFFR_H


typedef struct _Assignment {
  unsigned char sha[20];
  char name[20];
  int points;
  int grade;
  float weight;
} Assignment;

typedef struct _Student {
  float total_grade;
  int assignment_grade;
  Assignment assignments[40];
  char firstname[20];
  char lastname[20];
} Student;

typedef struct _Gradebook {
  int student_count;
  int assign_count;
  float total_weight;
  Student students[200];
} Gradebook;

typedef struct _CmdLineResult {
  char filename[20];
  char test_name[20];
  char first_name[20];
  char last_name[20];
  unsigned char key[32];
  int check_file;
  int check_add;
  int check_delete;
  int addstudent;
  int deletestudent;
  int addgrade;
  int printstudent;
  int printassign;
  int printfinal;
  int points;
  double weight;
  int grade;
  int alphabetical;
  int grade_order;
  int good;
} CmdLineResult;

void calculate_sha(unsigned char *input_data, int data_size, unsigned char *sha1_data);
void handleErrors(void);
int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
            unsigned char *iv, unsigned char *ciphertext);
int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
            unsigned char *iv, unsigned char *plaintext);

#endif
