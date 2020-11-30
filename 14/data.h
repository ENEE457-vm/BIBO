// #ifndef _BUFFR_H
// #define _BUFFR_H

#define MAX_NAME_LENGTH 100
#define MAX_STUD_AMOUNT 200
#define MAX_ASSI_AMOUNT 200

// I will be using AES 128, 
// so the key length should be 128 bits, or 16 hex bytes
#define KEY_LEN 16


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

typedef struct _Assignment {
  int totalPoints;
  int assignmentID;
  char name[MAX_NAME_LENGTH];
  float weight;
} Assignment;

typedef struct _Student {
	char firstName[MAX_NAME_LENGTH];
	char lastName[MAX_NAME_LENGTH];
	int grade;
	float final;
} Student;

void decrypt(char *filename, unsigned char *key);
void encrypt(char *filename, unsigned char *key, unsigned char *iv);




// Buffer read_from_path(char *path, unsigned char *key);
// void write_to_path(char *path, Buffer *B, unsigned char *key);
// Buffer concat_buffs(Buffer *A, Buffer *B);
// Buffer print_record(Gradebook *R);
// void dump_record(Gradebook *R);

// int read_records_from_path(char *path, unsigned char *key, Gradebook **, unsigned int *);

// #endif
