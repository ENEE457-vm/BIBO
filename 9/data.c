#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "data.h"

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/comp.h>
#include <openssl/rand.h>

//#define NON_VAR_LENGTH 0

int has_assignment(char *aname, Gradebook *R) {
    for (int i=0; i < R->assignments_length; i++) {
        Assignment a = R->assignments[i];
        if (strcmp(a.assignment_name, aname) == 0) {
            return 1;
        }
    }
    return 0;
}

int sum_weights_valid(float new_weight, Gradebook *R) {
    float weight_sum = new_weight;
    for (int i=0; i < R->assignments_length; i++) {
        weight_sum += R->assignments[i].weight;
    }
    if (weight_sum > 1) {
        return 0;
    }
    return 1;
}

int has_student(char *fname, char *lname, Gradebook *R) {
    for (int i=0; i < R->students_length; i++) {
        Student s = R->students[i];
        if (strcmp(s.fname, fname) == 0 && strcmp(s.lname, lname) == 0) {
            return 1;
        }
    }
    return 0;
}

Buffer print_Gradebook_to_buffer(Gradebook *R) {
    Buffer B = { 0 };
    unsigned char *s = NULL;

    // write assignment and student length values
    int al_digits = R->assignments_length == 0 ? 1 : floor(log10(R->assignments_length)) + 1;
    int sl_digits = R->students_length == 0 ? 1 : floor(log10(R->students_length)) + 1;
    unsigned char init[al_digits + sl_digits + 3];
    sprintf(init, "%d\n%d\n", R->assignments_length, R->students_length);
    size_t new_size = strlen(init) + 1;
    s = (unsigned char *)malloc(new_size);
    strcpy(s, init);

    for (int a_counter=0; a_counter < R->assignments_length; a_counter++) {
        Assignment a = R->assignments[a_counter];
        int an_len = strlen(a.assignment_name);
        int p_digits = a.points == 0 ? 1 : floor(log10(a.points)) + 1;
        unsigned char temp_str[an_len + p_digits + a.weight_char_count + 4];
        sprintf(temp_str, "%s|%d|%f|", a.assignment_name, a.points, a.weight);
        s = (unsigned char *)realloc(s, strlen(s) + strlen(temp_str) + 1);
        strcat(s, temp_str);

        for (int g_counter=0; g_counter < a.grades_length; g_counter++) {
            int g_digits = a.grades[g_counter] <= 0 ? abs(a.grades[g_counter]) + 1 : floor(log10(a.grades[g_counter])) + 1;
            unsigned char grade_str[g_digits + 2]; // + 2 for pipe and \0
            sprintf(grade_str, "%d|", a.grades[g_counter]);
            
            size_t new_size = strlen(s) + strlen(grade_str) + 1;
            s = (unsigned char *)realloc(s, new_size);
            strcat(s, grade_str);
        }
        
        int len = strlen(s);
        s = (unsigned char *)realloc(s, len + 2);
        s[len] = '\n';
        s[len+1] = '\0';
    }

    for (int s_counter=0; s_counter < R->students_length; s_counter++) {
        Student stud = R->students[s_counter];
        unsigned char temp_str[strlen(stud.fname) + strlen(stud.lname) + 4];
        sprintf(temp_str, "%s|%s|\n", stud.fname, stud.lname);

        size_t new_size = strlen(s) + strlen(temp_str) + 1;
        s = (unsigned char *)realloc(s, new_size);
        strcat(s, temp_str);
    }

    B.Buf = s;
    B.Length = strlen(s);

    return B;
}

void handleErrors(void) {
    printf("invalid\n");
    exit(255);
}

void write_to_path(char *path, Buffer *B, unsigned char *key_data) {
    int plaintext_len = B->Length;
    unsigned char *plaintext = B->Buf;
    unsigned char ciphertext[plaintext_len + 128];

    // generate iv
    unsigned char iv[16];
    if (!RAND_bytes(iv, 16)) {
        printf("ssl failure\n");
        exit(255);
    }

    EVP_CIPHER_CTX *ctx;
    int len;
    int ciphertext_len;

    if (!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key_data, iv))
        handleErrors();

    if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        handleErrors();
    ciphertext_len = len;

    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        handleErrors();
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    for (int j=0; j < 16; j++) {
        ciphertext[ciphertext_len+j] = iv[j];
    }
    ciphertext_len += 16;

    FILE *fp = fopen(path, "w");
    fwrite(ciphertext, ciphertext_len, 1, fp);
    fclose(fp);

    return;
}

Buffer read_from_path(char *path, unsigned char *key_data) {
    unsigned char *ciphertext;
    int total_length;
    FILE *fp = fopen(path, "r");
    if (!fp) {
        printf("error opening file\n");
        exit(255);
    }
    fseek(fp, 0, SEEK_END);
    total_length = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    ciphertext = malloc(total_length);
    if (ciphertext);
    fread(ciphertext, 1, total_length, fp);
    unsigned char plaintext[total_length + 128];
    fclose(fp);

    unsigned char iv[16];
    int j = 0;
    int i;
    for (i=total_length-16; i < total_length; i++) {
        iv[j] = ciphertext[i];
        j++;
    }
    int ciphertext_len = total_length - 16;
    ciphertext = (unsigned char *)realloc(ciphertext, ciphertext_len);

    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;

    if (!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();
    
    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key_data, iv))
        handleErrors();

    if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        handleErrors();
    
    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
        handleErrors();
    plaintext_len += len;

    free(ciphertext);
    EVP_CIPHER_CTX_free(ctx);

    int pt_len = strlen(plaintext);
    unsigned char *pt = malloc(pt_len + 1);
    pt[pt_len] = '\0';
    strncpy(pt, plaintext, pt_len);
    //pt[pt_len] = '\0';

    Buffer B = { pt, pt_len };

    return B;
}

void parse_line_and_add_assignment(unsigned char *line, Gradebook *R) {
    unsigned char *name;
    int points;
    float weight;
    int weight_char_count;
    int *grades = NULL;
    int grades_length = 0;

    unsigned char *curr_section = line;
    int section_num = 0;
    while(curr_section) {
        unsigned char *next_section = strchr(curr_section, '|');
        if (next_section) {
            *next_section = '\0';

            if (section_num == 0) {
                name = (unsigned char *)malloc(strlen(curr_section) + 1);
                strcpy(name, curr_section);
            } else if (section_num == 1) {
                points = atoi(curr_section);
            } else if (section_num == 2) {
                weight_char_count = (int)strlen(curr_section);
                weight = atof(curr_section);
            } else {
                int curr_grade = atoi(curr_section);
                grades_length++;
                grades = (int *)realloc(grades, grades_length * sizeof(int));
                grades[grades_length-1] = curr_grade;
            }

            *next_section = '|';
            curr_section = next_section + 1;
        } else {
            curr_section = NULL;
        }
        
        section_num++;
    }

    Assignment a = { name, points, weight, weight_char_count, grades, grades_length };
    
    int new_assignment_count = R->assignments_length + 1;
    size_t new_size = sizeof(Assignment) * new_assignment_count;
    Assignment *new_assignments = (Assignment *)realloc(R->assignments, new_size);

    new_assignments[new_assignment_count-1] = a;
    R->assignments = new_assignments;
    R->assignments_length = new_assignment_count;

    return;
}

void parse_line_and_add_student(unsigned char *line, Gradebook *R) {
    unsigned char *fname;
    unsigned char *lname;

    unsigned char *l = line;
    unsigned char *split_char = strchr(l, '|');
    if (split_char) {
        *split_char = '\0';
    }
    fname = (unsigned char *)malloc(strlen(l) + 1);
    strcpy(fname, l);
    if (split_char) {
        *split_char = '|';
        l = split_char + 1;
    }
    split_char = strchr(l, '|');
    if (split_char) {
        *split_char = '\0';
    }
    lname = (unsigned char *)malloc(strlen(l) + 1);
    strcpy(lname, l);
    if (split_char) {
        *split_char = '|';
        l = split_char + 1;
    }

    Student s = { fname, lname };

    int new_students_length = R->students_length + 1;
    size_t new_size = sizeof(Student) * new_students_length;
    Student *new_students = (Student *)realloc(R->students, new_size);

    new_students[new_students_length-1] = s;
    R->students = new_students;
    R->students_length = new_students_length;

    return;
}

int get_Gradebook(Gradebook *R, Buffer *B) {
    R->students = NULL;
    R->students_length = 0;
    R->assignments = NULL;
    R->assignments_length = 0;

    unsigned int bytesRead = 0;
    unsigned char *curr_line = B->Buf;
    int line_num = 0;
    int num_assignments = -1;
    int num_students = -1;
    while (curr_line) {
        unsigned char *next_line = strchr(curr_line, '\n');
        if (next_line) {
            *next_line = '\0';
        }
        // curr_line is now its own string
        // grab data from it depencing on what line we're in
        if (line_num == 0) {
            num_assignments = atoi(curr_line);
        } else if (line_num == 1) {
            num_students = atoi(curr_line);
        } else if (line_num >= 2 && line_num < 2 + num_assignments) {
            parse_line_and_add_assignment(curr_line, R);
        } else if (line_num >= 2 + num_assignments && line_num < 2 + num_assignments + num_students) {
            parse_line_and_add_student(curr_line, R);
        }

        if (next_line) {
            *next_line = '\n';
            curr_line = next_line + 1;
        } else {
            curr_line = NULL;
        }
        
        line_num++;
    }

    return bytesRead;
}

void add_assignment_to_gradebook(unsigned char *aname, int points, float weight, int weight_char_count, Gradebook *gb) {
    int grades_length = gb->students_length;
    int *grades = malloc(sizeof(int) * grades_length);
    
    // default student grades to -1
    for (int g=0; g < grades_length; g++) {
        grades[g] = -1;
    }
    
    Assignment a = { aname, points, weight, weight_char_count, grades, grades_length };

    // reallocate size for assignments_length
    int new_assignment_count = gb->assignments_length + 1;
    size_t new_size = sizeof(Assignment) * new_assignment_count;
    gb->assignments = realloc(gb->assignments, new_size);
    gb->assignments[new_assignment_count-1] = a;
    gb->assignments_length = new_assignment_count;

    return;
}

void add_student_to_gradebook(unsigned char *fname, unsigned char *lname, Gradebook *gb) {
    //int id = gb->students_length;
    Student s = { fname, lname };
    int new_students_length = gb->students_length + 1;
    
    // reallocate size for students_length

    size_t new_size = sizeof(Student) * new_students_length;
    //Student *new_ptr = (Student *)realloc(gb->students, new_size);
    gb->students = realloc(gb->students, new_size);
    gb->students[new_students_length - 1] = s;
    //new_ptr[new_students_length-1] = s;
    //gb->students = new_ptr;
    gb->students_length = new_students_length;

    // add a grade to this student for each assignment

    int a_counter;
    for (a_counter=0; a_counter < gb->assignments_length; a_counter++) {
        Assignment a = gb->assignments[a_counter];
        int new_len = a.grades_length + 1;
        a.grades = (int *)realloc(a.grades, sizeof(int) * new_len);
        a.grades[new_len-1] = -1;
        a.grades_length++;
        gb->assignments[a_counter] = a;
    }

    return;
}

void add_grade_to_gradebook(unsigned char *aname, unsigned char *fname, unsigned char *lname, int grade, Gradebook *gb) {
    // get index of student in *students
    // use that for index in *grades in the assignment

    int s_counter = 0;
    int student_index = -1;
    while (s_counter < gb->students_length && student_index < 0) {
        Student s = gb->students[s_counter];
        if (strcmp(s.fname, fname) == 0 && strcmp(s.lname, lname) == 0) {
            student_index = s_counter;
        }
        s_counter++;
    }

    if (student_index < 0) {
        // student not found, throw error
        printf("invalid\n");
        exit(255);
    }

    // find assignment and index *grades by student_index

    int a_counter = 0;
    int assignment_index = -1;
    while (a_counter < gb->assignments_length && assignment_index < 0) {
        Assignment a = gb->assignments[a_counter];
        if (strcmp(a.assignment_name, aname) == 0) {
            assignment_index = a_counter;
        }
        a_counter++;
    }

    if (assignment_index < 0) {
        // assignment not found, throw error
        printf("invalid\n");
        exit(255);
    }

    gb->assignments[assignment_index].grades[student_index] = grade;

    return;
}

void delete_all_grades_for_student(int student_index, Gradebook *gb) {
    // go through every assignment and delete grades[student_index]

    int a_counter;
    for (a_counter=0; a_counter < gb->assignments_length; a_counter++) {
        Assignment a = gb->assignments[a_counter];
        int new_grades_length = a.grades_length - 1;
        if (new_grades_length == 0) {
            a.grades = NULL;
            a.grades_length = 0;
        } else {
            int *new_grades = (int *)calloc(new_grades_length, sizeof(int));
            int old_g_counter = 0;
            int new_g_counter = 0;
            while (old_g_counter < a.grades_length) {
                if (old_g_counter != student_index) {
                    new_grades[new_g_counter] = a.grades[old_g_counter];
                    new_g_counter++;
                }
                old_g_counter++;
            }
            
            a.grades = new_grades;
            a.grades_length--;
        }

        gb->assignments[a_counter] = a;
    }

    return;
}

void delete_student_from_gradebook(unsigned char *fname, unsigned char *lname, Gradebook *gb) {
    // remove student from *students and remove that same index from every assignment's *grades
    
    int s_counter = 0;
    int student_index = -1;
    while (s_counter < gb->students_length && student_index < 0) {
        Student s = gb->students[s_counter];
        if (strcmp(s.fname, fname) == 0 && strcmp(s.lname, lname) == 0) {
            student_index = s_counter;
        }
        s_counter++;
    }

    if (student_index < 0) {
        // student not found, throw error
        printf("invalid\n");
        exit(255);
    }

    int new_students_length = gb->students_length - 1;
    if (new_students_length == 0) {
        gb->students = NULL;
        gb->students_length = 0;
        return;
    }
    Student *new_students = (Student *)calloc(new_students_length, sizeof(Student));
    
    int old_s_counter = 0;
    int new_s_counter = 0;
    while (old_s_counter < gb->students_length) {
        if (old_s_counter != student_index) {
            new_students[new_s_counter] = gb->students[old_s_counter];
            new_s_counter++;
        }
        old_s_counter++;
    }

    gb->students = new_students;
    gb->students_length = new_students_length;

    delete_all_grades_for_student(student_index, gb);
    
    return;
}

void delete_assignment_from_gradebook(unsigned char *aname, Gradebook *gb) {
    // remove assignment from *assignments
    // loop through to get index
    // malloc new ptr of old size - sifeof(Assignment)
    // loop through and add all assignments except for one at found index

    int a_counter = 0;
    int assignment_index = -1;
    while (a_counter < gb->assignments_length && assignment_index < 0) {
        if (strcmp(gb->assignments[a_counter].assignment_name, aname) == 0) {
            assignment_index = a_counter;
        }
        a_counter++;
    }

    if (assignment_index < 0) {
        // assignment not found, throw error
        printf("invalid\n");
        exit(255);
    }

    int new_assignments_length = gb->assignments_length - 1;
    if (new_assignments_length == 0) {
        gb->assignments = NULL;
        gb->assignments_length = 0;
        return;
    }
    Assignment *new_assignments = (Assignment *)calloc(new_assignments_length, sizeof(Assignment));

    int old_a_counter = 0;
    int new_a_counter = 0;
    while (old_a_counter < gb->assignments_length) {
        if (old_a_counter != assignment_index) {
            new_assignments[new_a_counter] = gb->assignments[old_a_counter];
            new_a_counter++;
        }
        old_a_counter++;
    }

    gb->assignments = new_assignments;
    gb->assignments_length = new_assignments_length;

    return;
}
