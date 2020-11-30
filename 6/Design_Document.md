# Build-It-Break-It Project

## Design Overview

#### Structure
- The main structure that encapsulates the entire functionality of the gradebook is a gradebook struct that stores an array of assignments and an array of students in the class as well as both of their lengths.
- A student is defined as a struct containing a string for a first name and a last name
- An assignment is defined as a struct that contains the name, the total points that can be given, and the weight of the assignment. Also, an assignment contains an array of grades (one for each student).
- A grade is defined as a struct that contains a integer point value and a student.

#### Storage
- A gradebook is created and stored to disk by creating a single gradebook struct and then writing the entire struct into a blank file with the name specified by the user.
- A gradebook can be modified by reading in the entire gradebook struct from the gradebook file specified by the user. Values in the gradebook can then be modified/overwritten as described in the functionality section. This entire edited gradebook then is re-written into the original read in file, replacing the old gradebook struct that was present.

#### Functionality
Note: All user input is validated and checked so that it adheres to buffer limits and character restrictions before it is passed to any of the core functionality of the program. See attack prevention.
###### Adding a student
- Check to see if student exists
- If student doesn't exist, add into the students array in the gradebook struct, and extend the length of the grades array in each assignment by 1.
###### Deleting a student
- Check to see if student exists
- If student exists, remove them from the student's array in the gradebook by shifting over to the left by 1 all students to the right of it in the array.
- Go through each assignment and do the same to each grades array, removing the grade in the same position as the deleted student.
###### Adding an assignment
- Check to see if assignment with assignment name exists.
- If the assignment does not exist, create a new assignment and add it to the assignments array, extending it by 1.
###### Deleting an assignment
- Check to see if assignment with assignment name exists.
- If the assignment exists, delete it from the assignments array by shifting over everything to its right to the left by 1.
###### Finding an assignment to print
- Go through the assignment array until the name of the assignment to be printed is found.
- Go through the grades array in the assignment that was just found and print out each grade along with its student.
###### Finding a student to print
- Go through the students array until a student matching the given name is found.
- Go into every assignment in the gradebook and get the grade in the grades array that is in the same position as the student that was just found.
- Print out each of these grades.
###### Adding a grade
- Find the assignment that matches the assignment name.
- Find the student that matches the student's name.
- Go to the grades array in the found assignment and go to the grade in the same position as the found student.
- Change the integer value to the one specified by the user.


#### Security Model
- Gradebook follows an encrypt then authenticate approach.
- When a user specifies a file for the gradebook, the program opens the encrypted file, reads the size, and then reads in the encrypted bytes into a dynamic buffer of the same size that was just read in.
- The SHA-256 HMAC that is stored on disk has its size read in, then has its contents read into a dynamic buffer of the same size that was just read in.
- The encrypted bytes from the encrypted gradebook file are passed through the same SHA-256 HMAC algorithm used to generate the stored signature. The user provided key is used as part of the algorithm.
- These 2 signatures are compared. If they match then the encrypted file's integrity has not been compromised.
- The encrypted bytes are then fed through a aes-128-cbc decryption scheme to get the plaintext bytes.
- These plaintext bytes are passed into a dynamically allocated buffer and are converted into a gradebook struct.
- This gradebook struct can then be viewed or modified. When a user is ready to re-encrypt the contents the struct is passed to the encryption function.
- This function takes the gradebook struct and writes it into a plaintext file.
- This file is then taken and encrypted using a aes-128-cbc encryption algorithm in a similar way that the decryption algorithm operated.
- Finally a SHA-256 HMAC is generated from the encrypted bytes and user provided key. This HMAC is stored to disk for later retrieval. 

## Attack Prevention
#### Attack 1: Buffer Overflow
##### Explanation
- Occurs when an attacker enters an input that extends beyond the length of a buffer that has been allocated for it on the stack.
- This could give the attacker the ability to inject code into the program, access local variables, or read from program memory.
##### Counter-Measure
- All buffers in the program that are not dynamically allocated with malloc are created from pre-defined constants of known lengths. When a user enters in input, that input is checked against the length of the buffer before it is transferred into it.
- This ensures that the program never places user input into a buffer that is smaller than its length.
##### Lines
- data.c | 97 - 100, 120
- data.h | 1 - 4
- setup.c | 83, 87

#### Attack 2: Heap Overflow Attack
##### Explanation
- Occurs when an attacker tries to overflow a buffer allocated by malloc.
- This could cause corruption of the data/structures in the program or give the attacker access to read restricted areas of memory.
##### Counter-Measure
- In the case where malloc is used to store the contents of read in files, the gradebook reads the size of files before creating a dynamically allocated buffer to store their contents. Even with a blank gradebook, there is a realistic expectation of how big the file should be, so the program will not read in the file if it is too big.
- The other case where malloc is used is when retrieving the gradebook struct and when calculating the final grades. The gradebook is of a known size and because the program validates the integrity of the gradebook file before converting its contents into a gradebook, it can be confident that it will get a gradebook with a constant expected size. On the other hand, the size of the final grades array that is dynamically allocated will be of variable length, but because the length is capped at a certain class size, the program can be confident it will not exceed an unsafe length.
- In all cases where malloc is used, the memory is subsequently freed with free().
##### Lines
- crypto.c | 19 - 32, 73 - 77, 90 - 115, 125, 155 - 159
- gradebookadd.c | 247 (among other locations)

#### Attack 3: Integer overflow attack
##### Explanation
- There is a potential vulnerability when a program attempts to convert a user inputted number (that is represented by a string) to an actual int value and then use it.
##### Counter-Measure
- When the program tries to parse user inputted numerical values, it uses the function strtol/strtof which provides full error checking. Furthermore, the integer/float number is checked to ensure it remains within the bounds of a valid int/float.
- The program also ensures that it never allocates a buffer based off a user inputted numerical value. The only time where a user is able enter a numerical value is in gradebookadd and it is for the specific grade of a student, the floating point weight of an assignment, or the max point value for an assignment. In all these cases the number is simply converted to an int/float (safely using strol/strof) and stored to disk. This means that even if a user is able to circumvent the security checks within the integer conversion functions, they still would not be able to cause an overflow to an allocated buffer.
##### Lines
- gradebookadd.c | 325 - 329, 332 - 336

#### Attack 4: Brute Force Key Attack
##### Vulnerability
- An attacker could potentially try every possible 16 byte key until they find the one that decrypts the gradebook correctly.
##### Counter-Measure
- While this attack will always be possible, we can prevent the likelihood that an attacker can 'guess' the correct key by using a 16 byte key that is cryptographically secure.
- To generate the key, the cryptographically secure functions, RAND_poll and RAND_bytes, are used to generate a random integer value that is used to select and alphanumeric value for the ith bit of the key. This process continues until the full key is generated.
- While normally having a 16 byte key would mean there are 2^128 total possible keys, the actual byte values are reduced to alphanumeric character to allow for easier command line parsing for the gradebook. This means in actuality there are (32! / 16!) or ~1.25 * 10^22 possible keys which is still enough to stall an attacker for a long time. 
##### Lines
- setup.c | 49 - 66

#### Attack 5: Bit flipping attack
##### Explanation
- If an attacker was able to determine where in the encrypted file certain values lie, they could attempt to change the value of certain encrypted bits that represented say a grade for certain student. This means that when the encrypted file is then decrypted, these values could be changed if the correct bits were changed.
- If the program does not keep track of the integrity of the file that is encrypted/decrypted it is vulnerable to this kind of attack.
##### Counter-Measure
- The gradebook uses an encrypt-then-authenticate scheme to protect the validity/integrity of the encrypted file.
- This means the program first encrypts the plaintext gradebook struct, then generates a 32 byte signature using the 16 byte user inputted key and the SHA-256 HMAC signature algorithm.
- This HMAC value is then stored on disk in a signature.pem file.
- When the program attempts to decrypt the file, it first recreates the signature of the encrypted file using the same method as described above and then checks to see if it matches the HMAC that is stored on disk.
##### Lines
- crypto.c | 61, 132 - 141

#### Attack 6: SQL Injection Attack
##### Explanation
- An attacker could potentially insert SQL code into a query.
- This SQL could potentially then get executed by the program and reveal info about or possibly modify the contents of the gradebook.
##### Counter-Measure
- The program does not actually use a database to keep state in the gradebook. Instead it continually updates a struct. Therefore even if the gradebook tried to parse SQL it would not have an affect on our current storage methods.
- The downside to this is that because we are using our own unique file structure, it takes longer for the program to open and edit the gradebook. For example, deleting a student from the gradebook is a much more resource intensive process than if an SQL table had been used instead.
- Furthermore, we sanitize user input using a blacklist, only allowing alphanumeric characters with some exceptions. Therefore even IF the gradebook used an SQL backend, the input would get cleaned before it was ever able to get executed when used in the program.
##### Lines
- data.c | 136 - 152
