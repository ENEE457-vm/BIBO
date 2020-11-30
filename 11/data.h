#ifndef DATA_H
#define DATA_H

#define MAX_NAME_BUFFER_LEN 32
#define MAX_NAME_INPUT_LEN 31
#define MAX_INT_DIGITS 6
#define MAX_CMD_LINE_ARGS 30

#define MAX_STUDENTS 500
#define MAX_ASSIGNMENTS 100
#define KEY_SIZE 32
#define IV_SIZE 32
#define MAC_SIZE 32

#define FAILURE -1
#define SUCCESS 0
#define INVALID 255

// Comment this out before submission.
//#define DEBUG


// Gradebookadd actions
typedef enum {
  add_assignment,
  delete_assignment,
  add_student,
  delete_student,
  add_grade
} ActionTypeAdd;


// Gradebookdisplay actions
typedef enum {
	print_assignment,
	print_student,
	print_final
} ActionTypeDisplay;


// Student structure stores grade information.
// Grade at Student.grades[i] corresponds to DecryptedGradebook.assignments[i]
typedef struct {
	char firstname[MAX_NAME_BUFFER_LEN];
	char lastname[MAX_NAME_BUFFER_LEN];
	int grades[MAX_ASSIGNMENTS];
} Student;


// Assignment structure stores assignment information.
typedef struct {
	char assignmentname[MAX_NAME_BUFFER_LEN];
	int maxpoints;
	float weight;
} Assignment;


// Helper structure that can be used to calculate size of gradebook structure. Do not use.
typedef struct {
	int num_students;
	bool student_index_filled[MAX_STUDENTS];
	Student students[MAX_STUDENTS];
	int num_assignments;
	bool assignment_index_filled[MAX_ASSIGNMENTS];
	Assignment assignments[MAX_ASSIGNMENTS];
} CalcGradebookSize;


// Decrypted gradebook. Program should modify this structure directly.
typedef struct {
	int num_students;
	bool student_index_filled[MAX_STUDENTS];
	Student students[MAX_STUDENTS];
	int num_assignments;
	bool assignment_index_filled[MAX_ASSIGNMENTS];
	Assignment assignments[MAX_ASSIGNMENTS];
	unsigned char padding[64 - (sizeof(CalcGradebookSize) % 64)];
} DecryptedGradebook;


// Encrypted gradebook. Should be stored directly on disk.
typedef struct {
	unsigned char iv[IV_SIZE];
	unsigned char encrypted_data[sizeof(DecryptedGradebook)];
	unsigned char mac[MAC_SIZE];
} EncryptedGradebook;


int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
	unsigned char *iv, unsigned char *ciphertext);
int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
	unsigned char *iv, unsigned char *plaintext);
int enc_and_auth(const DecryptedGradebook* gradebook, EncryptedGradebook* encrypted_gradebook,
	unsigned char* enc_key, size_t enc_key_size);
int verify_and_dec(const DecryptedGradebook* gradebook, EncryptedGradebook* encrypted_gradebook,
  unsigned char* enc_key, size_t enc_key_size);
int generate_random_number(unsigned char* val, size_t val_len);


#endif // DATA_H