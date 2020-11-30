#ifndef _BUFFR_H
#define _BUFFR_H

#define KEY_SIZE 16
#define IV_SIZE 16
#define MAC_SIZE 16
#define MAX_BUFFER_LEN 100
#define MAX_USER_INPUT_LEN 100
#define MAX_CMD_LINE_ARGS 30
#define MAX_INT_LEN 3
#define MAX_FLOAT_LEN 4
#define MAX_ASSIGNMENTS 20
#define MAX_STUDENTS 20
#define INVALID 255
#include <stdbool.h>

typedef struct {
	char name[MAX_BUFFER_LEN];
	int points;
	float weight;
} Assignment;

typedef struct {
	char firstname[MAX_BUFFER_LEN];
	char lastname[MAX_BUFFER_LEN];
	int grades[MAX_ASSIGNMENTS];
} Student;

typedef struct {
	int num_assignments;
	Assignment assignments[MAX_ASSIGNMENTS];
	bool assignment_slot_filled[MAX_ASSIGNMENTS];
	int num_students;
	Student students[MAX_STUDENTS];
	bool student_slot_filled[MAX_STUDENTS];
} DecryptedGradebookSize;

typedef struct {
	int num_assignments;
	Assignment assignments[MAX_ASSIGNMENTS];
	bool assignment_slot_filled[MAX_ASSIGNMENTS];
	int num_students;
	Student students[MAX_STUDENTS];
	bool student_slot_filled[MAX_STUDENTS];
	unsigned char padding[64 - sizeof(DecryptedGradebookSize) % 64];
} DecryptedGradebook;

typedef struct {
	unsigned char iv[IV_SIZE];
	unsigned char encrypted_data[sizeof(DecryptedGradebook)];
	unsigned char tag[MAC_SIZE];
} EncryptedGradebook;

typedef struct {
	unsigned char iv[IV_SIZE];
	unsigned char encrypted_data[sizeof(DecryptedGradebook)];
} EncryptedGradebookSize;

typedef enum _ActionType {
  add_assignment,
  delete_assignment,
  add_student,
  delete_student,
  add_grade
} ActionType;

typedef enum {
	print_assignment,
	print_student,
	print_final
} PrintAction;

typedef enum {
	alphabetical,
	grade
} PrintOrder;

// declaring prototypes for encryption and decryption
void handleErrors(void);

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key, unsigned char *iv, unsigned char *ciphertext);

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key, unsigned char *iv, unsigned char *plaintext);

#endif

