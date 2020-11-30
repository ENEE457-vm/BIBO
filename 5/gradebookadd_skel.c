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

//Gets all the relevant information from the command line
typedef struct _CmdLineResult {
  //Max gradebook name 100 characters
  unsigned char name[100];
  //Key is 32 characters
  unsigned char key[32];
  //Type is the flag inputted
  char type[2];
  int good;
  //Max assignment name 100 characters
  unsigned char assignment_name[100];
  int assignment_points;
  float assignemnt_weight;
  //Max student names 100 characters
  unsigned char student_FN[100];
  unsigned char student_LN[100];
  int grade;
} CmdLineResult;


CmdLineResult parse_cmdline(int argc, unsigned char *argv[]) {
  CmdLineResult R;
  R.assignemnt_weight = -1;
  R.assignment_points = -1;
  R.grade = -1;
  R.good = 0;
  R.name[0] = '\0';
  R.assignment_name[0] = '\0';
  R.student_FN[0] = '\0';
  R.student_LN[0] = '\0';
  if(argc <= 6) { 
      R.good = -1;
      return R;
  }
  else { 
      int counter;
      for(counter=1;counter<argc;counter++) 
        // -N
        if(counter == 1){
          if(strcmp(argv[1], "-N") != 0){
            R.good = -1;
            return (R);
          }
        }
        // Gradebook name
        else if(counter == 2){
          if(strlen(argv[2]) > 100){
            R.good = -1;
            return R;
          }
          strcpy(R.name, argv[2]);
        }
        // -K
        else if(counter == 3){
          if(strcmp(argv[3], "-K") != 0){
            R.good = -1;
            return (R);
          }
        }
        // Key string
        else if(counter == 4){
          int i;
          if(strlen(argv[4]) < 31 || strlen(argv[4]) > 34){
            R.good = -1;
            return R;
          }
          strcpy(R.key, argv[4]);
        }
        // {AA,DA,AS,DS,AG}
        else if(counter == 5){
          if(strcmp(argv[5], "-AA") == 0)
            strcpy(R.type, "AA");
          else if(strcmp(argv[5], "-DA") == 0)
            strcpy(R.type, "DA");
          else if(strcmp(argv[5], "-AS") == 0)
            strcpy(R.type, "AS");
          else if(strcmp(argv[5], "-DS") == 0)
            strcpy(R.type, "DS");
          else if(strcmp(argv[5], "-AG") == 0)
            strcpy(R.type, "AG");
          else {
            R.good = -1;
            return (R);
          }
        }
        else {
          // If we have -AA
          if(strcmp(R.type, "AA") == 0){
            //{AN,P,W}
            if(strcmp(argv[counter], "-AN") == 0){
              counter++;
              if(counter >= argc || strlen(argv[counter]) > 100){
                R.good = -1;

              }
              strcpy(R.assignment_name, argv[counter]);
            }
            else if(strcmp(argv[counter], "-P") == 0){
              counter++;
              if(counter >= argc){
                printf("invalid\n");
                R.good = -1;
                return R;
              }
              //Make sure points < 9999
              if(strlen(argv[counter]) > 4){
                printf("invalid\n");
                R.good = -1;
                return R;
              }
              R.assignment_points = atoi(argv[counter]);
            }
            else if(strcmp(argv[counter], "-W") == 0){
              counter++;
              if(counter >= argc){
                printf("invalid\n");
                R.good = -1;
                return R;
              }
              if(strlen(argv[counter]) > 6){
                printf("invalid\n");
                R.good = -1;
                return R;
              }
              R.assignemnt_weight = atof(argv[counter]);
            }
            else{
              R.good = -1;
              return (R);
            }
          }
          // If we have -DA
          else if(strcmp(R.type, "DA") == 0){
            //{AN}
            if(strcmp(argv[counter], "-AN") == 0){
              counter++;
              if(counter >= argc || strlen(argv[counter]) > 100){
                R.good = -1;
                return R;
              }
              strcpy(R.assignment_name, argv[counter]);
            }
            else{
              R.good = -1;
              return (R);
            }
          }
          //If we have -AS
          else if(strcmp(R.type, "AS") == 0){
            //{FN,LN}
            if(strcmp(argv[counter], "-FN") == 0){
              counter++;
              if(counter >= argc || strlen(argv[counter]) > 100){
                R.good = -1;
                return R;
              }
              strcpy(R.student_FN, argv[counter]);
            }
            else if(strcmp(argv[counter], "-LN") == 0){
              counter++;
              if(counter >= argc || strlen(argv[counter]) > 100){
                R.good = -1;
                return R;
              }
              strcpy(R.student_LN, argv[counter]);
            }
            else{
              R.good = -1;
              return (R);
            }
          }
          //If we have -DS
          else if(strcmp(R.type, "DS") == 0){
            //{FN,LN}
            if(strcmp(argv[counter], "-FN") == 0){
              if(counter >= argc || strlen(argv[counter]) > 100){
                R.good = -1;
                return R;
              }
              counter++;
              strcpy(R.student_FN, argv[counter]);
            }
            else if(strcmp(argv[counter], "-LN") == 0){
              counter++;
              if(counter >= argc || strlen(argv[counter]) > 100){
                R.good = -1;
                return R;
              }
              strcpy(R.student_LN, argv[counter]);
            }
            else{
              R.good = -1;
              return (R);
            }

          }
          //If we have -AG
          else if(strcmp(R.type, "AG") == 0){
            //{FN,LN,AN,G}
            if(strcmp(argv[counter], "-FN") == 0){
              counter++;
              if(counter >= argc || strlen(argv[counter]) > 100){
                R.good = -1;
                return R;
              }
              strcpy(R.student_FN, argv[counter]);
            }
            else if(strcmp(argv[counter], "-LN") == 0){
              counter++;
              if(counter >= argc || strlen(argv[counter]) > 100){
                R.good = -1;
                return R;
              }
              strcpy(R.student_LN, argv[counter]);
            }
            else if(strcmp(argv[counter], "-AN") == 0){
              counter++;
              if(counter >= argc || strlen(argv[counter]) > 100){
                R.good = -1;
                return R;
              }
              strcpy(R.assignment_name, argv[counter]);
            }
            else if(strcmp(argv[counter], "-G") == 0){
              counter++;
              if(counter >= argc){
                R.good = -1;
                return R;
              }
              if(strlen(argv[counter]) > 4){
                printf("invalid\n");
                R.good = -1;
                return R;
              }
              R.grade = atoi(argv[counter]);
            }
            else{
              R.good = -1;
              return (R);
            }
          }
        }

    } 
  //Make sure all relevant fields are filled
  if(strcmp(R.type, "AA") == 0 && (strlen(R.assignment_name) == 0 || R.assignment_points == -1 || R.assignemnt_weight == -1)){
    R.good = -1;
  }
  else if(strcmp(R.type, "DA") == 0 && strlen(R.assignment_name) == 0){
    R.good = -1;
  }
  else if(strcmp(R.type, "AS") == 0 && (strlen(R.student_FN) == 0 || strlen(R.student_LN) == 0)){
    R.good = -1;
  }
  else if(strcmp(R.type, "DS") == 0 && (strlen(R.student_FN) == 0 || strlen(R.student_LN) == 0)){
    R.good = -1;
  }
  else if(strcmp(R.type, "AG") == 0 && (strlen(R.student_FN) == 0 || strlen(R.student_LN) == 0 || strlen(R.assignment_name) == 0 || R.grade == -1)){
    R.good = -1;
  }
   return R;
}

//If -AA was chosen
int add_assignment(Gradebook *gb, unsigned char *name, int points, float weight){
  //Make sure that the assignment is not already in the gradebook
  int i;
  for(i = 0; i < gb->num_assignments; i++){
    if(strcmp(gb->assignments[i].name, name) == 0)
      return -1;
  }
  //Make sure we are not at the max number of assignments
  if(gb->num_assignments == 100){
    return -1;
  }
  //Copy The fields into the the new assignments location and add 1 assignment to the total number of assignments
  strcpy(gb->assignments[gb->num_assignments].name, name);
  gb->assignments[gb->num_assignments].total = points;
  gb->assignments[gb->num_assignments].weight = weight;
  gb->assignments[gb->num_assignments].num_students = 0;
  gb->num_assignments++;
  return 0;
}
//If -DA was chosen
int delete_assignment(Gradebook *gb, unsigned char *name){
  //Go through the array of assignments
  int i;
  for(i = 0; i < gb->num_assignments; i++){
    //If we have a match
    if(strcmp(gb->assignments[i].name, name) == 0){
      //Overwrite deleted assignment with current last assignment
      gb->assignments[i] = gb->assignments[gb->num_assignments -1];
      //decrement total number of assignments
      gb->num_assignments--;
      return 0;
    }
  }
  return -1;
}
//IF -AS was chosen
int add_student(Gradebook *gb, unsigned char *first, unsigned char *last){
  int i;
  //Make sure student doesn't already exist
  for(i = 0; i < gb->num_students; i++){
    if(strcmp(gb->students_first[i], first) == 0 && strcmp(gb->students_last[i], last) == 0)
      return -1;
  }
  //Make sure the number of students is not filled to the max
  if(gb->num_students == 1000){
    return -1;
  }
  //Copy first and last name and increment number of students
  strcpy(gb->students_first[gb->num_students], first);
  strcpy(gb->students_last[gb->num_students], last);
  gb->num_students++;
}
//If -DS was chosen
int delete_student(Gradebook *gb, unsigned char *first, unsigned char *last){
  //Go through the students
  int i, j, k;
  for(i = 0; i < gb->num_students; i++){
    //If we find a match
    if(strcmp(gb->students_first[i], first) == 0 && strcmp(gb->students_last[i], last) == 0){
      //Overwrite their information with the current last students information and decrement number of students
      strcpy(gb->students_first[i], gb->students_first[gb->num_students - 1]);
      strcpy(gb->students_last[i], gb->students_last[gb->num_students - 1]);
      gb->num_students--;
      //Go through all assignments
      for(j = 0; j < gb->num_assignments; j++){
        //Go through all students who have grades in current assignment
        for(k = 0; k < gb->assignments[j].num_students; k++){
          //If we have a match
          if(strcmp(gb->assignments[j].students_first[k], first) == 0 && strcmp(gb->assignments[j].students_last[k], last) == 0){
            ///Overwrite information with last students and overwrite points with last students, decrement number of students who have grades for assignment j
            strcpy(gb->assignments[j].students_first[k], gb->assignments[j].students_first[gb->assignments[j].num_students-1]);
            strcpy(gb->assignments[j].students_last[k], gb->assignments[j].students_last[gb->assignments[j].num_students-1]);
            gb->assignments[j].points[k] = gb->assignments[j].points[gb->assignments[j].num_students-1];
            gb->assignments[j].num_students--;
          }
        }
      }
      return 0;
    }
  }
  return -1;
}
//If -AG was chosen
int add_grade(Gradebook *gb, unsigned char *first, unsigned char *last, unsigned char *name, int grade){
  //Make sure the assignment and student exist
  int i, s_flag = -1, a_flag = -1;
  for(i = 0; i < gb->num_students; i++){
    if(strcmp(gb->students_first[i], first) == 0 && strcmp(gb->students_last[i], last) == 0){
      s_flag = i;
      break;
    }
  }
  for(i = 0; i < gb->num_assignments; i++){
    if(strcmp(gb->assignments[i].name, name) == 0){
      a_flag = i;
      break;
    }
  }
  //If both exist go through the students for the chosen assignment
  if(s_flag != -1 && a_flag != -1){
    for(i = 0; i < gb->assignments[a_flag].num_students; i++){
      //Check if the student already has a grade, if so just update the grade
      if(strcmp(gb->assignments[a_flag].students_first[i], first) == 0 && strcmp(gb->assignments[a_flag].students_last[i], last) == 0){
        gb->assignments[a_flag].points[i] = grade;
        return 0;
      }
    }
    //If not copy the students information and increment the number of students who have grades for this assignment
    strcpy(gb->assignments[a_flag].students_first[gb->assignments[a_flag].num_students], first);
    strcpy(gb->assignments[a_flag].students_last[gb->assignments[a_flag].num_students], last);
    gb->assignments[a_flag].points[gb->assignments[a_flag].num_students] = grade;
    gb->assignments[a_flag].num_students++;
    return 0;
  }
  return -1;
}

int main(int argc, unsigned char *argv[]) {
  //Get relevant command line args and make sure we don't have any bad information
  CmdLineResult R = parse_cmdline(argc, argv);
  if (R.good == 0) {
    static Gradebook new_gb;
    //decrypt and parse the encrypted gradebook, populate new_gb with all current values
    read_Gradebook_from_path(R.name, R.key, &new_gb);
    int valid = 0;
    //Call whichever function was chosen
    if(strcmp(R.type, "AA") == 0){
      valid = add_assignment(&new_gb, R.assignment_name, R.assignment_points, R.assignemnt_weight);
    }
    else if(strcmp(R.type, "DA") == 0){
      valid = delete_assignment(&new_gb, R.assignment_name);
    } 
    else if(strcmp(R.type, "AS") == 0){
      valid = add_student(&new_gb, R.student_FN, R.student_LN);
    } 
    else if(strcmp(R.type, "DS") == 0){
      valid = delete_student(&new_gb, R.student_FN, R.student_LN);
    }     
    else if(strcmp(R.type, "AG") == 0){
      valid = add_grade(&new_gb, R.student_FN, R.student_LN, R.assignment_name, R.grade);
    }
    else{
      printf("invalid\n");
      return(255);
    }    

    if(valid == -1){
      printf("invalid\n");
      return(255);
    }
    //Print the updated gradebook to a buffer
    static Buffer B = {0, 0};
    print_Gradebook(&new_gb, &B);

    //write the result back out to the file
    write_to_path(R.name, &B, R.key);
    return 0;
  }
  else {
    printf("invalid\n");
    return(255);
  }

  return(255);
}
