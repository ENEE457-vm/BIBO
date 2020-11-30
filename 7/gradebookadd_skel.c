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
#include "ctype.h"

typedef struct _CmdLineResult {
  //TODO probably put more things here
  int good;
  char *name;
  unsigned char *key;
  char *action;
  char *assignment;
  char *student;
  int total;
  float weight;
  char *fname;
  char *lname;
  int score;

} CmdLineResult;

int do_batch(char *);

CmdLineResult parse_cmdline(int argc, char *argv[]) {
  CmdLineResult R = { 0 };
  int opt,r = -1;

  R.good = 0;

    if(argc==1) {
        printf("No Extra Command Line Argument Passed Other Than Program Name\n"); 
    	R.good = -1;
    }
    if(argc>=2) 
    { 
        //printf("Number Of Arguments Passed: %d\n",argc); 
        //printf("----Following Are The Command Line Arguments Passed----\n"); 
	int counter;
        for(counter=0;counter<argc;counter++){
          //	printf("argv[%d]: %s\n",counter,argv[counter]);
	}
		if (!strcmp(argv[1], "-N")){
			R.name = argv[2];
		}
		else{
			printf("No gradebook name given\n");
			R.good = -1;
		}
		if (argv[3] != NULL && !strcmp(argv[3], "-K")){
			R.key = argv[4];
		}
		else{
			R.good = -1;
		}
		int i;
		if (argv[5] != NULL && !strcmp(argv[5], "-AS")){
			R.action = "as";
			if (argc>6){
				for (i = 6; i < argc; i+=2){
			
					if (!strcmp(argv[i], "-FN")){
						R.fname = argv[i+1];
					}
					else if (!strcmp(argv[i], "-LN")){
						R.lname = argv[i+1];
					}
					else{R.good = -1;}
				}
			}
			else{R.good = -1;}
		}
		
		else if(argv[5] != NULL && !strcmp(argv[5], "-AG")){
			R.action = "ag";
			if (argc >6){
				for (i = 6; i<argc; i+=2){
					if (!strcmp(argv[i], "-AN")){
						R.assignment = argv[i+1];
					}
					else if(!strcmp(argv[i], "-FN")){
						R.fname = argv[i+1];
					}
					else if (!strcmp(argv[i], "-LN")){
						R.lname = argv[i+1];
					}
					else if (!strcmp(argv[i], "-G")){
						R.score = atoi(argv[i+1]);
						}
					else{R.good = -1;}
				}
			}
			else{R.good = -1;}
		}

		else if (argv[5] != NULL && !strcmp(argv[5], "-DA")){
			R.action = "da";
			if (argc >6){
				for (i = 6; i<argc; i+=2){
					if (!strcmp(argv[i], "-AN")){
						R.assignment = argv[i+1];
					}
					else{R.good = -1;}
				}
			}
			else{R.good = -1;}
		}
		else if (argv[5] != NULL && !strcmp(argv[5], "-DS")){
			R.action = "ds";
			if (argc > 6){
				for (i = 6; i<argc; i+=2){
					if (!strcmp(argv[i], "-FN")){
						R.fname = argv[i+1];
					}
					else if (!strcmp(argv[i], "-LN")){
						R.lname = argv[i+1];
					}
					else{R.good = -1;}
				}
			}
			else {R.good = -1;}
		}

		else if(argv[5] != NULL && !strcmp(argv[5], "-AA")){
			R.action = "aa";
			if (argc > 6){
				for (i = 6; i<argc; i+=2){
					if(!strcmp(argv[i], "-AN")){
						R.assignment = argv[i+1];
					}
					else if(!strcmp(argv[i], "-P")){
						R.total = atoi(argv[i+1]);
					}
					else if(!strcmp(argv[i], "-W")){
						R.weight = atof(argv[i+1]);
					}
					else{R.good = -1;}
				}
			}
			else {R.good = -1;}
		}
		else {R.good = -1;}

    } 

  //TODO do stuff
 /* if(file != NULL) {
    R.good = do_something(file);
  } else {
    //TODO do stuff
    R.good = 0;
  }
*/
  return R;
}

int addGrade(char *fname, char *lname, char *aname, int grade, Gradebook **out){
	Gradebook *gb;
	gb = malloc(sizeof(Gradebook));
	gb = *out;
	Student *stud = NULL;
	stud = malloc(sizeof(Student));
	stud = gb->shead;
	while (stud!= NULL && (strcmp(stud->fname, fname) || strcmp(stud->lname, lname))){
		stud = stud->snext;
	}
	if (stud == NULL){
		printf("invalid\n");
		return 255;
	}
	Assignment *ass = NULL;
	ass = malloc(sizeof(Assignment));
	ass = gb->ahead;
	Score *last = NULL;
	last = malloc(sizeof(Score));
	last = stud->scorehead;
	if (grade<0){
		printf("invalid\n");
		return 255;
	}
	while (ass!=NULL){
		if (last == NULL){
			printf("invalid\n");
			return 255;
		}
		if (!strcmp(ass->name, aname)){
			last->score = grade;
		 	break;	
		}
		ass = ass->next;
		last = last->next;
		
	}
	//printf("YEAH\n");
	//printf("Tail Real: %d\n", stud->scorehead->score);
	if (ass == NULL){
		printf("invalid\n");
		return 255;
	}
	//printf("Started from the bottom\n");
	*out = gb;
	ass = NULL;
	stud = NULL;
	//last = NULL;
	gb = NULL;
	free(ass);
	free(stud);
	//free(last);
	free(gb);
	return 1;
}

int initGrade(char *fname, char *lname, char *aname, Gradebook **out){
	Gradebook *gb;
	gb = malloc(sizeof(Gradebook));
	gb = *out;
	Student *stud = NULL;
	stud = malloc(sizeof(Student));
	stud = gb->shead;
	Assignment *ass = NULL;
	ass = malloc(sizeof(Assignment));
	ass = gb->ahead;
	while (stud != NULL && ((stud->fname!= fname) || (stud->lname!= lname))){
		stud = stud->snext;
	}
	if (stud == NULL){
		printf("invalid\n");
		return 255;
	}
	Score *add = NULL;
	add = malloc(sizeof(Score));
	add->next = NULL;
	int val = -1;
	add->score = val;
	Score *last = NULL;
	last = malloc(sizeof(Score));
	if(stud->scorehead == NULL){
		stud->scorehead = add;
		stud->scoretail = add;
		stud->scorehead->next = NULL;
	}
	else{
		last = stud->scorehead;
		while(last->next !=NULL){
			last = last->next;
		}
		last->next = add;
		stud->scoretail = add;
		stud->scoretail->next = NULL;
	}

	*out = gb;
	gb= NULL;
	ass = NULL;
	stud = NULL;
	last = NULL;
	free(ass);
	free(stud);
	free(last);
	free(gb);
	return 1;
}

int addAssignment(char* name, int total, float weight, Gradebook **out){
	Gradebook *gb;
	gb = malloc(sizeof(Gradebook));
	gb = *out;
	Assignment *add = NULL;
	add = malloc(sizeof(Assignment));
	add->next = NULL;
	add->name = name;
	add->total = total;
	add->weight = weight;
	Assignment *last = NULL;
	last = malloc(sizeof(last));
	if (total == 0 || weight<=0){
		printf("invalid\n");
		return 255;
	}
	float sum = 0;
	if (gb->ahead == NULL){
		gb->ahead = add;
		gb->atail = add;
		gb->ahead->next = NULL;
		
	}
	else {
		last = gb->ahead;
		sum += last->weight;
		while (last->next != NULL){
			last = last->next;
			if (!strcmp(last->name, add->name)){
				printf("invalid\n");
				return 255;
			}
			sum += last->weight;
		}
		sum += add->weight;
		if (sum <= 1){
			last->next = add;
			gb->atail = add;
			gb->atail->next = NULL;
		}
		else {
			printf("invalid\n");
			return 255;
		}
	}
	//Init grade for this assignment for each student
	Student *temp;
	temp = malloc(sizeof(Student));
	if (gb->shead != NULL){
		temp = gb->shead;
		while (temp!=NULL){
			initGrade(temp->fname, temp->lname, gb->atail->name, &gb);
			temp = temp->snext;
		}
	}
	temp = NULL;
	free(temp);

	*out = gb;
	gb = NULL;
	last = NULL;
	free(gb);
	free(last);

	return 1;
}

int deleteAssignment(char *name, Gradebook **out){
	Gradebook *gb;
	gb = malloc(sizeof(Gradebook));
	gb = *out;
	Assignment *atemp = NULL;
	atemp = malloc(sizeof(Assignment));
	Assignment *aprev = NULL;
	aprev = malloc(sizeof(Assignment));
	Score *stemp = NULL;
	Score *sprev = NULL;
	Student *stud = NULL;
	stud = malloc(sizeof(Student));
	stud = gb->shead;
	stemp = malloc(sizeof(Score));
	sprev = malloc(sizeof(Score));
	atemp = gb->ahead;
	stemp = gb->shead->scorehead;
	if (atemp != NULL && !strcmp(atemp->name, name)){
		gb->ahead = atemp->next;
		while (stud != NULL){
			stemp = stud->scorehead;
			stud->scorehead = stemp->next;
			stud = stud->snext;
		}
		//free(aprev);
		//free(stud);
		//free(stemp);
		//free(sprev);
	}
	else {
		int count = 0;
		while (atemp != NULL && strcmp(atemp->name, name)){
			aprev = atemp;
			atemp = atemp->next;
			count++;
		}
		if (atemp == NULL){
			printf("invalid\n");
			return 255;
		}
		aprev->next = atemp->next;
		while (stud != NULL){
			stemp = stud->scorehead;
			int count2 = 0;

			while (count2 <count){
				sprev = stemp;
				stemp = stemp->next;
				count2++;
			}
			sprev->next = stemp->next;
			stud = stud->snext;
		}
		//free(stud);
		//free(stemp);
		//free(sprev);
		//free(atemp);
		//free(aprev);
	}
	*out = gb;
	return 0;





}

int deleteStudent(char *fname, char *lname, Gradebook **out){
	Gradebook *gb;
	gb = malloc(sizeof(Gradebook));
	gb = *out;
	Student *temp = NULL;
	temp = malloc(sizeof(Student));
	Student *prev = NULL;
	prev = malloc(sizeof(Student));
	temp = gb->shead;
	if (temp!= NULL && (!strcmp(fname, temp->fname) && !strcmp(lname, temp->lname))){
				gb->shead = temp->snext;
				free(temp);
	}
	else {
		while(temp!=NULL && (strcmp(fname, temp->fname) || strcmp(lname, temp->lname))){
			prev = temp;
			temp = temp->snext;
		}
		if (temp == NULL){
			printf("invalid\n");
			return 255;
		}
		prev->snext = temp->snext;
		free(temp);
	}
	*out = gb;
	gb = NULL;
	free(gb);
	return 1;

}

int addStudent(char *fname, char *lname, Gradebook **out){
	Gradebook *gb;
	gb = malloc(sizeof(Gradebook));
	gb = *out;
	Student *add = NULL;
	Student *last = NULL;
	last = malloc(sizeof(Student));
	add = malloc(sizeof(Student));
	add->snext = NULL;
	last->snext = NULL;
	//last->ahead = NULL;
	//last->atail = NULL;
	//add->ahead = NULL;
	//add->atail = NULL;
	add->fname = fname;
	add->lname = lname;
	if (gb->shead == NULL){
		gb->shead = add;
		gb->stail = add;
	}
	else {
		last = gb->shead;
		while (last != NULL){
			if (!strcmp(last->fname, fname) && !strcmp(last->lname, lname)){
				printf("invalid\n");
				return 255;
			}
			last = last->snext;
		}
		last = gb->shead;
		while (last->snext!=NULL){
			last = last->snext;
		}
		last->snext = add;
		gb->stail = add;
	}
	Assignment *ass = NULL;
	ass = malloc(sizeof(Assignment));
	ass = gb->ahead;
	int result = 0;
	while (ass!= NULL){
		result = initGrade(gb->stail->fname, gb->stail->lname,ass->name, &gb);
		ass = ass->next;
	}
	*out = gb;
	ass = NULL;
	free(ass);
	gb = NULL;
	free(gb);
	return result;
}


int main(int argc, char *argv[]) {
  int r = 0;
  CmdLineResult R;
  Gradebook *gb = NULL;
  gb = malloc(sizeof(Gradebook));
  gb->shead = NULL;
  gb->stail = NULL;
  gb->ahead = NULL;
  gb->atail = NULL;
  gb->name = NULL;
  unsigned int out;
  R = parse_cmdline(argc, argv);
  if (R.good == -1){
	  printf("invalid\n");
	  return 255;
  }
  if(R.good == 0) {
    /*printf("Name: %s\n", R.name);
    printf("Key: %s\n", R.key);
    printf("Action: %s\n", R.action);
    printf("Assignment: %s\n", R.assignment);
    printf("Points: %d\n", R.total);
    printf("Weight: %.2f\n", R.weight);
    printf("Fname: %s\n", R.fname);
    printf("Lname: %s\n", R.lname);
    printf("Grade: %d\n", R.score);*/
    r = read_Gradebook_from_path(R.name, R.key, &gb, &out);
    if (r == 255){
	    printf("invalid\n");
	    return 255;
    }
    //printf("Key from file %s\n", gb->key);
    /*if (strcmp(R.key, gb->key)){
	    printf("invalid key\n");
	    return 255;
    }*/

    int i;
    if (R.action == "as"){
	   int length = strlen(R.fname);
	   int name = strlen(R.lname);
	   for (i = 0; i<length; i++){
		   if(!isalpha(R.fname[i])){
			  printf("invalid\n");
			  return 255; 
		   }
	   }
	   for ( i =0; i<name; i++){
		   if (!isalpha(R.lname[i])){
			   printf("invalid\n");
			   return 255;
		   }
	   }
	   r = addStudent(R.fname, R.lname, &gb);
    }
    else if (R.action == "aa") {
	    int length = strlen(R.assignment);
	    for (i = 0; i<length; i++){
		    if(!isalnum(R.assignment[i])){
			    printf("invalid\n");
			    return 255;
		    }
	    }
	    if (R.total < 0){
		    printf("invalid\n");
		    return 255;
	    }
	    r = addAssignment(R.assignment, R.total,R.weight, &gb);
	    
    }
    else if (R.action == "ag"){
	    r = addGrade(R.fname, R.lname, R.assignment, R.score, &gb);
    } 
    else if (R.action == "ds"){
	    r = deleteStudent(R.fname, R.lname, &gb);
    }
    else if (R.action == "da"){
	    r = deleteAssignment(R.assignment, &gb);
    }
    //write the result back out to the file
    write_to_path(R.name, R.key, &gb, &out);
  }
  gb = NULL;
  free(gb);
  return r;
}
