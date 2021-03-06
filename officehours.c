/*
Name: Marium Mannan
ID:   1001541605
*/

/*
NOTES FOR TA:
- made changes in classA_enter(), classB_enter(), and prof thread only
- destroyed mutex and semphores in main
- mutexes and conditions are initialised using builti in initializer
- Reqirement 4 is not implemented in this code (kept deadlocking)

Thanks!
*/

//starter code taken from github 

#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

/*** Constants that define parameters of the simulation ***/

#define MAX_SEATS 3        /* Number of seats in the professor's office */
#define professor_LIMIT 10 /* Number of students the professor can help before he needs a break */
#define MAX_STUDENTS 1000  /* Maximum number of students in the simulation */

#define CLASSA 0
#define CLASSB 1


//MUTEXES
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

//CONDTIONS
pthread_cond_t student_leaves = PTHREAD_COND_INITIALIZER;
pthread_cond_t prof_back_from_break = PTHREAD_COND_INITIALIZER;
pthread_cond_t prof_break = PTHREAD_COND_INITIALIZER;

//SEMAPHORES
sem_t empty;
sem_t full;


/* Basic information about simulation.  They are printed/checked at the end 
 * and in assert statements during execution.
 *
 * You are responsible for maintaining the integrity of these variables in the 
 * code that you develop. 
 */

static int students_in_office;/* Total numbers of students currently in the office */
static int classa_inoffice; /* Total numbers of students from class A currently in the office */
static int classb_inoffice;/* Total numbers of students from class B in the office */
static int students_since_break = 0;/* Total numbers of students since last break */
static int total_classA_since_break = 0; /* Total numbers of class A students since last break */
static int total_classB_since_break = 0;/* Total numbers of class B students since last break */
static int prof_on_break; //1 if professor is on break 0 if not 


typedef struct 
{
  int arrival_time;  // time between the arrival of this student and the previous student
  int question_time; // time the student needs to spend with the professor
  int student_id;
  int class;
} student_info;

/* Called at beginning of simulation.  
 * TODO: Create/initialize all synchronization
 * variables and other global variables that you add.
 */
static int initialize(student_info *si, char *filename) 
{
  students_in_office = 0;
  classa_inoffice = 0;
  classb_inoffice = 0;
  students_since_break = 0;

  /* Initialize your synchronization variables (and 
   * other variables you might use) here
   */
  prof_on_break = 0;

  //initialize semaphores
  sem_init(&full,0, professor_LIMIT);
  sem_init(&empty,0, MAX_SEATS);

  /* Read in the data file and initialize the student array */
  FILE *fp;

  if((fp=fopen(filename, "r")) == NULL) 
  {
    printf("Cannot open input file %s for reading.\n", filename);
    exit(1);
  }

  int i = 0;
  while ( (fscanf(fp, "%d%d%d\n", &(si[i].class), &(si[i].arrival_time), &(si[i].question_time))!=EOF) && 
           i < MAX_STUDENTS ) 
  {
    i++;
  }

 fclose(fp);
 return i;
}

/* Code executed by professor to simulate taking a break 
 * You do not need to add anything here.  
 */
static void take_break() 
{
  //ADDED MUTEX for protection 
  pthread_mutex_lock(&mutex);
  printf("The professor is taking a break now.\n");
  sleep(5);
  assert( students_in_office == 0 );
  students_since_break = 0;
  total_classA_since_break = 0;
  total_classB_since_break = 0;
  pthread_mutex_unlock(&mutex);

}

/* Code for the professor thread. This is fully implemented except for synchronization
 * with the students.  See the comments within the function for details.
 */
void *professorthread(void *junk) 
{
  printf("The professor arrived and is starting his office hours\n");
  
  /* Loop while waiting for students to arrive. */
  while (1) 
  { 
    //added mutex for protection
    pthread_mutex_lock(&mutex); 

    //wait till its time to teak a break;
    pthread_cond_wait(&prof_break, &mutex);
    prof_on_break = 1; //reset to true
    //prepare to go on break when all students leave
    while(students_in_office != 0)
    {
      pthread_cond_wait(&student_leaves, &mutex);
    }
    //take a break
    pthread_mutex_unlock(&mutex);
    take_break();
    pthread_mutex_lock(&mutex);
    int k = 0;
    while(k++<10)
    {
      //semaphore to keep track of 10 students
      sem_post(&full);
    }
    //students waiting can enter when prof back from break
    pthread_cond_signal(&prof_back_from_break);
    prof_on_break = 0; //reset to false
    pthread_mutex_unlock(&mutex);
  }
  
  pthread_exit(NULL);
}


/* Code executed by a class A student to enter the office.
 * You have to implement this.  Do not delete the assert() statements,
 * but feel free to add your own.
 */
void classa_enter() 
{
  sem_wait(&empty); //wait till 1 of 3 seats is empty
  sem_wait(&full); //wait if youre the 11th student and prof is due for break
  
  //locks added for protection
  pthread_mutex_lock(&mutex);
  
  //wait till all class b students leave before entering
  while(classb_inoffice != 0)
  {
    pthread_cond_wait(&student_leaves, &mutex);
  }

  //wait till prof comes back from break to office
  while(prof_on_break == 1)
  {
    pthread_cond_wait(&prof_back_from_break, &mutex);
  } 

  //counters updated on entering
  students_in_office += 1;
  students_since_break += 1;
  classa_inoffice += 1;
  total_classA_since_break += 1;
  
  //if youre the 10th student let the prof know its time for a break
  if(students_since_break == professor_LIMIT)
  {
    pthread_cond_signal(&prof_break);
  }

  pthread_mutex_unlock(&mutex);

}

/* Code executed by a class B student to enter the office.
 * You have to implement this.  Do not delete the assert() statements,
 * but feel free to add your own.
 */
void classb_enter() 
{ 
  sem_wait(&empty);//wait till 1 of 3 seats is empty
  sem_wait(&full); //wait if youre the 11th student and prof is due for break

  //locks added for protection
  pthread_mutex_lock(&mutex);
  
  //wait till all class a students leave before entering
  while(classa_inoffice != 0 && prof_on_break != 1)
  {
    pthread_cond_wait(&student_leaves, &mutex);
  }

  //wait till prof comes back from break to office
  while(prof_on_break == 1)
  {
    pthread_cond_wait(&prof_back_from_break, &mutex);
  }
  
  //counters updated on entering
  students_in_office += 1;
  students_since_break += 1;
  classb_inoffice += 1;
  total_classB_since_break += 1;

  //if youre the 10th student let the prof know its time for a break
  if(students_since_break == professor_LIMIT)
  {
    pthread_cond_signal(&prof_break);
  }
  

  pthread_mutex_unlock(&mutex);
  

 
}

/* Code executed by a student to simulate the time he spends in the office asking questions
 * You do not need to add anything here.  
 */
static void ask_questions(int t) 
{
  sleep(t);
}


/* Code executed by a class A student when leaving the office.
 * You need to implement this.  Do not delete the assert() statements,
 * but feel free to add as many of your own as you like.
 */
static void classa_leave() 
{
  /* 
   *  TODO
   *  YOUR CODE HERE. 
   */
  
  sem_post(&empty);
  
  pthread_mutex_lock(&mutex);

  
  students_in_office -= 1;
  classa_inoffice -= 1;

  pthread_cond_signal(&student_leaves);

  pthread_mutex_unlock(&mutex);

  

}

/* Code executed by a class B student when leaving the office.
 * You need to implement this.  Do not delete the assert() statements,
 * but feel free to add as many of your own as you like.
 */
static void classb_leave() 
{
  /* 
   * TODO
   * YOUR CODE HERE. 
   */
  
  pthread_mutex_lock(&mutex);
  
  
  students_in_office -= 1;
  classb_inoffice -= 1;

  sem_post(&empty);

  pthread_cond_signal(&student_leaves);
  pthread_mutex_unlock(&mutex);
  




}

/* Main code for class A student threads.  
 * You do not need to change anything here, but you can add
 * debug statements to help you during development/debugging.
 */
void* classa_student(void *si) 
{
  student_info *s_info = (student_info*)si;

  /* enter office */
  classa_enter();

  printf("Student %d from class A enters the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classb_inoffice == 0 );
  
  /* ask questions  --- do not make changes to the 3 lines below*/
  printf("Student %d from class A starts asking questions for %d minutes\n", s_info->student_id, s_info->question_time);
  ask_questions(s_info->question_time);
  printf("Student %d from class A finishes asking questions and prepares to leave\n", s_info->student_id);

  /* leave office */
  classa_leave();  

  printf("Student %d from class A leaves the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);

  pthread_exit(NULL);
}

/* Main code for class B student threads.
 * You do not need to change anything here, but you can add
 * debug statements to help you during development/debugging.
 */
void* classb_student(void *si) 
{
  student_info *s_info = (student_info*)si;

  /* enter office */
  classb_enter();

  printf("Student %d from class B enters the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
  assert(classa_inoffice == 0 );

  printf("Student %d from class B starts asking questions for %d minutes\n", s_info->student_id, s_info->question_time);
  ask_questions(s_info->question_time);
  printf("Student %d from class B finishes asking questions and prepares to leave\n", s_info->student_id);

  /* leave office */
  classb_leave();        

  printf("Student %d from class B leaves the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);

  pthread_exit(NULL);
}

/* Main function sets up simulation and prints report
 * at the end.
 * GUID: 355F4066-DA3E-4F74-9656-EF8097FBC985
 */
int main(int nargs, char **args) 
{
  int i;
  int result;
  int student_type;
  int num_students;
  void *status;
  pthread_t professor_tid;
  pthread_t student_tid[MAX_STUDENTS];
  student_info s_info[MAX_STUDENTS];
  
  //initialize semaphore and mutex
  

  if (nargs != 2) 
  {
    printf("Usage: officehour <name of inputfile>\n");
    return EINVAL;
  }

  num_students = initialize(s_info, args[1]);
  if (num_students > MAX_STUDENTS || num_students <= 0) 
  {
    printf("Error:  Bad number of student threads. "
           "Maybe there was a problem with your input file?\n");
    return 1;
  }

  printf("Starting officehour simulation with %d students ...\n",
    num_students);

  result = pthread_create(&professor_tid, NULL, professorthread, NULL);

  if (result) 
  {
    printf("officehour:  pthread_create failed for professor: %s\n", strerror(result));
    exit(1);
  }

  for (i=0; i < num_students; i++) 
  {

    s_info[i].student_id = i;
    sleep(s_info[i].arrival_time);
                
    student_type = random() % 2;

    if (s_info[i].class == CLASSA)
    {
      result = pthread_create(&student_tid[i], NULL, classa_student, (void *)&s_info[i]);
    }
    else // student_type == CLASSB
    {
      result = pthread_create(&student_tid[i], NULL, classb_student, (void *)&s_info[i]);
    }

    if (result) 
    {
      printf("officehour: thread_fork failed for student %d: %s\n", 
            i, strerror(result));
      exit(1);
    }
  }

  /* wait for all student threads to finish */
  for (i = 0; i < num_students; i++) 
  {
    pthread_join(student_tid[i], &status);
  }

  /* tell the professor to finish. */
  pthread_cancel(professor_tid);

  printf("Office hour simulation done.\n");

  //destory semaphores and mutex
  pthread_mutex_destroy(&mutex);
  pthread_mutex_destroy(&mutex1);
  sem_destroy(&empty);
  sem_destroy(&full);


  return 0;
}