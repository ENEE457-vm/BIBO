#ifndef _BUFFR_H
#define _BUFFR_H

#define MAX_NAME_LEN 100
#define MAX_STUDENTS 1000
#define MAX_ASSIGNMENTS 100
#define IV_SIZE 32
#define MAC_SIZE 32
#define KEY_SIZE 32

typedef struct _Student {
	char first_name[MAX_NAME_LEN];
	char last_name[MAX_NAME_LEN];
	int grades[MAX_ASSIGNMENTS];
	float final_grade;
} Student;

typedef struct _Assignment {
	char assignment_name[MAX_NAME_LEN];
	int maxPts;
	float weight;
} Assignment;

typedef struct _DecryptedGradebook {
	char gradebook_name[MAX_NAME_LEN];
	int num_students;
	Student students[MAX_STUDENTS];
	int num_assignments;
	Assignment assignments[MAX_ASSIGNMENTS];
	float total_assignment_weight;
} DecryptedGradebook;

typedef struct _PaddedDecryptedGradebook {
	char gradebook_name[MAX_NAME_LEN];
	int num_students;
	Student students[MAX_STUDENTS];
	int num_assignments;
	float total_assignment_weight;
	Assignment assignments[MAX_ASSIGNMENTS];
	char padding[64 - (sizeof(DecryptedGradebook) % 64)];
} PaddedDecryptedGradebook;

typedef struct _EncryptedGradebook {
	unsigned char iv[IV_SIZE];
	unsigned char encrypted_data[sizeof(PaddedDecryptedGradebook)];
	unsigned char mac_tag[MAC_SIZE];
} EncryptedGradebook;


#endif
