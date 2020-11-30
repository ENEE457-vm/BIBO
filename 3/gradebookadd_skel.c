#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>
#include <ctype.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "data.h"

typedef struct _CmdLineResult {
  char* N;
  char* K;
  char* AN;
  char* FN;
  char* LN;
  char* P;
  char* W;
  char* G;
  char* setting;  
  int good;
} CmdLineResult;



CmdLineResult parse_cmdline(int argc, char *argv[]) {
  CmdLineResult R = {
    .N = NULL,
    .K = NULL,
    .AN = NULL,
    .FN = NULL,
    .LN = NULL,
    .P = NULL,
    .W = NULL,
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

    if(strcmp(argv[5],"-AA")==0 || strcmp(argv[5],"-DA")==0 || 
      strcmp(argv[5],"-AS")==0 || strcmp(argv[5],"-DS")==0 || 
      strcmp(argv[5],"-AG")==0) {
      R.setting = argv[5];
  } else {
    // printf(" Invalid Setting Option\n");
    return R;
  }
  for(int i = 7; i<argc; i+=2) {
    if (strcmp(argv[i-1],"-AN")==0)
      R.AN = argv[i];
    else if (strcmp(argv[i-1],"-FN")==0)
      R.FN = argv[i];
    else if (strcmp(argv[i-1],"-LN")==0)
      R.LN = argv[i];
    else if (strcmp(argv[i-1],"-P")==0)
      R.P = argv[i];
    else if (strcmp(argv[i-1],"-W")==0)
      R.W = argv[i];
    else if (strcmp(argv[i-1],"-G")==0)
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

  if(strcmp(R.setting, "-AA")==0 && R.AN && !R.FN && !R.LN && R.P && R.W && !R.G) {
    R.good = 1;
  }
  else if(strcmp(R.setting, "-DA")==0 && R.AN && !R.FN && !R.LN && !R.P && !R.W && !R.G) {
    R.good = 1;
  }
  else if(strcmp(R.setting, "-AS")==0 && !R.AN && R.FN && R.LN && !R.P && !R.W && !R.G) {
    R.good = 1;
  }
  else if(strcmp(R.setting, "-DS")==0 && !R.AN && R.FN && R.LN && !R.P && !R.W && !R.G) {
    R.good = 1;
  } 
  else if(strcmp(R.setting, "-AG")==0 && R.AN && R.FN && R.LN && !R.P && !R.W && R.G) {
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
int does_assignment_exist(CmdLineResult R, Gradebook * G){

  for(int i = 0; i<((*G).num_assignments); i++) {

    if(strcmp((*G).assignments[i].name, R.AN) == 0){
      // printf("assignment exists\n");
      return 1;
    }
  }

  return 0;
}

int is_weight_valid(CmdLineResult R, Gradebook * G) {
  float totalweight = 0;
  for(int i = 0; i<((*G).num_assignments); i++) {
    totalweight+=(*G).assignments[i].weight;
  }

  if(totalweight+atof(R.W) > 1){
    // printf("assignment weight invalid\n");
    return 0;
  }
  return 1;
}
int is_alphabetic(char * str) {
  for(int i = 0; i<strlen(str); i++){
    if(!isalpha(str[i])) {
      // printf("Not alphabetic\n");
      return 0;
    }
  }
  return 1;
}
int is_alphanumeric(char* str) {
  for(int i = 0; i<strlen(str); i++){
    if(!isalnum(str[i])) {
      // printf("Not alphanumeric\n");
      return 0;
    }
  }
  return 1;
}
int is_int (char* str) {
  for(int i = 0; i<strlen(str); i++){
    if(!(str[i]>='0' && str[i]<='9')) {
      // printf("Not int\n");
      return 0;
    }
  }
  return 1;
}
int is_float (char* str) {
  int length;
  float i;
  
  int j = sscanf(str, "%f %n", &i, &length);
  return (j==1 && !str[length]); 

}

int does_student_exist(char* fname, char* lname, Gradebook *G) {
  for(int i = 0; i<(*G).num_students; i++) {
    if(strcmp(fname, (*G).students[i].fname)==0 && strcmp(lname, (*G).students[i].lname)==0){
      // printf("Student exists\n");
      return 1;
    }
  }
  return 0;
}
Gradebook* add_assignment(Gradebook* G, CmdLineResult R) {
  if(!is_alphanumeric(R.AN) || !is_int(R.P) || !is_float(R.W)){
    printf("invalid\n");
    write_to_path(R.N, *G, R.K);
    exit(255);
    
  }

  if(atoi(R.P)<0 || atof(R.W)<0 || atof(R.W)>1){
    printf("invalid\n");
    write_to_path(R.N, *G, R.K);
    exit(255);
    
  }

  if(does_assignment_exist(R, G) || !is_weight_valid(R, G)){ /////ALSO checks if weights are correct
    printf("invalid\n");
    write_to_path(R.N, *G, R.K);
    exit(255);
    
  }
  (*G).assignments = realloc((*G).assignments, sizeof(Assignment)*(++((*G).num_assignments)));
  (*G).assignments[(*G).num_assignments-1].name = malloc((strlen(R.AN)+1)*sizeof(char));
  strncpy((*G).assignments[(*G).num_assignments-1].name, R.AN, (strlen(R.AN)+1)*sizeof(char));


  (*G).assignments[(*G).num_assignments-1].weight = atof(R.W);
  (*G).assignments[(*G).num_assignments-1].points = atoi(R.P);

  for(int i = 0; i<((*G).num_students); i++) {

    (*G).students[i].total_assignments = (*G).num_assignments;
    (*G).students[i].assignment_names_lst = realloc((*G).students[i].assignment_names_lst, ((*G).num_assignments)*sizeof(char*));
    (*G).students[i].assignment_names_lst[(*G).num_assignments-1] = malloc((strlen(R.AN)+1)*sizeof(char));
    strncpy((*G).students[i].assignment_names_lst[(*G).num_assignments-1], R.AN, (strlen(R.AN)+1)*sizeof(char));

    (*G).students[i].points_earned = realloc((*G).students[i].points_earned, ((*G).num_assignments)*sizeof(int));
    (*G).students[i].points_earned[(*G).num_assignments-1] = 0;
  }
  return G;
}

void add_student(Gradebook* G, CmdLineResult R) {
  if(!is_alphabetic(R.FN) || !is_alphabetic(R.LN) || does_student_exist(R.FN, R.LN, G)) {
    printf("invalid\n");
    write_to_path(R.N, *G, R.K);
    exit(255);
    
  }

  (*G).students = realloc((*G).students, ((++((*G).num_students)))*sizeof(Student));
  (*G).students[((*G).num_students)-1].fname = malloc(strlen(R.FN)+1);
  strncpy((*G).students[((*G).num_students)-1].fname, R.FN, strlen(R.FN)+1);
  (*G).students[((*G).num_students)-1].lname = malloc(strlen(R.LN)+1);
  strncpy((*G).students[((*G).num_students)-1].lname, R.LN, strlen(R.LN)+1);
  (*G).students[((*G).num_students)-1].total_assignments = (*G).num_assignments;
  
  (*G).students[((*G).num_students)-1].assignment_names_lst = malloc(sizeof(char*)*((*G).num_assignments));
  (*G).students[((*G).num_students)-1].points_earned = malloc(sizeof(int)*((*G).num_assignments));
  
  for(int i =0; i<(*G).num_assignments; i++){
    (*G).students[((*G).num_students)-1].points_earned[i] = 0;
    (*G).students[((*G).num_students)-1].assignment_names_lst[i] = malloc((strlen((*G).assignments[i].name)+1));
    strncpy((*G).students[((*G).num_students)-1].assignment_names_lst[i], (*G).assignments[i].name, strlen((*G).assignments[i].name)+1);
  }
}

void add_grade(Gradebook* G, CmdLineResult R) {
  if(!is_int(R.G) || atoi(R.G) < 0 || !does_student_exist(R.FN, R.LN, G) || !does_assignment_exist(R, G)){
    printf("invalid\n");
    write_to_path(R.N, *G, R.K);
    exit(255);
    
  }

  int assignmentloc = -1;
  for(int i = 0; i<((*G).num_assignments); i++){
    if(strcmp((*G).assignments[i].name, R.AN)==0){
      assignmentloc = i;
      break;
    }
  }
  for(int i = 0; i<((*G).num_students); i++){
    if(strcmp((*G).students[i].fname, R.FN)==0 && strcmp((*G).students[i].lname, R.LN)==0){
      (*G).students[i].points_earned[assignmentloc] = atoi(R.G);
      break;
    }
  }

}
void delete_assignment(Gradebook *G, CmdLineResult R) {

  int assignmentloc = -1;

  for(int i = 0; i<(*G).num_assignments; i++) {
    if(strcmp((*G).assignments[i].name, R.AN)==0){
      assignmentloc = i;
    }
  }

  if(assignmentloc==-1) {
    printf("invalid\n");
    write_to_path(R.N, *G, R.K);
    exit(255);
    
  }
  free((*G).assignments[assignmentloc].name);
  (*G).assignments[assignmentloc].name = NULL;


  for(int i = assignmentloc; i < (*G).num_assignments - 1; i++) 
    (*G).assignments[i] = (*G).assignments[i + 1];

  Assignment* temp = realloc((*G).assignments, ((*G).num_assignments - 1) * sizeof(Assignment));
  if(temp==NULL && (*G).num_assignments>1){
    printf("invalid\n");
    write_to_path(R.N, *G, R.K);
    exit(255);
    

  }
  
  (*G).assignments = temp;


  for(int i = 0; i<(*G).num_students; i++){
    free((*G).students[i].assignment_names_lst[assignmentloc]);
    (*G).students[i].assignment_names_lst[assignmentloc] = NULL;
    for(int j = assignmentloc; j < (*G).num_assignments - 1; j++) {
      (*G).students[i].assignment_names_lst[j] = (*G).students[i].assignment_names_lst[j+1];
      (*G).students[i].points_earned[j] = (*G).students[i].points_earned[j+1];
    }
    char** temp1 = realloc((*G).students[i].assignment_names_lst, ((*G).num_assignments-1)*sizeof(char*));
    int* temp2 = realloc((*G).students[i].points_earned, ((*G).num_assignments-1)*sizeof(int));
    if((temp1==NULL || temp2==NULL) && (*G).num_assignments>1){
      printf("invalid\n");
      write_to_path(R.N, *G, R.K);
      exit(255);
      

    }
    (*G).students[i].assignment_names_lst = temp1;
    (*G).students[i].points_earned = temp2;
    (*G).students[i].total_assignments = (*G).num_assignments - 1;
  }
  (*G).num_assignments = (*G).num_assignments - 1;
}

void delete_student(Gradebook *G, CmdLineResult R) {

  int location = -1;
  for(int i = 0; i<(*G).num_students; i++) {
    if(strcmp(R.FN, (*G).students[i].fname)==0 && strcmp(R.LN, (*G).students[i].lname)==0){
      location = i;
    }
  }
  if(location == -1) {
    printf("invalid\n");
    write_to_path(R.N, *G, R.K);
    exit(255);
    
  }
  free((*G).students[location].fname);
  (*G).students[location].fname = NULL;
  free((*G).students[location].lname);
  (*G).students[location].lname = NULL;
  free((*G).students[location].points_earned);
  (*G).students[location].points_earned = NULL;

  for(int i =0; i<(*G).num_assignments; i++) {
    free((*G).students[location].assignment_names_lst[i]);
    (*G).students[location].assignment_names_lst[i] = NULL;

  }

  free((*G).students[location].assignment_names_lst);
  (*G).students[location].assignment_names_lst = NULL;

  for(int i = location; i<(*G).num_students-1; i++) {
    (*G).students[i] = (*G).students[i + 1];
  }

  Student *tmp = realloc((*G).students, ((*G).num_students - 1) * sizeof(Student));

  if(tmp==NULL && (*G).num_students>1){
    printf("invalid\n");
    write_to_path(R.N, *G, R.K);
    exit(255);
    
  }
  (*G).num_students = (*G).num_students - 1;
  (*G).students = tmp;

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
int check_key(char * key, char* gname){

  unsigned char *iv = (unsigned char *)"0000000000000000";

  unsigned char ciphertext[128];

    /* Buffer for the decrypted text */
  unsigned char decryptedtext[128];

  int decryptedtext_len, ciphertext_len;

  char * buffer = 0;
  long length;
  char keyfileName[strlen(gname)+8+1];

    strncpy(keyfileName, "key-file", sizeof(keyfileName));

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
  int r = 0;
  CmdLineResult R;
  
  R = parse_cmdline(argc, argv);

  if(R.good == 0) {
    printf("invalid\n");
    return 255;

  }
  if(!does_file_exist(R.N)){
    printf("invalid\n");
    return 255;
  }
  if(!check_key(R.K, R.N)){
    printf("invalid\n");
    return 255;
  }

  Gradebook G = {NULL, NULL, 0, 0};
  G=read_from_path(R.N, R.K);

  if(!strcmp(R.setting, "-AA")){
    add_assignment(&G, R);
  }
  else if(!strcmp(R.setting, "-AS")){
    add_student(&G, R);
  }
  else if(!strcmp(R.setting, "-AG")){
    add_grade(&G, R);
  }
  else if(!strcmp(R.setting, "-DA")){
    delete_assignment(&G, R);
  }
  else if(!strcmp(R.setting, "-DS")){
    delete_student(&G, R);
  }
  else {
    printf("invalid\n");
    write_to_path(R.N, G, R.K);
    return 255;
    
  }

  // printf("-------------------------ALL ASSIGNMENTS-------------------------\n");
  // for(int i = 0; i<G.num_assignments; i++) {
  //   dump_assignment(G.assignments[i]);
  // }
  // printf("-----------------------------------------------------------------\n\n");
  

  // printf("-------------------------ALL STUDENTS-------------------------\n");
  // for(int j = 0; j<G.num_students; j++){
  //   printf("Student Name: %s %s\n", G.students[j].fname, G.students[j].lname);
  //   printf("Scores: ");
  //   for(int p = 0; p<G.num_assignments; p++){
  //     printf("%s: %i ",G.students[j].assignment_names_lst[p], G.students[j].points_earned[p]);
  //   }
  //   printf("\n");
  // }
  // printf("-----------------------------------------------------------------\n");

  write_to_path(R.N, G, R.K);
  return r;
}

