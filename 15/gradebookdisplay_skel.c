#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <ctype.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "data.h"
#include "crypto.h"

//#define DEBUG

char* stoupper( char* s ){
    char* p = s;
    while (*p) {
        *p= toupper( *p ); 
        p++;
    }
    return s;
}

void swap (Student *S1,Student *S2)
{
    char last_name[maxLength+1];
    char first_name[maxLength+1];
    int *grade;
    float final_grade;

    strcpy(last_name,S1->last_name);
    strcpy(S1->last_name,S2->last_name);
    strcpy(S2->last_name,last_name);

    strcpy(first_name,S1->first_name);
    strcpy(S1->first_name,S2->first_name);
    strcpy(S2->first_name,first_name);

    grade = S1->grade;
    S1->grade = S2->grade;
    S2->grade = grade;

    final_grade = S1->final_grade;
    S1->final_grade = S2->final_grade;
    S2->final_grade = final_grade;   
    return;
}

void sort_students_alphabetically(Student *S){
  int swapped;
  Student *thisS = S;
  Student *lastS =NULL;
  char temp_first1[maxLength+1];
  char temp_first2[maxLength+1];
  char temp_last1[maxLength+1];
  char temp_last2[maxLength+1];
  do{
    swapped = 0;
    thisS = S;
    while(thisS->next != lastS){
      strcpy(temp_last1, thisS->last_name);
      strcpy(temp_last2, thisS->next->last_name);
      strcpy(temp_first1, thisS->first_name);
      strcpy(temp_first2, thisS->next->first_name);
      if( strcmp(stoupper(temp_last1), stoupper(temp_last2)) > 0 || 
        (strcmp(stoupper(temp_last1), stoupper(temp_last2))==0 && strcmp(stoupper(temp_first1), stoupper(temp_first2))>0)){
          swap(thisS,thisS->next);
          swapped =1;
      }
      thisS = thisS->next;
    }
    lastS = thisS;

  }while(swapped); 

  return;
}

void sort_students_by_grade(Student *S, int grade_idx){
  int swapped;
  Student *thisS = S;
  Student *lastS =NULL;
  do{
    swapped = 0;
    thisS = S;
    while(thisS->next != lastS){
      if( thisS->grade[grade_idx] < thisS->next->grade[grade_idx]){
          swap(thisS,thisS->next);
          swapped =1;
      }
      thisS = thisS->next;
    }
    lastS = thisS;

  }while(swapped); 

  return;
}

void sort_students_by_final_grade(Student *S){
  int swapped;
  Student *thisS = S;
  Student *lastS =NULL;
  do{
    swapped = 0;
    thisS = S;
    while(thisS->next != lastS){
      if( thisS->final_grade < thisS->next->final_grade){
          swap(thisS,thisS->next);
          swapped =1;
      }
      thisS = thisS->next;
    }
    lastS = thisS;

  }while(swapped); 

  return;
}

int print_Assignment(CmdLineResult *R, Gradebook *G) {
  int AN_position;
  sortType sort_type;
  Assignment *thisA = G->A;
  int grade_idx=0;
  int assignment_flag= invalid;
  Student *thisS = G->S;

  // find the asignment name position and sorting type
  for(int i=0 ; i< R->numActionOptions; i++){
    if(strcmp(&R->actionOption[i][0], "-AN")==0){
      AN_position = i;
    }
    else if(strcmp(&R->actionOption[i][0], "-A")==0){
      sort_type = alphabetOrder;
    }
    else if(strcmp(&R->actionOption[i][0], "-G")==0){
      sort_type = gradeOrder;
    }
  }

  if(G->numAssignments ==0 || G->numStudents ==0){
    return invalid;
  }
  // get the assignment position;
  while(thisA!=NULL){
    if(strcmp(thisA->name,&R->actionOptionSpecifier[AN_position][0])==0){
      assignment_flag = valid;
      break;
    }
    grade_idx++;
    thisA=thisA->next;
  }
  if(assignment_flag == invalid){
    return invalid; // assignment not found
  }

  switch(sort_type){
    case alphabetOrder: // sort the student names (last, first) in alphabet order and print grade for the assignment
      sort_students_alphabetically(G->S);
      while(thisS!=NULL){
        printf("(%s, %s, %d)\n", thisS->last_name, thisS->first_name, thisS->grade[grade_idx]);
        thisS = thisS->next;
      }
      break;
    case gradeOrder:
      sort_students_by_grade(G->S, grade_idx);
      while(thisS!=NULL){
        printf("(%s, %s, %d)\n", thisS->last_name, thisS->first_name, thisS->grade[grade_idx]);
        thisS = thisS->next;
      }
      break;
    default:
      return invalid;
  }

  return valid;
}

int print_Student(CmdLineResult *R, Gradebook *G) {
  int FN_position, LN_position;
  Assignment *thisA = G->A;
  int grade_idx=0;
  int student_flag= invalid;
  Student *thisS = G->S;
  // find the student first and last name position
  for(int i=0 ; i< R->numActionOptions; i++){
    if(strcmp(&R->actionOption[i][0], "-FN")==0){
      FN_position = i;
    }
    else if(strcmp(&R->actionOption[i][0], "-LN")==0){
      LN_position = i;
    }
  }

  // check if the student exists in the gradebook and find the student
  while(thisS!=NULL){
    if(strcmp(thisS->first_name,&R->actionOptionSpecifier[FN_position][0])==0 && strcmp(thisS->last_name,&R->actionOptionSpecifier[LN_position][0])==0){
      student_flag = valid;
      break;
    }
    thisS=thisS->next;
  }
  if(student_flag == invalid){
    return invalid; // student not found
  }

  while(thisA!=NULL){
    printf("(%s, %d)\n", thisA->name, thisS->grade[grade_idx]);
    thisA=thisA->next;
    grade_idx++;
  }
  return valid;
}

int print_Final(CmdLineResult *R, Gradebook *G){
  Assignment *thisA = G->A;
  Student *thisS = G->S;
  sortType sort_type;
  float final_grade;

    // find the sorting type
  for(int i=0 ; i< R->numActionOptions; i++){
    if(strcmp(&R->actionOption[i][0], "-A")==0){
      sort_type = alphabetOrder;
    }
    else if(strcmp(&R->actionOption[i][0], "-G")==0){
      sort_type = gradeOrder;
    }
  }

  // calculate the final grades
  while(thisS!=NULL){
    final_grade=0;
    thisA = G->A;
    for(int i = 0; i< G->numAssignments; i++){
      final_grade += ((float)thisS->grade[i]/(float)thisA->points)*thisA->weight;
      thisA=thisA->next;
    }
    thisS->final_grade = final_grade;
    thisS=thisS->next;
  }

  thisS= G->S;
  switch(sort_type){
    case alphabetOrder:
      sort_students_alphabetically(G->S);
      while(thisS!=NULL){
        printf("(%s, %s, %0.3f)\n", thisS->last_name, thisS->first_name, thisS->final_grade);
        thisS = thisS->next;
      }
      break;
    case gradeOrder:
      sort_students_by_final_grade(G->S);
      while(thisS!=NULL){
        printf("(%s, %s, %0.3f)\n", thisS->last_name, thisS->first_name, thisS->final_grade);
        thisS = thisS->next;
      }
      break;
    default:
      return invalid;
  }
  return valid;
}

int convert_ascii_key_to_hex(CmdLineResult R, unsigned char* key){
  unsigned char curr_key[3];
  curr_key[2]='\0';
  int j=0;
  for(int i=0; i<keyLength*2;i=i+2){
    curr_key[0]=R.key[i];
    curr_key[1]=R.key[i+1];
    key[j]=(unsigned char) strtol(curr_key,NULL,16);
    j++;
  }
  #ifdef DEBUG
    printf("key:\n");
    for (int i = 0; i <keyLength; i++){
      printf("%02x", key[i]);
    }
    printf("\n");
  #endif
  return 0;
}

int gradebook_authenticate(CmdLineResult R){
  FILE *fp;
  int file_size;
  unsigned char mac[macLength];
  unsigned char *computedmac;
  struct stat st;
  unsigned char hexkey[keyLength];
  convert_ascii_key_to_hex(R,hexkey);
  char *file_buff;

  // check file size
  stat(R.name, &st);
  file_size = st.st_size;
  if(file_size<macLength+ivLength){
    printf("invalid\n");
    return invalid;
  }
  file_buff = (char*) malloc(file_size*sizeof(char));
  #ifdef DEBUG
    printf("The file size is %d\n", file_size);
  #endif

  computedmac = (unsigned char *)malloc((file_size-macLength)*sizeof(unsigned char*));
  fp =fopen(R.name,"r");

  // Read in the MAC
  fread(mac,1,macLength,fp);
  #ifdef DEBUG
    printf("mac:\n");
    BIO_dump_fp (stdout, (const char *)mac, macLength);
  #endif

  // Read in the rest of the encrypted file
  fread(file_buff,1,file_size-macLength,fp);
  #ifdef DEBUG
    printf("iv and cipher: \n");
    BIO_dump_fp (stdout, (const char *)file_buff, file_size-macLength);
  #endif

  // Compute the MAC
  get_mac(file_buff, file_size-macLength, hexkey, computedmac);


  // Compare the computed MAC with the actual MAC
  if(memcmp(computedmac,mac,macLength)==0){
    #ifdef DEBUG
      printf("we have a match!\n");
    #endif
      free(computedmac);
      computedmac=NULL;
      fclose(fp);
      return valid;
  }

  // authentication failed, return invalid
  free(computedmac);
  computedmac=NULL;
  fclose(fp);
  printf("invalid\n");
  return invalid;
}

int decrypt_and_read_gradebook(CmdLineResult R, Gradebook *G){
  FILE *fp;
  int file_size;
  unsigned char iv[ivLength];
  unsigned char *encrypted_contents, *decrypted_contents;
  int encrypted_len, decrypted_len;
  struct stat st;
  unsigned char hexkey[keyLength];

  convert_ascii_key_to_hex(R,hexkey);
  // check file size
  stat(R.name, &st);
  file_size = st.st_size;
  if(file_size <macLength+ivLength){
    printf("invalid\n");
    return invalid;
  }
  #ifdef DEBUG
    printf("The file size is %d\n", file_size);
  #endif

  encrypted_len = file_size - macLength - ivLength;
  encrypted_contents = (unsigned char *)malloc((encrypted_len)*sizeof(unsigned char*));
  decrypted_contents = (unsigned char *)malloc((encrypted_len)*sizeof(unsigned char*));

  fp =fopen(R.name,"r");
  fseek(fp, 16, SEEK_SET);
  // Read in the IV
  fread(iv,1,ivLength,fp);
  #ifdef DEBUG
    printf("iv:\n");
    BIO_dump_fp (stdout, (const char *)iv, ivLength);
  #endif

  fread(encrypted_contents,1,encrypted_len,fp);
  fclose(fp);
  decrypted_len = decrypt(encrypted_contents, encrypted_len, hexkey, iv, decrypted_contents);
  // Add a NULL terminator. We are expecting printable text
  decrypted_contents[decrypted_len] = '\0';


    // Show the decrypted text
  #ifdef DEBUG
    printf("Decrypted text is:\n");
    printf("%s\n", decrypted_contents);
  #endif

  sscanf(decrypted_contents,"%*s %d %*s %d", &G->numStudents, &G->numAssignments);

  Assignment **lastA= &G->A;
  Assignment *newA;
  // Create the Assignments
  if(G->numAssignments != 0){
    // create the first assignment
    G->A = (Assignment*) malloc(sizeof(Assignment));
    lastA = &G->A;
    //creat the rest of the assignments
    for(int i =0; i< G->numAssignments -1 ; i++){
        newA = (Assignment *)malloc(sizeof(Assignment));
        (*lastA)->next = newA;
        lastA = &(*lastA)->next;
    }
    (*lastA) -> next =NULL;
  }
  else{
    (*lastA)=NULL;
  }

  Student **lastS = &G->S;
  Student *newS;
  int *newgrade;
  // Create the Students
  if(G->numStudents != 0){
    // create the first student
    G->S = (Student*) malloc(sizeof(Student));
    if(G->numAssignments!=0){
      newgrade = (int*) malloc(G->numAssignments*sizeof(int));
      G->S->grade =newgrade;
    }
    lastS = &G->S;
    //creat the rest of the students
    for(int i =0; i< G->numStudents -1 ; i++){
        newS = (Student *)malloc(sizeof(Student));
        if(G->numAssignments!=0){
          newgrade = (int*) malloc(G->numAssignments*sizeof(int));
          newS->grade =newgrade;
        }
        (*lastS)->next = newS;
        lastS = &(*lastS)->next;
    }
    (*lastS) -> next =NULL;
  }
  else{
    *lastS=NULL;
  }

  char *token;
  Assignment *thisA = G->A;
  Student *thisS = G->S;
  token = strtok(decrypted_contents," ");
// Fill the linked lists of assignment and students with the file data
  for(int i = 0; i< 3;i++){
        token = strtok(NULL," \n"); // ignore the header info
  }
  for(int i=0; i < G->numAssignments;i++){
    token = strtok(NULL," \n");
    strcpy(thisA->name,token);
    token = strtok(NULL," \n");
    thisA->points = (int) strtol(token,NULL,10);
    token = strtok(NULL," \n");
    thisA->weight = (float) strtof(token,NULL);
    thisA=thisA->next;
  }

  for(int i=0; i < G->numStudents; i++){
    token = strtok(NULL," \n");
    strcpy(thisS->first_name, token);
    token = strtok(NULL," \n");
    strcpy(thisS->last_name, token);
    for(int j = 0; j < G->numAssignments; j++){
      token = strtok(NULL," \n");
      thisS->grade[j] = (int) strtol(token, NULL, 10);
    }
    thisS=thisS->next;
  }

  return 0;
}

int print_gradebook(CmdLineResult R, Gradebook *G){

  if(strcmp(R.action, "-PA")==0){
    if(print_Assignment(&R, G)==invalid){
      #ifdef DEBUG
        printf("something went wrong printing assignment\n");
      #endif
      printf("invalid\n");
      return invalid;
    }
  }
  else if(strcmp(R.action, "-PS")==0){
    if(print_Student(&R, G)==invalid){
      #ifdef DEBUG
        printf("something went wrong printing student\n");
      #endif
      printf("invalid\n");
      return invalid;
    }    
  }
  else if(strcmp(R.action, "-PF")==0){
    if(print_Final(&R, G)==invalid){
      #ifdef DEBUG
        printf("something went wrong printing final\n");
      #endif
      printf("invalid\n");
      return invalid;
    }    
  }
  else{
    #ifdef DEBUG
      printf("something went wrong printing gradebook\n");
    #endif
    printf("invalid\n");
    return invalid;
  }
  return valid;
}

int main(int argc, char *argv[]) {
  CmdLineResult R;
  Gradebook G;
  R = parse_cmdline(argc, argv, displayType);
   // printCmdLineResults(R);
if(R.good == valid) {
     // Authenticate the gradebook
     if(gradebook_authenticate(R)==invalid){
       return(255);
     }
    // Decrypt the gradebook and read
    if(decrypt_and_read_gradebook(R, &G)==invalid){
      return(255);
    }

    // Print the gradebook
    if(print_gradebook(R, &G)==invalid){
      //free_memory(&G);
      return(255);
    }   
  }
  if(R.good==invalid){
    return(255);
  }
  return 0;
}
