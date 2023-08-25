# Welcome to Homework 0!

For these questions you'll need the mini course and  "Linux-In-TheBrowser" virtual machine (yes it really does run in a web page using javascript) at -

http://cs-education.github.io/sys/

Let's take a look at some C code (with apologies to a well known song)-
```C
// An array to hold the following bytes. "q" will hold the address of where those bytes are.
// The [] mean set aside some space and copy these bytes into teh array array
char q[] = "Do you wanna build a C99 program?";

// This will be fun if our code has the word 'or' in later...
#define or "go debugging with gdb?"

// sizeof is not the same as strlen. You need to know how to use these correctly, including why you probably want strlen+1

static unsigned int i = sizeof(or) != strlen(or);

// Reading backwards, ptr is a pointer to a character. (It holds the address of the first byte of that string constant)
char* ptr = "lathe"; 

// Print something out
size_t come = fprintf(stdout,"%s door", ptr+2);

// Challenge: Why is the value of away equal to 1?
int away = ! (int) * "";


// Some system programming - ask for some virtual memory

int* shared = mmap(NULL, sizeof(int*), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
munmap(shared,sizeof(int*));

// Now clone our process and run other programs
if(!fork()) { execlp("man","man","-3","ftell", (char*)0); perror("failed"); }
if(!fork()) { execlp("make","make", "snowman", (char*)0); execlp("make","make", (char*)0)); }

// Let's get out of it?
exit(0);
```

## So you want to master System Programming? And get a better grade than B?
```C
int main(int argc, char** argv) {
	puts("Great! We have plenty of useful resources for you, but it's up to you to");
	puts(" be an active learner and learn how to solve problems and debug code.");
	puts("Bring your near-completed answers the problems below");
	puts(" to the first lab to show that you've been working on this.");
	printf("A few \"don't knows\" or \"unsure\" is fine for lab 1.\n"); 
	puts("Warning: you and your peers will work hard in this class.");
	puts("This is not CS225; you will be pushed much harder to");
	puts(" work things out on your own.");
	fprintf(stdout,"This homework is a stepping stone to all future assignments.\n");
	char p[] = "So, you will want to clear up any confusions or misconceptions.\n";
	write(1, p, strlen(p) );
	char buffer[1024];
	sprintf(buffer,"For grading purposes, this homework 0 will be graded as part of your lab %d work.\n", 1);
	write(1, buffer, strlen(buffer));
	printf("Press Return to continue\n");
	read(0, buffer, sizeof(buffer));
	return 0;
}
```
## Watch the videos and write up your answers to the following questions

**Important!**

The virtual machine-in-your-browser and the videos you need for HW0 are here:

http://cs-education.github.io/sys/

The coursebook:

http://cs341.cs.illinois.edu/coursebook/index.html

Questions? Comments? Use Ed: (you'll need to accept the sign up link I sent you)
https://edstem.org/

The in-browser virtual machine runs entirely in Javascript and is fastest in Chrome. Note the VM and any code you write is reset when you reload the page, *so copy your code to a separate document.* The post-video challenges (e.g. Haiku poem) are not part of homework 0 but you learn the most by doing (rather than just passively watching) - so we suggest you have some fun with each end-of-video challenge.

HW0 questions are below. Copy your answers into a text document (which the course staff will grade later) because you'll need to submit them later in the course. More information will be in the first lab.

## Chapter 1




In which our intrepid hero battles standard out, standard error, file descriptors and writing to files.

### Hello, World! (system call style)
1. Write a program that uses `write()` to print out "Hi! My name is `<Your Name>`".

```
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main() {
  char* str = "Hi! My name is Vlad Petru Nitu";
  write(1, str, strlen(str));
}
```
### Hello, Standard Error Stream!

2. Write a function to print out a triangle of height `n` to standard error.
   - Your function should have the signature `void write_triangle(int n)` and should use `write()`.
   - The triangle should look like this, for n = 3:
  ```C
   *
   **
   ***
   ```

   ```
   #include <stdio.h>
#include <string.h>
#include <unistd.h>

void write_triangle(int n) {
  for (int i = 1; i <= n; ++i) {
    write(2, "***", i);
    write(2, "\n", 1);
  }
  return;
}

int main() {
  write_triangle(3);
}

   ```
### Writing to files
3. Take your program from "Hello, World!" modify it write to a file called `hello_world.txt`.
   - Make sure to to use correct flags and a correct mode for `open()` (`man 2 open` is your friend).

```
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>


int main() {
  char* str = "Hi! My name is Vlad Petru Nitu";
  int fd = open("hello_world.txt", O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
  write(fd, str, strlen(str));
  close(fd);
}

   ```
### Not everything is a system call
4. Take your program from "Writing to files" and replace `write()` with `printf()`.
   - Make sure to print to the file instead of standard out!
   
```
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>


int main() {
  const char* str = "Hi! My name is Vlad Petru Nitu";
  FILE *file = fopen("hello_world.txt", "w");
  fprintf(file, "%s\n", str);
}
```

5. What are some differences between `write()` and `printf()`?
- When using `write()` we do not have to use a `\n` after the string in order to flush the buffer, while `printf()` has to. As `write()` is a system call, it does not have a buffer to flush. On contrary, `printf()` is a library call (which internally uses  `write()`). 
- Write takes an `int fd` as a file descriptor for the file you want to write to, whereas printf takes a `FILE *file` object. 
- In order to print to a file, we need to use `fprintf`, as `printf` prints to `stdout` by-default.



## Chapter 2

Sizing up C types and their limits, `int` and `char` arrays, and incrementing pointers

### Not all bytes are 8 bits?
1. How many bits are there in a byte?
In general: 1 Byte = 8 bits, but as the name of this chapter says: the number of bits in a byte can vary from system to system. Thus, 1 byte is AT LEAST 8 bits.
2. How many bytes are there in a `char`? 1 char = 1 byte
3. How many bytes the following are on your machine?
   - `int`, `double`, `float`, `long`, and `long long`
   - 4 bytes, 8 bytes, 4 bytes, 8 bytes, 8 bytes
   - Proof:
```
   #include <stdio.h>

int main() {
  printf("%ld\n", sizeof(int));
  printf("%ld\n", sizeof(double));
  printf("%ld\n", sizeof(float));
  printf("%ld\n", sizeof(long));
  printf("%ld\n", sizeof(long long));
}

```
### Follow the int pointer
4. On a machine with 8 byte integers:
```C
int main(){
    int data[8];
} 
```
If the address of data is `0x7fbd9d40`, then what is the address of `data+2`?
- 0x7fbd9d50

5. What is `data[3]` equivalent to in C?
   - Hint: what does C convert `data[3]` to before dereferencing the address?
   - *(data + 3)

### `sizeof` character arrays, incrementing pointers
  
Remember, the type of a string constant `"abc"` is an array.

6. Why does this segfault?
```C
char *ptr = "hello";
*ptr = 'J';
```
Because `char *ptr` points to the TEXT segment, where the string literal "hello" is stored. This memory is a READ_ONLY memory, and we try to write in that memory by `*ptr = 'J'`, this is why we get a segfault.

7. What does `sizeof("Hello\0World")` return?
- 12
8. What does `strlen("Hello\0World")` return?
- 5, as it counts the number of characters until finding `\0` = the null character.
9. Give an example of X such that `sizeof(X)` is 3.
- `char *X = "ab";` 
10. Give an example of Y such that `sizeof(Y)` might be 4 or 8 depending on the machine.
- `Y = int*`, as the sizeof returns the size of a pointer in our system. This may be 4 bytes or 8 bytes, depending on the system. Note that sizeof(int*) = sizeof(char*) = sizeof(void*) = ... (etc) on a given system. Thus, the type that the pointer points to does not matter, as the pointer is a MEMORY ADDRESS.

## Chapter 3

Program arguments, environment variables, and working with character arrays (strings)

### Program arguments, `argc`, `argv`
1. What are two ways to find the length of `argv`?
- argc - 1
- sizeof(argv) / sizeof(argv[0]))

2. What does `argv[0]` represent?
-  The name of the executable that we run. 

### Environment Variables
3. Where are the pointers to environment variables stored (on the stack, the heap, somewhere else)?
- Somewhere else, directly above the stack. See the memory layout that was discussed in the first lecture (i.e: draw of professor, where "Program Arguments" is). 
-
### String searching (strings are just char arrays)
4. On a machine where pointers are 8 bytes, and with the following code:
```C
char *ptr = "Hello";
char array[] = "Hello";
```
What are the values of `sizeof(ptr)` and `sizeof(array)`? Why?
- sizeof(ptr) = sizeof(char*) = 8 bytes, as the pointers on the machine in the problem are 8 bytes. 
- sizeof(array) = sizeof(char) * (Number of chars in array) = 1 * (5 + 1) = 6. Remember that char array[] will have a null-character appended.

### Lifetime of automatic variables

5. What data structure manages the lifetime of automatic variables?
- The stack. Automatic variable are allocated on the stack, for the lifetime of the function, and then deallocated, when the scope of the function ends.

## Chapter 4

Heap and stack memory, and working with structs

### Memory allocation using `malloc`, the heap, and time
1. If I want to use data after the lifetime of the function it was created in ends, where should I put it? How do I put it there?
- I should put it on the heap. I put it there by allocating memory (i.e: using `malloc(X)`, which asks the kernel for X bytes of memory. 
- An alternative would be to mark the variable as `static`, as this would make the variable persist in memory for the lifetime of the program. 

2. What are the differences between heap and stack memory?
- The stack is faster, as we only push and pop variables on it, and we can navigate it using the base pointer (BP). The heap is slower, as it allocates memory and this is a tedious process. Both are stored in RAM. Stack does static memory allocation, while the heap allocates memory dynamically. 
3. Are there other kinds of memory in a process?
- Yes, see C memory layout: TEXT segmemnt, DATA (Initialized variables), .bss (Uninitialized variables).
4. Fill in the blank: "In a good C program, for every malloc, there is a free".
### Heap allocation gotchas
5. What is one reason `malloc` can fail?
- If there is no memory left for the heap to allocate. For example, if there is no contiguous block of memory of the size the malloc asks for, it can fail.
6. What are some differences between `time()` and `ctime()`?
- `time()` -> calculates the current calendar time and stores it in an `time_t` data type
- `ctime()` -> transforms a `time_t*` into a string, making the current calendar time human-readable.
7. What is wrong with this code snippet?
```C
free(ptr);
free(ptr);
```
- Freeing the same pointer two times will result in undefined behavior, becuase the memory was released after the first `free()` call. After that, we may releaes the memory that some other process may have aquired in the meantime.

8. What is wrong with this code snippet?
```C
free(ptr);
printf("%s\n", ptr);
```
- We are trying to access a dangling pointer (it became dangling after `free(ptr)`, as we have released the memory. This will result in undefined behavior. 

9. How can one avoid the previous two mistakes? 
- By using memory error detector tools, such as "Memcheck" from Valgrind.

### `struct`, `typedef`s, and a linked list
10. Create a `struct` that represents a `Person`. Then make a `typedef`, so that `struct Person` can be replaced with a single word. A person should contain the following information: their name (a string), their age (an integer), and a list of their friends (stored as a pointer to an array of pointers to `Person`s).

```
struct Person {
  char *name;
  int age;
  struct Person *friends[50];
};

typedef struct Person person_t; 
```
11. Now, make two persons on the heap, "Agent Smith" and "Sonny Moore", who are 128 and 256 years old respectively and are friends with each other.

```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Person {
  char *name;
  int age;
  struct Person *friends[50];
};

typedef struct Person person_t;

void print_person(person_t *person) {
  printf("%s\n", person->name);
  printf("%d\n", person->age);

  printf("%s ", person->friends[0]->name);
  printf("%d\n", person->friends[0]->age);
}
int main() {
  person_t *person1 = (person_t*) malloc(sizeof(person_t));
  person1->name = "Agent Smith";
  person1->age = 128;

  person_t *person2 = (person_t*) malloc(sizeof(person_t));
  person2->name = "Sonny Moore";
  person2->age = 256;



  person1->friends[0] = person2;
  person2->friends[0] = person1;

  printf("Person 1\n");
  print_person(person1);
  printf("Person 2\n");
  print_person(person2);

  //Clear
  memset(person1->friends, 0, sizeof(person1->friends));
  memset(person2->friends, 0, sizeof(person2->friends));
  free(person1);
  free(person2);
}
```

### Duplicating strings, memory allocation and deallocation of structures
Create functions to create and destroy a Person (Person's and their names should live on the heap).

12. `create()` should take a name and age. The name should be copied onto the heap. Use malloc to reserve sufficient memory for everyone having up to ten friends. Be sure initialize all fields (why?).

```
struct Person {
  char *name;
  int age;
  struct Person *friends[50];
};

typedef struct Person person_t;

person_t* create(char *name, int age) {

  char *name_cp = strdup(name);
  person_t *person = (person_t*) malloc(sizeof(person_t));
  person->name = name_cp;
  person->age = age;
  memset(person->friends, 0, sizeof(person->friends)); // Init. this field also, to make sure we clean the memory we have aquired
  return person;
}

```

13. `destroy()` should free up not only the memory of the person struct, but also free all of its attributes that are stored on the heap. Destroying one person should not destroy any others.

```
void destroy(person_t *person) {
  free(person->name);
  memset(person->friends, 0, sizeof(person->friends));
  free(person);
}

```

## Chapter 5 

Text input and output and parsing using `getchar`, `gets`, and `getline`.

### Reading characters, trouble with gets
1. What functions can be used for getting characters from `stdin` and writing them to `stdout`?
2. Name one issue with `gets()`.
### Introducing `sscanf` and friends
3. Write code that parses the string "Hello 5 World" and initializes 3 variables to "Hello", 5, and "World".
### `getline` is useful
4. What does one need to define before including `getline()`?
5. Write a C program to print out the content of a file line-by-line using `getline()`.

## C Development

These are general tips for compiling and developing using a compiler and git. Some web searches will be useful here

1. What compiler flag is used to generate a debug build?
2. You modify the Makefile to generate debug builds and type `make` again. Explain why this is insufficient to generate a new build.
3. Are tabs or spaces used to indent the commands after the rule in a Makefile?
4. What does `git commit` do? What's a `sha` in the context of git?
5. What does `git log` show you?
6. What does `git status` tell you and how would the contents of `.gitignore` change its output?
7. What does `git push` do? Why is it not just sufficient to commit with `git commit -m 'fixed all bugs' `?
8. What does a non-fast-forward error `git push` reject mean? What is the most common way of dealing with this?

## Optional (Just for fun)
- Convert your a song lyrics into System Programming and C code and share on Ed.
- Find, in your opinion, the best and worst C code on the web and post the link to Ed.
- Write a short C program with a deliberate subtle C bug and post it on Ed to see if others can spot your bug.
- Do you have any cool/disastrous system programming bugs you've heard about? Feel free to share with your peers and the course staff on Ed.

