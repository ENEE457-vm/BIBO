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

int verbose = 0;
typedef struct _CmdLineResult {
  char* N;
  char* K;
  char* AN;
  char* FN;
  char* LN;
  char* A;
  char* G;
  char* setting;  
  int good;
} CmdLineResult;

typedef struct _StudentPoints {
  char* fname;
  char* lname;
  int points;
} StudentPoints;

typedef struct _StudentGrades {
  char* fname;
  char* lname;
  float grade;
} StudentGrades;

CmdLineResult parse_cmdline(int argc, char *argv[]) {
  CmdLineResult R = {
    .N = NULL,
    .K = NULL,
    .AN = NULL,
    .FN = NULL,
    .LN = NULL,
    .A = NULL,
    .G = NULL,
    .setting = NULL,  
    .good = 0
  };
  

  if(argc<6) {
    // printf("Invalid: Incorrect Number of Options\n"); 
    return R;
    
  }
  else 
  { 

    if(strcmp(argv[1],"-N")==0) {
      R.N = argv[2];
    }
    else {
      // printf("Invalid: No -N Option\n");
      return R;
    }
    if(strcmp(argv[3],"-K")==0) {
      R.K = argv[4];
    }
    else {
      // printf("Invalid: No -K Option\n");
      return R;
    }

    if(strcmp(argv[5],"-PA")==0 || strcmp(argv[5],"-PS")==0 || 
      strcmp(argv[5],"-PF")==0) {
      R.setting = argv[5];
  } else {
    // printf(" Invalid Setting Option\n");
    return R;
  }
  for(int i = 6; i<argc; i++) {
    if (strcmp(argv[i],"-AN")==0)
      R.AN = argv[++i];
    else if (strcmp(argv[i],"-FN")==0)
      R.FN = argv[++i];
    else if (strcmp(argv[i],"-LN")==0)
      R.LN = argv[++i];
    else if (strcmp(argv[i],"-A")==0)
      R.A = argv[i];
    else if (strcmp(argv[i],"-G")==0)
      R.G = argv[i];
    else{
      // printf("Invalid Option: %s\n", argv[i-1]);
      return R;
    }

  }

  // printf("AN: %s\n", R.AN);
  // printf("N: %s\n", R.N);
  // printf("K: %s\n", R.K);
  // printf("FN: %s\n", R.FN);
  // printf("LN: %s\n", R.LN);
  // printf("P: %s\n", R.P);
  // printf("W: %s\n", R.W);
  // printf("G: %s\n", R.G);
  // printf("P: %s\n", R.P);
  // printf("setting: %s\n", R.setting);
  // printf("good: %d\n", R.good);

  if(strcmp(R.setting, "-PA")==0  && ((!R.A && R.G) || (R.A && !R.G)) &&  R.AN && !R.FN  && !R.LN) {
    R.good = 1;
  }
  else if(strcmp(R.setting, "-PS")==0 && !R.A && !R.G &&  !R.AN && R.FN  && R.LN) {
    R.good = 1;
  }
  else if(strcmp(R.setting, "-PF")==0 && ((!R.A && R.G) || (R.A && !R.G)) &&  !R.AN && !R.FN  && !R.LN) {
    R.good = 1;
  }
  else{
    // printf("Invalid Combination of Options\n" );
    return R;
  }

} 
  // FILE* file = NULL; // added stuff
  // //TODO do stuff
  // if(file != NULL) {
  //   //R.good = do_something(file); //commented this out
  // } else {
  //   //TODO do stuff
  //   R.good = 0;
  // }

return R;

}

void print_Gradebook(Gradebook *gradebook) {
  unsigned int i;
  int num_assigment = 0; //added stuff
  for(i = 0; i < num_assigment; i++) {
    //dump_assignment(&gradebook[i]); //comment this out
    printf("----------------\n");
  }

  return;
}





int does_student_exist(char* fname, char* lname, Gradebook *G) {
  for(int i = 0; i<(*G).num_students; i++) {
    if(strcmp(fname, (*G).students[i].fname)==0 && strcmp(lname, (*G).students[i].lname)==0){
      // printf("Student exists\n");
      return i;
    }
  }
  return -1;
}

int does_assignment_exist(CmdLineResult R, Gradebook * G){

  for(int i = 0; i<((*G).num_assignments); i++) {

    if(strcmp((*G).assignments[i].name, R.AN) == 0){
      // printf("assignment exists\n");
      return i;
    }
  }

  return -1;
}
static int myAlphaComparator(const void* a, const void* b) {
  if(strcmp((*(const StudentPoints*)a).lname, (*(const StudentPoints*)b).lname)==0){
    return strcmp((*(const StudentPoints*)a).fname, (*(const StudentPoints*)b).fname);
  }
  return strcmp((*(const StudentPoints*)a).lname, (*(const StudentPoints*)b).lname);
} 

static int myGradeComparator(const void* a, const void* b) {
  return ((*(const StudentPoints*)b).points - (*(const StudentPoints*)a).points);
}

void print_Assignment(Gradebook * G, CmdLineResult R) {
  char* order = NULL;

  if(R.G && !R.A){
    order = R.G;
  }
  else if (!R.G && R.A) {
    order = R.A;
  }
  else{
    printf("invalid\n");
    write_to_path(R.N, *G,NULL);
    exit(255);
  }

  int loc = does_assignment_exist(R, G);
  if(loc==-1){
    printf("invalid\n");
    write_to_path(R.N, *G,NULL);
    exit(255);
  }
  StudentPoints *tmp_arr = malloc(sizeof(StudentPoints)*((*G).num_students));

  for(int i = 0; i<(*G).num_students; i++){
    tmp_arr[i].fname = malloc((strlen(((*G).students[i].fname))+1)*sizeof(char));
    strncpy(tmp_arr[i].fname , (*G).students[i].fname, (strlen(((*G).students[i].fname))+1)*sizeof(char));
    tmp_arr[i].lname = malloc((strlen(((*G).students[i].lname))+1)*sizeof(char));
    strncpy(tmp_arr[i].lname , (*G).students[i].lname, (strlen(((*G).students[i].lname))+1)*sizeof(char));
    tmp_arr[i].points = (*G).students[i].points_earned[loc];
  }
  if(strcmp(order, "-A")==0){

    qsort(tmp_arr, (*G).num_students, sizeof(StudentPoints), myAlphaComparator);

    for(int i = 0; i<(*G).num_students; i++){
      printf("(%s, %s, %d)\n", tmp_arr[i].lname, tmp_arr[i].fname, tmp_arr[i].points);
    }
  }
  else if (strcmp(order, "-G")==0){
    qsort(tmp_arr, (*G).num_students, sizeof(StudentPoints), myGradeComparator);

    for(int i = 0; i<(*G).num_students; i++){
      printf("(%s, %s, %d)\n", tmp_arr[i].lname, tmp_arr[i].fname, tmp_arr[i].points);
    }
  }
  else {
    printf("invalid\n");
    write_to_path(R.N,*G, NULL);
    exit(255);
  }
}

void print_Student(Gradebook * G, CmdLineResult R) {
  int loc = does_student_exist(R.FN, R.LN, G);

  if(loc==-1){
    printf("invalid\n");
    write_to_path(R.N, *G,NULL);
    exit(255);
  }

  for (int i = 0; i<(*G).num_assignments; i++){
    printf("(%s, %d)\n", (*G).students[loc].assignment_names_lst[i], (*G).students[loc].points_earned[i]);
  }
  return;
}
static int myAlphaStudentGradesComparator(const void* a, const void* b) {
  if(strcmp((*(const StudentGrades*)a).lname, (*(const StudentGrades*)b).lname)==0){
    return strcmp((*(const StudentGrades*)a).fname, (*(const StudentGrades*)b).fname);
  }
  return strcmp((*(const StudentGrades*)a).lname, (*(const StudentGrades*)b).lname);
} 
static int myGradeStudentGradesComparator(const void* a, const void* b) {
  return ((*(const StudentGrades*)a).grade - (*(const StudentGrades*)b).grade);
}
void print_Final(Gradebook *G, CmdLineResult R){
  char* order = NULL;
  if(R.G && !R.A){
    order = R.G;
  }
  else if (!R.G && R.A) {
    order = R.A;
  }
  else{
    printf("invalid\n");
    write_to_path(R.N, *G,NULL);
    exit(255);
  }

  StudentGrades *tmp_arr = malloc(sizeof(StudentGrades)*((*G).num_students));
  for(int i = 0; i<(*G).num_students; i++) {
    tmp_arr[i].fname = malloc((strlen(((*G).students[i].fname))+1)*sizeof(char));
    strncpy(tmp_arr[i].fname , (*G).students[i].fname, (strlen(((*G).students[i].fname))+1)*sizeof(char));
    tmp_arr[i].lname = malloc((strlen(((*G).students[i].lname))+1)*sizeof(char));
    strncpy(tmp_arr[i].lname , (*G).students[i].lname, (strlen(((*G).students[i].lname))+1)*sizeof(char));
    float grade = 0;
    for(int j = 0; j<(*G).num_assignments; j++) {
      grade += (((*G).students[i].points_earned[j])/((float)(*G).assignments[j].points))*((*G).assignments[j].weight);
    }
    tmp_arr[i].grade = grade;
  }

  if(strcmp(order, "-A")==0){

    qsort(tmp_arr, (*G).num_students, sizeof(StudentGrades), myAlphaStudentGradesComparator);

    for(int i = 0; i<(*G).num_students; i++){
      printf("(%s, %s, %f)\n", tmp_arr[i].lname, tmp_arr[i].fname, tmp_arr[i].grade);
    }
  }
  else if (strcmp(order, "-G")==0){
    qsort(tmp_arr, (*G).num_students, sizeof(StudentPoints), myGradeStudentGradesComparator);

    for(int i = 0; i<(*G).num_students; i++){
      printf("(%s, %s, %f)\n", tmp_arr[i].lname, tmp_arr[i].fname, tmp_arr[i].grade);
    }
  }
  else {
    printf("invalid\n");
    write_to_path(R.N, *G,NULL);
    exit(255);
  }
  return;
}
void handleErrors(void)
{
  ERR_print_errors_fp(stderr);
  abort();
}
int decryptKey(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
  unsigned char *iv, unsigned char *plaintext)
{
  EVP_CIPHER_CTX *ctx;

  int len;

  int plaintext_len;

    /* Create and initialise the context */
  if(!(ctx = EVP_CIPHER_CTX_new()))
    handleErrors();

    /*
     * Initialise the decryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
  if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv))
    handleErrors();

    /*
     * Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary.
     */
  if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
    handleErrors();
  plaintext_len = len;

    /*
     * Finalise the decryption. Further plaintext bytes may be written at
     * this stage.
     */
  if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
    handleErrors();
  plaintext_len += len;

    /* Clean up */
  EVP_CIPHER_CTX_free(ctx);

  return plaintext_len;
}
int check_key(char * key, char *gname){

  unsigned char *iv = (unsigned char *)"0000000000000000";

  unsigned char ciphertext[128];

    /* Buffer for the decrypted text */
  unsigned char decryptedtext[128];

  int decryptedtext_len, ciphertext_len;

  char * buffer = 0;
  long length;

  char keyfileName[strlen(gname)+8+1];

    strncpy(keyfileName, "key-file", strlen(gname)+8+1);

    for(int i = 0; i<strlen(gname); i++){
      keyfileName[i+8] = gname[i];
    }
    keyfileName[strlen(gname)+8] = '\0';
  FILE * f = fopen (keyfileName, "rb");

  if (f)
  {
    fseek (f, 0, SEEK_END);
    length = ftell (f);
    fseek (f, 0, SEEK_SET);
    buffer = malloc (length);
    if (buffer)
    {
      fread (buffer, 1, length, f);
    }
    fclose (f);
  }
   else{
    
    return 0;
  }
  // printf("%s\n", buffer);


  decryptedtext_len = decryptKey(buffer, strlen(buffer), key, iv,
    decryptedtext);
  // printf("decryptedtext: %s\n", decryptedtext);
  for(int i = 0; i<strlen(key); i++){
    if(key[i]!=decryptedtext[i]){
      return 0;
    }
  }
  // printf("decryptedtext: %s\n", decryptedtext);
  return 1;
}
int does_file_exist(char * fname) {
  FILE* fp = fopen(fname, "r");

  if(fp) {
    fclose(fp);
    return 1;
  }

  return 0; 
}
int main(int argc, char *argv[]) {

  CmdLineResult R = parse_cmdline(argc, argv);

  if(R.good == 0){
    printf("invalid\n");
    return 255;
  }

  if(!does_file_exist(R.N)){
    printf("invalid f\n");
    return 255;
  }
  if(!check_key(R.K, R.N)){
    printf("invalid key\n");
    return 255;
  }

  Gradebook G = {NULL, NULL, 0, 0};
  G=read_from_path(R.N, R.K);

  if(!strcmp(R.setting, "-PA")){
    print_Assignment(&G, R);
  }
  else if(!strcmp(R.setting, "-PS")){
    print_Student(&G, R);
  }
  else if(!strcmp(R.setting, "-PF")){
    print_Final(&G, R);
  }
  else {
    printf("invalid\n");
    write_to_path(R.N, G,R.K);
    return 255;
  }

  write_to_path(R.N, G, R.K);
  return 1;
}
