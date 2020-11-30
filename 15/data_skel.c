#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "data.h"

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/comp.h>

//#define DEBUG

void printCmdLineResults(CmdLineResult R){
  printf("---- Cmd Line Result ----\n");
  printf("name: %s\n",R.name);
  printf("key: %s\n",R.key);
  printf("action: %s\n", R.action);
  printf("----action options---\n");
  for(int i=0;i<R.numActionOptions;i++){
      printf("%s   %s\n",&R.actionOption[i][0], &R.actionOptionSpecifier[i][0]);
  }
  printf("Number of Action Options: %d\n", R.numActionOptions);

  printf("Results (0=good):%d\n", R.good);
  return;
}

// Parse the name and key actions are present and validiate the specifier format
int parse_name_and_key(CmdLineResult *R, char *argv[]){
  const char nameSpecifier[] = "-N";
  const char keySpecifier[] = "-K";
  // Check if the Name and Key Options have been specified    
    if(strcmp(nameSpecifier,argv[1])!=0 || strcmp(keySpecifier, argv[3])!=0){
         return invalid;
    }
    else{
      // Get the name and check validity
      if(strlen(argv[2])>maxLength){
        memcpy(R->name,argv[2],maxLength);
        R->name[maxLength]='\0';
      }
      else{
        strcpy(R->name, argv[2]);
      }
      if(check_valid_specifier(R->name,fileNameOption)!=valid){
        return invalid;
      }
      // Get the key and check validity
      if(strlen(argv[4])>maxLength){
        memcpy(R->key,argv[4],maxLength);
        R->key[maxLength]='\0';
      }
      else{
        strcpy(R->key, argv[4]);
      }
      if(check_valid_specifier(R->key,keyOption)!=valid){
        return invalid;
      }
    }
    return valid;
}

//Check if the specifier string is a valid input
int check_valid_specifier(char *str,int optionType){
  int i=0;
  char ch=str[i];
  switch(optionType){
    // File name must be alphanumeric including underscores and periods
    case fileNameOption:
      while(ch != '\0'){
        if(isalnum(ch)){
          i++;
          ch =str[i];
          continue;
        }
        else if(ch=='.'||ch=='_'){
          i++;
          ch =str[i];
          continue;
        }
        else{
          #ifdef DEBUG
            printf("file name option invalid!\n");
          #endif
          return invalid;
        }
      }
      return valid;
      break;
      // Key must be a string of hex digits
    case keyOption:
      while(ch != '\0'){
        if('0'<=ch && ch <='9'|| 'a'<=ch && ch<='f' || 'A'<=ch && ch<='F'){
          i++;
          ch =str[i];
          continue;
        }
        else{
          return invalid;
        }
      }
      return valid;
      break;
      // Student name must be alphabet characters upper or lowercase
    case studentNameOption:
      while(ch != '\0'){
        if('a'<=ch && ch<='z' || 'A'<=ch && ch<='Z'){
          i++;
          ch =str[i];
          continue;
        }
        else{
          return invalid;
        }
      }
      return valid;
      break;
      // Assignment name must be alphabet characters and numbers (0-9)
    case assignmentNameOption:
    //printf("here\n");
      while(ch != '\0'){
        if(isalnum(ch)){
          i++;
          ch =str[i];
          continue;
        }
        else{
          return invalid;
        }
      }
      return valid;
      break;
      // Assignment points must be non-negative integer
    case assignmentPointsOption:
      if(ch=='0'){
        return invalid; // integer has no leading zeros
      }
      while(ch != '\0'){
        if('0'<=ch && ch<='9'){
          i++;
          ch =str[i];
          continue;
        }
        else{
          return invalid;
        }
      }
      return valid;
      break;
      // Assignment Weight must be a real number[0,1]
    case assignmentWeightOption:{
      int decimalfound=-1;
      double weight;
      while(ch != '\0'){
        if('0'<=ch && ch<='9'){
          i++;
          ch =str[i];
          continue;
        }
        else if(ch=='.'){
          if(decimalfound==0){
            // 2 decimal points found
            return invalid;
          }
          i++;
          ch=str[i];
          decimalfound=0;
        }
        else{
          return invalid;
        }
      }
      weight = (double)atof(str);
      if(weight<0||weight>1){
        return invalid;
      }
      //weight has less precision then string, update string as weight
      gcvt(weight, maxLength,str);
      return valid;
      break;
    }
    // Assignment grade must be non-negative integer
    case assignmentGradeOption:
      if(ch=='0'){
        return invalid; // integer has no leading zeros
      }
      while(ch != '\0'){
        if('0'<=ch && ch<='9'){
          i++;
          ch =str[i];
          continue;
        }
        else{
          return invalid;
        }
      }
      return valid;
      break;
    deafult:
      #ifdef DEBUG
        printf("Not a valid action option!\n");
      #endif
      break;
  }

  return 1;
}

// Parse and check if the input action is a valid action 
int parse_action_add(CmdLineResult *R, char *argv[]){
    const char actions[numActionsAdd][4]= {"-AA", "-DA", "-AS", "-DS", "-AG"};
    for(int i=0;i<numActionsAdd;i++){
      if(strcmp(&actions[i][0],argv[5])==0){
        strcpy(R->action,&actions[i][0]);
        return valid;
      }
    }
  return invalid;
}

// Parse and check if the input action is a valid action 
int parse_action_display(CmdLineResult *R, char *argv[]){
    const char actions[numActionsDisplay][4]= {"-PA", "-PS", "-PF"};
    for(int i=0;i<numActionsDisplay;i++){
      if(strcmp(&actions[i][0],argv[5])==0){
        strcpy(R->action,&actions[i][0]);
        return valid;
      }
    }
  return invalid;
}

// Check if the specifier for the action option is valid
int check_valid_action_option_specifier(CmdLineResult *R,char *current_action_str, int current_action_num,char *argv[], int current_arg){
  char current_actionOptionSpecifier[maxLength+1]; // +1 for NULL terminating character
  if(strcmp(current_action_str,"-AN")==0){
    if(strlen(argv[current_arg])>maxLength){
        memcpy(current_actionOptionSpecifier,argv[current_arg],maxLength);
        current_actionOptionSpecifier[maxLength]='\0';
    }
    else{
      strcpy(current_actionOptionSpecifier, argv[current_arg]);
    }
    strcpy(&R->actionOptionSpecifier[current_action_num][0],current_actionOptionSpecifier);
    if(check_valid_specifier(&R->actionOptionSpecifier[current_action_num][0],assignmentNameOption)!=valid){
      #ifdef DEBUG
      printf("Assignement Name can only contain alphabet characters and numbers (0-9)\n");
      #endif
      return invalid;
    }
  }
  else if(strcmp(current_action_str,"-FN")==0){
    if(strlen(argv[current_arg])>maxLength){
      memcpy(current_actionOptionSpecifier,argv[current_arg],maxLength);
      current_actionOptionSpecifier[maxLength]='\0';
    }
    else{
      strcpy(current_actionOptionSpecifier, argv[current_arg]);
    }
    strcpy(&R->actionOptionSpecifier[current_action_num][0],current_actionOptionSpecifier);
    if(check_valid_specifier(&R->actionOptionSpecifier[current_action_num][0],studentNameOption)!=valid){
      #ifdef DEBUG
      printf("First Name can only contain alphabet characters\n");
      #endif
      return invalid;
    }
  }
  else if(strcmp(current_action_str,"-LN")==0){
     if(strlen(argv[current_arg])>maxLength){
      memcpy(current_actionOptionSpecifier,argv[current_arg],maxLength);
      current_actionOptionSpecifier[maxLength]='\0';
    }
    else{
      strcpy(current_actionOptionSpecifier, argv[current_arg]);
    }
    strcpy(&R->actionOptionSpecifier[current_action_num][0],current_actionOptionSpecifier);
    if(check_valid_specifier(&R->actionOptionSpecifier[current_action_num][0],studentNameOption)!=valid){
      #ifdef DEBUG
      printf("Last Name can only contain alphabet characters\n");
      #endif
      return invalid;
    }   
  }
  else if(strcmp(current_action_str,"-P")==0){
    if(strlen(argv[current_arg])>maxLength){
      memcpy(current_actionOptionSpecifier,argv[current_arg],maxLength);
      current_actionOptionSpecifier[maxLength]='\0';
    }
    else{
      strcpy(current_actionOptionSpecifier, argv[current_arg]);
    }
    strcpy(&R->actionOptionSpecifier[current_action_num][0],current_actionOptionSpecifier);
    if(check_valid_specifier(&R->actionOptionSpecifier[current_action_num][0],assignmentPointsOption)!=valid){
      #ifdef DEBUG
      printf("Number of points assignment must be nonnegative integer\n");
      #endif
      return invalid;
    }  
  }
  else if(strcmp(current_action_str,"-W")==0){
    if(strlen(argv[current_arg])>maxLength){
      memcpy(current_actionOptionSpecifier,argv[current_arg],maxLength);
      current_actionOptionSpecifier[maxLength]='\0';
    }
    else{
      strcpy(current_actionOptionSpecifier, argv[current_arg]);
    }
    strcpy(&R->actionOptionSpecifier[current_action_num][0],current_actionOptionSpecifier);
    if(check_valid_specifier(&R->actionOptionSpecifier[current_action_num][0],assignmentWeightOption)!=valid){
      #ifdef DEBUG
      printf("Weight of assignment must be a real number in [0,1]\n");
      #endif
      return invalid;
    }  
  }
  else if(strcmp(current_action_str,"-G")==0){
    if(strlen(argv[current_arg])>maxLength){
      memcpy(current_actionOptionSpecifier,argv[current_arg],maxLength);
      current_actionOptionSpecifier[maxLength]='\0';
    }
    else{
      strcpy(current_actionOptionSpecifier, argv[current_arg]);
    }
    strcpy(&R->actionOptionSpecifier[current_action_num][0],current_actionOptionSpecifier);
    if(check_valid_specifier(&R->actionOptionSpecifier[current_action_num][0],assignmentGradeOption)!=valid){
      #ifdef DEBUG
      printf("Grade Student Receives must be nonnegative integer\n");
      #endif
      return invalid;
    }    
  }
  return valid;
}

// validate that the correct action options were specified for the action
int check_valid_action_option(CmdLineResult R){
    // At this point the CmdLineResult contains distinct action options, check if they are valid
    const char actionsAA[numActionsAA][4]= {"-AN", "-P", "-W"};
    const char actionsDA[numActionsDA][4]= {"-AN"};
    const char actionsAS[numActionsAS][4]= {"-FN", "-LN"};
    const char actionsDS[numActionsDS][4]= {"-FN", "-LN"};
    const char actionsAG[numActionsAG][4]= {"-AN", "-FN", "-LN", "-G"};
    const char actionsPA[numActionsPA][4]= {"-AN", "-A", "-G"};
    const char actionsPS[numActionsPS][4]= {"-FN","-LN"};
    const char actionsPF[numActionsPF][4]= {"-A","-G"};
  // Assignment Add
  if(strcmp(R.action,"-AA")==0){
    int numMatchedActions=0;
    int matchedActionFound=-1;
    for(int i=0; i<R.numActionOptions;i++){
      matchedActionFound=-1;
      for(int j=0; j< numActionsAA;j++){
        if(strcmp(&R.actionOption[i][0], &actionsAA[j][0])==0){
          numMatchedActions++;
          matchedActionFound=0;
          break;
        }
      }
      if(matchedActionFound==-1){
        #ifdef DEBUG
          printf("Wrong Action Specified for AA!\n");
        #endif
        return invalid;
      }
    }
    if(numMatchedActions!=numActionsAA){
      #ifdef DEBUG
        printf("Not enough Action Options Specified for AA!\n");
      #endif
      return invalid;
    }
    return valid;
  }
  // Delete Assignment
  else if(strcmp(R.action,"-DA")==0){
    int numMatchedActions=0;
    int matchedActionFound=-1;
    for(int i=0; i<R.numActionOptions;i++){
      matchedActionFound=-1;
      for(int j=0; j< numActionsDA;j++){
        if(strcmp(&R.actionOption[i][0], &actionsDA[j][0])==0){
          numMatchedActions++;
          matchedActionFound=0;
          break;
        }
      }
      if(matchedActionFound==-1){
        #ifdef DEBUG
         printf("Wrong Action Specified for DA!\n");
        #endif
        return invalid;
      }
    }
    if(numMatchedActions!=numActionsDA){
      #ifdef DEBUG
        printf("Not enough Action Options Specified for DA!\n");
      #endif
      return invalid;
    }
    return valid;
  }
  // Add Student
  else if(strcmp(R.action,"-AS")==0){
    int numMatchedActions=0;
    int matchedActionFound=-1;
    for(int i=0; i<R.numActionOptions;i++){
      matchedActionFound=-1;
      for(int j=0; j< numActionsAS;j++){
        if(strcmp(&R.actionOption[i][0], &actionsAS[j][0])==0){
          numMatchedActions++;
          matchedActionFound=0;
          break;
        }
      }
      if(matchedActionFound==-1){
        #ifdef DEBUG
         printf("Wrong Action Specified for AS!\n");
        #endif
        return invalid;
      }
    }
    if(numMatchedActions!=numActionsAS){
      #ifdef DEBUG
        printf("Not enough Action Options Specified for AS!\n");
      #endif
      return invalid;
    }
    return valid;
  }
  // Delete Student
  else if(strcmp(R.action,"-DS")==0){
    int numMatchedActions=0;
    int matchedActionFound=-1;
    for(int i=0; i<R.numActionOptions;i++){
      matchedActionFound=-1;
      for(int j=0; j< numActionsDS;j++){
        if(strcmp(&R.actionOption[i][0], &actionsDS[j][0])==0){
          numMatchedActions++;
          matchedActionFound=0;
          break;
        }
      }
      if(matchedActionFound==-1){
        #ifdef DEBUG
         printf("Wrong Action Specified for DS!\n");
        #endif
        return invalid;
      }
    }
    if(numMatchedActions!=numActionsDS){
        #ifdef DEBUG
         printf("Not enough Action Options Specified for DS!\n");
        #endif
      return invalid;
    }
    return valid;
  }
  // Add Grade
  else if(strcmp(R.action,"-AG")==0){
    int numMatchedActions=0;
    int matchedActionFound=-1;
    for(int i=0; i<R.numActionOptions;i++){
      matchedActionFound=-1;
      for(int j=0; j< numActionsAG;j++){
        if(strcmp(&R.actionOption[i][0], &actionsAG[j][0])==0){
          numMatchedActions++;
          matchedActionFound=0;
          break;
        }
      }
      if(matchedActionFound==-1){
        #ifdef DEBUG
         printf("Wrong Action Specified for AG!\n");
        #endif
        return invalid;
      }
    }
    if(numMatchedActions!=numActionsAG){
        #ifdef DEBUG
         printf("Not enough Action Options Specified for AG!\n");
        #endif
      return invalid;
    }
    return valid;
  }
  //Print Assignment
  else if (strcmp(R.action, "-PA")==0){
    int numMatchedActions=0;
    int matchedActionFound=-1;
    for(int i=0; i<R.numActionOptions;i++){
      matchedActionFound=-1;
      for(int j=0; j< numActionsPA;j++){
        if(strcmp(&R.actionOption[i][0], &actionsPA[j][0])==0){
          numMatchedActions++;
          matchedActionFound=0;
          break;
        }
      }
      if(matchedActionFound==-1){
        #ifdef DEBUG
         printf("Wrong Action Specified for PA!\n");
        #endif
        return invalid;
      }
    }
    if(numMatchedActions!=totalActionsPA){
        #ifdef DEBUG
         printf("Not enough Action Options Specified for PA!\n");
        #endif
      return invalid;
    }
    return valid;
  }
    //Print Student
  else if (strcmp(R.action, "-PS")==0){
    int numMatchedActions=0;
    int matchedActionFound=-1;
    for(int i=0; i<R.numActionOptions;i++){
      matchedActionFound=-1;
      for(int j=0; j< numActionsPS;j++){
        if(strcmp(&R.actionOption[i][0], &actionsPS[j][0])==0){
          numMatchedActions++;
          matchedActionFound=0;
          break;
        }
      }
      if(matchedActionFound==-1){
        #ifdef DEBUG
         printf("Wrong Action Specified for PS!\n");
        #endif
        return invalid;
      }
    }
    if(numMatchedActions!=numActionsPS){
        #ifdef DEBUG
         printf("Not enough Action Options Specified for PS!\n");
        #endif
      return invalid;
    }
    return valid;
  }
    //Print Final
  else if (strcmp(R.action, "-PF")==0){
    int numMatchedActions=0;
    int matchedActionFound=-1;
    for(int i=0; i<R.numActionOptions;i++){
      matchedActionFound=-1;
      for(int j=0; j< numActionsPF;j++){
        if(strcmp(&R.actionOption[i][0], &actionsPF[j][0])==0){
          numMatchedActions++;
          matchedActionFound=0;
          break;
        }
      }
      if(matchedActionFound==-1){
        #ifdef DEBUG
         printf("Wrong Action Specified for PF!\n");
        #endif
        return invalid;
      }
    }
    if(numMatchedActions!=totalActionsPF){
        #ifdef DEBUG
         printf("Not enough Action Options Specified for PF!\n");
        #endif
      return invalid;
    }
    return valid;
  }
  else{
    #ifdef DEBUG
      printf("Something went wrong!\n");
    #endif
      return invalid;
  }
  return invalid;
}

// Parse the action options and validate
int parse_action_option(CmdLineResult *R, char *argv[],int argc){
  const char actionOptions[numTotalActionOptions][4]={"-AN","-FN","-LN","-P","-W","-G", "-A"};
  int valid_action=invalid;
  // Parse Action Option
  valid_action=invalid;
  char current_action_str[3];
  int current_action_num=-1;
  // iterate through the arguments
  for(int i =6; i<argc;i++){
    //printf("current looking at argument %d\n", i);
    // if the last argument was an action option, this argument may be an action option specifier.
    if(current_action_num>-1){
     //  printf("the last action is %s\n",current_action_str);
      // if the current action is -PA, -PS, -PF and the option is -A or -G, not an action option specifier.
      if((strcmp(R->action, "-PA")==0 || strcmp(R->action, "-PS")==0|| strcmp(R->action, "-PF")==0) &&
                               (strcmp(current_action_str,"-A")==0 || strcmp(current_action_str,"-G")==0)){
        strcpy(&R->actionOptionSpecifier[current_action_num][0],"");// no action option specifier for this option
        current_action_num =-1;
      }
      else{
        if(check_valid_action_option_specifier(R, current_action_str, current_action_num, argv, i)!=valid){
          return invalid;
        }
          current_action_num=-1;
          continue;   
      }
    }
    // for each argument, look for a valid action option
    valid_action = invalid;
   // printf("checking if %s is valid\n", argv[i]);
    for(int j =0;j< numTotalActionOptions;j++){
     // printf("comparing with %s\n", &actionOptions[j][0]);
      if(strcmp(&actionOptions[j][0], argv[i])==0){
       // printf("found a valid action option!\n");
        // if the argument is a valid action option, check if the action option already exists in the cmd line results
          for(int k=0;k<R->numActionOptions;k++){
            if(strcmp(&R->actionOption[k][0], &actionOptions[j][0])==0){
               // if the action option is already exists, break
               // update the action option specifier on the next argument
              strcpy(current_action_str, &actionOptions[j][0]);
             // printf("already exists!, the action str is %s\n", current_action_str);
              current_action_num=k;
              valid_action = valid;
              break;
            }
          }
          if(valid_action == valid){
            break;
          }
        // specified action option is valid and new, add to the cmdline results
        strcpy(current_action_str, &actionOptions[j][0]);
        strcpy(&R->actionOption[R->numActionOptions][0],&actionOptions[j][0]);
        current_action_num = R->numActionOptions;
        R->numActionOptions++;
        // Cannot exceed the max total of number of Action Options
        if(R->numActionOptions > maxSpecifiedActionOptions){
          return invalid;
        }
        valid_action = valid;
          if(i == argc-1 && (strcmp(current_action_str,"-AN")==0 || strcmp(current_action_str,"-FN")==0 || strcmp(current_action_str, "-LN")==0 ||
            strcmp(current_action_str, "-P")==0 || strcmp(current_action_str, "-W")==0 || (strcmp(current_action_str,"-G")==0 && R->action[1]!='P'))){
            return invalid; // no specifier exists for the last action option, invalid
          }
        break;
      }
    }
    if(valid_action==invalid){
      return invalid;
    }
  }

  // Check if action options are valid for the action
  if(check_valid_action_option(*R)==invalid){
    return invalid;
  }
  return valid;
}

CmdLineResult parse_cmdline(int argc, char *argv[], ProgramType progType) {
  CmdLineResult R = { 0 };
  int opt,r = -1;
  struct stat buffer;
  int valid_action=invalid;
  R.good = -1;

  if(argc==1){ 
      printf("invalid\n");
      return R;
  }
  if(argc>=2) 
  { 
    #ifdef DEBUG
      printf("Number Of Arguments Passed: %d\n",argc); 
      printf("----Following Are The Command Line Arguments Passed----\n"); 
      for(int counter=0;counter<argc;counter++){
        printf("argv[%d]: %s\n",counter,argv[counter]); 
      }
    #endif
  } 
  //Initial Check: There must atleast be 7 arguments
  if(argc<7){
    #ifdef DEBUG
      printf("arg count less than 7\n");
    #endif
    printf("invalid\n");
    return R;
  } 
  // Parse Name and Key     
  if(parse_name_and_key(&R, argv)==invalid){
    #ifdef DEBUG
      printf("-N or -K not specified\n");
    #endif
    printf("invalid\n");
    return R;
  }

  //Parse Action depending on the program
  if(progType == addType){
    if(parse_action_add(&R, argv)==invalid){
      #ifdef DEBUG
        printf("No valid action specified\n");
      #endif
      printf("invalid\n");
      return R;
    }
  }
  else{
    if(parse_action_display(&R, argv)==invalid){
      #ifdef DEBUG
        printf("No valid action specified\n");
      #endif
      printf("invalid\n");
      return R;
    }
  }

  // Parse Action Option
  if(parse_action_option(&R, argv,argc)==invalid){
    #ifdef DEBUG
      printf("Action Option is invalid\n");
    #endif
    printf("invalid\n");
    return R;
  }

  // Check If File Exists
  if(stat(R.name, &buffer) != 0){
    printf("invalid\n");
    return R;
  }

  R.good = 0;

  return R;
}
