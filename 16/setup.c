#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "gradebook.h"
#include "gradebook_crypto.h"

static void quit() {
	puts("invalid");
	exit(255);
}

static int valid_filename(const char *name) {
	int len = strlen(name);
	for (int i = 0; i < len; i++) {
		if (!isalnum(name[i]) && name[i] != '.' && name[i] != '_') {
			return 0;
		}
	}

	// https://stackoverflow.com/a/230068
	if (access(name, R_OK | W_OK) != -1) {
		return 0;
	}

	return 1;
}

int main(int argc, char *argv[]) {
	char *filename = NULL;
	int c, len;
	gradebook_t gradebook;
	FILE *gradebook_file, *random;
	uint8_t key[KEY_SIZE], iv[IV_SIZE], tag[TAG_SIZE];
	uint8_t ciphertext[sizeof(gradebook_t)];

	while ((c = getopt(argc, argv, "N:")) != -1) {
		switch (c) {
			case 'N':
				filename = optarg;
				break;
			default:
				quit();
		}
	}

	if (filename == NULL || !valid_filename(filename)) {
		quit();
	}

	gradebook_init(&gradebook);
	gradebook_file = fopen(filename, "wb+");

	random = fopen("/dev/urandom", "r");
	fread(key, sizeof(key), 1, random);
	fread(iv, sizeof(iv), 1, random);
	fclose(random);

	printf("Key: ");
	for (int i = 0; i < sizeof(key); i++) {
		printf("%02x", key[i]);
	}
	putchar('\n');

	len = gradebook_encrypt(&gradebook, key, iv, sizeof(iv), ciphertext, tag);

	fwrite(iv, sizeof(iv), 1, gradebook_file);
	fwrite(tag, sizeof(tag), 1, gradebook_file);
	fwrite(ciphertext, len, 1, gradebook_file);

	fclose(gradebook_file);
	return 0;
}
