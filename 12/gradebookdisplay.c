#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>
#include <ctype.h>
#include <stdbool.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "data.h"

#define DEBUG

/* Stores raw command line arguments */
typedef struct {
	char filename[MAX_BUFFER_LEN];
	PrintAction action;
	char assignment_name[MAX_BUFFER_LEN];
	char firstname[MAX_BUFFER_LEN];
	char lastname[MAX_BUFFER_LEN];
	PrintOrder order;
} CmdLineData_Print;

/* Parses command line and returns raw data */
CmdLineData_Print parse_commands(int argc, char *argv[]) {
	CmdLineData_Print C = {0};
	int i = 0, j = 0;
	int assignment_name_flag = 0, alphabet_order_flag = 0, grade_order_flag = 0,
	first_name_flag = 0, last_name_flag = 0;

  #ifdef DEBUG
	printf("Number of args passed: %d\n", argc);
	printf("----Command line args passed----\n");
	for (int counter = 0; counter < argc; counter++) {
	  printf("argv[%d]: %s\n", counter, argv[counter]);
	}
	#endif
	// blacklisting invalid argument count
  if (argc < 7) {
		#ifdef DEBUG
    printf("Not enough arguments.\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}
  if (argc > MAX_CMD_LINE_ARGS) {
		#ifdef DEBUG
		printf("Too many arguments.\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}

	// blacklisting invalid string lengths
	if ((strnlen(argv[0], MAX_USER_INPUT_LEN + 1) >= MAX_USER_INPUT_LEN + 1) || (strnlen(argv[1], 3) != 2) ||
			(strnlen(argv[2], MAX_USER_INPUT_LEN + 1) >= MAX_USER_INPUT_LEN + 1) || (strnlen(argv[3], 3) != 2) ||
			(strnlen(argv[4], KEY_SIZE * 2 + 1) != KEY_SIZE * 2) || (strnlen(argv[5], 4) != 3)) {
		#ifdef DEBUG
		printf("Invalid argument lengths for inputs 0-5\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}

	// validating each input
	// (1) gradebook name specified with -N
  if (strncmp(argv[1], "-N", 3)) {
  	#ifdef DEBUG
		printf("Second argument must be -N\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
  }
  // checking that gradebook name is valid
  for (i=0; i < strnlen(argv[2], MAX_USER_INPUT_LEN); i++) {
  	if (!(isalnum(argv[2][i]) || argv[2][i] == '.' || argv[2][i] == '_')) {
  		#ifdef DEBUG
			printf("Invalid filename\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
  	}
  	// checking that gradebook exists
  	if (access(argv[2], F_OK) == -1) {
			#ifdef DEBUG
			printf("File does not exist\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
		}
  }
	// (2) key is specified with -K
	if (strncmp(argv[3], "-K", 3)) {
		#ifdef DEBUG
		printf("Fourth argument must be -K\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}
	// checking that key is a string of hex digits
  for (i=0; i < strnlen(argv[4], KEY_SIZE * 2); i++) {
  	if(!isxdigit(argv[4][i])) {
  		#ifdef DEBUG
			printf("Invalid key\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
  	}
  }

	// saving command line arguments to data structure
  strncpy(C.filename, argv[2], MAX_USER_INPUT_LEN);

  // (3) exactly one of {-PA, -PS, -PF} must be specified
  if (!strncmp(argv[5], "-PA", 4) || !strncmp(argv[5], "-PS", 4) ||
  		!strncmp(argv[5], "-PF", 4)) {
  	// (4) after action option, other arguments may be specified in any order
  	if (!strncmp(argv[5], "-PA", 4)) { /* BEGINNING OF PRINT ASSIGNMENT */
  		for (i=6; i<argc; i++) {
  			// assignment name
				if (!strncmp(argv[i], "-AN", 4)) {
					// checking that assignment name is alphanumeric
					for (j=0; j < strnlen(argv[i+1], MAX_USER_INPUT_LEN); j++) {
						if (!(isalnum(argv[i+1][j]))) {
							#ifdef DEBUG
							printf("Invalid assignment name argument\n");
							#endif
							printf("invalid\n");
							exit(INVALID);
						}
					}
					assignment_name_flag++;
					strncpy(C.assignment_name, argv[i+1], MAX_USER_INPUT_LEN);
					i++; // increment to next flag
				}
				// alphabetical-order
				else if (!strncmp(argv[i], "-A", 3)) {
					alphabet_order_flag++;
				}
				// grade-order
				else if (!strncmp(argv[i], "-G", 3)) {
					grade_order_flag++;
				}
				else { // invalid argument
					#ifdef DEBUG
					printf("Invalid argument\n");
					#endif
					printf("invalid\n");
					exit(INVALID);
				}
			}
			if (alphabet_order_flag > 0 && grade_order_flag > 0) {
				#ifdef DEBUG
				printf("Can only specify one order\n");
				#endif
				printf("invalid\n");
				exit(INVALID);
			}
			else if (assignment_name_flag > 0 && alphabet_order_flag > 0) {
				C.order = alphabetical;
			}
			else if (assignment_name_flag > 0 && grade_order_flag > 0) {
				C.order = grade;
			}
			else {
				#ifdef DEBUG
				printf("Not enough options specified\n");
				#endif
				printf("invalid\n");
				exit(INVALID);
			}
			C.action = print_assignment; /* END OF PRINT ASSIGNMENT */
		}

		else if (!strncmp(argv[5], "-PS", 4)) { /* BEGINNING OF PRINT STUDENT */
			for (i=6; i<argc; i+=2) {
  			// first name
				if (!strncmp(argv[i], "-FN", 4)) {
					// checking that first name is alpha characters
					for (j=0; j < strnlen(argv[i+1], MAX_USER_INPUT_LEN); j++) {
						if (!(isalpha(argv[i+1][j]))) {
							#ifdef DEBUG
							printf("Invalid first name argument\n");
							#endif
							printf("invalid\n");
							exit(INVALID);
						}
					}
					first_name_flag++;
					strncpy(C.firstname, argv[i+1], MAX_USER_INPUT_LEN);
				}
				// last name
				else if (!strncmp(argv[i], "-LN", 4)) {
					// checking that last name is alpha characters
					for (j=0; j < strnlen(argv[i+1], MAX_USER_INPUT_LEN); j++) {
						if (!(isalpha(argv[i+1][j]))) {
							#ifdef DEBUG
							printf("Invalid last name argument\n");
							#endif
							printf("invalid\n");
							exit(INVALID);
						}
					}
					last_name_flag++;
					strncpy(C.lastname, argv[i+1], MAX_USER_INPUT_LEN);
				}
				else { // invalid argument
					#ifdef DEBUG
					printf("Invalid argument\n");
					#endif
					printf("invalid\n");
					exit(INVALID);
				}
			}
			if (first_name_flag < 1 && last_name_flag < 1) {
				#ifdef DEBUG
				printf("Not enough options specified\n");
				#endif
				printf("invalid\n");
				exit(INVALID);
			}
			C.action = print_student; /* END OF PRINT STUDENT */
		}

		else if (!strncmp(argv[5], "-PF", 4)) { /* BEGINNING OF PRINT FINAL */
			for (i=6; i<argc; i++) {
				// alphabetical-order
				if (!strncmp(argv[i], "-A", 3)) {
					alphabet_order_flag++;
				}
				// grade-order
				else if (!strncmp(argv[i], "-G", 3)) {
					grade_order_flag++;
				}
				else { // invalid argument
					#ifdef DEBUG
					printf("Invalid argument\n");
					#endif
					printf("invalid\n");
					exit(INVALID);
				}
			}
			if (alphabet_order_flag > 0 && grade_order_flag > 0) {
				#ifdef DEBUG
				printf("Can only specify one order\n");
				#endif
				printf("invalid\n");
				exit(INVALID);
			}
			else if (grade_order_flag < 1 && alphabet_order_flag > 0) {
				C.order = alphabetical;
			}
			else if (alphabet_order_flag < 1 && grade_order_flag > 0) {
				C.order = grade;
			}
			else {
				#ifdef DEBUG
				printf("Not enough options specified\n");
				#endif
				printf("invalid\n");
				exit(INVALID);
			}
			C.action = print_final; /* END OF PRINT FINAL */
		}
    else { // invalid flag
			#ifdef DEBUG
			printf("Invalid flag\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
  	}
  }
  return C;
}

void print_assignment_alphabetical(char *assignment, DecryptedGradebook *gradebook) {
	int i, j, k, pos, ret;
	bool found = false;
	char temp_lastname[MAX_USER_INPUT_LEN], temp_firstname[MAX_USER_INPUT_LEN];
	int temp_grades[MAX_ASSIGNMENTS];

	for (i = 0; i < MAX_ASSIGNMENTS; i++) {
		if (!strncmp(assignment, gradebook->assignments[i].name, MAX_USER_INPUT_LEN)) {
			found = true;
			pos = i;
		}
	}
	if (found == false) {
		#ifdef DEBUG
		printf("Assignment doesn't exist\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}

	// sort by lastname first
	for (i = 0; i < gradebook->num_students; i++) {
		for (j = i+1; j < gradebook->num_students; j++) {
			ret = strncmp(gradebook->students[i].lastname, gradebook->students[j].lastname, MAX_USER_INPUT_LEN);
			if(ret > 0) {
				strncpy(temp_lastname, gradebook->students[i].lastname, MAX_USER_INPUT_LEN);
				strncpy(gradebook->students[i].lastname, gradebook->students[j].lastname, MAX_USER_INPUT_LEN);
				strncpy(gradebook->students[j].lastname, temp_lastname, MAX_USER_INPUT_LEN);

				strncpy(temp_firstname, gradebook->students[i].firstname, MAX_USER_INPUT_LEN);
				strncpy(gradebook->students[i].firstname, gradebook->students[j].firstname, MAX_USER_INPUT_LEN);
				strncpy(gradebook->students[j].firstname, temp_firstname, MAX_USER_INPUT_LEN);

				for (k = 0; k < MAX_ASSIGNMENTS; k++) {
					temp_grades[k] = gradebook->students[i].grades[k];
				}
				for (k = 0; k < MAX_ASSIGNMENTS; k++) {
					gradebook->students[i].grades[k] = gradebook->students[j].grades[k];
				}
				for (k = 0; k < MAX_ASSIGNMENTS; k++) {
					gradebook->students[j].grades[k] = temp_grades[k];
				}
			}
			// then sort by firstname if lastnames are the same
			else if (ret == 0) {
				if (strncmp(gradebook->students[i].firstname, gradebook->students[j].firstname, MAX_USER_INPUT_LEN) > 0) {
					strncpy(temp_firstname, gradebook->students[i].firstname, MAX_USER_INPUT_LEN);
					strncpy(gradebook->students[i].firstname, gradebook->students[j].firstname, MAX_USER_INPUT_LEN);
					strncpy(gradebook->students[j].firstname, temp_firstname, MAX_USER_INPUT_LEN);

					for (k = 0; k < MAX_ASSIGNMENTS; k++) {
						temp_grades[k] = gradebook->students[i].grades[k];
					}
					for (k = 0; k < MAX_ASSIGNMENTS; k++) {
						gradebook->students[i].grades[k] = gradebook->students[j].grades[k];
					}
					for (k = 0; k < MAX_ASSIGNMENTS; k++) {
						gradebook->students[j].grades[k] = temp_grades[k];
					}
				}
			} else {
				// do nothing
			}
		}
	}
	// print assignment grades for sorted students
	for (i = 0; i < gradebook->num_students; i++) {
		if (gradebook->student_slot_filled[i] == true) {
			printf("(%s, %s, %d)\n", gradebook->students[i].lastname, gradebook->students[i].firstname, gradebook->students[i].grades[pos]);
		}
	}
	return;
}

void print_assignment_grade(char *assignment, DecryptedGradebook *gradebook) {
	int i, j, k, pos, ret;
	bool found = false;
	char temp_lastname[MAX_USER_INPUT_LEN], temp_firstname[MAX_USER_INPUT_LEN];
	int temp_grades[MAX_ASSIGNMENTS];

	// find position of assignment
	for (i = 0; i < MAX_ASSIGNMENTS; i++) {
		if (!strncmp(assignment, gradebook->assignments[i].name, MAX_USER_INPUT_LEN)) {
			found = true;
			pos = i;
		}
	}
	if (found == false) {
		#ifdef DEBUG
		printf("Assignment doesn't exist\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}

	// sort by grades (highest to lowest)
	for (i = 0; i < gradebook->num_students; i++) {
		for (j = i+1; j < gradebook->num_students; j++) {
			if (gradebook->students[i].grades[pos] < gradebook->students[j].grades[pos]) {
				strncpy(temp_lastname, gradebook->students[i].lastname, MAX_USER_INPUT_LEN);
				strncpy(gradebook->students[i].lastname, gradebook->students[j].lastname, MAX_USER_INPUT_LEN);
				strncpy(gradebook->students[j].lastname, temp_lastname, MAX_USER_INPUT_LEN);

				strncpy(temp_firstname, gradebook->students[i].firstname, MAX_USER_INPUT_LEN);
				strncpy(gradebook->students[i].firstname, gradebook->students[j].firstname, MAX_USER_INPUT_LEN);
				strncpy(gradebook->students[j].firstname, temp_firstname, MAX_USER_INPUT_LEN);

				for (k = 0; k < MAX_ASSIGNMENTS; k++) {
					temp_grades[k] = gradebook->students[i].grades[k];
				}
				for (k = 0; k < MAX_ASSIGNMENTS; k++) {
					gradebook->students[i].grades[k] = gradebook->students[j].grades[k];
				}
				for (k = 0; k < MAX_ASSIGNMENTS; k++) {
					gradebook->students[j].grades[k] = temp_grades[k];
				}
			}
		}
	}

	// print assignment grades for sorted students
	for (i = 0; i < gradebook->num_students; i++) {
		if (gradebook->student_slot_filled[i] == true) {
			printf("(%s, %s, %d)\n", gradebook->students[i].lastname, gradebook->students[i].firstname, gradebook->students[i].grades[pos]);
		}
	}
	return;
}

void print_student_grades(char *firstname, char *lastname, DecryptedGradebook *gradebook) {
	int i, pos;
	bool found = false;

	// find position of student
	for (i = 0; i < MAX_STUDENTS; i++) {
		if (!strncmp(firstname, gradebook->students[i].firstname, MAX_USER_INPUT_LEN) &&
				!strncmp(lastname, gradebook->students[i].lastname, MAX_USER_INPUT_LEN)) {
			found = true;
			pos = i;
		}
	}
	if (found == false) {
		#ifdef DEBUG
		printf("Student doesn't exist\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}

	// print assignment names and grades for specified student
	for (i = 0; i < gradebook->num_assignments; i++) {
		printf("(%s, %d)\n", gradebook->assignments[i].name, gradebook->students[pos].grades[i]);
	}
	return;
}


float calculate_final_grade(int *grades, int num_assignments, Assignment *assignments) {
	int j;
	float final_grade = 0.0;

	for (j = 0; j < num_assignments; j++) {
		final_grade += (((float)grades[j] / (float)assignments[j].points) * assignments[j].weight);
	}
	return final_grade;
}

void print_final_alphabetical(DecryptedGradebook *gradebook) {
	int i, j, k, ret;
	float final_grade = 0.0;
	char temp_lastname[MAX_USER_INPUT_LEN], temp_firstname[MAX_USER_INPUT_LEN];
	int temp_grades[MAX_ASSIGNMENTS];

	// sort by lastname first
	for (i = 0; i < gradebook->num_students; i++) {
		for (j = i+1; j < gradebook->num_students; j++) {
			ret = strncmp(gradebook->students[i].lastname, gradebook->students[j].lastname, MAX_USER_INPUT_LEN);
			if(ret > 0) {
				strncpy(temp_lastname, gradebook->students[i].lastname, MAX_USER_INPUT_LEN);
				strncpy(gradebook->students[i].lastname, gradebook->students[j].lastname, MAX_USER_INPUT_LEN);
				strncpy(gradebook->students[j].lastname, temp_lastname, MAX_USER_INPUT_LEN);

				strncpy(temp_firstname, gradebook->students[i].firstname, MAX_USER_INPUT_LEN);
				strncpy(gradebook->students[i].firstname, gradebook->students[j].firstname, MAX_USER_INPUT_LEN);
				strncpy(gradebook->students[j].firstname, temp_firstname, MAX_USER_INPUT_LEN);

				for (k = 0; k < MAX_ASSIGNMENTS; k++) {
					temp_grades[k] = gradebook->students[i].grades[k];
				}
				for (k = 0; k < MAX_ASSIGNMENTS; k++) {
					gradebook->students[i].grades[k] = gradebook->students[j].grades[k];
				}
				for (k = 0; k < MAX_ASSIGNMENTS; k++) {
					gradebook->students[j].grades[k] = temp_grades[k];
				}
			}
			// then sort by firstname if lastnames are the same
			else if (ret == 0) {
				if (strncmp(gradebook->students[i].firstname, gradebook->students[j].firstname, MAX_USER_INPUT_LEN) > 0) {
					strncpy(temp_firstname, gradebook->students[i].firstname, MAX_USER_INPUT_LEN);
					strncpy(gradebook->students[i].firstname, gradebook->students[j].firstname, MAX_USER_INPUT_LEN);
					strncpy(gradebook->students[j].firstname, temp_firstname, MAX_USER_INPUT_LEN);

					for (k = 0; k < MAX_ASSIGNMENTS; k++) {
						temp_grades[k] = gradebook->students[i].grades[k];
					}
					for (k = 0; k < MAX_ASSIGNMENTS; k++) {
						gradebook->students[i].grades[k] = gradebook->students[j].grades[k];
					}
					for (k = 0; k < MAX_ASSIGNMENTS; k++) {
						gradebook->students[j].grades[k] = temp_grades[k];
					}
				}
			} else {
				// do nothing
			}
		}
	}

	// print final assignment grades for sorted students
	for (i = 0; i < gradebook->num_students; i++) {
		if (gradebook->student_slot_filled[i] == true) {
			final_grade = calculate_final_grade(gradebook->students[i].grades, gradebook->num_assignments, gradebook->assignments);
		}
		printf("(%s, %s, %f)\n", gradebook->students[i].lastname, gradebook->students[i].firstname, final_grade);
	}
	return;
}


void print_final_grade(DecryptedGradebook *gradebook) {
	int i, j, k, ret;
	float final_grade = 0.0, temp_grade = 0.0;
	char temp_lastname[MAX_USER_INPUT_LEN], temp_firstname[MAX_USER_INPUT_LEN];
	float final_grades[MAX_ASSIGNMENTS];

	// calculate final assignment grades for each students
	for (i = 0; i < gradebook->num_students; i++) {
		if (gradebook->student_slot_filled[i] == true) {
			final_grade = calculate_final_grade(gradebook->students[i].grades, gradebook->num_assignments, gradebook->assignments);
		}
		final_grades[i] = final_grade;
	}

	// sort by grades (highest to lowest)
	for (i = 0; i < gradebook->num_students; i++) {
		for (j = i+1; j < gradebook->num_students; j++) {
			if (final_grades[i] < final_grades[j]) {
				strncpy(temp_lastname, gradebook->students[i].lastname, MAX_USER_INPUT_LEN);
				strncpy(gradebook->students[i].lastname, gradebook->students[j].lastname, MAX_USER_INPUT_LEN);
				strncpy(gradebook->students[j].lastname, temp_lastname, MAX_USER_INPUT_LEN);

				strncpy(temp_firstname, gradebook->students[i].firstname, MAX_USER_INPUT_LEN);
				strncpy(gradebook->students[i].firstname, gradebook->students[j].firstname, MAX_USER_INPUT_LEN);
				strncpy(gradebook->students[j].firstname, temp_firstname, MAX_USER_INPUT_LEN);

				temp_grade = final_grades[i];
				final_grades[i] = final_grades[j];
				final_grades[j] = temp_grade;
			}
		}

		// print final assignment grades
		for (i = 0; i < gradebook->num_students; i++) {
			if (gradebook->student_slot_filled[i] == true) {
				printf("(%s, %s, %f)\n", gradebook->students[i].lastname, gradebook->students[i].firstname, final_grades[i]);
			}
		}
	}
	return;
}

int main(int argc, char *argv[]) {
	CmdLineData_Print C = {0};
	EncryptedGradebook enc_gradebook = {0};
	DecryptedGradebook dec_gradebook = {0};
	FILE *fp;
	unsigned char key[KEY_SIZE];
	unsigned char iv[IV_SIZE];
	unsigned char tag[MAC_SIZE];
	unsigned char mac_tag[MAC_SIZE];
	unsigned char input_key[33] = {0};
	int ret;

	if (strnlen(argv[4], KEY_SIZE * 2 + 1) == KEY_SIZE * 2) {
		memcpy(input_key, argv[4], KEY_SIZE * 2);
	}
	else {
		#ifdef DEBUG
		printf("Invalid key\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}
	C = parse_commands(argc, argv);

	// open encrypted gradebook
	fp = fopen(C.filename, "r");
	if (fp == NULL) {
		#ifdef DEBUG
		printf("Could not open gradebook\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	} else {
		ret = fread(enc_gradebook.iv, 1, sizeof(EncryptedGradebook), fp);
		fflush(fp);
		fclose(fp);

		if (ret != sizeof(EncryptedGradebook)) {
			#ifdef DEBUG
			printf("Invalid file length\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
		}

		// converting input key from ascii to hex
		unsigned char *hex_key = input_key;
		for (int count = 0; count < KEY_SIZE; count++) {
			sscanf(hex_key, "%2hhx", &key[count]);
			hex_key += 2;
		}

		// get tag from encrypted gradebook
		memcpy(tag, enc_gradebook.tag, sizeof(tag));

		// compute the message authentication code
		unsigned char* mac_res;
		mac_res = HMAC(EVP_md5(), key, sizeof(key), (unsigned char *)&enc_gradebook.iv, sizeof(EncryptedGradebookSize), mac_tag, NULL);

		// compare tags
		if (!strncmp(tag, mac_tag, MAC_SIZE)) {
			// decrypt
			memcpy(iv, enc_gradebook.iv, sizeof(iv));
			ret = decrypt((unsigned char *)&enc_gradebook.encrypted_data, sizeof(DecryptedGradebook), key, iv, (unsigned char *)&dec_gradebook.num_assignments);
			if (ret == -1) {
				#ifdef DEBUG
				printf("Authentication failed\n");
				#endif
				printf("invalid\n");
				exit(INVALID);
			}
		} else {
			#ifdef DEBUG
			printf("Authentication failed\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
		}
	}

	// print gradebook data
	if (C.action == print_assignment && C.order == alphabetical) {
		print_assignment_alphabetical(C.assignment_name, &dec_gradebook);
	}
	else if (C.action == print_assignment && C.order == grade) {
		print_assignment_grade(C.assignment_name, &dec_gradebook);
	}
	else if (C.action == print_student) {
		print_student_grades(C.firstname, C.lastname, &dec_gradebook);
	}
	else if (C.action == print_final && C.order == alphabetical) {
		print_final_alphabetical(&dec_gradebook);
	}
	else if (C.action == print_final && C.order == grade) {
		print_final_grade(&dec_gradebook);
	}
	else { // invalid action
		#ifdef DEBUG
		printf("Could not process action\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}
	return 0;
}
