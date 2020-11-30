#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include "gradebook.h"

// Struct used in print_assignment()
typedef struct _student_grade_t {
	char *first, *last;
	int16_t grade;
} student_grade_t;

// Struct used in print_final()
typedef struct _final_grade_t {
	char *first, *last;
	double grade;
} final_grade_t;

static int comp_student_grade_alphabet(const void *a, const void *b) {
	const student_grade_t *a1 = a, *b1 = b;
	if (strcmp(a1->last, b1->last)) {
		return strcmp(a1->last, b1->last);
	} else {
		return strcmp(a1->first, b1->first);
	}
}

static int comp_student_grade_grade(const void *a, const void *b) {
	return ((student_grade_t *)b)->grade - ((student_grade_t *)a)->grade;
}

static int comp_final_grade_alphabet(const void *a, const void *b) {
	const final_grade_t *a1 = a, *b1 = b;
	if (strcmp(a1->last, b1->last)) {
		return strcmp(a1->last, b1->last);
	} else {
		return strcmp(a1->first, b1->first);
	}
}

static int comp_final_grade_grade(const void *a, const void *b) {
	double b_grade = ((final_grade_t *)b)->grade;
	double a_grade = ((final_grade_t *)a)->grade;
	if (a_grade < b_grade) {
		return 1;
	} else if (a_grade > b_grade) {
		return -1;
	} else {
		return 0;
	}
}

/**
 * Returns index of the assignment with the given name, or -1 if not exist. Sets
 * out parameter with assignment id.
 */
static int search_for_assignment(const gradebook_t *gradebook, const char *name, uint16_t *out_id) {
	for (int i = 0; i < gradebook->num_assignments; i++) {
		if (!strcmp(gradebook->assignments[i].name, name)) {
			if (out_id) {
				*out_id = gradebook->assignments[i].assignment_id;
			}
			return i;
		}
	}
	return -1;
}

static assignment_t * search_for_assignment_id(gradebook_t *gradebook, uint16_t id) {
	for (int i = 0; i < gradebook->num_assignments; i++) {
		if (gradebook->assignments[i].assignment_id == id) {
			return &gradebook->assignments[i];
		}
	}
	return NULL;
}

static int valid_assign_name(const char *name) {
	int len = strlen(name);
	for (int i = 0; i < len; i++) {
		if (!isalnum(name[i])) {
			return 0;
		}
	}
	return 1;
}

static int valid_student_name(const char *name) {
	int len = strlen(name);
	for (int i = 0; i < len; i++) {
		if (!isalpha(name[i])) {
			return 0;
		}
	}
	return 1;
}

/**
 * Returns index of student with given names, or -1 if not exist
 */
static int search_for_student(const gradebook_t *gradebook, const char *first, const char *last) {
	for (int i = 0; i < gradebook->num_students; i++) {
		const student_t *student = &gradebook->students[i];
		if (!strcmp(student->first_name, first) && !strcmp(student->last_name, last)) {
			return i;
		}
	}
	return -1;
}

void gradebook_init(gradebook_t *gradebook) {
	memset(gradebook, 0, sizeof(gradebook_t));
	gradebook->assignment_id_counter = 1;
	return;
}

return_code add_assignment(gradebook_t *gradebook, const char *name, int16_t points, double weight) {
	if (gradebook->num_assignments == MAX_ASSIGNMENTS) {
		return RETURN_FULL;
	} else if ((gradebook->total_weight + weight - 1.0) > 0.00001 || weight < 0.0 || weight > 1.0) {
		return RETURN_ERROR;
	} else if (points <= 0) {
		return RETURN_ERROR;
	} else if (!valid_assign_name(name)) {
		return RETURN_ERROR;
	} else if (strlen(name) >= MAX_NAME_LEN) {
		return RETURN_NAME_TOO_LONG;
	} else if (search_for_assignment(gradebook, name, NULL) >= 0) {
		return RETURN_ERROR;
	}

	assignment_t *new_assign = &gradebook->assignments[gradebook->num_assignments++];
	new_assign->assignment_id = gradebook->assignment_id_counter++;
	new_assign->points = points;
	new_assign->weight = weight;
	strcpy(new_assign->name, name);
	gradebook->total_weight += weight;

	return RETURN_OK;
}

return_code delete_assignment(gradebook_t *gradebook, const char *name) {
	uint16_t assignment_id;
	int index;
	assignment_t *assignments = gradebook->assignments;

	if (strlen(name) >= MAX_NAME_LEN) {
		return RETURN_NAME_TOO_LONG;
	} else if ((index = search_for_assignment(gradebook, name, &assignment_id)) < 0) {
		return RETURN_ERROR;
	}

	// delete the assignment by shifting everything after it 1 index left
	gradebook->total_weight -= assignments[index].weight;
	memmove(assignments + index, assignments + index + 1,
	        sizeof(assignment_t) * (gradebook->num_assignments - index));
	gradebook->num_assignments -= 1;

	// delete all occurrences of assignment_id in each student
	for (int i = 0; i < gradebook->num_students; i++) {
		student_t *student = &gradebook->students[i];
		for (int j = 0; j < student->num_grades; j++) {
			if (student->assignment_ids[j] == assignment_id) {
				memmove(student->assignment_ids + j, student->assignment_ids + j + 1,
				        sizeof(uint16_t) * (student->num_grades - j));
				memmove(student->grades + j, student->grades + j + 1,
				        sizeof(int16_t) * (student->num_grades - j));
				student->num_grades -= 1;
				break;
			}
		}
	}
	return RETURN_OK;
}

return_code add_student(gradebook_t *gradebook, const char *first, const char *last) {
	int index;
	if (gradebook->num_students == MAX_STUDENTS) {
		return RETURN_FULL;
	} else if (!valid_student_name(first) || !valid_student_name(last)) {
		return RETURN_ERROR;
	} else if (strlen(first) >= MAX_NAME_LEN || strlen(last) >= MAX_NAME_LEN) {
		return RETURN_NAME_TOO_LONG;
	} else if ((index = search_for_student(gradebook, first, last)) >= 0) {
		return RETURN_ERROR;
	}

	student_t *student = &gradebook->students[gradebook->num_students++];
	student->num_grades = 0;
	strcpy(student->first_name, first);
	strcpy(student->last_name, last);
	memset(student->grades, 0, sizeof(student->grades));
	memset(student->assignment_ids, 0, sizeof(student->assignment_ids));

	return RETURN_OK;
}

return_code delete_student(gradebook_t *gradebook, const char *first, const char *last) {
	int index;
	student_t *students = gradebook->students;
	if (strlen(first) >= MAX_NAME_LEN || strlen(last) >= MAX_NAME_LEN) {
		return RETURN_NAME_TOO_LONG;
	} else if ((index = search_for_student(gradebook, first, last)) < 0) {
		return RETURN_ERROR;
	}

	memmove(students + index, students + index + 1, sizeof(student_t) * (gradebook->num_students - index));
	gradebook->num_students -= 1;
	return RETURN_OK;
}

return_code add_grade(gradebook_t *gradebook, const char *first, const char *last,
                      const char *assignment_name, int16_t grade) {
	int student_index;
	uint16_t assignment_id;
	student_t *student;
	if (strlen(first) >= MAX_NAME_LEN || strlen(last) >= MAX_NAME_LEN
	    || strlen(assignment_name) >= MAX_NAME_LEN) {
		return RETURN_NAME_TOO_LONG;
	} else if ((student_index = search_for_student(gradebook, first, last)) < 0) {
		return RETURN_ERROR;
	} else if (search_for_assignment(gradebook, assignment_name, &assignment_id) < 0) {
		return RETURN_ERROR;
	}

	student = &gradebook->students[student_index];
	int grade_index = student->num_grades;
	for (int i = 0; i < student->num_grades; i++) {
		if (student->assignment_ids[i] == assignment_id) {
			grade_index = i;
			break;
		}
	}
	student->grades[grade_index] = grade;
	student->assignment_ids[grade_index] = assignment_id;
	if (grade_index == student->num_grades) {
		student->num_grades += 1;
	}
	return RETURN_OK;
}

return_code print_assignment(gradebook_t *gradebook, const char *name, order_t order) {
	student_grade_t grades[MAX_STUDENTS];
	int grades_len = 0;
	uint16_t assign_id;
	if (search_for_assignment(gradebook, name, &assign_id) < 0) {
		return RETURN_ERROR;
	}

	for (int i = 0; i < gradebook->num_students; i++) {
		student_t *student = &gradebook->students[i];
		for (int j = 0; j < student->num_grades; j++) {
			if (student->assignment_ids[j] == assign_id) {
				grades[grades_len].first = (char *)student->first_name;
				grades[grades_len].last = (char *)student->last_name;
				grades[grades_len].grade = student->grades[j];
				grades_len += 1;
				break;
			}
		}
	}

	if (order == ORDER_ALPHABET) {
		qsort(grades, grades_len, sizeof(student_grade_t), comp_student_grade_alphabet);
	} else {
		qsort(grades, grades_len, sizeof(student_grade_t), comp_student_grade_grade);
	}

	for (int i = 0; i < grades_len; i++) {
		const student_grade_t *student_grade = &grades[i];
		printf("(%s, %s, %d)\n", student_grade->last, student_grade->first,
		       student_grade->grade);
	}

	return RETURN_OK;
}

return_code print_student(gradebook_t *gradebook, const char *first, const char *last) {
	int student_ind = search_for_student(gradebook, first, last);
	if (student_ind < 0) {
		return RETURN_ERROR;
	}

	const student_t *student = &gradebook->students[student_ind];
	for (int i = 0; i < student->num_grades; i++) {
		uint16_t id = student->assignment_ids[i];
		const assignment_t *assign = search_for_assignment_id(gradebook, id);
		printf("(%s, %d)\n", assign->name, student->grades[i]);
	}

	return RETURN_OK;
}

void print_final(gradebook_t *gradebook, order_t order) {
	final_grade_t grades[MAX_STUDENTS] = {0};
	int grades_len = 0;

	for (int i = 0; i < gradebook->num_students; i++) {
		student_t *student = &gradebook->students[i];
		for (int j = 0; j < student->num_grades; j++) {
			uint16_t id = student->assignment_ids[j];
			assignment_t *assign = search_for_assignment_id(gradebook, id);
			grades[grades_len].first = student->first_name;
			grades[grades_len].last = student->last_name;
			grades[grades_len].grade += assign->weight * student->grades[j] / assign->points;
		}
		grades_len += 1;
	}

	if (order == ORDER_ALPHABET) {
		qsort(grades, grades_len, sizeof(final_grade_t), comp_final_grade_alphabet);
	} else {
		qsort(grades, grades_len, sizeof(final_grade_t), comp_final_grade_grade);
	}

	for (int i = 0; i < grades_len; i++) {
		printf("(%s, %s, %g)\n", grades[i].last, grades[i].first, grades[i].grade);
	}
}
