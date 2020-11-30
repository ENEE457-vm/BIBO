#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "my_data.h"
#include "crypto.c"
#include <openssl/sha.h>
#include <openssl/hmac.h>

unsigned char *mx_hmac_sha256(const void *key, int keylen,
                              const unsigned char *data, int datalen,
                              unsigned char *result, unsigned int *resultlen) {
    return HMAC(EVP_sha256(), key, keylen, data, datalen, result, resultlen);
}


int file_test(char* filename)  //test if file exists
{
	struct stat buffer;
	int exist=stat(filename, &buffer);
	if(exist==0)   //if it exists
		return 1;
	else 
		return 0;
}

int add_assignment(CmdLineResult cmd_result, paddedDecrypted_gradebook *p_gb_ptr)  
{
	int i=0;
	float sum_weights=0;
	if(p_gb_ptr->num_assignments==max_assignments)  //check if number of assignments is maximum assignments
	{
		printf("invalid\n");
		return(255);
	}
	for(i=0; i<max_assignments; i++)
	{
		if(strncmp(p_gb_ptr->a_array[i].assignment_name, cmd_result.a_name, max_n_len)==0)  //check if assignment already exists
		{
			printf("invalid\n");
			return(255);
		}
	}
	for(i=0; i<p_gb_ptr->num_assignments;i++)
	{
		sum_weights+=p_gb_ptr->a_array[i].weight;
	}
	sum_weights+=cmd_result.weight;
	if(sum_weights>1)  //checks sum of weights
	{
		printf("invalid\n");
		return(255);
	}
	p_gb_ptr->a_array[p_gb_ptr->num_assignments].weight=cmd_result.weight;
	memcpy(p_gb_ptr->a_array[p_gb_ptr->num_assignments].assignment_name, cmd_result.a_name, max_n_len);
	p_gb_ptr->a_array[p_gb_ptr->num_assignments].points=cmd_result.points;
	p_gb_ptr->num_assignments+=1;
	return(0);
}

void delete_grade(int index, paddedDecrypted_gradebook *p_gb_ptr)   //deletes student grade associated with deleted assignment
{
	int i, j;
	for(i=0; i<p_gb_ptr->num_students; i++)  //initialize grade back to zero
	{
		p_gb_ptr->s_array[i].grades[index]=0;
	}
	return;
}

int delete_assignment(CmdLineResult cmd_result, paddedDecrypted_gradebook *p_gb_ptr)
{
	int i, d_flag=0, index=0;
	for(i=0; i<p_gb_ptr->num_assignments; i++)
	{	
		if(strncmp(p_gb_ptr->a_array[i].assignment_name, cmd_result.a_name, max_n_len)==0)  //found assignment to be deleted
		{
			d_flag=1;
			index=i;
			break;
		}
	}
	if(d_flag!=1)  //asignment does not exist
	{
		printf("invalid");
		return(255);
	}
	p_gb_ptr->num_assignments=p_gb_ptr->num_assignments-1;
	strcpy(p_gb_ptr->a_array[index].assignment_name,"");
	p_gb_ptr->a_array[index].points=0;
	p_gb_ptr->a_array[index].weight=0;
	for(i=index; i<p_gb_ptr->num_assignments;i++)
	{
		p_gb_ptr->a_array[i]=p_gb_ptr->a_array[i+1];
	}
	delete_grade(index, p_gb_ptr);  //deletes student grades associated with deleted assignment
	return(0);
}

int add_student(CmdLineResult cmd_result, paddedDecrypted_gradebook *p_gb_ptr)
{
	int i, n_flag=0;
	if(p_gb_ptr->num_students==max_students)  //compare number of students with maximum number of students
	{
		printf("invalid\n");
		return(255);
	}
	for(i=0; i<=p_gb_ptr->num_students; i++)  //check if student exists already
	{
		if(strcmp(p_gb_ptr->s_array[i].first_name, cmd_result.f_name)==0)
		{
			if(strcmp(p_gb_ptr->s_array[i].last_name, cmd_result.l_name)==0)   
			{
				n_flag=1;
				break;
			}
		}
	}
	if(n_flag==1)    
	{
		printf("invalid\n");
		return(255);
	}
	memcpy(p_gb_ptr->s_array[p_gb_ptr->num_students].first_name, cmd_result.f_name, max_n_len);
	memcpy(p_gb_ptr->s_array[p_gb_ptr->num_students].last_name, cmd_result.l_name, max_n_len);
	p_gb_ptr->num_students+=1;
	return(0);
}

int delete_student(CmdLineResult cmd_result, paddedDecrypted_gradebook *p_gb_ptr)
{
	int i, d_flag=0, index;
	for(i=0; i<=p_gb_ptr->num_students; i++)
	{
		if(strcmp(p_gb_ptr->s_array[i].first_name, cmd_result.f_name)==0)
		{
			if(strcmp(p_gb_ptr->s_array[i].last_name, cmd_result.l_name)==0)  //found student to be deleted
			{
				d_flag=1;
				index=i;
				break;
			}
		}
	}
	if(d_flag!=1)  //student does not exist
	{
		printf("invalid\n");
		return(255);
	}
	p_gb_ptr->num_students=p_gb_ptr->num_students-1;
	strcpy(p_gb_ptr->s_array[index].first_name, "");
	strcpy(p_gb_ptr->s_array[index].last_name, "");
	for(i=0; i<max_assignments; i++)   //zero out grades array
	{
		p_gb_ptr->s_array[index].grades[i]=0;
	}
	for(i=index; i<p_gb_ptr->num_students; i++)
	{
		p_gb_ptr->s_array[i]=p_gb_ptr->s_array[i+1];
	} 
	return(0);

}

int add_grade(CmdLineResult cmd_result, paddedDecrypted_gradebook *p_gb_ptr)
{
	int i, n_flag=0, a_index, n_index;
	if(p_gb_ptr->num_assignments==max_assignments)  //compare current number of assignments with maximum number of assignments
	{
		printf("invalid\n");
		return(255);
	}
	for(i=0; i<=p_gb_ptr->num_assignments; i++)			//checks that assignment exists
	{
		if(strcmp(p_gb_ptr->a_array[i].assignment_name, cmd_result.a_name)==0)  //found assignment
		{
			n_flag=1;
			a_index=i;
			break;
		}
	}
	if(n_flag!=1)  //assignment does not exist
	{
		printf("invalid\n");
		return(255);
	}
	n_flag=0;
	for(i=0; i<=p_gb_ptr->num_students; i++)						//checks that student exists
	{	
		if(strcmp(p_gb_ptr->s_array[i].first_name, cmd_result.f_name)==0)
		{
			if(strcmp(p_gb_ptr->s_array[i].last_name, cmd_result.l_name)==0)  //found student
			{
				n_flag=1;
				n_index=i;
				break;
			}
		}
	}
	if(n_flag!=1)  //student does not exist
	{
		printf("invalid\n");
		return(255);
	}
	p_gb_ptr->s_array[n_index].grades[a_index]=cmd_result.grade;
	return(0);
}

int main(int argc, char** argv) 
{
	int first_arg=0, alnum=0, dec_check=0;
	int count=0, i, sum=0, length=0, j, done=0, truncated=0, check1=0, check2=0, check3=0, check4=0, check5=0, d_count=0;
	float x;
	unsigned char val[key_size];
	int new_flag=0, new_flag2, new_flag3, new_flag4, new_flag5, new_flag6, new_flag7, new_flag8, new_flag9,new_flag10, new_flag11, new_flag12; 
	FILE* fp, *fp_2;
	ActionType action;
	action.add_assignment=0;
	action.delete_assignment=0;
	action.add_student=0;
	action.delete_student=0;
	action.add_grade=0;
	CmdLineResult cmd_result;
	strcpy(cmd_result.filename,"");
	strcpy(cmd_result.key,"");
	cmd_result.index=0;
	strcpy(cmd_result.a_name,"");
	cmd_result.points=0;
	cmd_result.weight=0;
	Encrypted_gradebook e_stuff;
	Encrypted_gradebook* e_ptr=&e_stuff;
	strcpy(cmd_result.f_name,"");
	strcpy(cmd_result.l_name,"");
	if(argc<8)  
	{
		printf("invalid\n");
		return(255);
	}
	if(argc%2==1)
	{
		printf("invalid\n");
		return(255);
	}
	if(strcmp(argv[1],"-N")!=0)
	{
		printf("invalid\n");
		return(255);
	}
	else  
	{
		length=strlen(argv[2]);  
		if(length>=max_n_len)  //check length of inputted file name
		{
			printf("invalid\n");
			return(255);
		}
		for(i=0; i<length; i++)   //check that filename is alphanumeric characters
		{
			alnum=isalnum(argv[2][i]);
			if(alnum==0)
			{
				if((argv[2][i]!='.')&&(argv[2][i]!='_'))   //accept periods and underscores
				{  
					printf("invalid\n");
					return(255);
				}
			}
		}
	    if(!file_test(argv[2]))   //check that file exists
	    {
	    	printf("invalid\n");
	    	return(255);
	    } 
		memcpy(cmd_result.filename,argv[2], length);
	}
	if(strcmp(argv[3],"-K")!=0)   
	{
		printf("invalid\n");
		return(255);
	}
	else
	{
		length=strlen(argv[4]);
		if(length>key_size*2)  //check key length
		{
			printf("invalid\n");
			return(255);
		}
		for(i=0; i<length; i++)
		{
			if(isxdigit(argv[4][i])==0)    
			{
				printf("invalid\n");
				return(255);
			}
		}
		const char *pos=argv[4];   //converting user input to byte array
		for(size_t count=0; count<sizeof(val)/sizeof(*val); count++)
		{
			sscanf(pos, "%2hhx", &val[count]);
			pos+=2;
		}
	}
	for(i=5; i<argc; i++)  //go through to make sure that there is not more than one action specified
	{
		if(strcmp(argv[i],"-AA")==0)
		{
			action.add_assignment=1;
			cmd_result.index=i;
		}
		if(strcmp(argv[i],"-DA")==0)
		{
			action.delete_assignment=1;
			cmd_result.index=i;
		}
		if(strcmp(argv[i],"-AS")==0)
		{
			action.add_student=1;
			cmd_result.index=i;
		}
		if(strcmp(argv[i],"-DS")==0)
		{
			action.delete_student=1;
			cmd_result.index=i;
		}
		if(strcmp(argv[i],"-AG")==0)
		{
			action.add_grade=1;	
			cmd_result.index=i;
		}
	}
	sum=action.add_assignment+action.delete_assignment+action.add_student+action.delete_student+action.add_grade;
	if(sum!=1)
	{
		printf("invalid\n");  //user tried to do more than one action
	}
	if(cmd_result.index==5)
	{
		for(i=cmd_result.index+1;i<argc; i++)
		{
			if(action.add_assignment==1)
			{
				if((strcmp(argv[i],"-AN")==0))
				{
					if(strlen(argv[i+1])>=max_n_len)
					{
						printf("invalid\n");
						return(255);
					}
					new_flag9=0;
					for(j=0; j<strlen(argv[i+1]); j++)   //check that assignment name character is correct
					{
						if((isdigit(argv[i+1][j])==0)&&(isalpha(argv[i+1][j])==0))
						{
							new_flag9=1;
						}
					}
					check1++;
					if((i+1==argc-1)&&(check1<3))
					{
						printf("invalid\n");
						return(255);
					}
					strcpy(cmd_result.a_name,argv[i+1]);
					i+=1;
				}
				else if((strcmp(argv[i],"-P")==0))
				{
					check1++;
					char * point_c=argv[i+1];
					int point_i=atoi(point_c);
					new_flag10=0;
					for(j=0; j<strlen(argv[i+1]); j++)
					{
						if(isdigit(argv[i+1][j])==0)  //check that number is a non-negative integer
						{
							new_flag10=1;
						}
					}
					if((i+1==argc-1)&&(check1<3))    //maybe change this to i+1
					{
						printf("invalid\n");
						return(255);
					}
					cmd_result.points=point_i;
					d_count=0;
					i+=1;
				}
				else if((strcmp(argv[i],"-W")==0))
				{
					check1++;
					new_flag11=0;
					dec_check=0;
					for(j=0; j<strlen(argv[i+1]); j++)   //check for float
					{
						if(argv[i+1][j]=='.')
						{
							dec_check+=1;
							if(dec_check!=1)
							{
								new_flag11=1;
							}
						}
						if(isdigit(argv[i+1][j])==0) //check that number is positive float
						{
							if(argv[i+1][j]!='.')
							{
								new_flag11=1;
							}
						}
					}
					if((i+1==argc-1)&&(check1<3))   
					{
						printf("invalid\n");
						return(255);
					}
					x=atof(argv[i+1]);
					if(x<0||x>1)    //check if weight is not in the right range
					{
						printf("invalid\n");
						return(255);
					}
					cmd_result.weight=x;
					d_count=0;
					i+=1;
				}
				else
				{
					printf("invalid\n");
					return(255);
				}
				if((i+1==argc-1)&&(check1<3)||((i==argc-1)&&(check1<3)))
				{
					printf("invalid\n");
					return(255);
				}
				if(check1==3)
				{
					done=1;
				}
			}
			if(action.delete_assignment==1)  
			{
				check2++;
				if((strcmp(argv[i],"-AN")==0))
				{
					check2++;
					if(strlen(argv[i+1])>=max_n_len)
					{
						printf("invalid\n");
						return(255);
					}
					new_flag12=0;
					for(j=0; j<strlen(argv[i+1]); j++)   //check that assignment name character is correct
					{
						if((isdigit(argv[i+1][j])==0)&&(isalpha(argv[i+1][j])==0))
						{
							new_flag12=1;
						}
					}
					if((i+1==argc-1)&&(check2<2))
					{
						printf("invalid\n");
						return(255);
					}
					strcpy(cmd_result.a_name,argv[i+1]);
					if(i+1==argc-1)
					{
						done=1;
						break;
					}
				}
				if(check2<2)
				{
					printf("invalid\n");
					return(255);
				}
				if((i+1==argc-1)&&(check2<2)||((i==argc-1)&&(check2<2)))
				{
					printf("invalid\n");
					return(255);
				}
				if(done==1)
				{
					break;
				}
			}
			if(action.add_student==1) 
			{
				if((strcmp(argv[i],"-FN")==0))
				{
					check3++;
					if(strlen(argv[i+1])>=max_n_len)
					{
						printf("invalid\n");
						return(255);
					}
					new_flag=0; 
					for(j=0; j<strlen(argv[i+1]); j++)   //check that first name character is correct
					{
						if(isalpha(argv[i+1][j])==0)
						{
							new_flag=1;                   
						}
					}
					if((i+1==argc-1)&&(check3<2))
					{
						printf("invalid\n");
						return(255);
					}
					strcpy(cmd_result.f_name,argv[i+1]);
					i+=1;
				}
				else if((strcmp(argv[i],"-LN")==0))
				{
					check3++;
					if(strlen(argv[i+1])>=max_n_len)
					{
						printf("invalid\n");
						return(255);
					}
					new_flag2=0;
					for(j=0; j<strlen(argv[i+1]); j++)   //check that last name character is correct
					{
						if(isalpha(argv[i+1][j])==0)
						{
							new_flag2=1;
						}
					}
					if((i+1==argc-1)&&(check3<2))
					{
						printf("invalid\n");
						return(255);
					}
					strcpy(cmd_result.l_name,argv[i+1]);
					i+=1;
				}
				else
				{
					printf("invalid\n");
					return(255);
				}
				if((i+1==argc-1)&&(check3<2)||((i==argc-1)&&(check3<2)))
				{
					printf("invalid\n");
					return(255);
				}
				if(check3==2)
				{
					done=1;
				}
			}
			if(action.delete_student==1)  
			{
				if((strcmp(argv[i],"-FN")==0))
				{
					check4++;
					if(strlen(argv[i+1])>=max_n_len)
					{
						printf("invalid\n");
						return(255);
					}
					new_flag3=0;
					for(j=0; j<strlen(argv[i+1]); j++)   //check that first name character is correct
					{
						if(isalpha(argv[i+1][j])==0)
						{
							new_flag3=1;
						}
					}
					if((i+1==argc-1)&&(check4<2))
					{
						printf("invalid\n");
						return(255);
					}
					strcpy(cmd_result.f_name,argv[i+1]);
					i+=1;
				}
				else if((strcmp(argv[i],"-LN")==0))
				{
					check4++;
					if(strlen(argv[i+1])>=max_n_len)
					{
						printf("invalid\n");
						return(255);
					}
					new_flag4=0;
					for(j=0; j<strlen(argv[i+1]); j++)   //check that last name character is correct
					{
						if(isalpha(argv[i+1][j])==0)
						{
							new_flag4=1;
						}
					}
					if((i+1==argc-1)&&(check4<2))
					{
						printf("invalid\n");
						return(255);
					}
					strcpy(cmd_result.l_name,argv[i+1]);
					i+=1;
				}
				else
				{
					printf("invalid\n");
					return(255);
				}
				if((i+1==argc-1)&&(check4<2)||((i==argc-1)&&(check4<2)))
				{
					printf("invalid\n");
					return(255);
				}
				if(check4==2)
				{
					done=1;
				}
			}
			if(action.add_grade==1) 
			{
				if((strcmp(argv[i],"-FN")==0))
				{
					check5++;
					if(strlen(argv[i+1])>=max_n_len)
					{
						printf("invalid\n");
						return(255);
					}
					new_flag5=0;
					for(j=0; j<strlen(argv[i+1]); j++)   //check that first name character is correct
					{
						if(isalpha(argv[i+1][j])==0)
						{
							new_flag5=1;
						}
					}
					if((i+1==argc-1)&&(check5<4))
					{
						printf("invalid\n");
						return(255);
					}
					strcpy(cmd_result.f_name,argv[i+1]);
					i+=1;
				}
				else if((strcmp(argv[i],"-LN")==0))
				{
					check5++;
					if(strlen(argv[i+1])>=max_n_len)
					{
						printf("invalid\n");
						return(255);
					}
					new_flag6=0;
					for(j=0; j<strlen(argv[i+1]); j++)   //check that last name character is correct
					{
						if(isalpha(argv[i+1][j])==0)
						{
							new_flag6=1;
						}
					}
					if((i+1==argc-1)&&(check5<4))
					{
						printf("invalid\n");
						return(255);
					}
					strcpy(cmd_result.l_name,argv[i+1]);
					i+=1;
				}
				else if((strcmp(argv[i],"-AN")==0))
				{
					check5++;
					if(strlen(argv[i+1])>=max_n_len)
					{
						printf("invalid\n");
						return(255);
					}
					new_flag7=0;
					for(j=0; j<strlen(argv[i+1]); j++)   //check that assignment name character is correct
					{
						if((isdigit(argv[i+1][j])==0)&&(isalpha(argv[i+1][j])==0))
						{
							new_flag7=1;
						}
					}
					strcpy(cmd_result.a_name,argv[i+1]);
					if((i+1==argc-1)&&(check5<4))
					{
						printf("invalid\n");
						return(255);
					}
					i+=1;
				}
				else if((strcmp(argv[i],"-G")==0))
				{
					check5++;
					char* grade_c=argv[i+1];
					int grade_i=atoi(grade_c);
					new_flag8=0;
					for(j=0; j<strlen(argv[i+1])-1; j++)
					{
						if(isdigit(argv[i+1][j])==0)   //checks that value is a non-negative integer
						{
							new_flag8=1;
						}
					}
					if((i+1==argc-1)&&(check5<4))
					{
						printf("invalid\n");
						return(255);
					}
					cmd_result.grade=grade_i;
					i+=1;
				}
				else
				{
					printf("invalid\n");
					return(255);
				}
				if((i+1==argc-1)&&(check5<4)||((i==argc-1)&&(check5<4)))
				{
					printf("invalid\n");
					return(255);
				}
				if(check5==4)
				{
					done=1;
				}
			}
			if((done!=1)&&(i==argc-1))
			{
				printf("invalid\n");
				return(255);
			}
		}
		if(done!=1||(new_flag==1)||(new_flag2==1)||(new_flag3==1)||(new_flag4==1)||(new_flag5==1)||(new_flag6==1)||(new_flag7==1)||(new_flag8==1)||(new_flag9==1)||(new_flag10==1)||(new_flag11==1)||(new_flag12==1))
		{
			printf("invalid\n");
			return(255);
		}
		if(action.add_assignment==1)
		{
			if((i+1==argc-1)&&(check1<3)||((i==argc-1)&&(check1<3)))
			{
				printf("invalid\n");
				return(255);
			}
		}
	}
	else
	{
		printf("invalid\n");
		return(255);
	}
	unsigned char hash_a[MAC_Size];
	unsigned char *result=NULL;
	unsigned int resultlen=-1;
	int decrypt_len, rc, encrypt_len;
	Encrypted_gradebook e_gradebook;
	paddedDecrypted_gradebook p_gb = {0};
	paddedDecrypted_gradebook *p_gb_ptr=&p_gb;
	fp=fopen(argv[2],"r");
	fread(&e_stuff, sizeof(Encrypted_gradebook), 1, fp);
	// create mac tag from inputted key
	SHA256(val,MAC_Size,hash_a);
  	unsigned char temp[MAC_Size];
  	unsigned char temp2[MAC_Size];
  	// make mac using mac tag
	HMAC(EVP_sha256(), hash_a, 32, (const unsigned char*)&e_stuff.iv,(IV_Size + sizeof(paddedDecrypted_gradebook)), temp, &resultlen);
  	// make sure macs match
  	if(strncmp(temp,e_stuff.mac,MAC_Size)==0)
  	{
  		decrypt_len = decrypt(e_stuff.encrypted_data, sizeof(paddedDecrypted_gradebook), val, e_stuff.iv, (unsigned char*)&p_gb_ptr->g_name);
  	}
  	else
  	{
  		printf("invalid\n");
  		return(255);
  	}
  	if(action.add_assignment==1)
  	{
  		x=add_assignment(cmd_result, p_gb_ptr);
  		if(x==255)
  		{
  			return(255);
  		}
  	}
  	if(action.delete_assignment==1)
  	{
  		x=delete_assignment(cmd_result, p_gb_ptr);
  		if(x==255)
  		{
  			return(255);
  		}
  	}
  	if(action.add_student==1)
  	{
  		x=add_student(cmd_result, p_gb_ptr);
  		if(x==255)
  		{
  			return(255);
  		}
  	}
  	if(action.delete_student==1)
  	{
  		x=delete_student(cmd_result, p_gb_ptr);
  		if(x==255)
  		{
  			return(255);
  		}
  	}
  	if(action.add_grade==1)
  	{
  		x=add_grade(cmd_result, p_gb_ptr);
  		if(x==255)
  		{
  			return(255);
  		}
  	}
  	fflush(fp);
  	fclose(fp);  
  	fp_2 = fopen(argv[2], "w");
	strcpy(e_gradebook.iv,"");     
	strcpy(e_gradebook.encrypted_data,"");
	strcpy(e_gradebook.mac,"");
	rc=RAND_bytes(e_gradebook.iv,IV_Size);
	encrypt_len=encrypt((unsigned char*)&p_gb_ptr->g_name, sizeof(paddedDecrypted_gradebook), val, e_gradebook.iv, e_gradebook.encrypted_data);
	HMAC(EVP_sha256(), hash_a, 32, (const unsigned char*)&e_gradebook.iv,(IV_Size + sizeof(paddedDecrypted_gradebook)), e_gradebook.mac, &resultlen);
	fwrite(&e_gradebook, sizeof(struct Encrypted_gradebook), 1, fp_2);
  	fclose(fp_2);
	return(0);
}