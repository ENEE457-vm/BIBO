#ifndef _BUFFR_H
#define _BUFFR_H
#define MAX_STUDENTS 200
#define MAX_STR_LENGTH 100

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

typedef struct Assignment {
	char *name;
//	int *score;
	int total;
	float weight;
	struct Assignment *next;
} Assignment;

typedef struct Score {
	int score;
	struct Score *next;
} Score;

typedef struct Student {
	char *fname;
	char *lname;
	struct Student *snext;
	struct Score *scorehead;
	struct Score *scoretail;
//	struct Assignment *ahead;
//	struct Assignment *atail;
} Student;

typedef struct Gradebook {
  unsigned char *key;
  char *name;
  struct Student *shead;
  struct Student *stail;
  struct Assignment *ahead;
  struct Assignment *atail;
} Gradebook;


Buffer read_from_path(char *path, unsigned char *key);
void write_to_path(char *path, unsigned char *key, Gradebook **, unsigned int *o);
Buffer concat_buffs(Buffer *A, Buffer *B);
Buffer print_record(Gradebook *R);
void dump_record(Gradebook *R);

int read_Gradebook_from_path(char *path, unsigned char *key, Gradebook **, unsigned int *o);

#endif
