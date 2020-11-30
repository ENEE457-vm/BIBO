#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <wordexp.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "ctype.h"
#include "data.h"

int verbose = 0;

typedef struct _CmdLineResult {
	int good;
	char *name;
	unsigned char *key;
	char *action;
	char *assignment;
	char *student;
	char *fname;
	char *lname;
	char *option;
} CmdLineResult;

typedef struct Tuple {
	char *fname;
	char *lname;
	int score;
} Tuple;

typedef struct Tup{
	char *fname;
	char *lname;
	float final;
} Tup;

CmdLineResult parsecmdline(int argc, char *argv[]){
	CmdLineResult R = { 0 };
	int r = -1;
	R.good = 0;
	R.option = NULL;
	if(argc==1) {
		printf("No Extra Command Line Argument Passed Other Than Program Name\n");
		R.good = -1;
	}
	else if(argc>=2){
		//printf("Number Of Arguments Passed: %d\n",argc);
		//printf("----Following Are The Command Line Arguments Passed----\n");
		int counter;
		int i;
		int j;
		for(counter=0;counter<argc;counter++){
		//	printf("argv[%d]: %s\n",counter,argv[counter]);
		}
		if (argv[1]!=NULL && !strcmp(argv[1], "-N")){
			R.name = argv[2];
			int length = strlen(R.name);
			for (i = 0; i<length; i++){
				if(!isalnum(R.name[i])){
					R.good = -1;
				}
			}
		}
		else {
			R.good = -1;
		}
		if (argv[3] != NULL && !strcmp(argv[3], "-K")){
			R.key = argv[4];
		}
		else {
			R.good = -1;
		}
		if (argv[5] != NULL && !strcmp(argv[5], "-PA")){
			R.action = "pa";
			if (argc>6){
				for (i = 6; i<argc; i++){
					if (!strcmp(argv[i], "-AN")){
						R.assignment = argv[i+1];
						i++;
					}
					else if (!strcmp(argv[i], "-A") && R.option == NULL){
						R.option = "A";
					}
					else if(!strcmp(argv[i], "-G") && R.option == NULL){
						R.option = "G";
					}
					else{
						R.good = -1;}
				}
			}
			else {
				R.good = -1;
			}
		}
		else if (argv[5] != NULL && !strcmp(argv[5], "-PS")){
			R.action = "ps";
			if (argc>6){
				for (i = 6; i<argc; i+=2){
					if(!strcmp(argv[i], "-FN")){
						R.fname = argv[i+1];
					}
					else if (!strcmp(argv[i], "-LN")){
						R.lname = argv[i+1];
					}
					else {R.good = -1;}
				}
			}
			else {R.good = -1;}
		}
		else if(argv[5] != NULL && !strcmp(argv[5], "-PF")){
			R.action = "pf";
			if (argc == 7){
				if(!strcmp(argv[6], "-A")){
					R.option = "A";
				}
				else if (!strcmp(argv[6], "-G")){
					R.option = "G";
				}
				else {R.good = -1;}
			}
			else{R.good = -1;}
		}
		else{R.good = -1;}
	}
	return R;
}

void print_Gradebook(Gradebook *gradebook) {
  unsigned int i;
  int num_assignment = 0;
  for(i = 0; i < num_assignment; i++) {
    //dump_assignment(&gradebook[i]);
    printf("----------------\n");
  }

  return;
}
void swap(Tuple *xp, Tuple *yp){
	Tuple temp = *xp;
	*xp = *yp;
	*yp = temp;
}

int printAssignment(char* name, char* option, Gradebook **out) {
	Gradebook *gb;
	gb = malloc(sizeof(Gradebook));
	gb = *out;
	Assignment *ass = NULL;
	ass = malloc(sizeof(Assignment));
	int count = 0;
	Student *stud = NULL;
	stud = malloc(sizeof(Student));
	stud = gb->shead;
	ass = gb->ahead;
	while(stud != NULL){
		count++;
		stud = stud->snext;
	}
	Tuple tup [count];
	stud = gb->shead;
	Score *sco = NULL;
	sco = malloc(sizeof(Score));
	if (stud!=NULL){
		sco = stud->scorehead;
	}
	int i = 0;
	if (ass==NULL){
		printf("invalid\n");
		return 255;
	}
	while(stud != NULL){
		sco = stud->scorehead;
		ass = gb->ahead;
		while (ass!=NULL){
			if(!strcmp(ass->name, name)){
				break;
			}
			ass = ass->next;
			sco = sco->next;
		}
		if(ass == NULL){
			printf("invalid\n");
			return 255;
		}
		tup[i].fname = stud->fname;
		tup[i].lname = stud->lname;
		tup[i].score = sco->score;
		i++;
		stud = stud->snext;
	}
	if (option == NULL){
		printf("invalid\n");
		return 0;
	}

	if (!strcmp(option, "G")){
		int j, ind;
		for (i = 0; i<count-1; i++){
			ind = i;
			for(j = i+1; j<count; j++){
				if(tup[j].score > tup[ind].score){
					ind = j;
				}
			}
			/*Tuple *temp = malloc(sizeof(Tuple));
			temp = &tup[ind];
			tup[ind] = tup[j];
			tup[j] = *temp;*/
			swap(&tup[ind], &tup[i]);
		}
	}
	else if (!strcmp(option, "A")){
		int j, ind, k;
		char *temp;
		for (i=0; i<count-1;i++){
			ind = i;
			
			for(j=i+1;j<count;j++){
				if (!strcmp(tup[j].lname, tup[ind].lname)){
					if(strcmp(tup[j].fname, tup[ind].fname)<0){
						ind = j;
					}

				}
				else{
					if(strcmp(tup[j].lname, tup[ind].lname)<0){
						ind = j;
					}
				}
			}
			swap(&tup[ind], &tup[i]);
		}
	}

	for (i = 0; i<count; i++){
		printf("(%s, %s, %d)\n", tup[i].lname, tup[i].fname, tup[i].score);
	}
	ass = NULL;
	stud = NULL;
	gb = NULL;
	sco = NULL;
	free(ass);
	free(gb);
	free(stud);
	free(sco);
  return 0;
}

int print_Student(char *fname, char *lname, Gradebook **out) {
	Gradebook *gb;
	gb = malloc(sizeof(Gradebook));
	gb = *out;
	Student *stud = NULL;
	stud = malloc(sizeof(Student));
	Assignment *ass = NULL;
	ass = malloc(sizeof(Assignment));
	Score *sco = NULL;
	sco = malloc(sizeof(Score));
	stud = gb->shead;
	ass = gb->ahead;
	if (stud!=NULL){
		sco = stud->scorehead;
	}
	while(stud!=NULL){
		sco = stud->scorehead;
		if(!strcmp(stud->fname, fname) && !strcmp(stud->lname, lname)){
			while(ass!=NULL){
				printf("(%s, %d)\n", ass->name, sco->score);
				ass = ass->next;
				sco = sco->next;
			}
			break;
		}
		else{
			stud = stud->snext;
		}
	}
	if (stud == NULL){
		printf("invalid\n");
		return 255;
	}
	return 0;
}

void swaptup(Tup *xp, Tup *yp){
	Tup temp = *xp;
	*xp = *yp;
	*yp = temp;
}

int print_Final(char *option, Gradebook **gradebook){
  Gradebook *gb;
  gb = malloc(sizeof(Gradebook));
  gb = *gradebook;
  Assignment *ass = NULL;
  ass = malloc(sizeof(Assignment));
  Student *stud = NULL;
  stud = malloc(sizeof(Student));
  int count = 0;
  stud = gb->shead;
  ass = gb->ahead;
  while(stud!=NULL){
	  count++;
	  stud = stud->snext;
  }
  Tup tup [count];
  stud = gb->shead;
  Score *sco = NULL;
  sco = malloc(sizeof(Score));
  if (stud!=NULL){
  	sco = stud->scorehead;
  }
  int i = 0;
  float sum = 0;
  while (stud!=NULL){
	  sum = 0;
	  sco = stud->scorehead;
	  ass = gb->ahead;
	  while(ass!=NULL){
		  if (sco->score>=0){
		  	sum += ((float)sco->score/ass->total)*ass->weight;
		  }
		  ass = ass->next;
		  sco = sco->next;
	  }
	  tup[i].fname = stud->fname;
	  tup[i].lname = stud->lname;
	  tup[i].final = sum;
	  i++;
	  stud = stud->snext;
  }
  if (!strcmp(option, "G")){
	  int j, ind;
	  for(i = 0; i<count-1; i++){
		  ind =i;
		  for (j = i+1; j<count; j++){
			  if(tup[j].final > tup[j].final){
				  ind = j;
			  }
		  }
		  swaptup(&tup[ind], &tup[i]);
	  }
  }
  else if (!strcmp(option, "A")){
	  int j,ind;
	  char *temp;
	  for (i = 0; i<count-1; i++){
		  ind = i;
		  for(j=i+1; j<count; j++){
			  if (!strcmp(tup[j].lname, tup[ind].lname)){
				  if(strcmp(tup[j].fname, tup[ind].fname)<0){
					  ind = j;
				  }
			  }
			  else{
				  if(strcmp(tup[j].lname, tup[ind].lname)<0){
					  ind = j;
				  }
			  }
		  }
		  swaptup(&tup[ind], &tup[i]);
	  }
  }
  for(i = 0; i<count; i++){
	  printf("(%s, %s, %.4f)\n", tup[i].lname, tup[i].fname, tup[i].final);
  }
  ass = NULL;
  stud = NULL;
  gb = NULL;
  sco = NULL;
  free(ass);
  free(gb);
  free(stud);
  free(sco);

  return 0;
}


int main(int argc, char *argv[]) {
  int   opt,len;
  char  *logpath = NULL;
  int r;
  CmdLineResult R;
  Gradebook *gb = NULL;
  gb = malloc(sizeof(Gradebook));
  gb->shead = NULL;
  gb->stail = NULL;
  gb->ahead = NULL;
  gb->atail = NULL;
  gb->name = NULL;
  unsigned int out;
  R = parsecmdline(argc, argv);
  if (R.good == -1){
	  printf("invalid\n");
	  return 255;
  }
  else if(R.good == 0) {
    /*printf("Name: %s\n", R.name);
    printf("Key: %s\n", R.key);
    printf("Action: %s\n", R.action);
    printf("Assignment: %s\n", R.assignment);
    printf("Fname: %s\n", R.fname);
    printf("Lname: %s\n", R.lname);
    printf("Option: %s\n", R.option);*/
    r = read_Gradebook_from_path(R.name, R.key, &gb, &out);
    if (r == 255){
	    return 255;
    }
    /*printf("Key from file %s\n", gb->key);
    if (strcmp(R.key, gb->key)){
         printf("invalid key\n");
         return 255;
    }*/
    int i;
    if (R.action == "pa"){
	    int length = strlen(R.assignment);
	    for(i = 0; i<length; i++){
		    if(!isalnum(R.assignment[i])){
			    printf("invalid\n");
			    return 255;
		    }
	    }
	    r = printAssignment(R.assignment, R.option, &gb);
    }
    else if (R.action == "ps"){
	    r = print_Student(R.fname, R.lname, &gb);
    }
    else if (R.action == "pf"){
	    r = print_Final(R.option, &gb);
    }
    else {
	    printf("invalid\n");
	    return 255;
    }
    //write_to_path(R.name, R.key, &gb, &out);
  }
  gb = NULL;
  free(gb);
  return r;
}
