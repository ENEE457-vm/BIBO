
typedef struct _Buffer {
  unsigned char *Buf;
  unsigned long Length;
} Buffer;

typedef struct _Assignment {
  char* name;
  float weight;
  int points;
} Assignment;

typedef struct _Student {
  char* fname;
  char* lname;
  size_t total_assignments;
  char** assignment_names_lst;
  int* points_earned;
} Student;

typedef struct _Gradebook {
  
  Assignment* assignments;
  Student* students;
  size_t num_assignments;
  size_t num_students;
} Gradebook;




Gradebook read_from_path(char *path, unsigned char *key);
void write_to_path(char *path, Gradebook G, unsigned char *key);
Buffer concat_buffs(Buffer *A, Buffer *B);
Buffer print_record(Gradebook *R);
void dump_record(Gradebook *R);
void dump_assignment(Assignment A);
int read_records_from_path(char *path, unsigned char *key, Gradebook **, unsigned int *);


