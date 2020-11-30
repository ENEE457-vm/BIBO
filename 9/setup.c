#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>

#include "data.h"

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#define DEBUG

/* test whether the file exists */
int file_test(char* filename) {
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

int check_filename_valid(char *ch) {
    while (*ch != '\0') {
        if (!isalnum(*ch) && *ch != '.' && *ch != '_') {
            return -1;
        }
        ch++;
    }
    return 0;
}

int main(int argc, char** argv) {
    FILE *fp;

    if (argc < 3) {
        printf("Usage: setup -N <file name>\n");
        return 255;
    }

    if (strcmp(argv[1], "-N") != 0) {
        printf("invalid\n");
        return 255;
    }

    char *ptr = argv[2];
    if (check_filename_valid(ptr) == -1) {
        printf("invalid\n");
        return 255;
    }
    fp = fopen(argv[2], "w");
    if (fp == NULL){
        printf("setup: fopen() error could not create file\n");
        return 255;
    }

    if (file_test(argv[2]))
        printf("created file named %s\n", argv[2]);

    fclose(fp);

    Gradebook *gb = (Gradebook *)malloc(sizeof(Gradebook));
    gb->students = NULL;
    gb->students_length = 0;
    gb->assignments = NULL;
    gb->assignments_length = 0;

    //add_assignment_to_gradebook("midterm", 100, 0.25, 4, gb);
    //add_assignment_to_gradebook("final", 200, 0.75, 4, gb);
    //add_student_to_gradebook("John", "Smith", gb);
    //add_student_to_gradebook("Russel", "Tyler", gb);
    //add_student_to_gradebook("Ted", "Mason", gb);
    //add_grade_to_gradebook("midterm", "John", "Smith", 90, gb);
    //add_grade_to_gradebook("final", "John", "Smith", 175, gb);
    //add_grade_to_gradebook("midterm", "Russel", "Tyler", 75, gb);
    //add_grade_to_gradebook("final", "Russel", "Tyler", 150, gb);

    Buffer b = print_Gradebook_to_buffer(gb);
    //printf("%s\n", b.Buf);

    //generate key
    unsigned char key[16];
    if (!RAND_bytes(key, 16)) {
        printf("ssl failure\n");
        return 255;
    }
    printf("Key: ");
    for (int i = 0; i < 16; i++) {
        printf("%02x", key[i]);
    }
    printf("\n");

    write_to_path(argv[2], &b, key);

    //for (int s=0; s < gb->students_length; s++) {
    //    free(gb->students[s].fname);
    //    free(gb->students[s].lname);
    //}
    for (int a=0; a < gb->assignments_length; a++) {
    //    free(gb->assignments[a].assignment_name);
        free(gb->assignments[a].grades);
    }
    free(gb->students);
    free(gb->assignments);
    free(gb);
    free(b.Buf);
    
    return 0;
}
