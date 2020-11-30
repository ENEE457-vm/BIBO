#ifndef _BUFFR_H
#define _BUFFR_H

//Buffer has a string of max gradebook length and the current length of that string
typedef struct _Buffer {
  unsigned char Buf[4101100];
  unsigned long Length;
} Buffer;

//Information about an assignment
typedef struct _Assignment {
  //Maximum 1000 students, with first and last names <= 100 characters
  unsigned char students_first[1000][100];
  unsigned char students_last[1000][100];
  //Name of assignment, <= 100 characters
  unsigned char name[100];
  //Points for up to 1000 students
  int points[1000];
  //Total points for the assignment
  int total;
  //Weight of the assignment
  float weight;
  //Current number of students who have a grade
  int num_students;
} Assignment;

//Information about the gradebook
typedef struct _Gradebook {
  //Maximum 1000 students, with first and last names <= 100 characters
  unsigned char students_first[1000][100];
  unsigned char students_last[1000][100];
  //Maximum 100 assignments
  Assignment assignments[100];
  //current number of students in the gradebook
  int num_students;
  //current number of assignments in the gradebook
  int num_assignments;
} Gradebook;

void write_to_path(char *path, Buffer *B, unsigned char *key);
void print_Gradebook(Gradebook *R, Buffer *B);
int read_Gradebook_from_path(unsigned char *path, unsigned char *key, Gradebook *);
#endif
