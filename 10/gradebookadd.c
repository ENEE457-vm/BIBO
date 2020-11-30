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

struct arguments {
	char * n; //name
	unsigned char * k; //key as hex
	int aa; //add assignement
	int da; //delete assignment
	int as; //add student
	int ds; //delete student
	int ag; //add grade
	char * an; //assignment name (A-z 0-9)
	double p; //points assignment is out of  (must be +)
	double w; //assignment weight (must be real within [0,1])
	char * fn; //first name (A-z)
	char * ln; // last name (A-z)
	double g; //grade on an assignment (must be +)
};

void handleErrors(void)
{
    ERR_print_errors_fp(stderr);
    abort();
}


int gcm_encrypt(unsigned char *plaintext, int plaintext_len,
				unsigned char *key,
				unsigned char *iv, int iv_len,
				unsigned char *ciphertext,
				unsigned char *tag)
{
	EVP_CIPHER_CTX *ctx;
	int len;
	int ciphertext_len;


	/* Create and initialise the context */
	if(!(ctx = EVP_CIPHER_CTX_new()))
		handleErrors();

	/* Initialise the encryption operation. */
	if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL))
		handleErrors();

	/*
	 * Set IV length if default 12 bytes (96 bits) is not appropriate
	 */
	if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL))
		handleErrors();

	/* Initialise key and IV */
	if(1 != EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv))
		handleErrors();

	/*
	 * Provide the message to be encrypted, and obtain the encrypted output.
	 * EVP_EncryptUpdate can be called multiple times if necessary
	 */
	if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
		handleErrors();
	ciphertext_len = len;

	/*
	 * Finalise the encryption. Normally ciphertext bytes may be written at
	 * this stage, but this does not occur in GCM mode
	 */
	if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
		handleErrors();
	ciphertext_len += len;

	/* Get the tag */
	if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag))
		handleErrors();

	/* Clean up */
	EVP_CIPHER_CTX_free(ctx);

	return ciphertext_len;
}

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

int get_size() {
	int tot_size = 0;
	tot_size += sizeof(int);
	tot_size += sizeof(int);
	tot_size += sizeof(double);
	struct student * s = gb.students;
	struct assignment * a;
	for (int i = 0; i < gb.num_students; i++) {
		tot_size += sizeof(int);
		tot_size += strlen(s->first) + 1;
		tot_size += sizeof(int);
		tot_size += strlen(s->last) + 1;
		tot_size += sizeof(double);
		a = s->assignments;
		for (int j = 0; j < gb.num_assignments; j++) {
			tot_size += sizeof(int);
			tot_size += strlen(a->name) + 1;
			tot_size += sizeof(int);
			tot_size += sizeof(int);
			tot_size += sizeof(double);
			a = a->next;
		}
		s = s->next;
	}
	return tot_size;
}

void write_out_gb(FILE * fp, unsigned char * key) {
	int tot_size = get_size();
	unsigned char buf[tot_size];
	unsigned char * ptr = buf;
	memcpy(ptr, &gb.num_students, sizeof(int));
	ptr += sizeof(int);
	memcpy(ptr, &gb.num_assignments, sizeof(int));
	ptr += sizeof(int);
	memcpy(ptr, &gb.tot_weight, sizeof(double));
	ptr += sizeof(double);
	struct student * s = gb.students;
	struct assignment * a;

	for (int i = 0; i < gb.num_students; i++) {
		int flen = strlen(s->first) + 1;
		int llen = strlen(s->last) + 1;
		memcpy(ptr, &flen, sizeof(int));
		ptr += sizeof(int);
		memcpy(ptr, s->first, strlen(s->first) + 1);
		ptr += strlen(s->first) + 1;
		memcpy(ptr, &llen, sizeof(int));
		ptr += sizeof(int);
		memcpy(ptr, s->last, strlen(s->last) + 1);
		ptr += strlen(s->last) + 1;
		memcpy(ptr, &(s->grade), sizeof(double));
		ptr += sizeof(double);
		a = s->assignments;
		for (int j = 0; j < gb.num_assignments; j++) {
			int alen = strlen(a->name) + 1;
			memcpy(ptr, &alen, sizeof(int));
			ptr += sizeof(int);
			memcpy(ptr, a->name, strlen(a->name) + 1);
			ptr += strlen(a->name) + 1;
			memcpy(ptr, &(a->grade), sizeof(int));
			ptr += sizeof(int);
			memcpy(ptr, &(a->points), sizeof(int));
			ptr += sizeof(int);
			memcpy(ptr, &(a->weight), sizeof(double));
			ptr += sizeof(double);
			a = a->next;
		}
		s = s->next;
	}

	unsigned char tag[16];
	FILE * randfp;
	randfp = fopen("/dev/urandom", "r");
	unsigned char iv[16];
	fread(&iv, 16, 1, randfp);
	fclose(randfp);
	unsigned char ciphertext[tot_size];
	
	int cipher_size = gcm_encrypt(buf, tot_size, key, iv, 16, ciphertext, tag);
	unsigned char final[32 + cipher_size];
	unsigned char * fptr = final;
	memcpy(fptr, iv, 16);
	fptr += 16;
	memcpy(fptr, tag, 16);
	fptr += 16;
	memcpy(fptr, ciphertext, cipher_size);
	fwrite(final, cipher_size + 32, 1, fp);
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
		printf("decrypt bad\n");
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

int add_assignment(char * name, int points, double weight) {
	struct student * s = gb.students;
	struct assignment * a, * b;
	if (gb.tot_weight + weight > 1.0) {
		printf("Total assignment weights is over 1\n");
		return -1;
	}
	gb.tot_weight += weight;
	while(s) {
		a = s->assignments;
		if (!(a)) {
			struct assignment * n = malloc(sizeof(struct assignment));
			n->name = malloc(strlen(name) + 1);
			strcpy(n->name, name);
			n->grade = -1.0;
			n->points = points;
			n->weight = weight;
			s->assignments = n;
			s = s->next;
			continue;
		}
		while(a) {
			if (strcmp(a->name, name) == 0) {
				printf("Assignment already exists\n");
				return -1;
			} 
			b = a;
			a = a->next;
		}
		struct assignment * m = malloc(sizeof(struct assignment));
		m->name = malloc(strlen(name) + 1);
		strcpy(m->name, name);
		m->grade = -1.0;
		m->points = points;
		m->weight = weight;
		b->next = m;
		s = s->next;
	}

	gb.num_assignments += 1;
	return 0;
}

int delete_assignment(char * name) {
	struct student * s = gb.students;
	struct assignment * a;
	struct assignment * b;
	double tot = 0.0;
	int flag = 0;
	while(s) {
		a = s->assignments;
		b = NULL;
		while(a) {
			if (strcmp(a->name, name) == 0) {
				if (a->next) {
					if (b) {
						b->next = a->next;
						a = a->next;
					} else {
						s->assignments = a->next;
					}
				}
				if (strcmp(s->first, "xxx") != 0) {
					s->grade -= ((double)a->grade/(double)a->points)*a->weight;
				}
				tot = a->weight;
				flag = 1;
				free(a->name);
				a->name = NULL;
				a = NULL;
				break;
			}
			b = a;
			a = a->next;
		}
		if (flag == 0) {
			printf("Assignment %s does not exist\n", name);
			return -1;
		}
		s = s->next;
	}
	gb.tot_weight -= tot;
	gb.num_assignments -= 1;
	return 0;
}

int add_student(char * first, char * last) {
	struct student * temp = gb.students;
	struct student * n = malloc(sizeof(struct student));
	n->first = malloc(strlen(first) + 1);
	strcpy(n->first, first);
	n->last = malloc(strlen(last) + 1);
	strcpy(n->last, last);
	n->grade = 0.0;
	n->next = NULL;

	temp = gb.students;
	struct assignment * a;
	if (temp) {
		a = temp->assignments;
		if (gb.num_assignments != 0) {
			struct assignment * x = malloc(sizeof(struct assignment));
			n->assignments = x;
			for (int i = 0; i < gb.num_assignments; i++) {
				x->name = malloc(strlen(a->name) + 1);
				strcpy(x->name, a->name);
				x->grade = -1;
				x->points = a->points;
				x->weight = a->weight;
				if (i+1 < gb.num_assignments) {
					struct assignment * y = malloc(sizeof(struct assignment));
					x->next = y;
				}
				x = x->next;
				a = a->next;
			}
		}
	}
	temp = gb.students;
	while(temp->next) {
		temp = temp->next;
	}
	temp->next = n;
	gb.num_students += 1;
	return 0;
}

int delete_student(char * first, char * last) {
	struct student * s = gb.students;
	struct student * t = NULL;
	struct assignment * a, * b;
	int flag = 0;
	if (strcmp(first, "xxx") == 0) {
		printf("can't remove example student\n");
		return -1;
	}
	while(s) {
		if ((strcmp(s->first, first) == 0) && (strcmp(s->last, last) == 0)) {
			flag = 1;
			a = s->assignments;
			while(a) {
				b = a->next;
				free(a->name);
				a = b;
			}
			if (t) {
				t->next = s->next;
				free(s->first);
				free(s->last);
				s = NULL;
			} else {
				free(s->first);
				free(s->last);
				s = NULL;
			}
			break;
		}
		t = s;
		s = s->next;
	}
	if (flag == 0) {
		printf("student doesn't exist\n");
		return -1;
	}
	gb.num_students -= 1;
	return 0;
}

int add_grade(char * first, char * last, char * a_name, int grade) {
	struct student * s = gb.students;
	struct assignment * a;
	int flag = 0;
	if (strcmp(first, "xxx") == 0) {
		printf("can't add a grade for an example student\n");
		return -1;
	}
	while(s) {
		if (strcmp(s->first, first) == 0 && strcmp(s->last, last) == 0) {
			a = s->assignments;
			while(a) {
				if (strcmp(a->name, a_name) == 0) {
					flag = 1;
					a->grade = grade;
					s->grade += ((double)grade/(double)a->points)*a->weight;
				}
				a = a->next;
			}
		}
		s = s->next;
	}
	if (flag == 0) {
		printf("student or assignment does not exist\n");
		return -1;
	}
	return 0;
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

int validate_num(char * name) {
	for (int i = 0; i < strlen(name); i++) {
		if (!(isdigit(name[i]) || name[i] == '.')) {
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

int convert_k(char * key, unsigned char * outkey) {
	if (strlen(key) != 2 * 32) {
		printf("error\n");
		return -1;
	}
	for (int i = 0; i < 2 * 32; i += 2) {
		if (isxdigit(key[i]) && isxdigit(key[i+1])) {
			sscanf(&key[i], "%02hhx", &outkey[i/2]);
		}
	}
	return 0;
}


int main(int argc, char * argv[]) {
	struct arguments args;
	args.n = NULL;
	args.k = NULL;
	args.aa = 0;
	args.da = 0;
	args.as = 0;
	args.ds = 0;
	args.ag = 0;
	args.an = NULL;
	args.p = -1;
	args.w = -1.0;
	args.fn = NULL;
	args.ln = NULL;
	args.g = -1;

	static struct option long_options[] = {
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
		{0, 0, 0, 0},
	};
	int c;
	char * ptr;
	int option_index = 0;
	while((c = getopt_long_only(argc, argv, "a:b:cdefgh:i:j:k:l:m:", long_options, &option_index)) != -1) {
		switch(c) {
			case 'a':
				if (args.k || args.da || args.as || args.ds || args.ag || args.an || args.fn || args.ln || args.p != -1 || args.w != -1.0 || args.g != -1) {
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
						printf("invalid filename\n");
						exit(255);
					}
				} else {
					printf("option requires an argument\n");
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
				if ((!args.n) || args.aa || args.da || args.as || args.ds || args.ag || args.an || args.fn || args.ln || args.p != -1 || args.w != -1.0 || args.g != -1) {
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
					args.k = malloc(32);
					convert_k(optarg, args.k);
				} else {
					printf("option requires an argument\n");
					if (args.n) {
						free(args.n);
					}
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
				if ((!args.n) || !args.k || args.da || args.as || args.ds || args.ag || args.an || args.fn || args.ln || args.p != -1 || args.w != -1.0 || args.g != -1) {
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
				args.aa = 1;
				break;
			case 'd': //da
				if ((!args.n) || !args.k || args.aa || args.as || args.ds || args.ag || args.an || args.fn || args.ln || args.p != -1 || args.w != -1.0 || args.g != -1) {
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
				args.da = 1;
				break;
			case 'e': //as 
				if ((!args.n) || !args.k || args.da || args.aa || args.ds || args.ag || args.an || args.fn || args.ln || args.p != -1 || args.w != -1.0 || args.g != -1) {
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
				args.as = 1;
				break;
			case 'f': //ds
				if ((!args.n) || !args.k || args.da || args.as || args.aa || args.ag || args.an || args.fn || args.ln || args.p != -1 || args.w != -1.0 || args.g != -1) {
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
				args.ds = 1;
				break;
			case 'g': //ag
				if ((!args.n) || !args.k || args.da || args.as || args.ds || args.aa || args.an || args.fn || args.ln || args.p != -1 || args.w != -1.0 || args.g != -1) {
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
				args.ag = 1;
				break;
			case 'h': //an
				if (!args.n || !args.k || (!args.aa && !args.da && !args.as && !args.ds && !args.ag)) {
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
					if (validate_assignment(optarg) == 0){
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
			case 'i': //fn
				if (!args.n || !args.k || (!args.aa && !args.da && !args.as && !args.ds && !args.ag)) {
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
					if (validate_name(optarg) == 0) {
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
			case 'j': //ln
				if (!args.n || !args.k || (!args.aa && !args.da && !args.as && !args.ds && !args.ag)) {
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
					if (validate_name(optarg) == 0) {
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
			case 'k': //p
				if (!args.n || !args.k || (!args.aa && !args.da && !args.as && !args.ds && !args.ag)) {
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
					if (validate_num(optarg) != -1) {
						args.p = atoi(optarg);
						continue;
					}
				} 
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
			case 'l': //w
				if (!args.n || !args.k || (!args.aa && !args.da && !args.as && !args.ds && !args.ag)) {
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
					if (validate_num(optarg) != -1) {
						args.w = strtod(optarg, &ptr);
						continue;
					}
				}
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
			case 'm': //g
				if (!args.n || !args.k || (!args.aa && !args.da && !args.as && !args.ds && !args.ag)) {
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
					if (validate_num(optarg) != -1) {
						args.g = atoi(optarg);
						continue;
					}
				}
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
		}
	}
	if (args.n == NULL) {
		printf("invalid\n");
		exit(255);
	}


	FILE *fp;
	fp = fopen(args.n, "r");
	if (!fp) {
		printf("File doesn't exist\n");
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
	if (args.k) {
		if (read_in_gb(fp, args.k) == -1) {
			printf("issue decrypting\n");
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
		printf("need to supply the key\n");
		fclose(fp);
		exit(255);
	}
	fclose(fp);

	if (args.aa) {
		if (!args.an || !args.p || args.w == -1.0) {
			printf("invalid input for add assignment\n");
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
		if (add_assignment(args.an, args.p, args.w) == -1) {
			printf("add assignment error\n");
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
	} else if (args.da) {
		if (!args.an) {
			printf("incorrect arguments for delete assignment\n");
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
		if (delete_assignment(args.an) == -1) {
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
	} else if (args.as) {
		if (!args.ln || !args.fn) {
			printf("incorrect arguments for delete assignment\n");
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
		if (add_student(args.fn, args.ln) == -1) {
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
	} else if (args.ds) {
		if (!args.ln || !args.fn) {
			printf("incorrect arguments for delete assignment\n");
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
		if (delete_student(args.fn, args.ln) == -1) {
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
	} else if (args.ag){
		if (!args.ln || !args.fn || !args.an || !args.g) {
			printf("incorrect arguments for delete assignment\n");
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
		if (add_grade(args.fn, args.ln, args.an, args.g) == -1) {
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

	fp = fopen(args.n, "w");
	write_out_gb(fp, args.k);
	fclose(fp);
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
	destroy();
	return 0;
}