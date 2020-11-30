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

#include "data.h"

/* MISCELLANEOUS*/

// check if a file contain an entry, return 1 if it does and 0 otherwise
int fileContain(char *str, char* filename) {
  FILE *fp = fopen(filename, "r");
  int flag = 0;
  char infile[100];
    while (fgets(infile, 100, fp) != NULL) {
      if (strstr(infile, str) != NULL) {
        flag = 1;
        break;
      }
    }
  fclose(fp);

  return flag;
}

int compareByGrade(const void *a, const void *b) {
  Student *studentA = (Student *) a;
  Student *studentB = (Student *) b;

  return (studentB->grade - studentA->grade);
}

int compareByName(const void *a, const void *b) {
  Student *studentA = (Student *) a;
  Student *studentB = (Student *) b;

  if (strcmp(studentA->lastName, studentB->lastName)==0){
    return strcmp(studentA->firstName, studentB->firstName);
  } else {
    return strcmp(studentA->lastName, studentB->lastName);
  }
}

int compareByFinal(const void *a, const void *b) {
  Student *studentA = (Student *) a;
  Student *studentB = (Student *) b;

  float finalA = studentA->final;
  float finalB = studentB->final;

  if (finalA<finalB){
    return 1;
  } else {
    return -1;
  }
}

/* FUNCTIONS DEFINITIONS */

int printAssignment(char *assiName, char *filename, int option) {
  FILE *fp;
  char line[200];
  char buffer[1000]="";

  // creating an assignment to scan if the assignment exist
  char assignmentToCheck[100];
  strcpy(assignmentToCheck, "(Assignment,");
  strcat(assignmentToCheck, assiName);
  strcat(assignmentToCheck, ",");

  if (fileContain(assignmentToCheck, filename)==0) {
    printf("invalid\n");
    return(255);
  }

  // count the number of student
  fp = fopen(filename, "r");
  int stuCount = 0;
  while (fgets(line, 200, fp) != NULL) {
    if (strstr(line, "(Student,") != NULL) {
      stuCount++;
    }
  }
  fclose(fp);

  // creating the flag to scan for the assignment
  fp = fopen(filename, "r");
  // creating an empty student list 
  Student list[stuCount];
  int i = 0;

  char flag[100];
  strcpy(flag, "(Grade,");
  strcat(flag, assiName);
  strcat(flag, ",");

  while (fgets(line, 200, fp) != NULL) {
    if (strstr(line, flag) != NULL) {
      
      int fi = 0;
      // parsing the line with commas
      char *ptr;
      ptr = strtok (line,","); // comma separate the line
      
      while (ptr != NULL) {

        
        if (atoi(ptr)==0) {
          if ( strcmp(ptr, "(Grade")!=0 && strcmp(ptr, assiName)!=0 ) {
            
            if (fi==0){ // first name is not read
              strcpy(list[i].firstName, ptr);
              fi++;
            } else if (fi==1) {
              strcpy(list[i].lastName, ptr);
            }

          }
        } else {
          list[i].grade = atoi(ptr);
        }

        ptr = strtok (NULL, ","); // get the next token
      }

      i = i + 1;
    }
  }

  

  // sorting the student list
  if (option==1) {
    qsort(list, stuCount, sizeof(Student), compareByName);
  } else if (option==2) {
    qsort(list, stuCount, sizeof(Student), compareByGrade);
  }

  // add the student to the buffer to print

  for (int n=0; n<stuCount; n++) {
    
    char g[10];

    strcat(buffer, "(");
    strcat(buffer, list[n].lastName);
    strcat(buffer, ", ");
    strcat(buffer, list[n].firstName);
    strcat(buffer, ", ");
    sprintf(g, "%d", list[n].grade);
    strcat(buffer, g);
    strcat(buffer, ")\n");
  }

  printf("%s", buffer);

  return 0;
}

int printStudent(char *first, char *last, char *filename) {
  FILE *fp = fopen(filename, "r");
  char line[200];
  char buffer[1000]="";

  // creating a student to scan if the student exist
  char studentToCheck[100];
  strcpy(studentToCheck, "(Student,");
  strcat(studentToCheck, first);
  strcat(studentToCheck, ",");
  strcat(studentToCheck, last);
  strcat(studentToCheck, ")\n");

  if (fileContain(studentToCheck, filename)==0) {
    printf("invalid\n");
    return(255);
  }

  // creating a flag to scan for all assignments with the student
  char flag[100];
  strcpy(flag, ",");
  strcat(flag, first);
  strcat(flag, ",");
  strcat(flag, last);
  strcat(flag, ",");

  while (fgets(line, 200, fp) != NULL) {
    if (strstr(line, flag) != NULL) { // if a line with the flag is found
      
      char *ptr;
      ptr = strtok (line,","); // comma separate the line
      while (ptr != NULL) {

        if (atoi(ptr) == 0) {
          if (strcmp(ptr, "(Grade")!=0 && strcmp(ptr, first)!=0 && strcmp(ptr, last)!=0) {
            strcat(buffer, "(");
            strcat(buffer, ptr);
            strcat(buffer, ", ");
          }
        } else {
          strcat(buffer, ptr);
        }
        
        ptr = strtok (NULL, ","); // get the next token
      }
    }
  }

  printf("%s", buffer);
  return 0;
}

int printFinal(char *filename, int option) {
  FILE *fp;
  char line[200];

  // count number of assignment
  fp = fopen(filename, "r");
  int assiCount = 0;
  while (fgets(line, 200, fp) != NULL) {
    if (strstr(line, "(Assignment,") != NULL) {
      assiCount++;
    }
  }
  fclose(fp);


  // parsing the assignment info
  fp = fopen(filename, "r");
  int i = 0;
  float flist[assiCount];
  int totList[assiCount];
  char assiList[assiCount][100];

  while (fgets(line, 200, fp) != NULL) {
    if (strstr(line, "(Assignment") != NULL) {

      int flag = 0;
      char *ptr;
      ptr = strtok (line,","); // comma separate the line
      while (ptr != NULL) {

        if (flag==1) {
          strcpy(assiList[i], ptr);
        } else if (flag==2) {
          totList[i] = atoi(ptr);
        } else if (flag==3) {
          flist[i] = atof(ptr);
        }

        flag++;

        ptr = strtok (NULL, ","); // get the next token
      }

      i = i+1;
    }
  }

  fclose(fp);

  i=0;
  for (i; i<assiCount; i++) {
    flist[i] = flist[i]/totList[i];
  }

  // count the number of student
  fp = fopen(filename, "r");
  int stuCount = 0;
  while (fgets(line, 200, fp) != NULL) {
    if (strstr(line, "(Student,") != NULL) {
      stuCount++;
    }
  }

  fclose(fp);

  // creating an empty student list 
  Student list[stuCount];
  
  for (i=0; i<assiCount; i++) { // for each asisgnment
    fp = fopen(filename, "r");
    char assiName[100];
    strcpy(assiName, assiList[i]);
    char fs[100];
    strcpy(fs, "(Grade,");
    strcat(fs, assiName);
    strcat(fs, ",");

    int j = 0;
    while (fgets(line, 200, fp) != NULL) {
      if (strstr(line, fs) != NULL) {
        
        int fi = 0;
        // parsing the line with commas
        char *ptr;
        ptr = strtok (line,","); // comma separate the line
        
        while (ptr != NULL) {

          
          if (atoi(ptr)==0) {
            if ( strcmp(ptr, "(Grade")!=0 && strcmp(ptr, assiName)!=0 ) {
              
              if (fi==0){ // first name is not read
                strcpy(list[j].firstName, ptr);
                fi++;
              } else if (fi==1) {
                strcpy(list[j].lastName, ptr);
              }

            }
          } else {
            list[j].grade = atoi(ptr);
          }

          ptr = strtok (NULL, ","); // get the next token
        }

        list[j].final += list[j].grade*flist[i];
        j = j + 1;
      }
    }
    fclose(fp);
  }

  // sorting the student list
  if (option==1) {
    qsort(list, stuCount, sizeof(Student), compareByName);
  } else if (option==2) {
    qsort(list, stuCount, sizeof(Student), compareByFinal);
  }

  // add the student to the buffer to print


  char buffer[1000]="";

  for (int n=0; n<stuCount; n++) {
    
    char g[10];

    strcat(buffer, "(");
    strcat(buffer, list[n].lastName);
    strcat(buffer, ", ");
    strcat(buffer, list[n].firstName);
    strcat(buffer, ", ");
    sprintf(g, "%.4f", list[n].final);
    strcat(buffer, g);
    strcat(buffer, ")\n");
  }

  printf("%s", buffer);
  
  return 0;
}


int main(int argc, char *argv[]) {
  // init the params
  char *filename;
  FILE *fp;
  unsigned char *key;

  // TODO change example usage to gradebookdisplay
  if (argc < 5) {
    printf("Not enough argument to proceed.\n"
      "Example Usage:\n"
      "gradebookdisplay -N [gradebookName] -K [key] -PS -FN [firstName] -LN [lastName] --> Display a student\n"
      "gradebookdisplay -N [gradebookName] -K [key] -PA (-A OR -G) -AN [assignmentName] --> Display an assignment\n"
      "gradebookdisplay -N [gradebookName] -K [key] -PF (-A OR -G) --> Print final grade\n");

    printf("\nInvalid\n");
    return(255);
  } // if name and key were not provided


  if ( strcmp(argv[1], "-N")==0 && strcmp(argv[3], "-K")==0 ) {

    filename = argv[2];
    key = argv[4];

    //check if file exist

    /*function to decrypt file here*/


    // parse cmd
    if ( argc==10 && strcmp(argv[5], "-PS")==0 && strcmp(argv[6], "-FN")==0 && strcmp(argv[8], "-LN")==0 ) {
      char *first = argv[7];
      char *last = argv[9];

      if (strlen(first) > MAX_NAME_LENGTH || strlen(last) > MAX_NAME_LENGTH) {
        printf("invaid\n");
        return(255);
      }

      printStudent(first, last, filename);

    } else if ( argc==9 && strcmp(argv[5], "-PA")==0 && strcmp(argv[7], "-AN")==0 ) {
      char *assiName = argv[8];      

      if (strlen(assiName) > MAX_NAME_LENGTH) {
        printf("invaid\n");
        return(255);
      }

      if (strcmp(argv[6], "-A")==0) {
        // sort by alphabetical
        printAssignment(assiName, filename, 1);  

      } else if (strcmp(argv[6], "-G")==0) {
        // sort by grade
        printAssignment(assiName, filename, 2);

      }

    } else if (strcmp(argv[5], "-PF")==0 ) {

      if (strcmp(argv[6], "-A")==0) {
        // sort by alphabetical
        printFinal(filename, 1);

      } else if (strcmp(argv[6], "-G")==0) {
        // sort by grade
        printFinal(filename, 2);

      }

    } else {
      printf("Invalid.\n");
      return(255);
    }

  }
}
