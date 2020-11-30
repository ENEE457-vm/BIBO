#ifndef GB_OBJECTS
#define GB_OBJECTS

typedef struct assignment {
	char name[50];
	int points;
	double weight;
	int grade;
} Assignment;

typedef struct student {
	char first_name[50];
	char last_name[50];
	int num_assignments;
	Assignment assignments[100];
} Student;

typedef struct unpadded_gradebook {
	char name[50];
	int num_assignments;
	Assignment assignments[100];
	int num_students;
	Student students[500];
} unpadded_gradebook;

typedef struct gradebook {
	char name[50];
	int num_assignments;
	Assignment assignments[100];
	int num_students;
	Student students[500];
	char padding[64 - (sizeof(unpadded_gradebook) % 64)];
} Gradebook;

#endif