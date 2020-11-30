#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data.h"

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/comp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#define NON_VAR_LENGTH 0     //TODO change me
void encrypt(char* path, char* ckey){
  FILE *ifp = fopen(path, "rb"); 
  FILE *ofp = fopen("outputfile", "wb");

  // int AES_BLOCK_SIZE = 10;
  int bytes_read, bytes_written;
  unsigned char indata[AES_BLOCK_SIZE];
  unsigned char outdata[AES_BLOCK_SIZE];

  /* ckey and ivec are the two 128-bits keys necesary to
     en- and recrypt your data.  Note that ckey can be
     192 or 256 bits as well */
  unsigned char ivec[] = "dontusethisinput";

  
  AES_KEY key;

  /* set the encryption key */
  AES_set_encrypt_key(ckey, 128, &key);

  /* set where on the 128 bit encrypted block to begin encryption*/
  int num = 0;

  while (1) {
    bytes_read = fread(indata, 1, AES_BLOCK_SIZE, ifp);

    AES_cfb128_encrypt(indata, outdata, bytes_read, &key, ivec, &num,
     AES_ENCRYPT);

    bytes_written = fwrite(outdata, 1, bytes_read, ofp);
    if (bytes_read < AES_BLOCK_SIZE)
      break;
  }

  fclose(ifp);
  fclose(ofp);


  remove(path);

  rename("outputfile", path);
}

void decrypt(char* path, char* ckey){
  int bytes_read, bytes_written;
  unsigned char indata[AES_BLOCK_SIZE];
  unsigned char outdata[AES_BLOCK_SIZE];

     /* ckey and ivec are the two 128-bits keys necesary to
     en- and recrypt your data.  Note that ckey can be
     192 or 256 bits as well */
  unsigned char ivec[] = "dontusethisinput";

     /* data structure that contains the key itself */
  AES_KEY key;

     /* set the encryption key */
  AES_set_encrypt_key(ckey, 128, &key);


  int num = 0;

  FILE *ifp = fopen(path, "rb");
  FILE *ofp = fopen("decryptedfile", "wb");

  while (1) {
   bytes_read = fread(indata, 1, AES_BLOCK_SIZE, ifp);
   
   AES_cfb128_encrypt(indata, outdata, bytes_read, &key, ivec, &num, 
         AES_DECRYPT); //or AES_DECRYPT
   
   bytes_written = fwrite(outdata, 1, bytes_read, ofp);
   if (bytes_read < AES_BLOCK_SIZE) {

     break;
   }
 }
 fclose(ifp);
 fclose(ofp);

remove(path);

  rename("decryptedfile", path);
}

char * get_file_contents(char * path) {
  char * contents = NULL;
  long len;
  FILE * fileptr = fopen (path, "rb");

  if (fileptr)
  {
    fseek (fileptr, 0, SEEK_END);
    len = ftell (fileptr);
    fseek (fileptr, 0, SEEK_SET);
    contents = malloc (len);
    if (contents)
    {
      fread (contents, 1, len, fileptr);
    }
    fclose (fileptr);
  }

  return contents;
}
void write_to_path(char *path, Gradebook G, unsigned char *key_data) {



  FILE* fp = fopen(path, "w+");


  for(int i = 0; i<G.num_assignments; i++){
    fputs(G.assignments[i].name, fp);
    fputs(" ", fp);
    fprintf(fp, "%d", G.assignments[i].points);
    fputs(" ", fp);
    fprintf(fp, "%f", G.assignments[i].weight);
    fputs(" ", fp);
  }
  
  fputs("\n", fp);
  for(int i = 0; i<G.num_students; i++){
    fputs(G.students[i].fname, fp);
    fputs(" ", fp);
    fputs(G.students[i].lname, fp);
    fputs(" ", fp);
    for(int j = 0; j<G.num_assignments; j++){
      fprintf(fp, "%d", G.students[i].points_earned[j]);
      fputs(" ", fp);
    }
    fputs("\n", fp);
  }


  fclose(fp);



  /********************ENCRYPT DATA*************************/


  encrypt(path, key_data);

  // char * plaintext = get_file_contents(path);
  // printf("TO BE ENCRYPTED:\n%s\n", plaintext);

  // FILE *ifp = fopen(path, "rb"); 
  // FILE *ofp = fopen("output file", "wb");

  // // int AES_BLOCK_SIZE = 10;
  // int bytes_read, bytes_written;
  // unsigned char indata[AES_BLOCK_SIZE];
  // unsigned char outdata[AES_BLOCK_SIZE];

  // /* ckey and ivec are the two 128-bits keys necesary to
  //    en- and recrypt your data.  Note that ckey can be
  //    192 or 256 bits as well */
  // unsigned char ckey[] =  "thiskeyisverybad";
  // unsigned char ivec[] = "dontusethisinput";

  // data structure that contains the key itself 
  // AES_KEY key;

  // /* set the encryption key */
  // AES_set_encrypt_key(ckey, 128, &key);

  // /* set where on the 128 bit encrypted block to begin encryption*/
  // int num = 0;

  // while (1) {
  //   bytes_read = fread(indata, 1, AES_BLOCK_SIZE, ifp);

  //   AES_cfb128_encrypt(indata, outdata, bytes_read, &key, ivec, &num,
  //    AES_ENCRYPT);

  //   bytes_written = fwrite(outdata, 1, bytes_read, ofp);
  //   if (bytes_read < AES_BLOCK_SIZE)
  //     break;
  // }

  // fclose(ifp);
  // fclose(ofp);
  // remove(path);
  //    rename("output file", path);

}
int is_file_empty(char * path){
  FILE * fp = fopen(path, "r");
  fseek (fp, 0, SEEK_END);
  int size = ftell(fp);

  return size==0;

}
Gradebook read_from_path(char *path, unsigned char *key_data) {

  /***************DECRYPT**************/
  decrypt(path, key_data);
  // if(!is_file_empty(path))
  //  {int bytes_read, bytes_written;


  //   unsigned char indata[AES_BLOCK_SIZE];
  //    unsigned char outdata[AES_BLOCK_SIZE];

  //    /* ckey and ivec are the two 128-bits keys necesary to
  //    en- and recrypt your data.  Note that ckey can be
  //    192 or 256 bits as well */
  //    unsigned char ckey[] = "thiskeyisverybad";
  //    unsigned char ivec[] = "dontusethisinput";

  //    /* data structure that contains the key itself */
  //    AES_KEY key;

  //    /* set the encryption key */
  //    AES_set_encrypt_key(ckey, 128, &key);


  //    int num = 0;

  //    FILE *ifp = fopen(path, "rb");
  //    FILE *ofp = fopen("output file", "wb");

  //    while (1) {
  //      bytes_read = fread(indata, 1, AES_BLOCK_SIZE, ifp);

  //      AES_cfb128_encrypt(indata, outdata, bytes_read, &key, ivec, &num, 
  //        AES_DECRYPT); //or AES_DECRYPT

  //      bytes_written = fwrite(outdata, 1, bytes_read, ofp);
  //      if (bytes_read < AES_BLOCK_SIZE) {

  //        break;
  //      }
  //    }

  //    fclose(ifp);
  //    fclose(ofp);

  //    remove(path);
  //    rename("output file", path);
  //  }
  /******************************************************/

  Gradebook  G = {NULL, NULL, 0, 0};
  
  char* line = NULL;
  size_t len = 0;
  ssize_t read;

  FILE *fp = fopen(path, "r");
  if (fp == NULL){
    printf("invalid\n");
    exit(255);
  }



  getline(&line, &len, fp);


  char* assignment_word = strtok(line, " ");
  int i = 0;
  
  //READS ASSIGNMENT LIST
  while(assignment_word){

    if(i%3==0) {
     G.assignments = realloc(G.assignments, sizeof(Assignment)*((i/3)+1));
     G.assignments[i/3].name = malloc((strlen(assignment_word)+1)*sizeof(char));

     strncpy(G.assignments[i/3].name, assignment_word, (strlen(assignment_word)+1)*sizeof(char));
     
     i++;
   }
   else if(i%3==1) {

     G.assignments[i/3].points = atoi(assignment_word);
     i++;
   }
   else if(i%3==2) {

     G.assignments[i/3].weight = atof(assignment_word);
     i++;
   }

   assignment_word = strtok(NULL, " ");
 }
 G.num_assignments = i/3;
//  for (int i = 0; i<G.num_assignments; i++) {
//   printf("Assignment name:%s|\n", G.assignments[i].name);
//   printf("Points:%i|\n", G.assignments[i].points);
//   printf("Weight:%f|\n", G.assignments[i].weight);
// }
  //READ STUDENT LIST
 int num_students = 0;
 while((read = getline(&line, &len, fp)) != -1){
  char* student_fname = strtok(line, " ");
  G.students = realloc(G.students, sizeof(Student)*(++num_students));
  G.students[num_students-1].fname = malloc((strlen(student_fname)+1)*sizeof(char));
  strncpy(G.students[num_students-1].fname, student_fname, (strlen(student_fname)+1)*sizeof(char));

  char* student_lname = strtok(NULL, " ");
  
  G.students[num_students-1].lname = malloc((strlen(student_lname)+1)*sizeof(char));
  strncpy(G.students[num_students-1].lname, student_lname, (strlen(student_lname)+1)*sizeof(char));

  i = 0;
  char * student_score = strtok(NULL, " ");
  G.students[num_students-1].points_earned = NULL;
  while(student_score && strcmp(student_score, "\n")!=0){



    G.students[num_students-1].points_earned = realloc(G.students[num_students-1].points_earned, (++i)*sizeof(int));
    G.students[num_students-1].points_earned[i-1] = atoi(student_score);
    student_score = strtok(NULL, " ");
    G.students[num_students-1].total_assignments = i;

  }
  G.students[num_students-1].assignment_names_lst = malloc(G.num_assignments*sizeof(char*));
  for(int i = 0; i<G.num_assignments; i++) {
    char* assignment_name = G.assignments[i].name;
    
    G.students[num_students-1].assignment_names_lst[i] = malloc((strlen(assignment_name)+1)*sizeof(char));
    strncpy(G.students[num_students-1].assignment_names_lst[i], assignment_name,(strlen(assignment_name)+1)*sizeof(char));
  }

}

G.num_students = num_students;
fclose(fp);
// printf("\n\n");
// for(int j = 0; j<num_students; j++){
//   printf("Student Name: %s %s\n", G.students[j].fname, G.students[j].lname);
//   printf("Scores: ");
//   for(int p = 0; p<G.students[j].total_assignments; p++){
//     printf("%s: %i ",G.students[j].assignment_names_lst[p], G.students[j].points_earned[p]);
//   }
//   printf("\n");
// }
// printf("\n");
return G;


}

void dump_assignment(Assignment A) { 


  printf("\nName: %s\n", A.name);
  printf("Points: %i\n", A.points);
  printf("Weight: %f\n", A.weight);

  return;
}

int get_Gradebook(Gradebook *R, Buffer *B) {
  unsigned int  bytesRead = 0;

  //TODO Code this

  return bytesRead;
}

// int read_Gradebook_from_path(char *path, unsigned char *key, Gradebook **outbuf, unsigned int *outnum) {
//   *outnum = 0;
//   *outbuf = NULL;

//   //read in a file 
//   Buffer  B = read_from_path(path, key);

//   //TODO Code this

//   return 0;
// }
