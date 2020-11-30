#define max_n_len 30
#define max_na_len 30
#define max_assignments 100
#define max_students 400
#define IV_Size 32
#define MAC_Size 32
#define key_size 32

typedef struct Student
{
	char first_name[max_n_len];
	char last_name[max_n_len];
	int grades[max_assignments];
} Student;


typedef struct Assignment
{
	char assignment_name[max_na_len];
	int points;
	float weight;
}Assignment;


typedef struct Decrypted_gradebook
{
	char g_name[max_n_len];	 
	int num_students;
	int num_assignments;
	Student s_array[max_students];
	Assignment a_array[max_assignments];
}Decrypted_gradebook;


typedef struct paddedDecrypted_gradebook
{
	char g_name[max_n_len];	 
	int num_students;
	int num_assignments;
	Student s_array[max_students];
	Assignment a_array[max_assignments];
	char padding[64 - (sizeof(Decrypted_gradebook) % 64)];
}paddedDecrypted_gradebook;

typedef struct Encrypted_gradebook
{
	unsigned char iv[IV_Size];
	unsigned char encrypted_data[sizeof(paddedDecrypted_gradebook)];
	unsigned char mac[MAC_Size];
}Encrypted_gradebook;


typedef struct ActionType 
{
  int add_assignment;
  int delete_assignment;
  int add_student;
  int delete_student;
  int add_grade;
} ActionType;

typedef struct D_ActionType
{
	int print_assignment;
	int print_student;
	int print_final;
}D_ActionType;

typedef struct D_CmdLineResult
{
	char filename[max_n_len];
	char a_name[max_n_len];
	char f_name[max_n_len];
  	char l_name[max_n_len];
  	int index;
  	int order;
}D_CmdLineResult;

typedef struct Student_go
{
	char first_name[max_n_len];
	char last_name[max_n_len];
	int grades[max_assignments];
	int index;
} Student_go;

typedef struct Student_fa
{
	char first_name[max_n_len];
	char last_name[max_n_len];
	int grades[max_assignments];
	float final;
} Student_fa;

typedef struct _CmdLineResult 
{
  int good;
  char filename[max_n_len];
  unsigned char key[key_size];
  int index;
  char a_name[max_n_len];
  int points;
  int grade;
  float weight;
  char f_name[max_n_len];
  char l_name[max_n_len];
}CmdLineResult;