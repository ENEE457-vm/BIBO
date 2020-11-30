#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include "gradebook.h"
#include "gradebook_crypto.h"

typedef enum _action_t {
	ACTION_PRINT_ASSIGN = 0,
	ACTION_PRINT_STUDENT = 1,
	ACTION_PRINT_FINAL = 2,
} action_t;

void quit() {
	puts("invalid");
	exit(255);
}

int main(int argc, char *argv[]) {
	int c, option_index;
	action_t action = -1;
	order_t order = -1;
	char *filename = NULL, *key = NULL, *assign_name = NULL, *first_name = NULL,
	     *last_name = NULL;
	FILE *gradebook_file = NULL;
	gradebook_t gradebook;
	uint8_t key_val[KEY_SIZE], iv[IV_SIZE], tag[TAG_SIZE], ciphertext[sizeof(gradebook_t)];

	struct option options[] = {
		{"N", required_argument, 0, 'a'},
		{"K", required_argument, 0, 'b'},
		{"PA", no_argument, 0, 'c'},
		{"PS", no_argument, 0, 'd'},
		{"PF", no_argument, 0, 'e'},
		{"AN", required_argument, 0, 'f'},
		{"FN", required_argument, 0, 'g'},
		{"LN", required_argument, 0, 'h'},
		{"A", no_argument, 0, 'i'},
		{"G", no_argument, 0, 'j'},
		{0, 0, 0, 0}
	};

	while ((c = getopt_long_only(argc, argv, "a:b:cdef:g:h:ij", options, &option_index)) != -1) {
		switch (c) {
			case 'a':
				filename = optarg;
				break;
			case 'b':
				if (filename != NULL) {key = optarg;}
				else {quit();}
				break;
			case 'c':
				if (filename && key && action == -1) {action = ACTION_PRINT_ASSIGN;}
				else {quit();}
				break;
			case 'd':
				if (filename && key && action == -1) {action = ACTION_PRINT_STUDENT;}
				else {quit();}
				break;
			case 'e':
				if (filename && key && action == -1) {action = ACTION_PRINT_FINAL;}
				else {quit();}
				break;
			case 'f':
				if (filename && key && action != -1) {assign_name = optarg;}
				else {quit();}
				break;
			case 'g':
				if (filename && key && action != -1) {first_name = optarg;}
				else {quit();}
				break;
			case 'h':
				if (filename && key && action != -1) {last_name = optarg;}
				else {quit();}
				break;
			case 'i':
				if (filename && key && action != -1 && order == -1) {order = ORDER_ALPHABET;}
				else {quit();}
				break;
			case 'j':
				if (filename && key && action != -1 && order == -1) {order = ORDER_GRADE;}
				else {quit();}
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
		case ACTION_PRINT_ASSIGN:
			if (assign_name && order != -1 && !first_name && !last_name) {
				print_assignment(&gradebook, assign_name, order);
			} else {
				quit();
			}
			break;
		case ACTION_PRINT_STUDENT:
			if (first_name && last_name && !assign_name && order == -1) {
				print_student(&gradebook, first_name, last_name);
			} else {
				quit();
			}
			break;
		case ACTION_PRINT_FINAL:
			if (order != -1 && !assign_name && !first_name && !last_name) {
				print_final(&gradebook, order);
			} else {
				quit();
			}
			break;
		default:
			quit();
	}
	return 0;
}