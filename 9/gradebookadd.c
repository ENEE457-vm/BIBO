#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "data.h"
#include "input.h"

/*
1.
gb name required: N
2.
gb key required: K
3.
assignment preceeded by add or delete: AA DA
student preceeded by add or delete: AS DS
grade preceeded by add: AG
4.
AA -> AN P W
DA -> AN
AS -> FN LN
DS -> FN LN
AG -> FN LN AN G
*/

int main(int argc, char *argv[]) {
    CmdLineResult R;

    R = parse_cmdline(argc, argv);

    if(R.good == 0) {
        Buffer B = read_from_path(R.gb_name, R.key);
        Gradebook *GB = (Gradebook *)malloc(sizeof(Gradebook));
        get_Gradebook(GB, &B);

        // if deleting or adding grade, assert names exist in GB or throw error
        switch(R.action) {
            case add_assignment:
                if (has_assignment(R.aname, GB)) {
                    printf("invalid\n");
                    return 255;
                }
                if (!sum_weights_valid(R.weight, GB)) {
                    printf("invalid\n");
                    return 255;
                }
                add_assignment_to_gradebook(R.aname, R.points, R.weight, R.weight_char_count, GB);
                break;
            case delete_assignment:
                delete_assignment_from_gradebook(R.aname, GB);
                break;
            case add_student:
                if (has_student(R.fname, R.lname, GB)) {
                    printf("invalid\n");
                    return 255;
                }
                add_student_to_gradebook(R.fname, R.lname, GB);
                break;
            case delete_student:
                delete_student_from_gradebook(R.fname, R.lname, GB);
                break;
            case add_grade:
                add_grade_to_gradebook(R.aname, R.fname, R.lname, R.grade, GB);
                break;
            default:
                printf("invalid\n");
                return 255;
        }
    
        Buffer P = print_Gradebook_to_buffer(GB);
        write_to_path(R.gb_name, &P, R.key);

        free(B.Buf);
        free(P.Buf);
        free(GB->students);
        free(GB->assignments);
    } else {
        printf("invalid\n");
        return 255;
    }

    return 0;
}
