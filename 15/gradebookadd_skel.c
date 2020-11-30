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
#include <openssl/conf.h>
#include <openssl/evp.h>

#include "data.h"
#include "crypto.h"

//#define DEBUG


int print_assignment(Gradebook *G){
  Assignment *thisA = G->A;
 do{
    printf("Assignment Name: %s \t Points: %d \t Weight: %f\n", thisA->name, thisA->points, thisA->weight);
    thisA= thisA->next;
  } while(thisA !=NULL);

return valid;
}

int addAssignment(CmdLineResult R, Gradebook *G){
  Assignment *newA;
  Assignment **thisA = &G->A;
  Student *thisS = G->S;
  int AN_position;
  float weight;
  
  // First assignment creation case
  if(G->numAssignments==0){
    newA = (Assignment *) malloc(sizeof(Assignment));
    (*thisA)=newA;
    (*thisA)->next =NULL;
    G->numAssignments++;
    // increase the grade buffer for each student
    if(G->numStudents!=0){
      while(thisS != NULL){
        thisS->grade = (int*) realloc(thisS->grade, G->numAssignments*sizeof(int));
        thisS->grade[G->numAssignments-1]=0;
        thisS=thisS->next;
      }
    }
  }
  // else: check if the assignment already exists, reach the end of the List to add an assignment
  else{
    // Get the assignment name position and weight position
    for(int i=0; i<R.numActionOptions; i++){
      if(strcmp(&R.actionOption[i][0], "-AN")==0){
        AN_position = i;
      }
      else if(strcmp(&R.actionOption[i][0], "-W")==0){
        weight = (float) strtof(&R.actionOptionSpecifier[i][0],NULL);
      }
    }
    while(*thisA !=NULL){
      // invalid if the assignment already exists in the gradebook
      if(strcmp((*thisA)->name, &R.actionOptionSpecifier[AN_position][0])==0){
        return invalid;
      }
      weight += (*thisA)->weight;
      thisA=&(*thisA)->next;
    }
    //weights cannot add up to more than 1
    if(weight>1){
      return invalid;
    }
    // everything is good, add the assignment
    G->numAssignments++;
    newA = (Assignment *) malloc(sizeof(Assignment));
    (*thisA)= newA;
    (*thisA)->next = NULL;
    // increase the grade buffer for each student
    if(G->numStudents!=0){
      while(thisS != NULL){
        thisS->grade = (int*) realloc(thisS->grade, G->numAssignments*sizeof(int));
        thisS->grade[G->numAssignments-1]=0;
        thisS=thisS->next;
      }
    }
  }

  // search command line results for action option and insert the corresponding specifiers into the assignement
  for(int i=0; i<R.numActionOptions; i++){
    if(strcmp(&R.actionOption[i][0], "-AN")==0){
      strcpy((*thisA)->name, &R.actionOptionSpecifier[i][0]);
    }
    else if(strcmp(&R.actionOption[i][0], "-P")==0){
      (*thisA)->points = strtol(&R.actionOptionSpecifier[i][0], NULL,10);
    }
    else if(strcmp(&R.actionOption[i][0], "-W")==0){
      (*thisA)->weight = atof(&R.actionOptionSpecifier[i][0]);
    }
  }

  return valid;
}

int deleteAssignment(CmdLineResult R, Gradebook *G){
  Assignment **thisA= &G->A;
  Assignment *temp;
  Student *thisS = G->S;
  int AN_position;
  int assignment_pos=0;

  if(G->numAssignments ==0){
    return invalid;
  }
    // Get the assignment name position
  for(int i=0; i<R.numActionOptions; i++){
    if(strcmp(&R.actionOption[i][0], "-AN")==0){
      AN_position = i;
      break;
    }
  }

  // Delete the Assignment node and free the memory
  while(*thisA !=NULL){
    // find the assignment in the assignment list
    if(strcmp((*thisA)->name, &R.actionOptionSpecifier[AN_position][0])==0){
      temp = *thisA;
      *thisA = (*thisA)->next;
      free(temp);
      temp=NULL;
      G->numAssignments--;
      // remove the assignment grade from the student
      if(G->numStudents!=0){
        while(thisS != NULL){
          if(assignment_pos < G->numAssignments-1){
            memmove(thisS->grade+assignment_pos, thisS->grade+assignment_pos+1,G->numAssignments - (assignment_pos+1) );
          }
          thisS->grade = (int*) realloc(thisS->grade, G->numAssignments*sizeof(int));
          thisS=thisS->next;
        }
      }
      return valid;
    }
    thisA=&(*thisA)->next;
    assignment_pos++;
  }
  return invalid;
}

int addStudent(CmdLineResult R, Gradebook *G){
  Student *newS;
  Student **thisS = &G->S;
  int FN_position, LN_position;
  
  // First student creation case
  if(G->numStudents==0){
    newS = (Student *) malloc(sizeof(Student));
    // initialize the grades to -1 for each assignment for the student
    if(G->numAssignments!=0){
      newS->grade = (int*) malloc(G->numAssignments*sizeof(int));
      for(int i =0; i < G->numAssignments; i++){
        newS->grade[i] = 0;
      }
    }
    else{
      newS->grade = NULL;
    }
    (*thisS)=newS;
    (*thisS)->next =NULL;
    G->numStudents++;
  }
  // else: check if the student already exists, reach the end of the List to add the student
  else{
    // Get the first and last name position
    for(int i=0; i<R.numActionOptions; i++){
      if(strcmp(&R.actionOption[i][0], "-FN")==0){
        FN_position = i;
      }
      else if(strcmp(&R.actionOption[i][0], "-LN")==0){
        LN_position =i;
      }
    }
    while(*thisS !=NULL){
      // invalid if the student already exists in the gradebook
      if(strcmp((*thisS)->first_name, &R.actionOptionSpecifier[FN_position][0])==0 &&
               strcmp((*thisS)->last_name, &R.actionOptionSpecifier[LN_position][0])==0){
        return invalid;
      }
      thisS=&(*thisS)->next;
    }
    // create new student
    G->numStudents++;
    newS = (Student *) malloc(sizeof(Student));
    if(G->numAssignments!=0){
      newS->grade = (int*) malloc(G->numAssignments*sizeof(int));
      for(int i =0; i < G->numAssignments; i++){
        newS->grade[i] = 0;
      }
    }
    else{
      newS->grade = NULL;
    }
    (*thisS)= newS;
    (*thisS)->next = NULL;
  }

  // search command line results for first and last name positions and insert the corresponding name into the assignement
  for(int i=0; i<R.numActionOptions; i++){
    if(strcmp(&R.actionOption[i][0], "-FN")==0){
       strcpy((*thisS)->first_name, &R.actionOptionSpecifier[i][0]);
    }
    else if(strcmp(&R.actionOption[i][0], "-LN")==0){
       strcpy((*thisS)->last_name, &R.actionOptionSpecifier[i][0]);
    }
  }
         
  return valid;
}
int deleteStudent(CmdLineResult R, Gradebook *G){
  Student **thisS= &G->S;
  Student *temp;
  int FN_position;
  int LN_position;

  if(G->numStudents ==0){
    return invalid;
  }
    // Get the first and last name position
  for(int i=0; i<R.numActionOptions; i++){
    if(strcmp(&R.actionOption[i][0], "-FN")==0){
      FN_position = i;
    }
    else if(strcmp(&R.actionOption[i][0], "-LN")==0){
      LN_position = i;
    }
  }

  // Delete the Student node and free the memory
  while(*thisS !=NULL){
    // find the student in the student list
    if(strcmp((*thisS)->first_name, &R.actionOptionSpecifier[FN_position][0])==0 &&
          strcmp((*thisS)->last_name, &R.actionOptionSpecifier[LN_position][0])==0){
      temp = *thisS;
      *thisS = (*thisS)->next;
      free(temp->grade);
      free(temp);
      temp=NULL;
      G->numStudents--;
      return valid;
    }
    thisS=&(*thisS)->next;
  }
  // Student was not found in the list, invalid
  return invalid;
}

int addGrade(CmdLineResult R, Gradebook *G){
  int FN_position, LN_position, AN_position;
  Assignment *thisA = G->A;
  int assignment_idx=0;
  int flag =invalid;
  Student *thisS = G->S;
  int grade;

    // Get the first, last, and assignment name position
  for(int i=0; i<R.numActionOptions; i++){
    if(strcmp(&R.actionOption[i][0], "-FN")==0){
      FN_position = i;
    }
    else if(strcmp(&R.actionOption[i][0], "-LN")==0){
      LN_position = i;
    }
    else if(strcmp(&R.actionOption[i][0], "-AN")==0){
      AN_position = i;
    }
    else if(strcmp(&R.actionOption[i][0], "-G")==0){
      grade = (int) strtol(&R.actionOptionSpecifier[i][0],NULL,10);
    }
  }
  // Check if the assignment exists and get its position
  while(thisA!=NULL){
    if(strcmp(thisA->name, &R.actionOptionSpecifier[AN_position][0])==0){
      flag = valid;
      break;
    }
    thisA=thisA->next;
    assignment_idx++;
  }
  if(flag == invalid){
    return invalid;
  }

  // find the student in the list
  while(thisS !=NULL){
    // find the student in the student list
    if(strcmp(thisS->first_name, &R.actionOptionSpecifier[FN_position][0])==0 &&
          strcmp(thisS->last_name, &R.actionOptionSpecifier[LN_position][0])==0){
        thisS->grade[assignment_idx]= grade;
      return valid;
    }
    thisS=thisS->next;
  }

  // Student was not found in the list, invalid
  return invalid;
}

int modify_gradebook(CmdLineResult R, Gradebook *G){

  if(strcmp(R.action, "-AA")==0){
    if(addAssignment(R, G)==invalid){
      #ifdef DEBUG
        printf("Assignment already exists or weights add up to more than 1\n");
      #endif
      printf("invalid\n");
      return invalid;
    }
  }
  else if(strcmp(R.action, "-DA")==0){
    if(deleteAssignment(R,G)==invalid){
      #ifdef DEBUG
        printf("Could not delete assignment\n");
      #endif
      printf("invalid\n");
      return invalid;
    }
  }
  else if(strcmp(R.action, "-AS")==0){
    if(addStudent(R,G)==invalid){
      #ifdef DEBUG
        printf("Could not add the student\n");
      #endif
      printf("invalid\n");
      return invalid;
    }
  }
  else if(strcmp(R.action, "-DS")==0){
    if(deleteStudent(R,G)==invalid){
      #ifdef DEBUG
        printf("Could not delete the student\n");
      #endif
      printf("invalid\n");
      return invalid;
    }
  }
  else if(strcmp(R.action, "-AG")==0){
    if(addGrade(R,G)==invalid){
      #ifdef DEBUG
        printf("Could not add the grade\n");
      #endif
      printf("invalid\n");
      return invalid;
    }
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

int encrypt_and_write_gradebook(CmdLineResult R, Gradebook *G){
  int strptr;
  char buff[maxLength+1];
  char *filestr;
  int filestr_length;
  char *encryption;
  int encryption_length;
  unsigned char hexkey[keyLength];
 // unsigned char key[16] ={0};
  int iv_length = 16;
  char *mac;
  int mac_length;
  FILE *fp;
  Assignment *thisA;
  Student *thisS;
  int numchars;

  convert_ascii_key_to_hex(R,hexkey);
  // Get the number of students in ascii form to calculate the number of characters
  sprintf(buff, "%d", G->numStudents); // numStudents has max number of chars = maxLength
  // calculate the number of bytes for the string
  filestr_length = strlen("numStudents ") +strlen(buff);
  strptr = filestr_length;

  filestr = (char*) malloc((filestr_length+1)*sizeof(char));
  sprintf(filestr, "numStudents %s", buff);

  // Get the number of assignments in ascii form
  sprintf(buff, "%d", G->numAssignments);
  // calculate the number of bytes for the string
  filestr_length += strlen(" numAssignments ")+strlen(buff)+strlen("\n");

  filestr = (char*) realloc(filestr, (filestr_length+1)*sizeof(char));
  // store the string at the location directly after the last string
  sprintf(filestr+strptr, " numAssignments %s\n",buff);
  strptr = filestr_length;


  if(G->numAssignments!=0){
      thisA = G->A;
      while(thisA!=NULL){
        sprintf(buff, "%d", thisA->points);
        filestr_length += strlen(thisA->name) + strlen(" ") + strlen(buff) + strlen(" ");
        filestr = (char*) realloc(filestr, (filestr_length+1)*sizeof(char)); 
        sprintf(filestr+strptr, "%s %s ", thisA->name, buff);
        strptr = filestr_length;

        sprintf(buff, "%f", thisA->weight);
        filestr_length += strlen(buff) + strlen("\n");
        filestr = (char*) realloc(filestr, (filestr_length+1)*sizeof(char));
        sprintf(filestr+strptr, "%s\n",buff);
        strptr = filestr_length;

        thisA=thisA->next;
      }
  }
  if(G->numStudents!=0){
      thisS = G->S;
      while(thisS!=NULL){
        filestr_length += strlen(thisS->first_name) + strlen(" ") + strlen(thisS->last_name) + strlen(" ");
        filestr = (char*) realloc(filestr, (filestr_length+1)*sizeof(char)); 
        sprintf(filestr+strptr,"%s %s ", thisS->first_name, thisS->last_name);
        strptr = filestr_length;
        for(int i=0; i < G->numAssignments; i++){
            sprintf(buff, "%d", thisS->grade[i]);
            if(i==G->numAssignments-1){
              filestr_length += strlen(buff) + strlen("\n");
              filestr = (char*) realloc(filestr, (filestr_length+1)*sizeof(char));
              sprintf(filestr+strptr,"%s\n", buff);
              strptr = filestr_length;
            }
            else{
              filestr_length += strlen(buff) + strlen(" ");
              filestr = (char*) realloc(filestr, (filestr_length+1)*sizeof(char));
              sprintf(filestr+strptr,"%s ", buff);
              strptr = filestr_length;
            }
        }
        thisS =thisS->next;
      }
  }
  #ifdef DEBUG
    printf("length in bytes: %d\n",filestr_length+1);
    printf("string length: %d\n%s\n",strlen(filestr),filestr);
  #endif

  // Prepare the encrypted text buffer
  encryption_length = filestr_length +32; // add 16 to make sure padding will fit in the buffer and 16 for the iv
  encryption = (char *) malloc(encryption_length*sizeof(char));

  // Generate the cryptographically secure random key and IV
  // if (!RAND_bytes(key, sizeof key)) {
  //   printf("key generation error: could not generate a key\n");
  //   return (0);
  // }
  if (!RAND_bytes(encryption, iv_length)) {
    printf("IV generation error: could not generate an IV\n");
  }

  // encrypt the file string
  encryption_length = encrypt(filestr, filestr_length, hexkey, encryption, encryption+iv_length);  // iv= encryption, ciphertext = encryption+iv_length
  #ifdef DEBUG
    printf("encryption length: %d\n",encryption_length);
    printf("iv and encryption:\n");
    BIO_dump_fp (stdout, (const char *)encryption, encryption_length+iv_length);
  #endif

  // Prepare the mac buffer
  mac = (char *) malloc((encryption_length+iv_length+16)*sizeof(char));
  //MAC the IV and ciphertext
  mac_length = get_mac(encryption, iv_length+encryption_length, hexkey, mac);
  #ifdef DEBUG
    printf("mac:\n");
    BIO_dump_fp (stdout, (const char *)mac, mac_length);
  #endif

  fp = fopen(R.name, "w");
   // Print MAC to file
  for(int i=0;i<mac_length;i++){
    fprintf(fp,"%c",mac[i]);    
  }
  // Print IV and ciphertext to file
  for (int i=0; i<encryption_length+iv_length;i++){
      fprintf(fp,"%c",encryption[i]);
  }
  fclose(fp);
  free(mac);
  mac=NULL;
  free(encryption);
  encryption=NULL;
  free(filestr);
  filestr=NULL;

  #ifdef DEBUG
    printf("key:\n");
    for (int i = 0; i <sizeof(hexkey); i++){
      printf("%02x", hexkey[i]);
    }
    printf("\n");
  #endif
  return valid;
}

int free_memory(Gradebook *G){
  Assignment *thisA= G->A;
  Assignment *tempA;
  Student *thisS = G->S;
  Student *tempS;

  // Free the Assignments from memory
  while(thisA !=NULL){;
    tempA = thisA;
    thisA = thisA->next;
    free(tempA);
  }
  // free the students and grade from memory
  while(thisS !=NULL){
    tempS = thisS;
    if(G->numAssignments!=0){
      free(thisS->grade);
    }
    thisS=thisS->next;
    free(tempS);
  }
  return 0;
}

int main(int argc, char *argv[]) {
  CmdLineResult R;
  Gradebook G;
  R = parse_cmdline(argc, argv, addType);
  //printCmdLineResults(R);

  if(R.good == valid) {

    // Authenticate the gradebook
    if(gradebook_authenticate(R)==invalid){
      return(255);
    }
    // Decrypt the gradebook and read
    if(decrypt_and_read_gradebook(R, &G)==invalid){
      return(255);
    }

    // Modify the gradebook
    if(modify_gradebook(R, &G)==invalid){
      free_memory(&G);
      return(255);
    }

    // encrypt and write gradebook to file
    encrypt_and_write_gradebook(R,&G);
    free_memory(&G);

  }
  if(R.good==invalid){
    return(255);
  }

  return 0;
}
