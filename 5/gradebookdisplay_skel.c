#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "data.h"

//All relevant fields that could be used for command line args
typedef struct _CmdLineResult_display {
  unsigned char name[100];
  unsigned char key[100];
  char type[2];
  int good;
  int alpha_flag;
  int grade_flag;
  unsigned char assignment_name[100];
  unsigned char student_FN[100];
  unsigned char student_LN[100];
} CmdLineResult_display;

//If -PA was chosen, flag == 0 for sort by grade, flag == 1 for sort alphabetically
int print_assignment(Gradebook *gb, unsigned char *name, int flag) {
  if(gb->num_assignments == 0 || gb->num_students == 0)
    return 0;
  int i, j, found = -1;
  Assignment a;
  //Make sure the assignment exists
  for(i = 0; i < gb->num_assignments; i++){
    if(strcmp(gb->assignments[i].name, name) == 0){
      a = gb->assignments[i];
      found = 0;
      break;
    }
  }
  if(found == -1){
    printf("invalid\n");
    return -1;
  }
  //Bubble sort based on the students points for a particular assignment
  if(flag == 0){
    for(i = 0; i < a.num_students - 1; i++){
      int s = -1;
      for(j = 0; j < a.num_students - 1; j++){
        if(a.points[j] > a.points[j+1]){
          int temp_grade = a.points[j];
          char temp_f[100];
          char temp_l[100];
          s = 0;
          strcpy(temp_f, a.students_first[j]);
          strcpy(temp_l, a.students_last[j]);
          a.points[j] = a.points[j+1];
          strcpy(a.students_first[j], a.students_first[j+1]);
          strcpy(a.students_last[j], a.students_last[j+1]);
          strcpy(a.students_first[j+1], temp_f);
          strcpy(a.students_last[j+1], temp_l);
          a.points[j+1] = temp_grade;
        }
      }
      if(s == -1)
        break;
    }
    //Print sorted array
    for(i = a.num_students - 1; i >= 0; i--){
      printf("(%s, %s, %d)\n", a.students_last[i], a.students_first[i], a.points[i]);
    }
    return 0;
  }
  //Bubble sort based on students names for a particular assignment
  else{
    for(i = 0; i < a.num_students -1; i++){
      int s = -1;
      for(j = 0; j < a.num_students - 1; j++){
        if(strcmp(a.students_last[j], a.students_last[j+1]) > 0  || (strcmp(a.students_last[j], a.students_last[j+1]) == 0 && strcmp(a.students_first[j], a.students_first[j+1]) >0)){
          int temp_grade = a.points[j];
          char temp_f[100];
          char temp_l[100];
          s = 0;
          strcpy(temp_f, a.students_first[j]);
          strcpy(temp_l, a.students_last[j]);
          a.points[j] = a.points[j+1];
          strcpy(a.students_first[j], a.students_first[j+1]);
          strcpy(a.students_last[j], a.students_last[j+1]);
          strcpy(a.students_first[j+1], temp_f);
          strcpy(a.students_last[j+1], temp_l);
          a.points[j+1] = temp_grade;
        }
      }
      if(s == -1)
        break;
    }
    //Print sorted array
    for(i = 0; i < a.num_students; i++){
      printf("(%s, %s, %d)\n", a.students_last[i], a.students_first[i], a.points[i]);
    }
    return 0;
  }
}

//If -PS was chosen
int print_student(Gradebook *gb, unsigned char *first, unsigned char *last) {
  if(gb->num_assignments == 0 || gb->num_students == 0)
    return 0;
  int i, j, found = -1;
  //Make sure student exists
  for(i = 0; i < gb->num_students; i++){
    if(strcmp(first, gb->students_first[i]) == 0 && strcmp(last, gb->students_last[i]) == 0){
      found = 0;
      break;
    }
  }
  if(found == -1){
    return -1;
  }
  //Go through all assignments and if the students has a grade, print it out
  for(i = 0; i < gb->num_assignments; i++){
    for(j = 0; j < gb->assignments[i].num_students; i++){
      if(strcmp(first, gb->assignments[i].students_first[j]) == 0 && strcmp(last, gb->assignments[i].students_last[j]) == 0){
        printf("(%s, %d)\n", gb->assignments[i].name, gb->assignments[i].points[j]);
        break;
      }
    }
  }
  return 0;
}

//If -PF was chosen
int print_final(Gradebook *gb, int flag){
  if(gb->num_assignments == 0 || gb->num_students == 0)
    return 0;
  float scores[1000] = {0};
  float tot_weight = 0;
  int i, j, z;

  //Get total weight of all assignments
  for(i = 0; i < gb->num_assignments; i++)
    tot_weight += gb->assignments[i].weight;

  //Go through each students
  for(i = 0; i < gb->num_students; i++){
    //Go through each assignment
    for(j = 0; j < gb->num_assignments; j++){
      //Go through each student in a particular assignment
      for(z = 0; z < gb->assignments[j].num_students; z++){
        //If we have a match then add that score to the final tally (points earned/total points)*weight
        if(strcmp(gb->students_first[i], gb->assignments[j].students_first[z]) == 0 && strcmp(gb->students_last[i], gb->assignments[j].students_last[z]) == 0){
          scores[i] += (((float) gb->assignments[j].points[z])/gb->assignments[j].total)*gb->assignments[j].weight;
        }
      }
    }
  }

  //Bubble sort by scores, prints sorted array
  if(flag == 0){
    for(i = 0; i < gb->num_students - 1; i++){
      int s = -1;
      for(j = 0; j < gb->num_students -1; j++){
        if(scores[j] > scores[j+1]){
          float temp_grade = scores[j];
          char temp_f[100];
          char temp_l[100];
          s = 0;
          strcpy(temp_f, gb->students_first[j]);
          strcpy(temp_l, gb->students_last[j]);
          scores[j] = scores[j+1];
          strcpy(gb->students_first[j], gb->students_first[j+1]);
          strcpy(gb->students_last[j], gb->students_last[j+1]);
          strcpy(gb->students_first[j+1], temp_f);
          strcpy(gb->students_last[j+1], temp_l);
          scores[j+1] = temp_grade;
        }
      }
      if(s == -1)
        break;
    }    
    for(i = gb->num_students - 1; i >= 0; i--){
      printf("(%s, %s, %g)\n", gb->students_last[i], gb->students_first[i], scores[i]);
    }
  }
  //Bubble sort by name, prints sorted array
  else{
      for(i = 0; i < gb->num_students - 1; i++){
      int s = -1;
      for(j = 0; j < gb->num_students -1; j++){
        if(strcmp(gb->students_last[j], gb->students_last[j+1]) > 0 || (strcmp(gb->students_last[j], gb->students_last[j+1]) == 0 && strcmp(gb->students_first[j], gb->students_first[j+1]) > 0)){
          float temp_grade = scores[j];
          char temp_f[100];
          char temp_l[100];
          s = 0;
          strcpy(temp_f, gb->students_first[j]);
          strcpy(temp_l, gb->students_last[j]);
          scores[j] = scores[j+1];
          strcpy(gb->students_first[j], gb->students_first[j+1]);
          strcpy(gb->students_last[j], gb->students_last[j+1]);
          strcpy(gb->students_first[j+1], temp_f);
          strcpy(gb->students_last[j+1], temp_l);
          scores[j+1] = temp_grade;
        }
      }
      if(s == -1)
        break;
    }    

    for(i = 0; i < gb->num_students; i++){
      printf("(%s, %s, %g)\n", gb->students_last[i], gb->students_first[i], scores[i]);
    }
  }
  return 0;
}

//Parses commandline args and if everything checks out, calls a print function
int main(int argc, unsigned char *argv[]) {
  CmdLineResult_display R;
  R.good = 0;
  R.alpha_flag = -1;
  R.grade_flag = -1;
  R.name[0] = '\0';
  R.assignment_name[0] = '\0';
  R.student_FN[0] = '\0';
  R.student_LN[0] = '\0';
  //Must have executable, -N, gradebook name, -K, key, {PS,PA,PF} atleast
  if(argc < 5) {
      printf("invalid \n");
      return 255;
  } 
  else if(argc >= 5) { 
    int i;
    //Go through commandline args
    for(i=1;i<argc;i++){
      //-N
      if(i == 1){
        if(strcmp(argv[1], "-N") != 0){
          printf("invalid\n");
          return 255;
        }
      }
      //Gradebook name
      else if(i == 2){
        if(strlen(argv[2]) > 100){
          printf("invalid\n");
          return(255);
        }
        strcpy(R.name, argv[2]);
      }
      //-K
      else if(i == 3){
        if(strcmp(argv[3], "-K") != 0){
          printf("invalid\n");
          return 255;
        }
      }
      //Key
      else if(i == 4){
        if(strlen(argv[4]) > 33){
          printf("invalid\n");
          return 255;
        }
        strcpy(R.key, argv[4]);
      }
      //{PA,PS,PF}
      else if(i == 5){
        if(strcmp(argv[5], "-PA") == 0){
          strcpy(R.type, "PA");
        }
        else if(strcmp(argv[5], "-PS") == 0){
          strcpy(R.type, "PS");
        }
        else if(strcmp(argv[5], "-PF") == 0){
          strcpy(R.type, "PF");
        }
        else{
          printf("invalid\n");
          return 255;
        }
      }
      else{
        if(strcmp(R.type, "PA") == 0){
          //If we have PA we must have {AN,A||G}
          if(strcmp(argv[i], "-AN") == 0){
            i++;
            if(i >= argc || strlen(argv[i]) > 100){
              printf("invalid\n");
              return 255;
            }
            strcpy(R.assignment_name, argv[i]);
          }
          else if(strcmp(argv[i], "-A") == 0){
            R.alpha_flag = 0;
            R.grade_flag = -1;
          }
          else if(strcmp(argv[i], "-G") == 0){
            R.grade_flag = 0;
            R.alpha_flag = -1;
          }
          else{
            printf("invalid\n");
            return 255;
          }
        }
        else if(strcmp(R.type, "PS") == 0){
          //Must have {FN,LN}
          if(strcmp(argv[i], "-FN") == 0){
            i++;
            if(i >= argc || strlen(argv[i]) > 100){
              printf("invalid\n");
              return 255;
            }
            strcpy(R.student_FN, argv[i]);
          }
          else if(strcmp(argv[i], "-LN") == 0){
            i++;
            if(i >= argc || strlen(argv[i]) > 100){
              printf("invalid\n");
              return 255;
            }
            strcpy(R.student_LN, argv[i]);
          }
          else{
            printf("invalid\n");
            return 255;
          }
        }
        else if(strcmp(R.type, "PF") == 0){
          //Must have {A||G}
          if(strcmp(argv[i], "-A") == 0){
            R.alpha_flag = 0;
            R.grade_flag = -1;
          }
          else if(strcmp(argv[i], "-G") == 0){
            R.grade_flag = 0;
            R.alpha_flag = -1;
          }
          else{
            printf("invalid\n");
            return 255;
          }
        }
        else{
          printf("invalid\n");
          return 255;
        }
      }
    } 
  } 
  int valid = -1;
  static Gradebook gb;
  //Get the current gradebook
  read_Gradebook_from_path(R.name, R.key, &gb);
  if(strcmp(R.type, "PA") == 0){
    //Make sure assignment name is filled
    if(strlen(R.assignment_name) == 0){
      printf("invalid\n");
      return 255;
    }
    //Make sure they chose -A or -G
    if(R.alpha_flag == -1 && R.grade_flag == -1){
      printf("invalid\n");
      return 255;
    }
    if(R.grade_flag == 0)
      valid = print_assignment(&gb, R.assignment_name, 0);
    else
      valid = print_assignment(&gb, R.assignment_name, 1);
  }
  else if(strcmp(R.type, "PS") == 0){
    //Make sure first and last name are filled
    if(strlen(R.student_FN) == 0 || strlen(R.student_LN) == 0){
      printf("invalid\n");
      return 255;
    }
    valid = print_student(&gb, R.student_FN, R.student_LN);
  }
  else if(strcmp(R.type, "PF") == 0){
    //Make sure they chose either -A or -G
    if(R.alpha_flag == -1 && R.grade_flag == -1){
      printf("invalid\n");
      return 255;
    }
    if(R.grade_flag == 0)
      valid = print_final(&gb, 0);
    else
      valid = print_final(&gb, 1); 
  }

  if(valid == -1){
    printf("invalid\n");
    return 255;
  }
  return valid;

}
