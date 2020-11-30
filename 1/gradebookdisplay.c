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

int file_test(char* filename) 
{
	struct stat buffer;
	int exist=stat(filename, &buffer);
	if(exist==0)   //if it exists
		return 1;
	else 
		return 0;
}

int comparator_alph(const void *p, const void* q)  //helps alphabetize
{
	if(strcmp(((struct Student*)p)->last_name, ((struct Student*)q)->last_name)!=0)
	{
		return strcmp(((struct Student*)p)->last_name, ((struct Student*)q)->last_name);
	}
	else
	{
		return strcmp(((struct Student*)p)->first_name, ((struct Student*)q)->first_name);
	}
}

void alph_order(paddedDecrypted_gradebook *p_gb_ptr, int index)  //puts into alphabetical order
{
	int i, j;
	char str[p_gb_ptr->num_students+1][max_n_len], temp[p_gb_ptr->num_students+1];
	Student info_copy[p_gb_ptr->num_students+1];
	for(i=0; i<p_gb_ptr->num_students; i++)
	{
		memcpy(info_copy[i].first_name, p_gb_ptr->s_array[i].first_name, max_n_len);
		memcpy(info_copy[i].last_name, p_gb_ptr->s_array[i].last_name, max_n_len);
		for(j=0; j<max_assignments; j++)
		{
			info_copy[i].grades[j]=p_gb_ptr->s_array[i].grades[j];
		}
	}
	qsort(info_copy, p_gb_ptr->num_students, sizeof(struct Student), comparator_alph);
	for(i=0; i<p_gb_ptr->num_students; i++)
	{
		printf("(%s, %s, %d)\n", info_copy[i].last_name, info_copy[i].first_name, info_copy[i].grades[index]);
	}
	return;
}


int comparator_grade(const void *p, const void* q)  //helps order by grade
{
	return(((struct Student_go*)q)->grades[((struct Student_go*)q)->index]-((struct Student_go*)p)->grades[((struct Student_go*)p)->index]);
}


void grade_order(paddedDecrypted_gradebook * p_gb_ptr, int index, Student_go * info_copy)  //print in grade order
{
	int i, j;
	char str[p_gb_ptr->num_students+1][max_n_len], temp[p_gb_ptr->num_students+1];
	for(i=0; i<p_gb_ptr->num_students; i++)
	{
		memcpy(info_copy[i].first_name, p_gb_ptr->s_array[i].first_name, max_n_len);
		memcpy(info_copy[i].last_name, p_gb_ptr->s_array[i].last_name, max_n_len);
		info_copy[i].index=index;
		for(j=0; j<max_assignments; j++)
		{
			info_copy[i].grades[j]=p_gb_ptr->s_array[i].grades[j];
		}
	}
	qsort(info_copy, p_gb_ptr->num_students, sizeof(struct Student_go), comparator_grade);
	for(i=0; i<p_gb_ptr->num_students; i++)
	{
		printf("(%s, %s, %d)\n", info_copy[i].last_name, info_copy[i].first_name, info_copy[i].grades[index]);
	}
	return;
}

int print_assignment(D_CmdLineResult d_cmd_result, paddedDecrypted_gradebook *p_gb_ptr)
{
	int i, f_flag=0, index=0;
	for(i=0; i<p_gb_ptr->num_assignments; i++)
	{
		if(strncmp(p_gb_ptr->a_array[i].assignment_name, d_cmd_result.a_name, max_n_len)==0) //finds assignment
		{
			f_flag=1;
			index=i;
		}
	}
	if(f_flag!=1)  //if assignment does not exist
	{
		printf("invalid\n");
		return(255);
	}
	if(d_cmd_result.order==1)
	{
		alph_order(p_gb_ptr, index);
	}
	if(d_cmd_result.order==2)
	{
		Student_go info_copy[p_gb_ptr->num_students+1];
		grade_order(p_gb_ptr, index, info_copy);
	}
	return(0);
}

int print_student(D_CmdLineResult d_cmd_result, paddedDecrypted_gradebook *p_gb_ptr)
{
	int i, f_flag=0, index=0;
	for(i=0; i<p_gb_ptr->num_assignments; i++)
	{
		if(strncmp(p_gb_ptr->s_array[i].first_name, d_cmd_result.f_name, max_n_len)==0)
		{
			if(strncmp(p_gb_ptr->s_array[i].last_name, d_cmd_result.l_name, max_n_len)==0)
			{
				f_flag=1;
				index=i;
			}
		}
	}
	if(f_flag!=1)  //student does not exist
	{
		printf("invalid\n");
		return(255);
	}
	for(i=0; i<p_gb_ptr->num_assignments; i++)
	{
		printf("(%s, %d)\n", p_gb_ptr->a_array[i].assignment_name, p_gb_ptr->s_array[index].grades[i]);  
	}
	return(0);
}

int comparator_alph_f(const void *p, const void* q)
{
	if(strcmp(((struct Student_fa*)p)->last_name, ((struct Student_fa*)q)->last_name)!=0)
	{
		return strcmp(((struct Student_fa*)p)->last_name, ((struct Student_fa*)q)->last_name);
	}
	else
	{
		return strcmp(((struct Student_fa*)p)->first_name, ((struct Student_fa*)q)->first_name);
	}
}

int alph_order_f(paddedDecrypted_gradebook *p_gb_ptr)
{
	int i, j;
	float final=0, temp=0, temp2=0; 
	Student_fa info_copy[p_gb_ptr->num_students+1];
	for(i=0; i<p_gb_ptr->num_students; i++)
	{
		memcpy(info_copy[i].first_name, p_gb_ptr->s_array[i].first_name, max_n_len);
		memcpy(info_copy[i].last_name, p_gb_ptr->s_array[i].last_name, max_n_len);
		for(j=0; j<max_assignments; j++)
		{
			info_copy[i].grades[j]=p_gb_ptr->s_array[i].grades[j];
		}
	}
	for(i=0; i<p_gb_ptr->num_students; i++)
	{
		for(j=0; j<p_gb_ptr->num_assignments; j++)
		{
			if(p_gb_ptr->a_array[j].points==0)  //if points for assignment is zero return (can't divide by zero)
			{
				printf("invalid\n");
				return(255);
			}
			temp2=(float)info_copy[i].grades[j]/p_gb_ptr->a_array[j].points;
			temp=temp2*p_gb_ptr->a_array[j].weight;
			final+=temp;
		}
		info_copy[i].final=final;
		final=0;
		temp=0;
	}
	qsort(info_copy, p_gb_ptr->num_students, sizeof(struct Student_fa), comparator_alph_f);
	for(i=0; i<p_gb_ptr->num_students; i++)
	{
		printf("(%s, %s, %f)\n", info_copy[i].last_name, info_copy[i].first_name, info_copy[i].final);
	}
	return 0;
}

int comparator_grade_f(const void *p, const void* q)
{
	return(((struct Student_fa*)q)->final-((struct Student_fa*)p)->final);
}

int grade_order_f(paddedDecrypted_gradebook *p_gb_ptr, Student_fa *info_copy)
{
	int i, j;
	float  final=0, temp=0, temp2=0;
	for(i=0; i<p_gb_ptr->num_students; i++)
	{
		memcpy(info_copy[i].first_name, p_gb_ptr->s_array[i].first_name, max_n_len);
		memcpy(info_copy[i].last_name, p_gb_ptr->s_array[i].last_name, max_n_len);
		for(j=0; j<max_assignments; j++)
		{
			info_copy[i].grades[j]=p_gb_ptr->s_array[i].grades[j];
		}
	}
	for(i=0; i<p_gb_ptr->num_students; i++)
	{
		for(j=0; j<p_gb_ptr->num_assignments; j++)
		{
			if(p_gb_ptr->a_array[j].points==0)  //if points for assignment is zero return (can't divide by zero)
			{
				printf("invalid\n");
				return(255);
			}
			temp2=(float)info_copy[i].grades[j]/p_gb_ptr->a_array[j].points;
			temp=temp2*p_gb_ptr->a_array[j].weight;
			final+=temp;
		}
		info_copy[i].final=final;
		final=0;
		temp=0;
	}
	qsort(info_copy, p_gb_ptr->num_students, sizeof(struct Student_fa), comparator_grade_f);
	for(i=0; i<p_gb_ptr->num_students; i++)
	{
		printf("(%s, %s, %f)\n", info_copy[i].last_name, info_copy[i].first_name, info_copy[i].final);
	}
	return 0;
}

int print_final(D_CmdLineResult d_cmd_result, paddedDecrypted_gradebook *p_gb_ptr)
{
	int i;
	if(d_cmd_result.order==1)
	{
		i=alph_order_f(p_gb_ptr);
		return(i);
	}
	if(d_cmd_result.order==2)
	{
		Student_fa info_copy[p_gb_ptr->num_students+1];
		i=grade_order_f(p_gb_ptr, info_copy);
		return(i);
	}
}

int main(int argc, char** argv)
{
	D_CmdLineResult d_cmd_result;
	D_ActionType d_action;
	d_action.print_assignment=0;
	d_action.print_student=0;
	d_action.print_final=0;
	d_cmd_result.index=0;
	Encrypted_gradebook e_stuff;
	Encrypted_gradebook* e_ptr=&e_stuff;
	FILE* fp;
	int length, i, alnum=0, sum=0, pa_check1=0, pa_check2=0, pa_check3=0, done=0, j, ps_check1=0, pf_check1=0, pf_check2=0;
	unsigned char val[key_size];
	int num_flag, num_flag2, num_flag3;
	if(argc<7)
	{
		printf("invalid\n");
		return(255);
	}
	if(strcmp(argv[1],"-N")!=0)
	{
		printf("invalid\n");
		return(255);
	}
	else  //look for filename
	{
		length=strlen(argv[2]);   //checking length of inputted filename
		if(length>=max_n_len)
		{
			printf("invalid\n");
			return(255);
		}
		for(i=0; i<length; i++)
		{
			alnum=isalnum(argv[2][i]);
			if(alnum==0)
			{
				if((argv[2][i]!='.')&&(argv[2][i]!='_')) 
				{
					printf("invalid\n");  //not an alphanumeric character, including period, or underscores
					return(255);
				}
			}
		}
		if(!file_test(argv[2]))  //if file does not exist
	    {
	    	printf("invalid\n");
	    	return(255);
	    } 
		memcpy(d_cmd_result.filename,argv[2], length);
	}
	if(strcmp(argv[3],"-K")!=0)
	{
		printf("invalid\n");
		return(255);
	}
	else
	{
		length=strlen(argv[4]);
		if(length>key_size*2)  //key size of inputted key
		{
			printf("invalid\n");
			return(255);
		}
		for(i=0; i<length; i++)
		{
			if(isxdigit(argv[4][i])==0)    //check if key consists of hex digits
			{
				printf("invalid\n");
				return(255);
			}
		}
		const char *pos=argv[4];   //converting user input
		for(size_t count=0; count<sizeof(val)/sizeof(*val); count++)  //convert to byte array
		{
			sscanf(pos, "%2hhx", &val[count]);
			pos+=2;
		}
	}
	for(i=5; i<argc; i++)  //go through to make sure that there is not more than one action
	{
		if(strcmp(argv[i],"-PA")==0)
		{
			d_action.print_assignment=1;
			d_cmd_result.index=i;
		}
		if(strcmp(argv[i],"-PS")==0)
		{
			d_action.print_student=1;
			d_cmd_result.index=i;
		}
		if(strcmp(argv[i],"-PF")==0)
		{
			d_action.print_final=1;
			d_cmd_result.index=i;
		}
	}
	sum=d_action.print_assignment+d_action.print_student+d_action.print_final;
	if(sum!=1)
	{
		printf("invalid\n");  //tried to do more than one action
		return(255);
	}
	if(d_cmd_result.index==5)
	{
		for(i=d_cmd_result.index+1;i<argc; i++)
		{
			if(d_action.print_assignment==1)
			{
				if(strcmp(argv[i],"-AN")==0)
				{

					pa_check2++;
					length=strlen(argv[i+1]);
					if(length>=max_n_len)  //check length of assignment name
					{
						printf("invalid\n");
						return(255);
					}
					num_flag=0;
					for(j=0; j<strlen(argv[i+1]); j++)   //check that assignment name character is correct
					{
						if((isdigit(argv[i+1][j])==0)&&(isalpha(argv[i+1][j])==0))
						{
							num_flag=1;
						}
					}
					strcpy(d_cmd_result.a_name, argv[i+1]);
					i+=1;
				}
				else if(strcmp(argv[i],"-A")==0)   //order=1
				{
					pa_check1++;
					d_cmd_result.order=1;
				}
				else if(strcmp(argv[i],"-G")==0)   //order=2
				{
					pa_check3++;
					d_cmd_result.order=2;
				}
				else
				{
					printf("invalid\n");
					return(255);
				}
				if(((pa_check1!=0)&&(pa_check2!=0)&&(pa_check3==0)&&(i==argc-1))||((pa_check3!=0)&&(pa_check2!=0)&&(pa_check1==0)&&(i==argc-1)))
				{
					done=1;
					break;
				}
			}
			if(d_action.print_student==1)
			{
				if(strcmp(argv[i],"-FN")==0)
				{	
					ps_check1++;
					length=strlen(argv[i+1]);
					if(length>=max_n_len)
					{
						printf("invalid\n");
						return(255);
					}
					num_flag2=0;
					for(j=0; j<strlen(argv[i+1]); j++)   //check that assignment name character is correct
					{
						if(isalpha(argv[i+1][j])==0)
						{
							num_flag2=1;
						}
					}
					strcpy(d_cmd_result.f_name,argv[i+1]);
					i+=1;
				}
				else if(strcmp(argv[i],"-LN")==0)
				{
					ps_check1++;
					length=strlen(argv[i+1]);
					if(length>=max_n_len)
					{
						printf("invalid\n");
						return(255);
					}
					num_flag3=0;
					for(j=0; j<strlen(argv[i+1]); j++)   //check that assignment name character is correct
					{
						if(isalpha(argv[i+1][j])==0)
						{
							num_flag3=1;
						}
					}
					strcpy(d_cmd_result.l_name,argv[i+1]);
					i+=1;
				}
				else
				{
					printf("invalid\n");
					return(255);
				}
				if(ps_check1==2)
				{
					done=1;
				}
			}
			if(d_action.print_final==1)
			{
				if(strcmp(argv[i],"-A")==0)   //order=1
				{
					pf_check1++;
					d_cmd_result.order=1;
				}
				else if(strcmp(argv[i],"-G")==0)    //order=2
				{
					pf_check2++;
					d_cmd_result.order=2;
				}
				else
				{
					printf("invalid\n");
					return(255);
				}
				if(((pf_check1!=0)&&(pf_check2==0)&&(i==argc-1))||((pf_check1==0)&&(pf_check2!=0)&&(i==argc-1)))
				{
					done=1;
				}
			}
		}
		if(done!=1||(num_flag==1)||(num_flag2==1)||(num_flag3==1))
		{
			printf("invalid\n");
			return(255);
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
	int decrypt_len, rc, encrypt_len, x=0;
	Encrypted_gradebook e_gradebook;
	paddedDecrypted_gradebook p_gb = {0};
	paddedDecrypted_gradebook *p_gb_ptr=&p_gb;
	fp=fopen(argv[2],"r");
	fread(&e_stuff, sizeof(Encrypted_gradebook), 1, fp);
	SHA256(val,MAC_Size,hash_a);
  	unsigned char temp[MAC_Size];
  	HMAC(EVP_sha256(), hash_a, 32, (const unsigned char*)&e_stuff.iv,(IV_Size + sizeof(paddedDecrypted_gradebook)), temp, &resultlen);
  	if(strncmp(temp,e_stuff.mac,MAC_Size)==0)  //make sure macs match
  	{
  		decrypt_len = decrypt(e_stuff.encrypted_data, sizeof(paddedDecrypted_gradebook), val, e_stuff.iv, (unsigned char*)&p_gb_ptr->g_name);
  	}
  	else
  	{
  		printf("invalid\n");
  		return(255);
  	}
  	if(d_action.print_assignment==1)
  	{
  		x=print_assignment(d_cmd_result, p_gb_ptr);
  		if(x==255)
  		{
  			return(255);
  		}
  	}
  	if(d_action.print_student==1)
  	{
  		x=print_student(d_cmd_result, p_gb_ptr);
  		if(x==255)
  		{
  			return(255);
  		}
  	}
  	if(d_action.print_final==1)
  	{
  		x=print_final(d_cmd_result, p_gb_ptr);
  		if(x==255)
  		{
  			return(255);
  		}
  	}
	return(0);
}