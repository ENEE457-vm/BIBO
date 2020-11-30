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
#include <openssl/conf.h>

#define NON_VAR_LENGTH 0     //TODO change me

int compute_Gradebook_size(Gradebook *R) {
  //TODO do stuff
  
  int len = sizeof(R);
  return len;
}

Buffer print_Gradebooks(Gradebook *R) {
  Buffer  B = {0};
 
  //TODO Code this

  return B;
}

//produce A | B
Buffer concat_buffs(Buffer *A, Buffer *B) {
  Buffer C = {0};
  C.Buf = malloc(strlen(A->Buf) + strlen(B->Buf) +1);
  C.Length = A->Length +1;
  strncpy(C.Buf, A->Buf, strlen(A->Buf));
  C.Buf[strlen(C.Buf)] = '\0';
  strcat(C.Buf, B->Buf);
  return C;
}

void write_to_path(char *path, unsigned char *key_data, Gradebook **out, unsigned int *o) {
  Gradebook *gb;
  gb = malloc(sizeof(Gradebook));
  gb = *out;
  Student *stud = NULL;
  stud = malloc(sizeof(Student));
  Assignment *ass = NULL;
  ass = malloc(sizeof(Assignment));
  Score *score = NULL;
  score = malloc(sizeof(Score));
  unsigned char* iv;
  unsigned char buff[1000];
  unsigned char ivbuf[256];
  FILE *fpr = fopen(path, "r");
  if(fgets(buff,sizeof(buff), fpr)){
	  strncpy(ivbuf, buff, 32);
	  //len = strlen(ivbuf);
	  //ivbuf[len-5] = '\0';
	  ivbuf[32] = '\0';
	  iv = ivbuf;
  }
  fclose(fpr);

  FILE *fp = fopen(path, "w");
  if (fp == NULL){
	  printf("invalid\n");
	  return;
  }
  else{
	  fprintf(fp, "%s", iv);
	  fprintf(fp, "\n");
	  fprintf(fp, "%s", key_data);
	  fprintf(fp, "\n");
	  stud = gb->shead;
	  ass = gb->ahead;
	  if (stud == NULL && ass != NULL){
		 while (ass != NULL){
			fprintf(fp, ass->name);
		        fprintf(fp, ",");
                        fprintf(fp, "%d", ass->total);
                        fprintf(fp, ",");
                        fprintf(fp, "%.2f", ass->weight);
                        fprintf(fp, ",");
                        ass = ass->next;
		 }
		 fprintf(fp, "\n");
	  }
				 
	  else if (stud != NULL){
	  	while (stud != NULL ){
			score = stud->scorehead;
			fprintf(fp, stud->fname);
			fprintf(fp, ",");
			fprintf(fp, stud->lname);
			fprintf(fp, ",");
			while (ass != NULL){
				fprintf(fp, ass->name);
				fprintf(fp, ",");
				fprintf(fp, "%d", score->score);
				fprintf(fp, ",");
				fprintf(fp, "%d", ass->total);
				fprintf(fp, ",");
				fprintf(fp, "%.2f", ass->weight);
				fprintf(fp, ",");
				ass = ass->next;
				score = score->next;
			}

			fprintf(fp, "\n");
			stud = stud->snext;
			ass = gb->ahead;
	  	}
	  }
	  
  }
  fclose(fp);
  return;
}


void handleErrors(void){
        ERR_print_errors_fp(stderr);
        abort();
}

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,            unsigned char *iv, unsigned char *plaintext){
        EVP_CIPHER_CTX *ctx;
        int len;
        int plaintext_len;    /* Create and initialise the context */
        if(!(ctx = EVP_CIPHER_CTX_new()))
	        handleErrors();    /*     * Initialise the decryption operation. IMPORTANT - ensure you use a key     * and IV size appropriate for your cipher     * In this example we are using 256 bit AES (i.e. a 256 bit key). The     * IV size for *most* modes is the same as the block size. For AES this     * is 128 bits     */
	if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv))
	        handleErrors();    /*     * Provide the message to be decrypted, and obtain the plaintext output.     * EVP_DecryptUpdate can be called multiple times if necessary.     */
	if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))  
             handleErrors();
	plaintext_len = len;
		        /*     * Finalise the decryption. Further plaintext bytes may be written at     * this stage.     */
	if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
	   ERR_print_errors_fp(stderr);
	plaintext_len += len;    /* Clean up */
	EVP_CIPHER_CTX_free(ctx);
	return plaintext_len;
}



Buffer read_from_path(char *path, unsigned char *key_data) {
  Buffer B = {0};
  Buffer A = {0};
  unsigned char buff[100000];
  unsigned char ivbuf[128];
  unsigned char* ciphertext;
  unsigned char cipherbuf[256];
  int ciphertext_len;
  unsigned char* iv;
  unsigned char *plaintext;
  unsigned char decryptedtext[128];
  //plaintext = plainbuf;
  A.Buf = "";
  A.Length = 0;
  FILE *fp = fopen(path, "r");
  if (fp == NULL) {
	  printf("invalid\n");
	  A.Length = -1;
	  return A;
  }
  if(fgets(buff,sizeof(buff), fp)){
	  int len = strlen(buff);
	  strncpy(ivbuf, buff, len);
	  len = strlen(ivbuf);
	  ivbuf[len-1] = '\0';
	  iv = (unsigned char *)ivbuf;
  }

  if (fgets(buff, sizeof(buff), fp)){
	  buff[strlen(buff)-1] = '\0';
	  if (strcmp(buff, key_data)){
		  //printf("Buff %s\n", buff);
		  //printf("Key %s\n", key_data);
		  //printf("invalid\n");
		  A.Length = -1;
	  }
  }
/*  if (fgets(buff, sizeof(buff),fp)){
	  ciphertext_len = strlen(buff);
	  strncpy(cipherbuf, buff, ciphertext_len);
	  cipherbuf[ciphertext_len-1] = '\0';
	  ciphertext = (unsigned char *)cipherbuf;
	  ciphertext_len = strlen((char *)cipherbuf)/2;
	  //ciphertext_len = strlen(ciphertext);
  }
  unsigned char * key;
  key = (unsigned char *) key_data;
  int i;
  printf("Key Data: ");
	  printf("%.s\n", key);
  printf("Ciphertext: %s.\n", ciphertext);
  printf("Length: %d\n", ciphertext_len);

  int decrypted_len;
  decrypted_len = decrypt((unsigned char *)ciphertext,ciphertext_len, (unsigned char *)key, (unsigned char *)iv,decryptedtext);
*/

  //plainbuf[decrypted_len-1] = '\0';
/*  printf("Decrypt length: %d\n", decrypted_len);
  printf("Decrypted Plaintext: ");
  for (i = 0; i< decrypted_len; i++){
  	printf("%x", decryptedtext[i]);
  }
  printf("\ndecrypted text: ");
  printf("%s", decryptedtext);
  printf("\n");*/
  while(1){
	  if(!fgets(buff, sizeof(buff), fp)){
		  break;
	  }
	buff[99999] = '\0';
	B.Buf = buff;
	A = concat_buffs(&A, &B);
	
  }
  fclose(fp);
  //TODO Code this
  return A;
}

void dump_assignment(Assignment *A) { 
  
  //TODO Code this
  
  return;
}

int get_Gradebook(Gradebook *R, Buffer *B) {
  unsigned int  bytesRead = 0;
 
  //TODO Code this

  return bytesRead;
}

int read_Gradebook_from_path(char *path, unsigned char *key, Gradebook **outbuf, unsigned int *outnum) {
  *outnum = 0;
  *outbuf = NULL;
  //read in a file 
  Gradebook *gb;
  gb = malloc(sizeof(Gradebook));
 /* gb.name = path;
  gb.shead = NULL;
  gb.stail = NULL;*/
  gb->name = path;
  gb->shead = NULL;
  gb->stail = NULL;
  gb->ahead = NULL;
  gb->atail = NULL;
  //gb.snext = NULL;

  Buffer B = read_from_path(path, key);
  if (B.Length == -1){
	  return 255;
  }
  else if (strlen(B.Buf) > 0){
  	B.Buf[strlen(B.Buf)-1] ='\0';
  }
  char *lines[B.Length];
  int i = 0;
  char *tok = strtok(B.Buf, "\n");
  while(tok != NULL){
	  lines[i] = tok;
	  tok = strtok(NULL, "\n");
	  i++;
  }
  //gb->key = lines[0];
  Student *last = NULL;
  last = malloc(sizeof(Student)); 
  last->snext = NULL;
  //last->ahead = NULL;
  //last->atail = NULL;
  Assignment *alast = NULL;
  alast = malloc(sizeof(Assignment));
  alast->next = NULL;
  Score *scorelast = NULL;
  scorelast = malloc(sizeof(Score));
  scorelast->next = NULL;
  for (i = 0; i<B.Length; i++){
	  int j = 0;
	  int count = 0;
	  char *p = lines[i];
	  for (j = 0; p[j]; j++){
		  if(p[j] == 44){
			  count++;
		  }
	  }
	  count = (count-2)/4;
	  //printf("Assignments cnt: %d\n", count);
	  Student *add = NULL;
	  add = malloc(sizeof(Student));
	  add->snext = NULL;
//	  add->ahead = NULL;
//	  add->atail = NULL;
	  add->fname = strtok(lines[i], ",");
	  tok = strtok(NULL, ",");
	  add->lname = tok;//strtok(NULL, ",");
	  /*if (gb.shead == NULL){
		  gb.shead = add;
		  gb.stail = add;
	  }*/
	  if(gb->shead == NULL){
		  gb->shead = add;
		  gb->stail = add;
	  }
	  else {
		 //last = gb.shead;
		 last = gb->shead;
		 while(last->snext!=NULL){
			 last = last->snext;
		 }
		 last->snext = add;
		 //gb.stail = add;
		 gb->stail = add;
	  }
	  tok = strtok(NULL, ",");
//	  Assignment *alast = NULL;
//	  alast = malloc(sizeof(Assignment));
//	  alast->next = NULL;
	  int loop = 0;
	  while (tok!=NULL && loop ==0){
		Assignment *ass = NULL;
		Score *s = NULL;
		s = malloc(sizeof(Score));
		s->next = NULL;
		ass = malloc(sizeof(Assignment));
		ass->next = NULL;
		ass->name = tok;
		tok = strtok(NULL, ",");
		s->score = atoi(tok);
		/*if (!(s->score >= 0)){
			printf("Int problem");
			s->score = (int*)-1;
		}*/
		//printf(".%s.\n", s->score);
		if (gb->stail->scorehead == NULL){
			
			gb->stail->scorehead = s;
			gb->stail->scoretail = s;
			gb->stail->scorehead->next = NULL;
		}
		else {
			scorelast = gb->stail->scorehead;
			while(scorelast->next != NULL){
				scorelast =scorelast->next;
			}
			scorelast->next = s;
			gb->stail->scoretail = s;
			gb->stail->scoretail->next = NULL;
		}
		//ass->score = (int*) tok;
		tok = strtok(NULL, ",");
		ass->total = atoi(tok);
		tok = strtok(NULL, ",");
		ass->weight = atof(tok);
		//printf("WEIGHT %f\n", ass->weight);
		/*if (gb.stail->ahead == NULL){
			gb.stail->ahead = ass;
			gb.stail->atail = ass;
			gb.stail->ahead->next = NULL;
		}*/
		if (gb->ahead == NULL){
			gb->ahead =ass;
			gb->atail = ass;
			gb->ahead->next = NULL;
		}
		else {
			//alast = gb.stail->ahead;
			alast = gb->ahead;
			int b = 1;
			while(alast!=NULL){
				if (!(strcmp(alast->name, ass->name))){
					b = 0;
				}
				alast = alast->next;
			}
			if (b==1){
				alast = gb->ahead;
				while(alast->next!=NULL){
					alast = alast->next;
				}
				alast->next = ass;
			//gb.stail->atail = ass;
				gb->atail = ass;
			//gb.stail->atail->next = NULL;
				gb->atail->next = NULL;
			}
		}
	
		//printf("%s\n", tok);
		tok = strtok(NULL, ",");
          }
//	  free(alast);
  }
  //free(last);
  //free(alast);
  //printf("Output: %s\n", gb.stail->atail->name);
  //TODO Code this
  *outbuf = gb;
  //printf("Printing %d\n", gb->shead->snext->scorehead->score);
  gb = NULL;
//  free(last);
 // free(alast);
  free(gb);
  return 0;
}
