#ifndef _BUFFR_H
#define _BUFFR_H

typedef struct _Buffer {
    unsigned char *Buf;
    unsigned long Length;
} Buffer;

typedef enum _ActionType {
    add_assignment,
    delete_assignment,
    add_student,
    delete_student,
    add_grade,
    print_assignment,
    print_student,
    print_final
} ActionType;

typedef struct _Student {
    unsigned char *fname;
    unsigned char *lname;
} Student;

typedef struct _Assignment {
    unsigned char *assignment_name;
    int points;
    float weight;
    int weight_char_count;
    int *grades;
    int grades_length;
} Assignment;

typedef struct _Gradebook {
    Student *students;
    int students_length;
    Assignment *assignments;
    int assignments_length;
} Gradebook;

typedef struct _FinalGrade {
    unsigned char *fname;
    unsigned char *lname;
    float final;
} FinalGrade;

int has_assignment(char *aname, Gradebook *R);
int sum_weights_valid(float new_weight, Gradebook *R);
int has_student(char *fname, char *lname, Gradebook *R);

Buffer read_from_path(char *path, unsigned char *key);
int get_Gradebook(Gradebook *R, Buffer *B);
Buffer print_Gradebook_to_buffer(Gradebook *R);
void write_to_path(char *path, Buffer *B, unsigned char *key);

void add_student_to_gradebook(unsigned char *fname, unsigned char *lname, Gradebook *gb);
void add_assignment_to_gradebook(unsigned char *name, int points, float weight, int weight_char_count, Gradebook *gb);
void add_grade_to_gradebook(unsigned char *aname, unsigned char *fname, unsigned char *lname, int grade, Gradebook *gb);
void delete_all_grades_for_student(int student_index, Gradebook *gb);
void delete_student_from_gradebook(unsigned char *fname, unsigned char *lname, Gradebook *gb);
void delete_assignment_from_gradebook(unsigned char *aname, Gradebook *gb);

#endif
