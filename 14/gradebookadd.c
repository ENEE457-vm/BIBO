#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>
#include <stdbool.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "data.h"

/* FUNCTIONS DECLARATIONS */ 

int addStudent(char *first, char *last, char *filename);
int delStudent(char *first, char *last, char *filename);

float totalAssignmentWeights(char *filename);
int addAssignment(char *name, int totalPoints, float weight, char *filename);
int delAssignment(char *name, char *filename);

int addGrade(char *assiName, char *first, char *last, int grade, char * filename);

int fileContain(char *str, char* filename);


int main(int argc, char *argv[]) {

	// init the params
	char *filename;
	unsigned char *key;
	FILE *fp;

	// parse cmd 
	if (argc < 5) {
		printf("Not enough argument to proceed.\n"
			"Example Usage:\n"
			"gradebookadd -N [gradebookName] -K [key] -AS -FN [firstName] -LN [lastName] --> Add a student\n"
			"gradebookadd -N [gradebookName] -K [key] -DS -FN [firstName] -LN [lastName] --> Del a student\n"
			"gradebookadd -N [gradebookName] -K [key] -AA -AN [assignmentName] -P [totalPoints] -W [weight] --> Add an assignment\n"
			"gradebookadd -N [gradebookName] -K [key] -DA -AN [assignmentName] -P [totalPoints] -W [weight] --> Del an assignment\n"
			"gradebookadd -N [gradebookName] -K [key] -AG -AN [assignmentName] -FN [firstName] -LN [lastName] -G [grade] --> Add a grade\n");

		printf("\nInvalid\n");
		return(255);
	} // if name and key were not provided

	if ( strcmp(argv[1], "-N")==0 && strcmp(argv[3], "-K")==0 ) {

		filename = argv[2];
		key = argv[4];

		/*function to decrypt file here*/



		// for student:
		if ( strcmp(argv[5], "-AS")==0 && strcmp(argv[6], "-FN")==0 && strcmp(argv[8], "-LN")==0 ) {
			// add student
			char *first = argv[7];
			char *last = argv[9];
			if (strlen(first) > MAX_NAME_LENGTH || strlen(last) > MAX_NAME_LENGTH) {
				printf("invaid\n");
				return(255);
			}
			//
			printf("Current mode: adding student of %s, %s, with filename = %s\n", first, last, filename);

			// function to add student here
			addStudent(first, last, filename);
			

		} else if ( strcmp(argv[5], "-DS")==0 && strcmp(argv[6], "-FN")==0 && strcmp(argv[8], "-LN")==0 ) {
			// del student
			char *first = argv[7];
			char *last = argv[9];
			if (strlen(first) > MAX_NAME_LENGTH || strlen(last) > MAX_NAME_LENGTH) {
				printf("invaid\n");
				return(255);
			}
			printf("Current mode: deleting student of %s, %s, with filename = %s\n", first, last, filename);

			// function to del student here
			delStudent(first, last, filename);
		}

		// for assignment:
		else if ( strcmp(argv[5], "-AA")==0 && strcmp(argv[6], "-AN")==0 && strcmp(argv[8], "-P")==0 && strcmp(argv[10], "-W")==0 ) {
			char *assiName = argv[7];
			int totPoint = atoi(argv[9]);
			float weight = atof(argv[11]);

			if (strlen(assiName) > MAX_NAME_LENGTH) {
				printf("invaid\n");
				return(255);
			}
			printf("Current mode: Adding Assignment\n");
			addAssignment(assiName, totPoint, weight, filename);
		} else if ( strcmp(argv[5], "-DA")==0 && strcmp(argv[6], "-AN")==0 ) {
			char *assiName = argv[7];

			if (strlen(assiName) > MAX_NAME_LENGTH) {
				printf("invaid\n");
				return(255);
			}
			printf("Current mode: Deleting Assignment\n");

			delAssignment(assiName, filename);
		}

		// for grade entry
		else if ( strcmp(argv[5], "-AG")==0 && strcmp(argv[6], "-AN")==0 && strcmp(argv[8], "-FN")==0 && strcmp(argv[10], "-LN")==0 && strcmp(argv[12], "-G")==0 ) {
			char *assiName = argv[7];
			char *first = argv[9];
			char *last = argv[11];
			int grade = atoi(argv[13]);

			if (strlen(first) > MAX_NAME_LENGTH || strlen(last) > MAX_NAME_LENGTH || strlen(assiName)>MAX_NAME_LENGTH) {
				printf("invaid\n");
				return(255);
			}

			printf("Current mode: Adding Grade \n");
			addGrade(assiName, first, last, grade, filename);
		} else {
			printf("invalid\n");
			return(255);
		}

	}

	/*function to encrypt file here*/


	return 0;
}


/* FUNCTIONS DEFINITIONS */

/* regarding students */

// adding a student with first and last name
int addStudent(char *first, char *last, char *filename) {
	
	// creating the student to add
	char studentToAdd[100];
	strcpy(studentToAdd, "(Student,");
	strcat(studentToAdd, first);
	strcat(studentToAdd, ",");
	strcat(studentToAdd, last);
	strcat(studentToAdd, ")\n");

	int flag = fileContain(studentToAdd, filename);
	FILE *fp = fopen(filename, "a");
	if (flag == 0) {
		fprintf(fp, "%s", studentToAdd);
	} 

	if (flag == 1) {
		printf("Student already existed.\n");
		return(255);
	}
	
	return 0; // adding successfully
}

// deleting a student with first and last name
int delStudent(char *first, char *last, char *filename) {

	// buffer to store remaining entries
	char *buffer = (char *) malloc(MAX_STUD_AMOUNT * MAX_NAME_LENGTH * 2 * sizeof(char));
	char *ptr = buffer;
	char line[200];

	// create the student to del
	char studentToDel[100];
	strcpy(studentToDel, "(Student,");
	strcat(studentToDel, first);
	strcat(studentToDel, ",");
	strcat(studentToDel, last);
	strcat(studentToDel, ")\n");	

	if (fileContain(studentToDel, filename) == 0) {
		printf("There is no student of the same name.\n");
		return -1;
	}

	FILE *fp = fopen(filename, "r");
	while (fgets(line, 200, fp) != NULL) {

		if (strcmp(line, studentToDel) != 0) {
			strcpy(ptr, line);
			ptr += strlen(line);
		}
	}

	fclose(fp);
	fp = fopen(filename, "w");
	fprintf(fp, "%s", buffer);
	fclose(fp);

	return 1; // no student of the same name
}



/* regarding assignments */

int addAssignment(char *name, int totalPoints, float weight, char *filename) {
	char p[10];
	char w[10];
	float weightAfter = totalAssignmentWeights(filename) + weight;

	if (weightAfter > 1) {
		printf("Total Assigment weight = %f which is greater than 1.\n", weightAfter);
		return(255);
	}

	// create the assignment to add
	char assignmentToAdd[100];
	sprintf(p, "%d", totalPoints);
	sprintf(w, "%f", weight);
	strcpy(assignmentToAdd, "(Assignment,");
	strcat(assignmentToAdd, name);
	strcat(assignmentToAdd, ",");
		

	int flag = fileContain(assignmentToAdd, filename);
	FILE *fp = fopen(filename, "a");
	if (flag == 0) {
		strcat(assignmentToAdd, p);
		strcat(assignmentToAdd, ",");
		strcat(assignmentToAdd, w);
		strcat(assignmentToAdd, ")\n");
		fprintf(fp, "%s", assignmentToAdd);
	}

	if (flag == 1) {
		printf("Assignment already existed.\n");
		return(255);
	}

	return 0;
}


int delAssignment(char *name, char* filename) {
	// buffer to store remaining entries
	char *buffer = (char *) malloc(MAX_STUD_AMOUNT * MAX_NAME_LENGTH * 2 * sizeof(char));
	char *ptr = buffer;
	char line[200];

	// creating the assignment to del
	char assignmentToDel[100];
	strcpy(assignmentToDel, "(Assignment,");
	strcat(assignmentToDel, name);
	strcat(assignmentToDel, ",");

	// creating assignment grade to del
	char gradeToDel[100];
	strcpy(gradeToDel, "(Grade,");
	strcat(gradeToDel, name);
	strcat(gradeToDel, ",");

	int flag = fileContain(assignmentToDel, filename);
	if (flag == 0) {
		printf("Assignment does not exist.\n");
		return(255);
	}

	if (flag == 1) {
		FILE *fp = fopen(filename, "r");
		while (fgets(line, 200, fp) != NULL) {

			if ((strstr(line, assignmentToDel) == NULL) && (strstr(line, gradeToDel) == NULL)) {
				strcpy(ptr, line);
				ptr += strlen(line);
			}
		}

		fclose(fp);
		fp = fopen(filename, "w");
		fprintf(fp, "%s", buffer);
		fclose(fp);
	}

	return 1;
}

float totalAssignmentWeights(char *filename) {
	FILE *fp = fopen(filename, "r");
	float totalWeights = 0.0;
	char *flag = "(Assignment,";
	char line[100];
	char *ptr;
	char weight[10];

	while (fgets(line, 100, fp) != NULL) {
		if (strstr(line, flag) != NULL) {
			ptr = line;
			int startIndex = strcspn(line, ".");
			int endIndex = strcspn(line, ")");
			ptr = ptr + startIndex-1;
			strncpy(weight, ptr, endIndex-startIndex);
			totalWeights += atof(weight);
		}
	}

	return totalWeights;
}

/* regarding grade */
int addGrade(char *assiName, char *first, char* last, int grade, char *filename) {
	char g[10];

	// creating the grade entry
	char gradeEntry[100];
	sprintf(g, "%d", grade);
	strcpy(gradeEntry, "(Grade,");
	strcat(gradeEntry, assiName);
	strcat(gradeEntry, ",");
	strcat(gradeEntry, first);
	strcat(gradeEntry, ",");
	strcat(gradeEntry, last);
	strcat(gradeEntry, ",");


	// making sure the student and the grade for the grade entry exist
	// create the assignment to check
	int flagAssi = 0;
	char assignmentToCheck[100];
	strcpy(assignmentToCheck, "(Assignment,");
	strcat(assignmentToCheck, assiName);
	strcat(assignmentToCheck, ",");

	if (fileContain(assignmentToCheck, filename) == 1) {
		flagAssi = 1;
	}

	// create the student to check
	// create the student to del
	int flagStud = 0;
	char studentToCheck[100];
	strcpy(studentToCheck, "(Student,");
	strcat(studentToCheck, first);
	strcat(studentToCheck, ",");
	strcat(studentToCheck, last);
	strcat(studentToCheck, ")\n");

	if (fileContain(studentToCheck, filename) == 1) {
		flagStud =1;
	}

	if (flagAssi == 1 && flagStud == 1) {
		// add the grade only if the student and the assignment already existed

		char *buffer = (char *) malloc(MAX_STUD_AMOUNT*MAX_ASSI_AMOUNT*100*sizeof(char));
		char *ptr = buffer;
		char line[200];
		FILE *fp = fopen(filename, "r");
		while (fgets(line, 200, fp) != NULL) {
			if (strstr(line, gradeEntry) == NULL) {
				strcpy(ptr, line);
				ptr += strlen(line);
			}
		}
		fclose(fp);

		strcat(gradeEntry, g);
		strcat(gradeEntry, ")\n");
		strcpy(ptr, gradeEntry);

		fp = fopen(filename, "w");
		fprintf(fp, "%s", buffer);
		fclose(fp);

	} else {
		printf("Either the student or the assignment does not exsist.\n");
		return(255);
	}

}

/* MISCELLANEOUS*/

// check if a file contain an entry, return 1 if it does and 0 otherwise
int fileContain(char *str, char* filename) {
	FILE *fp = fopen(filename, "r");
	int flag = 0;
	char infile[100];
		while (fgets(infile, 100, fp) != NULL) {
			if (strstr(infile, str) != NULL) {
				flag = 1;
				break;
			}
		}
	fclose(fp);

	return flag;
}

