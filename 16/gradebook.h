#ifndef GRADEBOOK_H
#define GRADEBOOK_H
#include <stdint.h>

#define MAX_STUDENTS 500
#define MAX_ASSIGNMENTS 500
#define MAX_NAME_LEN 50

typedef enum _return_code {
	RETURN_OK = 0,
	RETURN_FULL = -1,
	RETURN_ERROR = -2,
	RETURN_NAME_TOO_LONG = -3,
} return_code;

typedef enum _order_t {
	ORDER_ALPHABET,
	ORDER_GRADE,
} order_t;

typedef struct _assignment_t {
	char name[MAX_NAME_LEN];
	int16_t points;
	double weight;
	uint16_t assignment_id;
} assignment_t;

typedef struct _student_t {
	int16_t num_grades;
	char first_name[MAX_NAME_LEN];
	char last_name[MAX_NAME_LEN];
	int16_t grades[MAX_ASSIGNMENTS];
	uint32_t assignment_ids[MAX_ASSIGNMENTS];
} student_t;

typedef struct _gradebook_t {
	int16_t num_assignments;
	int16_t num_students;
	uint32_t assignment_id_counter;
	double total_weight;
	assignment_t assignments[MAX_ASSIGNMENTS];
	student_t students[MAX_STUDENTS];
} gradebook_t;

/**
 * Sets all bytes of gradebook to 0. Called if no existing gradebook file is found.
 * @param gradebook Gradebook to initialize
 */
void gradebook_init(gradebook_t *gradebook);

/**
 * Adds an assignment with the given name, points, and weight to gradebook.
 * Checks that an assignment with the given name does not exist in gradebook,
 * that the new total weight does not exceed 1, and that that gradebook is not
 * full of assignments.
 * @param gradebook Gradebook to insert into
 * @param name Name of new assignment
 * @param points Points for new assignment
 * @param weight Weight of new assignment
 * @return RETURN_OK if successful, RETURN_FULL if gradebook is full of assignments,
 * RETURN_ERROR if name or weight are invalid, RETURN_NAME_TOO_LONG if name is too long
 */
return_code add_assignment(gradebook_t *gradebook, const char *name, int16_t points, double weight);

/**
 * Deletes the assignment with the given name from gradebook.
 * @param gradebook Gradebook to delete from
 * @param name Name of assignment to delete
 * @return RETURN_OK if successful, RETURN_ERROR if an assignment with the given
 * name does not exist, RETURN_NAME_TOO_LONG if name is too long
 */
return_code delete_assignment(gradebook_t *gradebook, const char *name);

/**
 * Adds a student with the given first and last name to the gradebook. Checks
 * that a student with this name does not exist and that the gradebook is not
 * full of students.
 * @param gradebook Gradebook to insert into
 * @param first First name of student
 * @param last Last name of student
 * @return RETURN_OK if successful, RETURN_ERROR if student already exists,
 * RETURN_FULL if the gradebook is full of students, RETURN_NAME_TOO_LONG if
 * either name is too long
 */
return_code add_student(gradebook_t *gradebook, const char *first, const char *last);

/**
 * Deletes the student with the given name from gradebook.
 * @param gradebook Gradebook to delete from
 * @param first First name of student to delete
 * @param last Last name of student to delete
 * @return RETURN_OK if successful, RETURN_ERROR if a student with the given
 * name does not exist, RETURN_NAME_TOO_LONG if either name is too long
 */
return_code delete_student(gradebook_t *gradebook, const char *first, const char *last);

/**
 * Adds the given grade to the student with given first/last name to the
 * assignment with the given name.
 * @param gradebook Gradebook to add to
 * @param first First name of student to give grade
 * @param last Last name of student to give grade
 * @param assignment_name Name of assignment to give grade for
 * @param grade The grade to give
 * @return RETURN_OK if successful, RETURN_ERROR if either the student name
 * or assignment name does not exist, RETURN_NAME_TOO_LONG if any name is too long
 */
return_code add_grade(gradebook_t *gradebook, const char *first, const char *last,
                      const char *assignment_name, int16_t grade);

/**
 * Prints all students' grades for the given assignment. Ordered alphabetically
 * by first/last name or by high to low grade.
 * @param gradebook Gradebook to read from
 * @param name Assignment name
 * @param order Enum value from order_t
 * @return RETURN_OK if good, RETURN_ERROR if assignment name doesn't exist
 */
return_code print_assignment(gradebook_t *gradebook, const char *name, order_t order);

/**
 * Prints all grades for the given student.
 * @param gradebook Gradebook to read from
 * @param first Student first name
 * @param last Student last name
 * @return RETURN_OK if good, RETURN_ERROR if student doesn't exist
 */
return_code print_student(gradebook_t *gradebook, const char *first, const char *last);

/**
 * Prints final grades for all students. Ordered alphabetically by first/last
 * name or by high to low grade.
 * @param gradebook Gradebook to read from
 * @param order Enum value from order_t
 */
void print_final(gradebook_t *gradebook, order_t order);
#endif
