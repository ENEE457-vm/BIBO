#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "input.h"
#include "data.h"

/*
1.
gb name required: N
2.
gb key required: K
3.
print assignment: PA
print student: PS
print final: PF
4.
PA -> AN A/G
PS -> GN LN
PF -> A/G 
*/

int not_visited(int *indices, int length, int index) {
    for (int i=0; i < length; i++) {
        if (indices[i] == index) {
            return 0;
        }
    }
    return 1;
}

void print_Assignment(char *aname, int order, Gradebook *GB) {
    // loop through a.grades and students with using same index
    int assignment_index = -1;
    int a_counter = 0;
    while (a_counter < GB->assignments_length && assignment_index < 0) {
        //printf("%s\n", GB->assignments[a_counter].assignment_name);
        if (strcmp(GB->assignments[a_counter].assignment_name, aname) == 0) {
            assignment_index = a_counter;
        }
        a_counter++;
    }

    if (assignment_index < 0) {
        printf("invalid\n");
        exit(255);
    }

    // best idea
    // malloc an array of indexes
    // also keep int length
    // make helper function to see if index has already been used
    // loop through and print lowest, ignore ones that have been used

    Assignment a = GB->assignments[assignment_index];
    int *indices_used = (int *)malloc(sizeof(a.grades));
    int indices_used_length = 0;
    if (order == 1) {
        // alphabetical
        //for (int ctr=0; ctr < GB->students_length; ctr++) {
        while (indices_used_length < GB->students_length) {
            for (int i=0; i < GB->students_length; i++) {
                if (not_visited(indices_used, indices_used_length, i)) {
                    int first_index = i;
                    for (int j=i+1; j < GB->students_length; j++) {
                        if (not_visited(indices_used, indices_used_length, j)) {
                            Student cmp_student = GB->students[j];
                            int cmp_lname = strcmp(GB->students[first_index].lname, cmp_student.lname);
                            if (cmp_lname > 0) {
                                first_index = j;
                            } else if (cmp_lname == 0) {
                                // compare fnames
                                int cmp_fname = strcmp(GB->students[first_index].fname, cmp_student.fname);
                                if (cmp_fname > 0) {
                                    first_index = j;                        
                                }
                            }
                        }
                    }
                    Student s = GB->students[first_index];
                    printf("(%s, %s, %d)\n", s.lname, s.fname, a.grades[first_index]);
                    indices_used[indices_used_length] = first_index;
                    indices_used_length++;
                    i = GB->students_length;
                }
            }
        }
    } else if (order == 2) {
        // grade
        for (int ctr=0; ctr < GB->students_length; ctr++) {
            int greatest = -1;
            int greatest_index = -1;
            for (int i=0; i < GB->students_length; i++) {
                if (a.grades[i] >= greatest && not_visited(indices_used, indices_used_length, i)) {
                    greatest = a.grades[i];
                    greatest_index = i;
                }
            }
            Student s = GB->students[greatest_index];
            printf("(%s, %s, %d)\n", s.lname, s.fname, greatest);
            indices_used[indices_used_length] = greatest_index;
            indices_used_length++;
        }
    } else {
        printf("invalid\n");
        exit(255);
    }
    
    //free(indices_used);
}

void print_Student(char *fname, char *lname, Gradebook *GB) {
    int student_index = -1;
    int i = 0;
    while (i < GB->students_length && student_index < 0) {
        Student s = GB->students[i];
        if (strcmp(s.fname, fname) == 0 && strcmp(s.lname, lname) == 0) {
            student_index = i;
        }
        i++;
    }

    if (student_index < 0) {
        printf("invalid\n");
        exit(255);
    }

    for (int a_counter=0; a_counter < GB->assignments_length; a_counter++) {
        Assignment a = GB->assignments[a_counter];
        printf("(%s, %d)\n", a.assignment_name, a.grades[student_index]);
    }
}

void print_Final(int order, Gradebook *GB) {
    // setup final grades array
    FinalGrade *final_grades = (FinalGrade *)malloc(sizeof(FinalGrade) * GB->students_length);
    for (int s_counter=0; s_counter < GB->students_length; s_counter++) {
        Student s = GB->students[s_counter];
        final_grades[s_counter].fname = (char *)malloc(strlen(s.fname) + 1);
        final_grades[s_counter].lname = (char *)malloc(strlen(s.lname) + 1);
        strcpy(final_grades[s_counter].fname, s.fname);
        strcpy(final_grades[s_counter].lname, s.lname);
        final_grades[s_counter].final = 0;
        for (int a_counter=0; a_counter < GB->assignments_length; a_counter++) {
            Assignment a = GB->assignments[a_counter];
            if (a.grades[s_counter] != -1) {
                float percentage = (float)a.grades[s_counter] / (float)a.points;
                final_grades[s_counter].final += percentage * a.weight;
            }
        }
    }
    
    int *indices_used = (int *)malloc(sizeof(int) * GB->students_length);
    int indices_used_length = 0;
    if (order == 1) {
        // alphabetically
        //for (int ctr=0; ctr < GB->students_length; ctr++) {
        while (indices_used_length < GB->students_length) {
            for (int i=0; i < GB->students_length; i++) {
                if (not_visited(indices_used, indices_used_length, i)) {
                    int first_index = i;
                    for (int j=i+1; j < GB->students_length; j++) {
                        if (not_visited(indices_used, indices_used_length, j)) {
                            int cmp_lname = strcmp(final_grades[first_index].lname, final_grades[j].lname);
                            //printf("test: %s, %s, %d", final_grades[first_index].lname, final_grades[j].lname, cmp_lname);
                            if (cmp_lname > 0) {
                                first_index = j;
                            } else if (cmp_lname == 0) {
                                // compare fnames
                                int cmp_fname = strcmp(final_grades[first_index].fname, final_grades[j].fname);
                                if (cmp_fname > 0) {
                                    first_index = j;                        
                                }
                            }
                        }
                    }
                    printf("(%s, %s, %f)\n", final_grades[first_index].lname, final_grades[first_index].fname, final_grades[first_index].final);
                    indices_used[indices_used_length] = first_index;
                    indices_used_length++;
                    i = GB->students_length;
                }
            }
        }
    } else if (order == 2) {
        // by grade
        for (int ctr=0; ctr < GB->students_length; ctr++) {
            float greatest = -1;
            int greatest_index = -1;
            for (int i=0; i < GB->students_length; i++) {
                if (final_grades[i].final >= greatest && not_visited(indices_used, indices_used_length, i)) {
                    greatest = final_grades[i].final;
                    greatest_index = i;
                }
            }
            printf("(%s, %s, %f)\n", final_grades[greatest_index].lname, final_grades[greatest_index].fname, greatest);
            indices_used[indices_used_length] = greatest_index;
            indices_used_length++;
        }
    } else {
        printf("invalid\n");
        exit(255);
    }

    // free final grades array memory
    for (int i=0; i < GB->students_length; i++) {
        free(final_grades[i].fname);
        free(final_grades[i].lname);
    }
    free(final_grades);
    // free(indices_used);
}


int main(int argc, char *argv[]) {
    CmdLineResult R;

    R = parse_cmdline(argc, argv);

    if(R.good == 0) {
        Buffer B = read_from_path(R.gb_name, R.key);
        Gradebook *GB = (Gradebook *)malloc(sizeof(Gradebook));
        get_Gradebook(GB, &B);

        switch(R.action) {
            case print_assignment:
                print_Assignment(R.aname, R.order, GB);
                break;
            case print_student:
                print_Student(R.fname, R.lname, GB);
                break;
            case print_final:
                print_Final(R.order, GB);
                break;
            default:
                printf("invalid\n");
                return 255;
        }

        free(B.Buf);
        free(GB->students);
        free(GB->assignments);
        free(GB);
    } else {
        printf("invalid\n");
        return 255;
    }

    return 0;
}
