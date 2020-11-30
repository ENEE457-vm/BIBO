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

/* Stores gradebook data converted from command line */
typedef struct {
	ActionType action;
	char assignment_name[MAX_BUFFER_LEN];
	int assignment_points;
	float assignment_weight;
	char firstname[MAX_BUFFER_LEN];
	char lastname[MAX_BUFFER_LEN];
	int grade;
} GradebookData;

/* Stores raw command line arguments */
typedef struct {
	char filename[MAX_BUFFER_LEN];
	ActionType action;
	char assignment_name[MAX_BUFFER_LEN];
	char assignment_points[MAX_BUFFER_LEN];
	char assignment_weight[MAX_BUFFER_LEN];
	char firstname[MAX_BUFFER_LEN];
	char lastname[MAX_BUFFER_LEN];
	char grade[MAX_BUFFER_LEN];
} CmdLineData;

/* Parses command line and returns raw data */
CmdLineData parse_cmdline(int argc, char *argv[]) {
	CmdLineData C = {0};
	int i = 0, j = 0, k = 0;
	int assignment_name_flag = 0, first_name_flag = 0, last_name_flag = 0,
	assignment_points_flag = 0, assignment_weight_flag = 0, grade_flag = 0;

	#ifdef DEBUG
	printf("Number of args passed: %d\n", argc);
	printf("----Command line args passed----\n");
	for (int counter = 0; counter < argc; counter++) {
	  printf("argv[%d]: %s\n", counter, argv[counter]);
	}
	#endif
	// blacklisting invalid argument count
  if (argc < 8) {
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
	for (i=6; i<argc; i+=2) {
		if (strnlen(argv[i], 4) >= 4 || strnlen(argv[i], 4) <= 1) {
			#ifdef DEBUG
			printf("Invalid flag lengths for inputs 6+\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
		}
	}
	for (i=7; i<argc; i+=2) {
		if (strnlen(argv[i], MAX_USER_INPUT_LEN + 1) >= MAX_USER_INPUT_LEN + 1) {
			#ifdef DEBUG
			printf("Invalid argument lengths for inputs 7+\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
		}
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

  // (3) exactly one of {-AA, -DA, -AS, -DS, -AG} must be specified
  if (!strncmp(argv[5], "-AA", 4) || !strncmp(argv[5], "-DA", 4) ||
  		!strncmp(argv[5], "-AS", 4) || !strncmp(argv[5], "-DS", 4) ||
  		!strncmp(argv[5], "-AG", 4)) {
		// (4) after action option, other arguments may be specified in any order
  	if (!strncmp(argv[5], "-AA", 4)) { /* BEGINNING OF ADD ASSIGNMENT */
  		for (i=6; i<argc; i+=2) {
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
				}
				// assignment points
				else if (!strncmp(argv[i], "-P", 3)) {
					// checking that assignment points is a number
					for (j=0; j < strnlen(argv[i+1], MAX_USER_INPUT_LEN); j++) {
						if (!(isdigit(argv[i+1][j]))) {
							#ifdef DEBUG
							printf("Invalid assignment points argument\n");
							#endif
							printf("invalid\n");
							exit(INVALID);
						}
					}
					assignment_points_flag++;
					strncpy(C.assignment_points, argv[i+1], MAX_USER_INPUT_LEN);
				}
				// assignment weight
				else if (!strncmp(argv[i], "-W", 3)) {
					k=0;
					// checking that assignment weight is a float
					for (j=0; j < strnlen(argv[i+1], MAX_USER_INPUT_LEN); j++) {
						if (!(isdigit(argv[i+1][j]) || argv[i+1][j] == '.')) {
							#ifdef DEBUG
							printf("Invalid assignment weight argument\n");
							#endif
							printf("invalid\n");
							exit(INVALID);
						}
						if (argv[i+1][j] == '.') {
							k++;
						}
					}
					if (k > 1) { // not a valid float
						#ifdef DEBUG
						printf("Invalid assignment weight argument\n");
						#endif
						printf("invalid\n");
						exit(INVALID);
					}
					assignment_weight_flag++;
					strncpy(C.assignment_weight, argv[i+1], MAX_USER_INPUT_LEN);
				}
				else { // invalid argument
					#ifdef DEBUG
					printf("Invalid argument\n");
					#endif
					printf("invalid\n");
					exit(INVALID);
				}
  		}
  		if (assignment_name_flag < 1 &&
  				assignment_points_flag < 1 &&
  				assignment_weight_flag < 1) {
				#ifdef DEBUG
				printf("Not enough options specified\n");
				#endif
				printf("invalid\n");
				exit(INVALID);
			}
  		C.action = add_assignment; /* END OF ADD ASSIGNMENT */
  	}

  	else if (!strncmp(argv[5], "-DA", 4)) { /* BEGINNING OF DELETE ASSIGNMENT */
  		for (i=6; i<argc; i+=2) {
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
				}
				else { // invalid argument
					#ifdef DEBUG
					printf("Invalid argument\n");
					#endif
					printf("invalid\n");
					exit(INVALID);
				}
			}
			if (assignment_name_flag < 1) {
				#ifdef DEBUG
				printf("Assignment name not specified\n");
				#endif
				printf("invalid\n");
				exit(INVALID);
			}
  		C.action = delete_assignment; /* END OF DELETE ASSIGNMENT */
  	}

  	else if(!strncmp(argv[5], "-AS", 4)) { /* BEGINNING OF ADD STUDENT */
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
  		C.action = add_student; /* END OF ADD STUDENT */
  	}

  	else if(!strncmp(argv[5], "-DS", 4)) { /* BEGINNING OF DELETE STUDENT */
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
  		C.action = delete_student; /* END OF DELETE STUDENT */
  	}

		else { /* BEGINNING OF ADD GRADE */
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
				// assignment name
				else if (!strncmp(argv[i], "-AN", 4)) {
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
				}
				// grade
				else if (!strncmp(argv[i], "-G", 3)) {
					// checking that grade is a number
					for (j=0; j < strnlen(argv[i+1], MAX_USER_INPUT_LEN); j++) {
						if (!(isdigit(argv[i+1][j]))) {
							#ifdef DEBUG
							printf("Invalid assignment points argument\n");
							#endif
							printf("invalid\n");
							exit(INVALID);
						}
					}
					grade_flag++;
					strncpy(C.grade, argv[i+1], MAX_USER_INPUT_LEN);
				}
				else { // invalid argument
					#ifdef DEBUG
					printf("Invalid argument\n");
					#endif
					printf("invalid\n");
					exit(INVALID);
				}
			}
			if (first_name_flag < 1 && last_name_flag < 1 &&
					assignment_name_flag < 1 && grade_flag < 1) {
				#ifdef DEBUG
				printf("Not enough options specified\n");
				#endif
				printf("invalid\n");
				exit(INVALID);
			}
  		C.action = add_grade; /* END OF ADD GRADE */
		}
  } else { // invalid flag
  	#ifdef DEBUG
		printf("Invalid flag\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
  }
  return C;
}

GradebookData convert_cmdlineargs(const CmdLineData C) {
	GradebookData G = {0};

	G.action = C.action;

	strncpy(G.assignment_name, C.assignment_name, MAX_USER_INPUT_LEN);

	if (strnlen(C.assignment_points, MAX_INT_LEN + 1) >= MAX_INT_LEN + 1) {
		#ifdef DEBUG
		printf("Assignment points go beyond limit\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}
	G.assignment_points = atoi(C.assignment_points);
	if (G.action == add_assignment && G.assignment_points == 0) {
		#ifdef DEBUG
		printf("Assignment points must be greater than 0\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}
	G.assignment_weight = atof(C.assignment_weight);
	if (G.action == add_assignment && G.assignment_weight > 1.00) {
		#ifdef DEBUG
		printf("Assignment weight must be in [0,1]\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}

	strncpy(G.firstname, C.firstname, MAX_USER_INPUT_LEN);

	strncpy(G.lastname, C.lastname, MAX_USER_INPUT_LEN);

	if (strnlen(C.grade, MAX_INT_LEN + 1) >= MAX_INT_LEN + 1) {
		#ifdef DEBUG
		printf("Grade goes beyond limit\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}
	G.grade = atoi(C.grade);

	return G;
}

void modify(GradebookData G, DecryptedGradebook *gradebook) {
	int i = 0, j = 0, k = 0;
	float weight = 0.0;

	if (G.action == add_assignment) { /* BEGINNING OF ADD ASSIGNMENT */
		if (gradebook->num_assignments >= MAX_ASSIGNMENTS) {
			#ifdef DEBUG
			printf("Gradebook full\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
		}
		weight = G.assignment_weight;
		int index = -1;
		for (i = 0; i < MAX_ASSIGNMENTS; i++) {
			// checking for pre-existing assignment name
			if (!strncmp(G.assignment_name, gradebook->assignments[i].name, MAX_USER_INPUT_LEN)) {
				#ifdef DEBUG
				printf("Assignment already exists\n");
				#endif
				printf("invalid\n");
				exit(INVALID);
			}
			// finding index in which we place the assignment into
			if (index == -1 && gradebook->assignment_slot_filled[i] == false) {
				index = i;
			}
			// calculating total weight of all assignments
			weight += gradebook->assignments[i].weight;
			if (weight > 1.00) {
				#ifdef DEBUG
				printf("Total weight will exceed 1\n");
				#endif
				printf("invalid\n");
				exit(INVALID);
			}
		}
		// add assignment to gradebook
		gradebook->num_assignments++;
		gradebook->assignment_slot_filled[index] = true;
		strncpy(gradebook->assignments[index].name, G.assignment_name, MAX_USER_INPUT_LEN);
		gradebook->assignments[index].points = G.assignment_points;
		gradebook->assignments[index].weight = G.assignment_weight;
	} /* END OF ADD ASSIGNMENT */

	else if (G.action == delete_assignment) { /* BEGINNING OF DELETE ASSIGNMENT */
		if (gradebook->num_assignments <= 0) {
			#ifdef DEBUG
			printf("Gradebook doesn't have any assignments\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
		}
		int index = -1;
		for (i = 0; i < MAX_ASSIGNMENTS; i++) {
			// searching for assignment to be deleted
			if (!strncmp(G.assignment_name, gradebook->assignments[i].name, MAX_USER_INPUT_LEN)) {
				index = i;
			}
		}
		if (index == -1) {
			#ifdef DEBUG
			printf("Assignment doesn't exist\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
		}
		// delete assignment from gradebook
		gradebook->num_assignments--;
		gradebook->assignment_slot_filled[index] = false;
		memset(gradebook->assignments[index].name, '\0', MAX_USER_INPUT_LEN);
		gradebook->assignments[index].points = 0;
		gradebook->assignments[index].weight = 0.0;
		// clear student grades for deleted assignment
		for (i = 0; i < MAX_STUDENTS; i++) {
			gradebook->students[i].grades[index] = 0;
		}
	} /* END OF DELETE ASSIGNMENT */

	else if (G.action == add_student) { /* BEGINNING OF ADD STUDENT */
		if (gradebook->num_students >= MAX_STUDENTS) {
			#ifdef DEBUG
			printf("Gradebook full\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
		}

		int index = -1;
		for (i = 0; i < MAX_STUDENTS; i++) {
			// checking for pre-existing student name
			if (!(strncmp(G.firstname, gradebook->students[i].firstname, MAX_USER_INPUT_LEN)) &&
					!(strncmp(G.lastname, gradebook->students[i].lastname, MAX_USER_INPUT_LEN))) {
				#ifdef DEBUG
				printf("Student already exists\n");
				#endif
				printf("invalid\n");
				exit(INVALID);
			}
			// finding index in which we place the student into
			if (index == -1 && gradebook->student_slot_filled[i] == false) {
				index = i;
			}
		}
		// add student to gradebook
		gradebook->num_students++;
		gradebook->student_slot_filled[index] = true;
		strncpy(gradebook->students[index].firstname, G.firstname, MAX_USER_INPUT_LEN);
		strncpy(gradebook->students[index].lastname, G.lastname, MAX_USER_INPUT_LEN);
		// initialize student grades
		for (i = 0; i < MAX_ASSIGNMENTS; i++) {
			gradebook->students[index].grades[i] = 0;
		}
	} /* END OF ADD STUDENT */

	else if (G.action == delete_student) { /* BEGINNING OF DELETE STUDENT */
		if (gradebook->num_students <= 0) {
			#ifdef DEBUG
			printf("Gradebook doesn't have any students\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
		}
		int index = -1;
		for (i = 0; i < MAX_STUDENTS; i++) {
			// searching for student to be deleted
			if (!(strncmp(G.firstname, gradebook->students[i].firstname, MAX_USER_INPUT_LEN)) &&
					!(strncmp(G.lastname, gradebook->students[i].lastname, MAX_USER_INPUT_LEN))) {
				index = i;
			}
		}
		if (index == -1) {
			#ifdef DEBUG
			printf("Student doesn't exist\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
		}

		// delete student from gradebook
		gradebook->num_students--;
		gradebook->student_slot_filled[index] = false;
		memset(gradebook->students[index].firstname, '\0', MAX_USER_INPUT_LEN);
		memset(gradebook->students[index].lastname, '\0', MAX_USER_INPUT_LEN);
		// clear student grades
		for (i = 0; i < MAX_STUDENTS; i++) {
			gradebook->students[index].grades[i] = 0;
		}
	} /* END OF DELETE STUDENT */

	else if (G.action == add_grade) { /* BEGINNING OF ADD GRADE */
		// checking student existence
		if (gradebook->num_students <= 0) {
			#ifdef DEBUG
			printf("Gradebook doesn't have any students\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
		}
		int index = -1;
		for (i = 0; i < MAX_STUDENTS; i++) {
			// searching for student
			if (!(strncmp(G.firstname, gradebook->students[i].firstname, MAX_USER_INPUT_LEN)) &&
					!(strncmp(G.lastname, gradebook->students[i].lastname, MAX_USER_INPUT_LEN))) {
				index = i;
			}
		}
		if (index == -1) {
			#ifdef DEBUG
			printf("Student doesn't exist\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
		}
		// checking assignment existence
		if (gradebook->num_assignments <= 0) {
			#ifdef DEBUG
			printf("Gradebook doesn't have any assignments\n");
			#endif
			printf("invalid\n");
			exit(INVALID);
		}
		int index2 = -1;
		for (i = 0; i < MAX_ASSIGNMENTS; i++) {
			// searching for assignment
			if (!strncmp(G.assignment_name, gradebook->assignments[i].name, MAX_USER_INPUT_LEN)) {
				index2 = i;
			}
		}
		// add grade to gradebook
		gradebook->students[index].grades[index2] = G.grade;
	} /* END OF ADD GRADE */
	else { // invalid action
		#ifdef DEBUG
		printf("Could not process action\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}
}

int main(int argc, char *argv[]) {
	CmdLineData C = {0};
	GradebookData G = {0};
	DecryptedGradebook dec_gradebook = {0};
	EncryptedGradebook enc_gradebook = {0};
	FILE *infile, *outfile;
	unsigned char key[KEY_SIZE];
	unsigned char iv[IV_SIZE];
	unsigned char tag[MAC_SIZE];
	unsigned char mac_tag[MAC_SIZE];
	unsigned char input_key[33] = {0};
	int ret, i;

	// parse command line
	C = parse_cmdline(argc, argv);

	// validating input key
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

	// store raw data into useful data structures
  G = convert_cmdlineargs(C);

	// open encrypted gradebook
	infile = fopen(C.filename, "r");
	if (infile == NULL) {
		#ifdef DEBUG
		printf("Could not open gradebook\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	} else {
		ret = fread(enc_gradebook.iv, 1, sizeof(EncryptedGradebook), infile);
		fflush(infile);
		fclose(infile);

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

	// modify decrypted gradebook
	modify(G, &dec_gradebook);

	// generate random IV
	ret = RAND_bytes(iv, sizeof(iv));
	if (ret == -1) {
		#ifdef DEBUG
		printf("Random IV generation failed\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}

	// encrypt
	memcpy(enc_gradebook.iv, iv, sizeof(iv));
	ret = encrypt((unsigned char *)&dec_gradebook.num_assignments, sizeof(DecryptedGradebook), key, iv, (unsigned char *)&enc_gradebook.encrypted_data);
	if (ret == -1) {
		#ifdef DEBUG
		printf("Encryption failed\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}

	// compute the message authentication code
	unsigned char* mac_res;
	mac_res = HMAC(EVP_md5(), key, sizeof(key), (unsigned char *)&enc_gradebook.iv, sizeof(EncryptedGradebookSize), tag, NULL);

	// store tag in gradebook
	memcpy(enc_gradebook.tag, tag, sizeof(tag));

	// open encrypted gradebook
	outfile = fopen(C.filename, "w");
	if (outfile == NULL) {
		#ifdef DEBUG
		printf("Could not create file\n");
		#endif
		printf("invalid\n");
		exit(INVALID);
	}

	// write encrypted gradebook to file
	fwrite(enc_gradebook.iv, 1, sizeof(EncryptedGradebook), outfile);
	fflush(outfile);
	fclose(outfile);
  	return 0;
}
