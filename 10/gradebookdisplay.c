#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <unistd.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

//read in iv and tag

struct arguments {
	char * n;
	unsigned char * k;
	int pa; //print grades of all students for specific assignment
	int ps; //print all grades for a student
	int pf; //print final grades
	char * an;
	char * fn;
	char * ln;
	int a; //alphabetical order
	int g; //grade order
};

void handleErrors(void)
{
    ERR_print_errors_fp(stderr);
    abort();
}

struct student {
	char * first;
	char * last;
	double grade;
	struct assignment * assignments;
	struct student * next;
};

struct assignment {
	char * name;
	int grade;
	int points;
	double weight;
	struct assignment * next;
};

struct gradebook {
	int num_students;
	int num_assignments;
	double tot_weight;
	struct student * students;
};

struct gradebook gb;
int size;

int gcm_decrypt(unsigned char *ciphertext, int ciphertext_len,
				unsigned char *tag,
				unsigned char *key,
				unsigned char *iv, int iv_len,
				unsigned char *plaintext)
{
	EVP_CIPHER_CTX *ctx;
	int len;
	int plaintext_len;
	int ret;

	/* Create and initialise the context */
	if(!(ctx = EVP_CIPHER_CTX_new()))
		handleErrors();

	/* Initialise the decryption operation. */
	if(!EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL))
		handleErrors();

	/* Set IV length. Not necessary if this is 12 bytes (96 bits) */
	if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL))
		handleErrors();

	/* Initialise key and IV */
	if(!EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv))
		handleErrors();

	/*
	 * Provide the message to be decrypted, and obtain the plaintext output.
	 * EVP_DecryptUpdate can be called multiple times if necessary
	 */
	if(!EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
		handleErrors();
	plaintext_len = len;

	/* Set expected tag value. Works in OpenSSL 1.0.1d and later */
	if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag))
		handleErrors();

	/*
	 * Finalise the decryption. A positive return value indicates success,
	 * anything else is a failure - the plaintext is not trustworthy.
	 */
	ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);

	/* Clean up */
	EVP_CIPHER_CTX_free(ctx);

	if(ret > 0) {
		/* Success */
		plaintext_len += len;
		return plaintext_len;
	} else {
		/* Verify failed */
		return -1;
	}
}

int read_in_gb(FILE * fp, unsigned char * key) {
	unsigned char iv[16];
	unsigned char tag[16];

	fseek(fp, 0L, SEEK_END);
	int ciphertext_len = ftell(fp) - 32;
	rewind(fp);
	fread(iv, 16, 1, fp);
	fread(tag, 16, 1, fp);

	unsigned char ciphertext[ciphertext_len];
	unsigned char plaintext[ciphertext_len];
	//get size of file
	fread(ciphertext, ciphertext_len, 1, fp);
	int x = gcm_decrypt(ciphertext, ciphertext_len, tag, key, iv, 16, plaintext);
	if (x == -1) {
		printf("corrupted file or incorrect fee\n");
		return -1;
	}
	unsigned char * ptr = plaintext;
	memcpy(&gb.num_students, ptr, sizeof(int));
	ptr += sizeof(int);
	memcpy(&gb.num_assignments, ptr, sizeof(int));
	ptr += sizeof(int);
	memcpy(&gb.tot_weight, ptr, sizeof(double));
	ptr += sizeof(double);
	struct student * s = malloc(sizeof(struct student));
	gb.students = s;
	int first, last, a_name;
	for (int i = 0; i < gb.num_students; i++) {
		if (i != 0) {
			struct student * t = malloc(sizeof(struct student));
			s->next = t;
			s = s->next;
		}
		memcpy(&first, ptr, sizeof(int));
		ptr += sizeof(int);
		s->first = malloc(first);
		memcpy(s->first, ptr, first);
		ptr += first;
		memcpy(&last, ptr, sizeof(int));
		ptr += sizeof(int);
		s->last = malloc(last);
		memcpy(s->last, ptr, last);
		ptr += last;
		memcpy(&s->grade, ptr, sizeof(double));
		ptr += sizeof(double);
		if (gb.num_assignments == 0) {
			s->assignments = NULL;
			continue;
		}
		struct assignment * a = malloc(sizeof(struct assignment));
		s->assignments = a;
		for (int j = 0; j < gb.num_assignments; j++) {
			if (j != 0) {
				struct assignment * b = malloc(sizeof(struct assignment));
				a->next = b;
				a = a->next;
			}
			memcpy(&a_name, ptr, sizeof(int));
			ptr += sizeof(int);
			a->name = malloc(a_name);
			memcpy(a->name, ptr, a_name);
			ptr += a_name;
			memcpy(&a->grade, ptr, sizeof(int));
			ptr += sizeof(int);
			memcpy(&a->points, ptr, sizeof(int));
			ptr += sizeof(int);
			memcpy(&a->weight, ptr, sizeof(double));
			ptr += sizeof(double);
		}
	}
	return 0;
}

int convert_k(char * key, unsigned char * outkey) {
	if (strlen(key) != 2 * 32) {
		printf("invalid\n");
		return -1;
	}
	for (int i = 0; i < 2 * 32; i += 2) {
		if (isxdigit(key[i]) && isxdigit(key[i+1])) {
			sscanf(&key[i], "%02hhx", &outkey[i/2]);
		}
	}
	return 0;
}

void sort_alpha() {
	struct student * s = gb.students;
	struct student * k = NULL;
	struct student * tmp = NULL;
	// s = gb.students;
	for (int i = 0; i < gb.num_students; i++) {
		s = gb.students;
		k = NULL;
		while(s && s->next) {
			if (!s->next->last || !s->last) {
				break;
			}
			//same last name
			if (strcmp(s->last, s->next->last) == 0) {
				//check the first names
				if (strcmp(s->first, s->next->first) > 0) {
					tmp = s;
					s = s->next;
					tmp->next = s->next;
					s->next = tmp;
					if (k == NULL) {
						gb.students = s;
					} else {
						k->next = s;
					}
				}
			} else if (strcmp(s->last, s->next->last) > 0) {
				tmp = s;
				s = s->next;
				tmp->next = s->next;
				s->next = tmp;
				if (k == NULL) {
					gb.students = s;
				} else {
					k->next = s;
				}
			}
			k = s;
			s = s->next;
		}
	}
	tmp = NULL;
}

int sort_grade(char * a) {
	struct student * s = gb.students;
	struct student * t, *k;
	struct assignment * a1, *a2;
	int g1, g2, flag = 0;

	for (int i = 0; i < gb.num_students; i++) {
		s = gb.students;
		k = NULL;
		flag = 0;
		while(s && s->next){
			a1 = s->assignments;
			while(a1) {
				if (strcmp(a1->name, a) == 0) {
					flag = 1;
					g1 = a1->grade;
				}
				a1 = a1->next;
			}
			if (flag == 0) {
				printf("assignment doesn't exist\n");
				return -1;
			}
			a2 = s->next->assignments;
			while(a2) {
				if (strcmp(a2->name, a) == 0) {
					g2 = a2->grade;
				}
				a2 = a2->next;
			}
			if (g1 < g2) {
				t = s;
				s = s->next;
				t->next = s->next;
				s->next = t;
				if (k == NULL) {
					gb.students = s;
				} else {
					k->next = s;
				}
			}
			k = s;
			s = s->next;
		}
	}
	return 0;
}

void sort_final_grade() {
	struct student * s = gb.students;
	struct student * tmp, *k = NULL;
	while(s && s->next) {
		if (s->grade < s->next->grade) {
			tmp = s;
			s = s->next;
			tmp->next = s->next;
			s->next = tmp;
			if (k == NULL) {
				gb.students = s;
			} else {
				k->next = s;
			}
		}
		k = s;
		s = s->next;
	}
}

int print_assignment(char * name, int order) {
	if (order == 1) {
		sort_alpha();
	} else {
		if (sort_grade(name) == -1) {
			return -1;
		}
	}
	struct student * s = gb.students;
	struct assignment * a;
	int flag = 0;

	while(s) {
		a = s->assignments;
		while(a) {
			if (strcmp(a->name, name) == 0) {
				if (a->grade != -1) {
					flag = 1;
					printf("(%s, %s, %d)\n", s->last, s->first, a->grade);
				}
				break;
			}
			a = a->next;
		}
		s = s->next;
	}
	if (flag == 0 ){
		printf("couldn't find the assignment\n");
		return -1;
	}
	return 0;
}

int print_student(char * last, char * first) {
	struct student * s = gb.students;
	struct assignment * a;
	int flag = 0;
	while(s) {
		if (strcmp(s->last, last) == 0 && strcmp(s->first, first) == 0) {
			flag = 1;
			a = s->assignments;
			while(a) {
				if (a->grade != -1 && a->name) {
					printf("(%s, %d)\n", a->name, a->grade);
				}
				a = a->next;
			}
			break;
		}
		s = s->next;
	}
	if (flag == 0) {
		printf("%s, %s does not exist\n", last, first);
		return -1;
	}
	return 0;
}

void print_final(int order) {
	if (order == 1) {
		sort_alpha();
	} else {
		sort_final_grade();
	}
	struct student * s = gb.students;
	while(s) {
		if (s->grade != -1) {
			printf("(%s, %s, %g)\n", s->last, s->first, s->grade);
		}
		s = s->next;
	}
}

int validate_file(char * name) {
	for (int i = 0; i < strlen(name); i++) {
		if (!(isalnum(name[i]) || name[i] == '_' || name[i] == '.')) {
			return -1;
		}
	}
	return 0;
}

int validate_assignment(char * name) {
	for (int i = 0; i < strlen(name); i++) {
		if (!(isalnum(name[i]))) {
			return -1;
		}
	}
	return 0;
}

int validate_name(char * name) {
	for (int i = 0; i < strlen(name); i++) {
		if (!(isalpha(name[i]))) {
			return -1;
		}
	}
	return 0;
}

void destroy() {
	struct student * s = gb.students;
	struct student * t;
	struct assignment *a, *b;
	while(s) {
		t = s->next;
		a = s->assignments;
		while(a) {
			b = a->next;
			free(a->name);
			a = b;
		}
		free(s->first);
		free(s->last);
		s = t;
	}
}

int main(int argc, char * argv[]) {
	int c;
	struct arguments args;
	args.n = NULL;
	args.k = NULL;
	args.pa = 0;
	args.ps = 0;
	args.pf = 0;
	args.an = NULL;
	args.fn = NULL;
	args.ln = NULL;
	args.a = 0;
	args.g = 0;
	size = 0;

	static struct option long_options[] = {
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
		{0, 0, 0, 0},
	};

	int option_index = 0;
	while((c = getopt_long_only(argc, argv, "a:b:cdef:g:h:ij", long_options, &option_index)) != -1) {
		switch(c) {
			case 'a':
				if (args.k || args.pa || args.ps || args.pf || args.an || args.fn || args.ln || args.a || args.g) {
					printf("wrong order of arguments\n");
					if (args.n) {
						free(args.n);
						args.n = NULL;
					} 
					if (args.k) {
						free(args.k);
						args.k = NULL;	
					}
					if (args.an) {
						free(args.an);
						args.an = NULL;
					}
					if (args.fn) {
						free(args.fn);
						args.fn = NULL;
					}
					if (args.ln) {
						free(args.ln);
						args.ln = NULL;
					}
					exit(255);
				}
				if (optarg) {
					if (validate_file(optarg) == 0) {
						args.n = malloc(strlen(optarg) + 1);
						strcpy(args.n, optarg);
					} else {
						printf("invalid\n");
						if (args.n) {
							free(args.n);
							args.n = NULL;
						} 
						if (args.k) {
							free(args.k);
							args.k = NULL;	
						}
						if (args.an) {
							free(args.an);
							args.an = NULL;
						}
						if (args.fn) {
							free(args.fn);
							args.fn = NULL;
						}
						if (args.ln) {
							free(args.ln);
							args.ln = NULL;
						}
						exit(255);
					}
				} else {
					printf("invalid\n");
					if (args.n) {
						free(args.n);
						args.n = NULL;
					} 
					if (args.k) {
						free(args.k);
						args.k = NULL;	
					}
					if (args.an) {
						free(args.an);
						args.an = NULL;
					}
					if (args.fn) {
						free(args.fn);
						args.fn = NULL;
					}
					if (args.ln) {
						free(args.ln);
						args.ln = NULL;
					}
					exit(255);
				}
				break;
			case 'b':
				if ((!args.n) || args.pa || args.ps || args.pf || args.an || args.fn || args.ln || args.a || args.g) {
					printf("invalid\n");
					if (args.n) {
						free(args.n);
						args.n = NULL;
					} 
					if (args.k) {
						free(args.k);
						args.k = NULL;	
					}
					if (args.an) {
						free(args.an);
						args.an = NULL;
					}
					if (args.fn) {
						free(args.fn);
						args.fn = NULL;
					}
					if (args.ln) {
						free(args.ln);
						args.ln = NULL;
					}
					exit(255);
				}
				if (optarg) {
					args.k = malloc(32);
					convert_k(optarg, args.k);
				} else {
					printf("invalid\n");
					if (args.n) {
						free(args.n);
						args.n = NULL;
					} 
					if (args.k) {
						free(args.k);
						args.k = NULL;	
					}
					if (args.an) {
						free(args.an);
						args.an = NULL;
					}
					if (args.fn) {
						free(args.fn);
						args.fn = NULL;
					}
					if (args.ln) {
						free(args.ln);
						args.ln = NULL;
					}
					exit(255);
				}
				break;
			case 'c':
				if ((!args.n) || (!args.k) || args.ps || args.pf || args.an || args.fn || args.ln || args.a || args.g) {
					printf("invalid\n");
					if (args.n) {
						free(args.n);
						args.n = NULL;
					} 
					if (args.k) {
						free(args.k);
						args.k = NULL;	
					}
					if (args.an) {
						free(args.an);
						args.an = NULL;
					}
					if (args.fn) {
						free(args.fn);
						args.fn = NULL;
					}
					if (args.ln) {
						free(args.ln);
						args.ln = NULL;
					}
					exit(255);
				}
				args.pa = 1;
				break;
			case 'd':
				if ((!args.n) || (!args.k) || args.pa || args.pf || args.an || args.fn || args.ln || args.a || args.g) {
					printf("invalid\n");
					if (args.n) {
						free(args.n);
						args.n = NULL;
					} 
					if (args.k) {
						free(args.k);
						args.k = NULL;	
					}
					if (args.an) {
						free(args.an);
						args.an = NULL;
					}
					if (args.fn) {
						free(args.fn);
						args.fn = NULL;
					}
					if (args.ln) {
						free(args.ln);
						args.ln = NULL;
					}
					exit(255);
				}
				args.ps = 1;
				break;
			case 'e':
				if ((!args.n) || (!args.k) || args.pa || args.ps || args.an || args.fn || args.ln || args.a || args.g) {
					printf("invalid\n");
					if (args.n) {
						free(args.n);
						args.n = NULL;
					} 
					if (args.k) {
						free(args.k);
						args.k = NULL;	
					}
					if (args.an) {
						free(args.an);
						args.an = NULL;
					}
					if (args.fn) {
						free(args.fn);
						args.fn = NULL;
					}
					if (args.ln) {
						free(args.ln);
						args.ln = NULL;
					}
					exit(255);
				}
				args.pf = 1;
				break;
			case 'f':
				if ((!args.n) || (!args.k) || ((!args.pa) && (!args.ps) && (!args.pf))) {
					printf("invalid\n");
					if (args.n) {
						free(args.n);
						args.n = NULL;
					} 
					if (args.k) {
						free(args.k);
						args.k = NULL;	
					}
					if (args.an) {
						free(args.an);
						args.an = NULL;
					}
					if (args.fn) {
						free(args.fn);
						args.fn = NULL;
					}
					if (args.ln) {
						free(args.ln);
						args.ln = NULL;
					}
					exit(255);
				}
 				if (optarg) {
 					if (validate_assignment(optarg) == 0) {
						args.an = malloc(strlen(optarg) + 1);
						strcpy(args.an, optarg);
 					} else {
 						printf("invalid\n");
 						if (args.n) {
							free(args.n);
							args.n = NULL;
						} 
						if (args.k) {
							free(args.k);
							args.k = NULL;	
						}
						if (args.an) {
							free(args.an);
							args.an = NULL;
						}
						if (args.fn) {
							free(args.fn);
							args.fn = NULL;
						}
						if (args.ln) {
							free(args.ln);
							args.ln = NULL;
						}
						exit(255);
 					}
				} else {
					printf("invalid\n");
					if (args.n) {
						free(args.n);
						args.n = NULL;
					} 
					if (args.k) {
						free(args.k);
						args.k = NULL;	
					}
					if (args.an) {
						free(args.an);
						args.an = NULL;
					}
					if (args.fn) {
						free(args.fn);
						args.fn = NULL;
					}
					if (args.ln) {
						free(args.ln);
						args.ln = NULL;
					}
					exit(255);
				}
				break;
			case 'g':
				if ((!args.n) || (!args.k) || ((!args.pa) && (!args.ps) && (!args.pf))) {
					printf("invalid\n");
					if (args.n) {
						free(args.n);
						args.n = NULL;
					} 
					if (args.k) {
						free(args.k);
						args.k = NULL;	
					}
					if (args.an) {
						free(args.an);
						args.an = NULL;
					}
					if (args.fn) {
						free(args.fn);
						args.fn = NULL;
					}
					if (args.ln) {
						free(args.ln);
						args.ln = NULL;
					}
					exit(255);
				}
				if (optarg) {
					if (validate_name(optarg) != 0) {
						printf("invalid\n");
						if (args.n) {
							free(args.n);
							args.n = NULL;
						} 
						if (args.k) {
							free(args.k);
							args.k = NULL;	
						}
						if (args.an) {
							free(args.an);
							args.an = NULL;
						}
						if (args.fn) {
							free(args.fn);
							args.fn = NULL;
						}
						if (args.ln) {
							free(args.ln);
							args.ln = NULL;
						}
						exit(255);
					}
					args.fn = malloc(strlen(optarg) + 1);
					strcpy(args.fn, optarg);
				} else {
					printf("invalid\n");
					if (args.n) {
						free(args.n);
						args.n = NULL;
					} 
					if (args.k) {
						free(args.k);
						args.k = NULL;	
					}
					if (args.an) {
						free(args.an);
						args.an = NULL;
					}
					if (args.fn) {
						free(args.fn);
						args.fn = NULL;
					}
					if (args.ln) {
						free(args.ln);
						args.ln = NULL;
					}
					exit(255);
				}
				break;
			case 'h':
				if ((!args.n) || (!args.k) || ((!args.pa) && (!args.ps) && (!args.pf))) {
					printf("invalid\n");
					if (args.n) {
						free(args.n);
						args.n = NULL;
					} 
					if (args.k) {
						free(args.k);
						args.k = NULL;	
					}
					if (args.an) {
						free(args.an);
						args.an = NULL;
					}
					if (args.fn) {
						free(args.fn);
						args.fn = NULL;
					}
					if (args.ln) {
						free(args.ln);
						args.ln = NULL;
					}
					exit(255);
				}
				if (optarg) {
					if (validate_name(optarg) != 0) {
						printf("invalid\n");
						if (args.n) {
							free(args.n);
							args.n = NULL;
						} 
						if (args.k) {
							free(args.k);
							args.k = NULL;	
						}
						if (args.an) {
							free(args.an);
							args.an = NULL;
						}
						if (args.fn) {
							free(args.fn);
							args.fn = NULL;
						}
						if (args.ln) {
							free(args.ln);
							args.ln = NULL;
						}
						exit(255);
					}
					args.ln = malloc(strlen(optarg) + 1);
					strcpy(args.ln, optarg);
				} else {
					printf("invalid\n");
					if (args.n) {
						free(args.n);
						args.n = NULL;
					} 
					if (args.k) {
						free(args.k);
						args.k = NULL;	
					}
					if (args.an) {
						free(args.an);
						args.an = NULL;
					}
					if (args.fn) {
						free(args.fn);
						args.fn = NULL;
					}
					if (args.ln) {
						free(args.ln);
						args.ln = NULL;
					}
					exit(255);
				}
				break;
			case 'i':
				if ((!args.n) || (!args.k) || ((!args.pa) && (!args.ps) && (!args.pf))) {
					printf("invalid\n");
					if (args.n) {
						free(args.n);
						args.n = NULL;
					} 
					if (args.k) {
						free(args.k);
						args.k = NULL;	
					}
					if (args.an) {
						free(args.an);
						args.an = NULL;
					}
					if (args.fn) {
						free(args.fn);
						args.fn = NULL;
					}
					if (args.ln) {
						free(args.ln);
						args.ln = NULL;
					}
					exit(255);
				}
				if (args.g) {
					printf("invalid\n");
					if (args.n) {
						free(args.n);
						args.n = NULL;
					} 
					if (args.k) {
						free(args.k);
						args.k = NULL;	
					}
					if (args.an) {
						free(args.an);
						args.an = NULL;
					}
					if (args.fn) {
						free(args.fn);
						args.fn = NULL;
					}
					if (args.ln) {
						free(args.ln);
						args.ln = NULL;
					}
					exit(255);
				}
				args.a = 1;
				break;
			case 'j':
				if ((!args.n) || (!args.k) || ((!args.pa) && (!args.ps) && (!args.pf))) {
					printf("invalid\n");
					if (args.n) {
						free(args.n);
						args.n = NULL;
					} 
					if (args.k) {
						free(args.k);
						args.k = NULL;	
					}
					if (args.an) {
						free(args.an);
						args.an = NULL;
					}
					if (args.fn) {
						free(args.fn);
						args.fn = NULL;
					}
					if (args.ln) {
						free(args.ln);
						args.ln = NULL;
					}
					exit(255);
				}
				if (args.a) {
					printf("invalid\n");
					if (args.n) {
						free(args.n);
						args.n = NULL;
					} 
					if (args.k) {
						free(args.k);
						args.k = NULL;	
					}
					if (args.an) {
						free(args.an);
						args.an = NULL;
					}
					if (args.fn) {
						free(args.fn);
						args.fn = NULL;
					}
					if (args.ln) {
						free(args.ln);
						args.ln = NULL;
					}
					exit(255);
				}
				args.g = 1;
				break;
			default:
				printf("invalid\n");
				if (args.n) {
					free(args.n);
					args.n = NULL;
				} 
				if (args.k) {
					free(args.k);
					args.k = NULL;	
				}
				if (args.an) {
					free(args.an);
					args.an = NULL;
				}
				if (args.fn) {
					free(args.fn);
					args.fn = NULL;
				}
				if (args.ln) {
					free(args.ln);
					args.ln = NULL;
				}
				exit(255);
				break;
		}
	}
	if (args.n == NULL) {
		printf("invalid\n");
		exit(255);
	} 
	FILE * fp;
	fp = fopen(args.n, "r");
	read_in_gb(fp, args.k);
	fclose(fp);

	if (args.pa) {
		if ((!args.a && !args.g) || !args.an || args.fn || args.ln) {
			printf("invalid\n");
			if (args.n) {
				free(args.n);
				args.n = NULL;
			} 
			if (args.k) {
				free(args.k);
				args.k = NULL;	
			}
			if (args.an) {
				free(args.an);
				args.an = NULL;
			}
			if (args.fn) {
				free(args.fn);
				args.fn = NULL;
			}
			if (args.ln) {
				free(args.ln);
				args.ln = NULL;
			}
			exit(255);
		}
		if (args.a) {
			if (print_assignment(args.an, 1) == -1) {
				if (args.n) {
					free(args.n);
					args.n = NULL;
				} 
				if (args.k) {
					free(args.k);
					args.k = NULL;	
				}
				if (args.an) {
					free(args.an);
					args.an = NULL;
				}
				if (args.fn) {
					free(args.fn);
					args.fn = NULL;
				}
				if (args.ln) {
					free(args.ln);
					args.ln = NULL;
				}
				exit(255);
			} 
		} else {
			if (print_assignment(args.an, 2) == -1) {
				if (args.n) {
					free(args.n);
					args.n = NULL;
				} 
				if (args.k) {
					free(args.k);
					args.k = NULL;	
				}
				if (args.an) {
					free(args.an);
					args.an = NULL;
				}
				if (args.fn) {
					free(args.fn);
					args.fn = NULL;
				}
				if (args.ln) {
					free(args.ln);
					args.ln = NULL;
				}
				exit(255);
			} 
		}
	} else if (args.ps) {
		if (!args.fn || !args.ln || args.a || args.g || args.an) {
			printf("invalid\n");
			if (args.n) {
				free(args.n);
				args.n = NULL;
			} 
			if (args.k) {
				free(args.k);
				args.k = NULL;	
			}
			if (args.an) {
				free(args.an);
				args.an = NULL;
			}
			if (args.fn) {
				free(args.fn);
				args.fn = NULL;
			}
			if (args.ln) {
				free(args.ln);
				args.ln = NULL;
			}
			exit(255);
		} else {
			if (print_student(args.ln, args.fn) == -1) {
				if (args.n) {
					free(args.n);
					args.n = NULL;
				} 
				if (args.k) {
					free(args.k);
					args.k = NULL;	
				}
				if (args.an) {
					free(args.an);
					args.an = NULL;
				}
				if (args.fn) {
					free(args.fn);
					args.fn = NULL;
				}
				if (args.ln) {
					free(args.ln);
					args.ln = NULL;
				}
				exit(255);
			} 
		}	
	} else if (args.pf) {
		if ((!args.a && !args.g) || args.fn || args.ln || args.an) {
			if (args.n) {
				free(args.n);
				args.n = NULL;
			} 
			if (args.k) {
				free(args.k);
				args.k = NULL;	
			}
			if (args.an) {
				free(args.an);
				args.an = NULL;
			}
			if (args.fn) {
				free(args.fn);
				args.fn = NULL;
			}
			if (args.ln) {
				free(args.ln);
				args.ln = NULL;
			}
			exit(255);
			printf("invalid\n");
		} else {
			if (args.a) {
				print_final(1);
			} else {
				print_final(2);
			}
		}
	}
	destroy();
	return 0;
}