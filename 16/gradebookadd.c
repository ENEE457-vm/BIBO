#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "gradebook.h"
#include "gradebook_crypto.h"

typedef enum _action_t {
	ACTION_ADD_ASSIGN = 0,
	ACTION_DELETE_ASSIGN = 1,
	ACTION_ADD_STUDENT = 2,
	ACTION_DELETE_STUDENT = 3,
	ACTION_ADD_GRADE = 4,
} action_t;

void quit() {
	puts("invalid");
	exit(255);
}

int main(int argc, char *argv[]) {
	int c, option_index;
	action_t action = -1;
	char *filename = NULL, *key = NULL, *assign_name = NULL, *first_name = NULL,
	     *last_name = NULL, *points = NULL, *weight = NULL, *grade = NULL;
	FILE *gradebook_file = NULL, *random = NULL;
	gradebook_t gradebook;
	uint8_t key_val[KEY_SIZE], iv[IV_SIZE], tag[TAG_SIZE], ciphertext[sizeof(gradebook_t)];

	struct option options[] = {
		{"N", required_argument, 0, 'a'},
		{"K", required_argument, 0, 'b'},
		{"AA", no_argument, 0, 'c'},
		{"DA", no_argument, 0, 'd'},
		{"AS", no_argument, 0, 'e'},
		{"DS", no_argument, 0, 'f'},
		{"AG", no_argument, 0, 'g'},
		{"AN", required_argument, 0, 'h'},
		{"FN", required_argument, 0, 'i'},
		{"LN", required_argument, 0, 'j'},
		{"P", required_argument, 0, 'k'},
		{"W", required_argument, 0, 'l'},
		{"G", required_argument, 0, 'm'},
		{0, 0, 0, 0}
	};

	while ((c = getopt_long_only(argc, argv, "a:b:cdefgh:i:j:k:l:m:", options, &option_index)) != -1) {
		switch (c) {
			case 'a':
				filename = optarg;
				break;
			case 'b':
				if (filename != NULL) {key = optarg;}
				else {quit();}
				break;
			case 'c':
				if (filename && key && action == -1) {action = ACTION_ADD_ASSIGN;}
				else {quit();}
				break;
			case 'd':
				if (filename && key && action == -1) {action = ACTION_DELETE_ASSIGN;}
				else {quit();}
				break;
			case 'e':
				if (filename && key && action == -1) {action = ACTION_ADD_STUDENT;}
				else {quit();}
				break;
			case 'f':
				if (filename && key && action == -1) {action = ACTION_DELETE_STUDENT;}
				else {quit();}
				break;
			case 'g':
				if (filename && key && action == -1) {action = ACTION_ADD_GRADE;}
				else {quit();}
				break;
			case 'h':
				if (filename && key && action != -1) {assign_name = optarg;}
				else {quit();}
				break;
			case 'i':
				if (filename && key && action != -1) {first_name = optarg;}
				else {quit();}
				break;
			case 'j':
				if (filename && key && action != -1) {last_name = optarg;}
				else {quit();}
				break;
			case 'k':
				if (filename && key && action != -1) {points = optarg;}
				else {quit();}
				break;
			case 'l':
				if (filename && key && action != -1) {weight = optarg;}
				else {quit();};
				break;
			case 'm':
				if (filename && key && action != -1) {grade = optarg;}
				else {quit();}
				break;
			case '?':
				break;
		}
	}

	if (filename && key) {
		gradebook_file = fopen(filename, "rb");
		if (gradebook_file) {
			fseek(gradebook_file, 0, SEEK_END);
			long fsize = ftell(gradebook_file);
			fseek(gradebook_file, 0, SEEK_SET);
			if (fsize == sizeof(gradebook_t) + sizeof(iv) + sizeof(tag)) {
				fread(iv, sizeof(iv), 1, gradebook_file);
				fread(tag, sizeof(tag), 1, gradebook_file);
				fread(ciphertext, sizeof(ciphertext), 1, gradebook_file);
				if (parse_key(key, key_val)) {
					int len = gradebook_decrypt(ciphertext, key_val, iv,
					                            sizeof(iv), (uint8_t *)&gradebook,
					                            tag);
					if (len > 0) {
						goto good;
					}
				}
			}
		}
	}
	quit();

	good:
	fclose(gradebook_file);
	switch (action) {
		case ACTION_ADD_ASSIGN:
			if (assign_name && points && weight && !first_name && !last_name && !grade) {
				int16_t points_val = strtol(points, NULL, 10);
				if (errno == ERANGE) {quit();}
				double weight_val = strtod(weight, NULL);
				if (errno == ERANGE || !weight_val) {quit();}
				switch (add_assignment(&gradebook, assign_name, points_val, weight_val)) {
					case RETURN_OK:
						printf("Added assignment %s\n", assign_name);
						break;
					case RETURN_FULL:
						printf("Gradebook %s has reached the limit of assignments\n", filename);
						return 255;
					case RETURN_NAME_TOO_LONG:
						printf("Assignment name %s is too long\n", assign_name);
						return 255;
					case RETURN_ERROR:
						quit();
				}
			} else {
				quit();
			}
			break;
		case ACTION_DELETE_ASSIGN:
			if (assign_name && !first_name && !last_name && !points && !points && !grade) {
				switch (delete_assignment(&gradebook, assign_name)) {
					case RETURN_OK:
						printf("Deleted assignment %s\n", assign_name);
						break;
					default:
						quit();
				}
			} else {
				quit();
			}
			break;
		case ACTION_ADD_STUDENT:
			if (first_name && last_name && !assign_name && !points && !weight && !grade) {
				switch (add_student(&gradebook, first_name, last_name)) {
					case RETURN_OK:
						printf("Added student %s %s\n", first_name, last_name);
						break;
					case RETURN_FULL:
						printf("Gradebook %s has reached the limit of students\n", filename);
						return 255;
					case RETURN_NAME_TOO_LONG:
						printf("Student name %s %s is too long\n", first_name, last_name);
					case RETURN_ERROR:
						quit();
				}
			} else {
				quit();
			}
			break;
		case ACTION_DELETE_STUDENT:
			if (first_name && last_name && !assign_name && !points && !weight && !grade) {
				switch (delete_student(&gradebook, first_name, last_name)) {
					case RETURN_OK:
						printf("Deleted student %s %s\n", first_name, last_name);
						break;
					default:
						quit();
				}
			} else {
				quit();
			}
			break;
		case ACTION_ADD_GRADE:
			if (first_name && last_name && assign_name && grade && !points && !weight) {
				int16_t grade_val = strtol(grade, NULL, 10);
				if (errno == ERANGE) {quit();}
				switch (add_grade(&gradebook, first_name, last_name, assign_name, grade_val)) {
					case RETURN_OK:
						printf("Added grade for %s %s for %s\n", first_name, last_name, assign_name);
						break;
					default:
						quit();
				}
			} else {
				quit();
			}
			break;
		default:
			quit();
	}

	gradebook_file = fopen(filename, "wb");

	random = fopen("/dev/urandom", "r");
	fread(iv, sizeof(iv), 1, random);
	fclose(random);

	int len = gradebook_encrypt(&gradebook, key_val, iv, sizeof(iv), ciphertext, tag);

	fwrite(iv, sizeof(iv), 1, gradebook_file);
	fwrite(tag, sizeof(tag), 1, gradebook_file);
	fwrite(ciphertext, len, 1, gradebook_file);

	fclose(gradebook_file);
	return 0;
}
