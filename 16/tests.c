#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>
#include "gradebook.h"
#include "gradebook_crypto.h"

static void test_gradebook_init(void **state) {
	gradebook_t gradebook;
	gradebook_init(&gradebook);
	assert_int_equal(gradebook.num_assignments, 0);
	assert_int_equal(gradebook.num_students, 0);
	assert_int_equal(gradebook.assignment_id_counter, 1);
	assert_float_equal(gradebook.total_weight, 0.0, 0.00001);
}

static void test_add_assignment(void **state) {
	gradebook_t gradebook = {0};
	assignment_t *assign = &gradebook.assignments[0];
	gradebook.assignment_id_counter = 1;

	assert_int_equal(add_assignment(&gradebook, "hw1", 10, 0.1), RETURN_OK);
	assert_int_equal(gradebook.num_assignments, 1);
	assert_int_equal(gradebook.num_students, 0);
	assert_int_equal(gradebook.assignment_id_counter, 2);
	assert_float_equal(gradebook.total_weight, 0.1, 0.00001);

	assert_string_equal(assign->name, "hw1");
	assert_int_equal(assign->points, 10);
	assert_float_equal(assign->weight, 0.1, 0.00001);
	assert_int_equal(assign->assignment_id, 1);
}

static void test_add_assignment_max_weight(void **state) {
	gradebook_t gradebook = {0};
	gradebook.assignment_id_counter = 1;

	assert_int_equal(add_assignment(&gradebook, "hw1", 10, 1.0), RETURN_OK);
}

static void test_add_assignment_bad_name(void **state) {
	gradebook_t gradebook = {0};
	gradebook.assignment_id_counter = 1;

	assert_int_equal(add_assignment(&gradebook, "bad__", 10, 1.0), RETURN_ERROR);
	assert_int_equal(add_assignment(&gradebook, " bad", 10, 1.0), RETURN_ERROR);
}

static void test_add_assignment_full(void **state) {
	gradebook_t gradebook = {0};
	gradebook.num_assignments = MAX_ASSIGNMENTS;

	assert_int_equal(add_assignment(&gradebook, "hw1", 10, 0.1), RETURN_FULL);
}

static void test_add_assignment_invalid_numbers(void **state) {
	gradebook_t gradebook = {0};

	assert_int_equal(add_assignment(&gradebook, "hw1", -5, 0.5), RETURN_ERROR);
	assert_int_equal(add_assignment(&gradebook, "hw2", 5, 1.01), RETURN_ERROR);
}

static void test_add_assignment_name_too_long(void **state) {
	gradebook_t gradebook = {0};
	char str[MAX_NAME_LEN + 1];
	memset(str, 'a', sizeof(str) - 1);
	str[sizeof(str) - 1] = '\0';

	assert_int_equal(add_assignment(&gradebook, str, 10, 0.1), RETURN_NAME_TOO_LONG);
}

static void test_add_assignment_duplicate(void **state) {
	gradebook_t gradebook = {0};
	gradebook.num_assignments = 1;
	gradebook.assignment_id_counter = 2;
	gradebook.total_weight = 0.1;
	assignment_t *assign = &gradebook.assignments[0];
	strcpy(assign->name, "hw1");
	assign->points = 10;
	assign->weight = 0.1;
	assign->assignment_id = 1;

	assert_int_equal(add_assignment(&gradebook, "hw1", 20, 0.2), RETURN_ERROR);
}

static void test_add_assignment_weight_overflow(void **state) {
	gradebook_t gradebook = {0};
	gradebook.num_assignments = 1;
	gradebook.assignment_id_counter = 2;
	gradebook.total_weight = 1;
	assignment_t *assign = &gradebook.assignments[0];
	strcpy(assign->name, "hw1");
	assign->points = 10;
	assign->weight = 1;
	assign->assignment_id = 1;

	assert_int_equal(add_assignment(&gradebook, "hw2", 10, 0.1), RETURN_ERROR);
}

static void test_add_student(void **state) {
	gradebook_t gradebook = {0};
	gradebook.assignment_id_counter = 1;
	student_t *student = &gradebook.students[0];

	assert_int_equal(add_student(&gradebook, "John", "Heide"), RETURN_OK);
	assert_int_equal(gradebook.num_assignments, 0);
	assert_int_equal(gradebook.num_students, 1);
	assert_int_equal(gradebook.assignment_id_counter, 1);
	assert_float_equal(gradebook.total_weight, 0.0, 0.00001);
	assert_int_equal(student->num_grades, 0);
	assert_string_equal(student->first_name, "John");
	assert_string_equal(student->last_name, "Heide");
}

static void test_add_student_duplicate(void **state) {
	gradebook_t gradebook = {0};
	gradebook.num_students = 1;
	student_t *student = &gradebook.students[0];
	student->num_grades = 0;
	strcpy(student->first_name, "John");
	strcpy(student->last_name, "Heide");

	assert_int_equal(add_student(&gradebook, "John", "Heide"), RETURN_ERROR);
}

static void test_add_student_bad_name(void **state) {
	gradebook_t gradebook = {0};

	assert_int_equal(add_student(&gradebook, "bad__", "Heide"), RETURN_ERROR);
	assert_int_equal(add_student(&gradebook, "bad1", "Heide"), RETURN_ERROR);
	assert_int_equal(add_student(&gradebook, " bad", "Heide"), RETURN_ERROR);
	assert_int_equal(add_student(&gradebook, "John", "bad__"), RETURN_ERROR);
	assert_int_equal(add_student(&gradebook, "John", "bad1"), RETURN_ERROR);
	assert_int_equal(add_student(&gradebook, "John", " bad"), RETURN_ERROR);
}

static void test_add_student_full(void **state) {
	gradebook_t gradebook = {0};
	gradebook.num_students = MAX_STUDENTS;

	assert_int_equal(add_student(&gradebook, "John", "Heide"), RETURN_FULL);
}

static void test_add_student_name_too_long(void **state) {
	gradebook_t gradebook = {0};
	char first[MAX_NAME_LEN + 1], last[MAX_NAME_LEN + 1];
	memset(first, 'a', sizeof(first) - 1);
	first[sizeof(first) - 1] = '\0';
	memset(last, 'a', sizeof(last) - 1);
	last[sizeof(last) - 1] = '\0';

	assert_int_equal(add_student(&gradebook, first, "Heide"), RETURN_NAME_TOO_LONG);
	assert_int_equal(add_student(&gradebook, "John", last), RETURN_NAME_TOO_LONG);
	assert_int_equal(add_student(&gradebook, first, last), RETURN_NAME_TOO_LONG);
	assert_int_equal(gradebook.num_students, 0);
}

static void test_delete_assignment(void **state) {
	gradebook_t gradebook = {0};
	gradebook.assignment_id_counter = 2;
	gradebook.num_assignments = 1;
	gradebook.num_students = 1;
	gradebook.total_weight = 0.2;
	assignment_t *assign = &gradebook.assignments[0];
	strcpy(assign->name, "hw1");
	assign->points = 10;
	assign->weight = 0.2;
	assign->assignment_id = 1;
	student_t *student = &gradebook.students[0];
	student->num_grades = 1;
	strcpy(student->first_name, "John");
	strcpy(student->last_name, "Heide");
	student->grades[0] = 9;
	student->assignment_ids[0] = 1;

	assert_int_equal(delete_assignment(&gradebook, "hw1"), RETURN_OK);
	assert_int_equal(gradebook.num_assignments, 0);
	assert_float_equal(gradebook.total_weight, 0.0, 0.00001);
	assert_int_equal(student->num_grades, 0);
	assert_int_equal(student->assignment_ids[0], 0);
}

static void test_delete_assignment_multiple_students(void **state) {
	gradebook_t gradebook = {0};
	gradebook.assignment_id_counter = 3;
	gradebook.num_assignments = 2;
	gradebook.num_students = 2;
	gradebook.total_weight = 0.8;
	assignment_t *assign1 = &gradebook.assignments[0];
	strcpy(assign1->name, "hw1");
	assign1->points = 10;
	assign1->weight = 0.2;
	assign1->assignment_id = 1;
	assignment_t *assign2 = &gradebook.assignments[1];
	strcpy(assign2->name, "hw2");
	assign2->points = 20;
	assign2->weight = 0.6;
	assign2->assignment_id = 2;
	student_t *student1 = &gradebook.students[0];
	student1->num_grades = 2;
	strcpy(student1->first_name, "John");
	strcpy(student1->last_name, "Heide");
	student1->grades[0] = 9;
	student1->grades[1] = 18;
	student1->assignment_ids[0] = 1;
	student1->assignment_ids[1] = 2;
	student_t *student2 = &gradebook.students[1];
	student2->num_grades = 1;
	strcpy(student2->first_name, "Other");
	strcpy(student2->last_name, "Student");
	student2->grades[0] = 5;
	student2->assignment_ids[0] = 1;

	assert_int_equal(delete_assignment(&gradebook, "hw1"), RETURN_OK);
	assert_int_equal(gradebook.num_assignments, 1);
	assert_float_equal(gradebook.total_weight, 0.6, 0.00001);
	assert_string_equal(assign1->name, "hw2");
	assert_int_equal(assign1->points, 20);
	assert_float_equal(assign1->weight, 0.6, 0.00001);
	assert_int_equal(assign1->assignment_id, 2);
	assert_int_equal(student1->num_grades, 1);
	assert_int_equal(student1->grades[0], 18);
	assert_int_equal(student1->assignment_ids[0], 2);
	assert_int_equal(student2->num_grades, 0);
	assert_int_equal(delete_assignment(&gradebook, "hw1"), RETURN_ERROR);
}

static void test_delete_assignment_name_too_long(void **state) {
	gradebook_t gradebook = {0};
	char str[MAX_NAME_LEN + 1];
	memset(str, 'a', sizeof(str) - 1);
	str[sizeof(str) - 1] = '\0';

	assert_int_equal(delete_assignment(&gradebook, str), RETURN_NAME_TOO_LONG);
}

static void test_delete_assignment_error(void **state) {
	gradebook_t gradebook = {0};
	gradebook.assignment_id_counter = 2;
	gradebook.num_assignments = 1;
	gradebook.num_students = 0;
	gradebook.total_weight = 0.2;
	assignment_t *assign = &gradebook.assignments[0];
	strcpy(assign->name, "hw1");
	assign->points = 10;
	assign->weight = 0.2;
	assign->assignment_id = 1;

	assert_int_equal(delete_assignment(&gradebook, "hw2"), RETURN_ERROR);
	assert_int_equal(gradebook.num_assignments, 1);
	assert_float_equal(gradebook.total_weight, 0.2, 0.00001);
}

static void test_delete_student(void **state) {
	gradebook_t gradebook = {0};
	gradebook.num_students = 1;
	student_t *student = &gradebook.students[0];
	student->num_grades = 0;
	strcpy(student->first_name, "John");
	strcpy(student->last_name, "Heide");

	assert_int_equal(delete_student(&gradebook, "John", "Heide"), RETURN_OK);
	assert_int_equal(gradebook.num_students, 0);
}

static void test_delete_student_multiple(void **state) {
	gradebook_t gradebook = {0};
	gradebook.num_students = 2;
	student_t *student1 = &gradebook.students[0];
	student1->num_grades = 0;
	strcpy(student1->first_name, "John");
	strcpy(student1->last_name, "Heide");
	student_t *student2 = &gradebook.students[1];
	student2->num_grades = 0;
	strcpy(student2->first_name, "Other");
	strcpy(student2->last_name, "Student");

	assert_int_equal(delete_student(&gradebook, "John", "Heide"), RETURN_OK);
	assert_int_equal(gradebook.num_students, 1);
	assert_int_equal(student1->num_grades, 0);
	assert_string_equal(student1->first_name, "Other");
	assert_string_equal(student1->last_name, "Student");
}

static void test_delete_student_name_too_long(void **state) {
	gradebook_t gradebook = {0};
	char first[MAX_NAME_LEN + 1], last[MAX_NAME_LEN + 1];
	memset(first, 'a', sizeof(first) - 1);
	first[sizeof(first) - 1] = '\0';
	memset(last, 'a', sizeof(last) - 1);
	last[sizeof(last) - 1] = '\0';

	assert_int_equal(delete_student(&gradebook, first, "Heide"), RETURN_NAME_TOO_LONG);
	assert_int_equal(delete_student(&gradebook, "John", last), RETURN_NAME_TOO_LONG);
	assert_int_equal(delete_student(&gradebook, first, last), RETURN_NAME_TOO_LONG);
}

static void test_delete_student_error(void **state) {
	gradebook_t gradebook = {0};
	student_t *student = &gradebook.students[0];
	student->num_grades = 0;
	strcpy(student->first_name, "John");
	strcpy(student->last_name, "Heide");

	assert_int_equal(delete_student(&gradebook, "Wrong", "Name"), RETURN_ERROR);
}

static void test_add_grade(void **state) {
	gradebook_t gradebook = {0};
	gradebook.num_assignments = 1;
	gradebook.num_students = 1;
	gradebook.assignment_id_counter = 2;
	student_t *student = &gradebook.students[0];
	student->num_grades = 0;
	strcpy(student->first_name, "John");
	strcpy(student->last_name, "Heide");
	assignment_t *assign = &gradebook.assignments[0];
	strcpy(assign->name, "hw1");
	assign->points = 10;
	assign->weight = 0.2;
	assign->assignment_id = 1;

	assert_int_equal(add_grade(&gradebook, "John", "Heide", "hw1", 8), RETURN_OK);
	assert_int_equal(student->num_grades, 1);
	assert_int_equal(student->grades[0], 8);
	assert_int_equal(student->assignment_ids[0], 1);
}

static void test_add_grade_twice(void **state) {
	gradebook_t gradebook = {0};
	gradebook.num_assignments = 1;
	gradebook.num_students = 1;
	gradebook.assignment_id_counter = 2;
	student_t *student = &gradebook.students[0];
	student->num_grades = 0;
	strcpy(student->first_name, "John");
	strcpy(student->last_name, "Heide");
	assignment_t *assign = &gradebook.assignments[0];
	strcpy(assign->name, "hw1");
	assign->points = 10;
	assign->weight = 0.2;
	assign->assignment_id = 1;

	assert_int_equal(add_grade(&gradebook, "John", "Heide", "hw1", 8), RETURN_OK);
	assert_int_equal(add_grade(&gradebook, "John", "Heide", "hw1", 7), RETURN_OK);
	assert_int_equal(student->num_grades, 1);
	assert_int_equal(student->grades[0], 7);
	assert_int_equal(student->assignment_ids[0], 1);
}

static void test_add_grade_error(void **state) {
	gradebook_t gradebook = {0};
	gradebook.num_assignments = 1;
	gradebook.num_students = 1;
	gradebook.assignment_id_counter = 2;
	student_t *student = &gradebook.students[0];
	student->num_grades = 0;
	strcpy(student->first_name, "John");
	strcpy(student->last_name, "Heide");
	assignment_t *assign = &gradebook.assignments[0];
	strcpy(assign->name, "hw1");
	assign->points = 10;
	assign->weight = 0.2;
	assign->assignment_id = 1;

	assert_int_equal(add_grade(&gradebook, "Wrong", "Heide", "hw1", 8), RETURN_ERROR);
	assert_int_equal(add_grade(&gradebook, "John", "Wrong", "hw1", 8), RETURN_ERROR);
	assert_int_equal(add_grade(&gradebook, "John", "Heide", "Wrong", 8), RETURN_ERROR);
}

static void test_add_grade_name_too_long(void **state) {
	char first[MAX_NAME_LEN + 1], last[MAX_NAME_LEN + 1];
	memset(first, 'a', sizeof(first) - 1);
	first[sizeof(first) - 1] = '\0';
	memset(last, 'a', sizeof(last) - 1);
	last[sizeof(last) - 1] = '\0';
	char hw_name[MAX_NAME_LEN + 1];
	memset(hw_name, 'a', sizeof(hw_name) - 1);
	hw_name[sizeof(hw_name) - 1] = '\0';

	gradebook_t gradebook = {0};
	gradebook.num_assignments = 1;
	gradebook.num_students = 1;
	gradebook.assignment_id_counter = 2;
	student_t *student = &gradebook.students[0];
	student->num_grades = 0;
	strcpy(student->first_name, "John");
	strcpy(student->last_name, "Heide");
	assignment_t *assign = &gradebook.assignments[0];
	strcpy(assign->name, "hw1");
	assign->points = 10;
	assign->weight = 0.2;
	assign->assignment_id = 1;

	assert_int_equal(add_grade(&gradebook, first, "Heide", "hw1", 8), RETURN_NAME_TOO_LONG);
	assert_int_equal(add_grade(&gradebook, "John", last, "hw1", 8), RETURN_NAME_TOO_LONG);
	assert_int_equal(add_grade(&gradebook, "John", "Heide", hw_name, 8), RETURN_NAME_TOO_LONG);
}

static void gradebookadd_integration_test(void **state) {
	gradebook_t gradebook;
	gradebook_init(&gradebook);
	assert_int_equal(add_assignment(&gradebook, "hw1", 100, 0.2), RETURN_OK);
	assert_int_equal(add_assignment(&gradebook, "hw2", 200, 0.5), RETURN_OK);
	assert_int_equal(add_assignment(&gradebook, "hw3", 300, 0.3), RETURN_OK);
	assert_int_equal(add_student(&gradebook, "John", "Heide"), RETURN_OK);
	assert_int_equal(add_student(&gradebook, "Student", "Two"), RETURN_OK);
	assert_int_equal(add_student(&gradebook, "Student", "Three"), RETURN_OK);
	assert_int_equal(add_grade(&gradebook, "John", "Heide", "hw1", 75), RETURN_OK);
	assert_int_equal(add_grade(&gradebook, "John", "Heide", "hw3", 200), RETURN_OK);
	assert_int_equal(add_grade(&gradebook, "Student", "Two", "hw2", 160), RETURN_OK);
	assert_int_equal(add_grade(&gradebook, "Student", "Two", "hw3", 260), RETURN_OK);
	assert_int_equal(add_grade(&gradebook, "Student", "Three", "hw2", 60), RETURN_OK);
	assert_int_equal(delete_student(&gradebook, "Student", "Three"), RETURN_OK);
	assert_int_equal(delete_assignment(&gradebook, "hw3"), RETURN_OK);
	assert_int_equal(add_assignment(&gradebook, "hw4", 400, 0.3), RETURN_OK);

	assignment_t *assign1 = &gradebook.assignments[0];
	assignment_t *assign2 = &gradebook.assignments[1];
	assignment_t *assign3 = &gradebook.assignments[2];
	student_t *student1 = &gradebook.students[0];
	student_t *student2 = &gradebook.students[1];

	assert_int_equal(gradebook.assignment_id_counter, 5);
	assert_int_equal(gradebook.num_assignments, 3);
	assert_int_equal(gradebook.num_students, 2);
	assert_float_equal(gradebook.total_weight, 1.0, 0.00001);

	assert_string_equal(assign1->name, "hw1");
	assert_int_equal(assign1->points, 100);
	assert_float_equal(assign1->weight, 0.2, 0.00001);
	assert_int_equal(assign1->assignment_id, 1);

	assert_string_equal(assign2->name, "hw2");
	assert_int_equal(assign2->points, 200);
	assert_float_equal(assign2->weight, 0.5, 0.00001);
	assert_int_equal(assign2->assignment_id, 2);

	assert_string_equal(assign3->name, "hw4");
	assert_int_equal(assign3->points, 400);
	assert_float_equal(assign3->weight, 0.3, 0.00001);
	assert_int_equal(assign3->assignment_id, 4);

	assert_int_equal(student1->num_grades, 1);
	assert_string_equal(student1->first_name, "John");
	assert_string_equal(student1->last_name, "Heide");
	assert_int_equal(student1->assignment_ids[0], 1);
	assert_int_equal(student1->grades[0], 75);

	assert_int_equal(student2->num_grades, 1);
	assert_string_equal(student2->first_name, "Student");
	assert_string_equal(student2->last_name, "Two");
	assert_int_equal(student2->assignment_ids[0], 2);
	assert_int_equal(student2->grades[0], 160);
}

static void test_parse_key(void **state) {
	uint8_t key_val[KEY_SIZE + 1], expected_key_val[KEY_SIZE];
	char key_str[2 * KEY_SIZE + 1];

	// Set key_str to "5555..." with null terminator
	memset(key_str, '5', 2 * KEY_SIZE);
	key_str[2 * KEY_SIZE] = '\0';
	// Set expected key val to {0x55, 0x55, ...} with an extra null byte
	// at the end to detect buffer overwrites
	memset(expected_key_val, 0x55, KEY_SIZE);
	key_val[KEY_SIZE] = 0;

	assert_int_equal(parse_key(key_str, key_val), 1);
	assert_memory_equal(key_val, expected_key_val, KEY_SIZE);
	assert_int_equal(key_val[KEY_SIZE], 0);

	expected_key_val[0] = 0x0A;
	expected_key_val[1] = 0x0B;
	expected_key_val[2] = 0x0C;
	expected_key_val[3] = 0x0D;
	expected_key_val[4] = 0x0E;
	key_str[0] = '0'; key_str[1] = 'A';
	key_str[2] = '0'; key_str[3] = 'B';
	key_str[4] = '0'; key_str[5] = 'c';
	key_str[6] = '0'; key_str[7] = 'D';
	key_str[8] = '0'; key_str[9] = 'e';

	assert_int_equal(parse_key(key_str, key_val), 1);
	assert_memory_equal(key_val, expected_key_val, KEY_SIZE);
	assert_int_equal(key_val[KEY_SIZE], 0);

	key_str[0] = 'g';
	assert_int_equal(parse_key(key_str, key_val), 0);
}

int __wrap_printf(const char *format, ...) {
	char *arg2_c, *arg3_c;
	int arg3_i, arg4_i;
	int num_args = mock();
	va_list valist;
	va_start(valist, num_args);

	switch (num_args) {
		case 2:
			arg2_c = va_arg(valist, char *);
			arg3_i = va_arg(valist, int);
			check_expected(arg2_c);
			check_expected(arg3_i);
			break;
		case 3:
			arg2_c = va_arg(valist, char *);
			arg3_c = va_arg(valist, char *);
			arg4_i = va_arg(valist, int);
			check_expected(arg2_c);
			check_expected(arg3_c);
			check_expected(arg4_i);
			break;
		default:
			break;
	}
	va_end(valist);
	return num_args;
}

static void test_print_student(void **state) {
	gradebook_t gradebook;
	gradebook_init(&gradebook);
	assert_int_equal(add_assignment(&gradebook, "hw1", 100, 0.2), RETURN_OK);
	assert_int_equal(add_assignment(&gradebook, "hw2", 200, 0.5), RETURN_OK);
	assert_int_equal(add_assignment(&gradebook, "hw3", 300, 0.3), RETURN_OK);
	assert_int_equal(add_student(&gradebook, "John", "Heide"), RETURN_OK);
	assert_int_equal(add_student(&gradebook, "Student", "Two"), RETURN_OK);
	assert_int_equal(add_student(&gradebook, "Student", "Three"), RETURN_OK);
	assert_int_equal(add_grade(&gradebook, "John", "Heide", "hw1", 75), RETURN_OK);
	assert_int_equal(add_grade(&gradebook, "John", "Heide", "hw3", 200), RETURN_OK);
	assert_int_equal(add_grade(&gradebook, "John", "Heide", "hw2", 250), RETURN_OK);
	assert_int_equal(add_grade(&gradebook, "Student", "Two", "hw2", 160), RETURN_OK);
	assert_int_equal(add_grade(&gradebook, "Student", "Two", "hw3", 260), RETURN_OK);
	assert_int_equal(add_grade(&gradebook, "Student", "Three", "hw2", 60), RETURN_OK);

	will_return_count(__wrap_printf, 2, 3);
	expect_string(__wrap_printf, arg2_c, "hw1");
	expect_value(__wrap_printf, arg3_i, 75);
	expect_string(__wrap_printf, arg2_c, "hw3");
	expect_value(__wrap_printf, arg3_i, 200);
	expect_string(__wrap_printf, arg2_c, "hw2");
	expect_value(__wrap_printf, arg3_i, 250);
	print_student(&gradebook, "John", "Heide");

	will_return_count(__wrap_printf, 2, 2);
	expect_string(__wrap_printf, arg2_c, "hw2");
	expect_value(__wrap_printf, arg3_i, 160);
	expect_string(__wrap_printf, arg2_c, "hw3");
	expect_value(__wrap_printf, arg3_i, 260);
	print_student(&gradebook, "Student", "Two");

	will_return_count(__wrap_printf, 2, 1);
	expect_string(__wrap_printf, arg2_c, "hw2");
	expect_value(__wrap_printf, arg3_i, 60);
	print_student(&gradebook, "Student", "Three");
}

static void test_print_assignment(void **state) {
	gradebook_t gradebook;
	gradebook_init(&gradebook);
	assert_int_equal(add_assignment(&gradebook, "hw1", 100, 0.2), RETURN_OK);
	assert_int_equal(add_student(&gradebook, "John", "Heide"), RETURN_OK);
	assert_int_equal(add_student(&gradebook, "Student", "Two"), RETURN_OK);
	assert_int_equal(add_student(&gradebook, "Student", "Three"), RETURN_OK);
	assert_int_equal(add_student(&gradebook, "Student", "Four"), RETURN_OK);
	assert_int_equal(add_student(&gradebook, "Student", "Five"), RETURN_OK);
	assert_int_equal(add_grade(&gradebook, "John", "Heide", "hw1", 75), RETURN_OK);
	assert_int_equal(add_grade(&gradebook, "Student", "Two", "hw1", 160), RETURN_OK);
	assert_int_equal(add_grade(&gradebook, "Student", "Three", "hw1", 60), RETURN_OK);
	assert_int_equal(add_grade(&gradebook, "Student", "Four", "hw1", 50), RETURN_OK);
	assert_int_equal(add_grade(&gradebook, "Student", "Five", "hw1", 80), RETURN_OK);

	will_return_count(__wrap_printf, 3, 5);
	expect_string(__wrap_printf, arg2_c, "Five");
	expect_string(__wrap_printf, arg3_c, "Student");
	expect_value(__wrap_printf, arg4_i, 80);
	expect_string(__wrap_printf, arg2_c, "Four");
	expect_string(__wrap_printf, arg3_c, "Student");
	expect_value(__wrap_printf, arg4_i, 50);
	expect_string(__wrap_printf, arg2_c, "Heide");
	expect_string(__wrap_printf, arg3_c, "John");
	expect_value(__wrap_printf, arg4_i, 75);
	expect_string(__wrap_printf, arg2_c, "Three");
	expect_string(__wrap_printf, arg3_c, "Student");
	expect_value(__wrap_printf, arg4_i, 60);
	expect_string(__wrap_printf, arg2_c, "Two");
	expect_string(__wrap_printf, arg3_c, "Student");
	expect_value(__wrap_printf, arg4_i, 160);
	assert_int_equal(print_assignment(&gradebook, "hw1", ORDER_ALPHABET), RETURN_OK);

	will_return_count(__wrap_printf, 3, 5);
	expect_string(__wrap_printf, arg2_c, "Two");
	expect_string(__wrap_printf, arg3_c, "Student");
	expect_value(__wrap_printf, arg4_i, 160);
	expect_string(__wrap_printf, arg2_c, "Five");
	expect_string(__wrap_printf, arg3_c, "Student");
	expect_value(__wrap_printf, arg4_i, 80);
	expect_string(__wrap_printf, arg2_c, "Heide");
	expect_string(__wrap_printf, arg3_c, "John");
	expect_value(__wrap_printf, arg4_i, 75);
	expect_string(__wrap_printf, arg2_c, "Three");
	expect_string(__wrap_printf, arg3_c, "Student");
	expect_value(__wrap_printf, arg4_i, 60);
	expect_string(__wrap_printf, arg2_c, "Four");
	expect_string(__wrap_printf, arg3_c, "Student");
	expect_value(__wrap_printf, arg4_i, 50);
	assert_int_equal(print_assignment(&gradebook, "hw1", ORDER_GRADE), RETURN_OK);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_gradebook_init),
		cmocka_unit_test(test_add_assignment),
		cmocka_unit_test(test_add_assignment_max_weight),
		cmocka_unit_test(test_add_assignment_bad_name),
		cmocka_unit_test(test_add_assignment_full),
		cmocka_unit_test(test_add_assignment_invalid_numbers),
		cmocka_unit_test(test_add_assignment_name_too_long),
		cmocka_unit_test(test_add_assignment_duplicate),
		cmocka_unit_test(test_add_assignment_weight_overflow),
		cmocka_unit_test(test_add_student),
		cmocka_unit_test(test_add_student_duplicate),
		cmocka_unit_test(test_add_student_bad_name),
		cmocka_unit_test(test_add_student_full),
		cmocka_unit_test(test_add_student_name_too_long),
		cmocka_unit_test(test_delete_assignment),
		cmocka_unit_test(test_delete_assignment_multiple_students),
		cmocka_unit_test(test_delete_assignment_name_too_long),
		cmocka_unit_test(test_delete_assignment_error),
		cmocka_unit_test(test_delete_student),
		cmocka_unit_test(test_delete_student_multiple),
		cmocka_unit_test(test_delete_student_name_too_long),
		cmocka_unit_test(test_delete_student_error),
		cmocka_unit_test(test_add_grade),
		cmocka_unit_test(test_add_grade_twice),
		cmocka_unit_test(test_add_grade_error),
		cmocka_unit_test(test_add_grade_name_too_long),
		cmocka_unit_test(gradebookadd_integration_test),
		cmocka_unit_test(test_parse_key),
		cmocka_unit_test(test_print_student),
		cmocka_unit_test(test_print_assignment),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
