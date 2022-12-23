//
// Created by dario on 11/4/2022.
//

//first off there is no input from the user
//we will need a function for the two producers: producer type 1 and producer type 2
//these producers are child processes(so create two child processes) and
//will execute in a loop every 0.01 - 0.2 seconds(random seconds):
//call random seconds with clock_gettime() and use nanoseconds. possibly with srand
//produce "something" and assign "product type number"
//increment "product production count"
//send type number and count to a pipe
//you can only send text through a pipe
//create a process called the distributor(main) at the other end of the pipe
//distributor uses put() to send info to 2 queues
//two queues: q1 and q2 (Should create these buffers before the distributor since it will get/put from
//q1 and q2
//q1 creates two threads for a type 1 product and q2 creates two threads for the type 2 products
//the 4 threads use get to take the info from the queues and write to a file
//all that the threads do is write to a fileS
//all four threads write to one file

/*
 * # 11/2/22 Lab

- Everything we need to know in 3EP (4 chapters)
- Provides code from book we should use in slides
- new thread has to be of void type
    - Cast argument to right type
    - has to be pointer
- Look at mesa semantics to structure locks and CV’s
    - putting critical section under lock control
    - Find out if lock avail
    - Find out if other conditions prevent you from using cs
    - Using a while loop
- Suggests a circular queue instead of array/linked list
    - ~5 lines of code vs. 15+
- Read link in slide
- Queue’s need to be under lock control
    - We will have 3 queues
- Circular Queue
    - You have array, pointer, and count of # of items in array
    - You have mod function to restart pointer at 0 index of array
- 2 processes (2 producers) share a pipe
    - There is no race condition as long as you write less than 4096 chars
    - Pipe shared between 2 processes goes to distributor
        - distributor reads from pipe
        - distributes into 2 circular queues
            - If Queue is full wait on CV
- 4 threads
    - 2 threads look for p0, 2 threads look for p1
    - want to try and use same code for all 4 threads
    - you want to pointer to queues and product it is trying to consume
- Locks and CV’s MUST be global
- Once you have pipe FD you always have it
    - Distributor uses read side of pipe
    - Make sure you close end you aren’t using
- in the end they all write to a file
    - file is opened in distributor
        - everyone will be able to access FD but need to know what FD is
    - It’s ok if they write in random order
- If you define a struct you can reuse code for threads
    - pointer to lock and cv (whichever you are using)
    - pointer to a queue (should already know how to access queue)
    - FD
- Might want to fork in code to go to another function
    - 7 execution units
        - 3 processes 4 threads
    - forking will happen when you figure out 2 producer processes
 */


#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <time.h>
#include <pthread.h>

#define MAXBUFF 4096
#define MAXBUFF1 50
#define MAXBUFF2 75

char** buffItems1[MAXBUFF1];
int front1 = -1;
int rear1 = -1;

char** buffItems2[MAXBUFF2];
int front2 = -1;
int rear2 = -1;

struct proInfo{
    int count;
    int consumption;
    int type;
    FILE *fptr;
};

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t fill = PTHREAD_COND_INITIALIZER;

int theType = 0;
int consumption1 = 1;
int consumption2 = 1;
int ready = 0;

char* produce(struct proInfo *p1, struct proInfo *p2);
void myWait();
void inBuff1(char* deliverable);
void inBuff2(char* deliverable);
char* outBuff1();
char* outBuff2();

void threadFunction(struct proInfo *info);

void* output(void* p);

//main will be the distributor
int main(int argc, char *argv[]){
    char* deliverable;

    //there's two types of threads to create. thread type 1 and thread type 2.
    //each type of thread will have two threads of its type

    struct proInfo p1;
    p1.count = 1;
    p1.consumption = 1;
    p1.type = 1;
    struct proInfo p2;
    p2.count = 1;
    p2.consumption = 1;
    p2.type = 2;

    FILE *fptr = fopen("ProConOutput.txt", "w");

    p1.fptr = fptr;
    p2.fptr = fptr;

    int i = 5;
    while(i--){
        char* count = malloc(MAXBUFF);
        //get the deliverable from producer
        deliverable = produce(&p1, &p2);
        //get the count into its own string
        for(int j = 1; deliverable[j] != '\0'; j++){
            count[j-1] = deliverable[j];
        }
        //start of buffer type 1
        if(deliverable[0] == 49){
            theType = 1;
            printf("We got a one\n");

            pthread_mutex_lock(&lock);

            while(ready == 1){
                pthread_cond_wait(&empty, &lock);
            }

            inBuff1(deliverable);//put it in buff

            pthread_cond_signal(&fill);
            pthread_mutex_unlock(&lock);

            threadFunction(&p1);
            consumption1++;
        }
        else {//start of buffer type 2
            theType = 2;
            printf("We got a two\n");

            pthread_mutex_lock(&lock);

            while(ready == 0){
                pthread_cond_wait(&empty, &lock);
            }

            inBuff2(deliverable);

            pthread_cond_signal(&fill);
            pthread_mutex_unlock(&lock);

            threadFunction(&p2);
            consumption2++;
        }
        //free the count char for reuse
        free(count);
    }
    fclose(fptr);
}

char* produce(struct proInfo *p1, struct proInfo *p2){
    int status1 = 0;
    int status2 = 0;
    //child pids
    pid_t pid2;
    pid_t pid;
    //pid_t w;
    //call pipe then fork
    //fd[0] will be for the read end of the pipe
    //fd[1] will be for the write end of the pipe
    int fd[2];
    //allocate space for these strings
    char *type1args = malloc(MAXBUFF);
    char *type2args = malloc(MAXBUFF);

    char *buffer = malloc(MAXBUFF);
    char *buffer2 = malloc(MAXBUFF);

    char *deliverable = malloc(MAXBUFF);

    int c = pipe(fd);
    if(c == 0) {
        pid = fork();
        if (pid == -1) {
            printf("Error could not create child 1\n");
            exit(1);
        } else if (pid == 0) {//child 1
            close(fd[0]);//close the end you are not using
            //store the two values as text, send across the pipe. Will then scan the buffer for these
            //values later on

            sprintf(buffer, "%d", p1->count);
            strncpy(type1args, "1", MAXBUFF);
            strncat(type1args, buffer, MAXBUFF);

            //here we are converting count to a string, copying it to a char *, then adding the process type
            int d = write(fd[1], type1args, sizeof(type1args));
            //add to pipe
            if(d == -1){
                printf("Child 1 failed to write into pipe\n");
                kill(pid, 1);
            }

            close(fd[1]);
            //now close the end you used
            myWait();
            //we need to wait a random amount of time before we move on
            exit(0);
            //kill this child
        } else {
            pid2 = fork();
            if (pid2 == -1) {
                printf("Could not create child 2\n");
            } else if (pid2 == 0) {//child 2
                close(fd[0]);

                sprintf(buffer2, "%d", p2->count);
                strncpy(type2args, "2", MAXBUFF);
                strncat(type2args, buffer2, MAXBUFF);

                int e = write(fd[1], type2args, sizeof(type2args));

                if(e == -1){
                    printf("Child 2 failed to write into pipe\n");
                    kill(pid2, 1);
                }

                close(fd[1]);
                myWait();
                exit(1);
            } else {//parent
                close(fd[1]);

                //while ((w = wait(&status)) > 0);//parent waits for both children to finish
                while((waitpid(pid, &status1, WUNTRACED) || waitpid(pid2, &status2, WUNTRACED)) != true){
                    //just wait for one or the other
                }

                read(fd[0], deliverable, MAXBUFF);

                if(deliverable[0] == 49){
                    p1->count++;
                }
                else{
                    p2->count++;
                }

                close(fd[0]);
                free(type1args);
                free(type2args);
                free(buffer);
                free(buffer2);
            }
            //I think ill have to reset the all the strings here or in parent
        }
    }
    else{
        printf("Failed to create pipe\n");
        exit(1);
    }
    return deliverable;
}

void myWait(){
    int lower = 10000;
    int upper = 200000;
    //upper and lower bounds in microseconds
    int range = upper - lower;
    int div = RAND_MAX / range;
    int num = lower + (rand()/div);
    //these three functions get the random time
    usleep((useconds_t)num);
    //sleep for that time
}

void inBuff1(char* deliverable){
    //checks for full queue
    //two cases could be a full: the front position is one more than the rear position
    //or the front is at 0 and the rear is at the position before it
    if((front1 == rear1 + 1) || (front1 == 0 && rear1 == MAXBUFF1 - 1)){
        printf("Buffer 1 is full\n");
        exit(1);
    }
    if(front1 == -1){
        front1 = 0;
    }
    rear1 = (rear1 + 1) % MAXBUFF1;
    buffItems1[rear1] = &deliverable;
    printf("Inserted %s to the buffer\n", *buffItems1[rear1]);
}
void inBuff2(char* deliverable){
    //checks for full queue
    if((front2 == rear2 + 1) || (front2 == 0 && rear2 == MAXBUFF2 - 1)){
        printf("Buffer 2 is full\n");
        exit(1);
    }
    if(front2 == -1){
        front2 = 0;
    }
    rear2 = (rear2 + 1) % MAXBUFF2;
    buffItems2[rear2] = &deliverable;
    printf("Inserted %s to the buffer\n", *buffItems2[rear2]);
}

char* outBuff1(){
    char *info1;
    char *thisCount = malloc(MAXBUFF);
    char *outputStr = malloc(MAXBUFF);
    //if front is still -1 than queue never filled
    if(front1 == -1){
        printf("The queue is empty\n");
        exit(1);
    }
    //get the deliverable
    info1 = *buffItems1[front1];
    for(int j = 1; info1[j] != '\0'; j++){//this is for the count
        thisCount[j-1] = info1[j];
    }
    //below is the output that I would like, which will be sent to output(consumer)
    sprintf(outputStr, "Process Type: %c, Process Count: %s, Consumption Number: %d\n", info1[0], thisCount, consumption1);

    if(front1 == rear1){
        front1 = -1;
        rear1 = -1;
    }
    else{
        front1 = (front1 +1) % MAXBUFF1;
    }
    printf("The element: %s has been returned and dequeued\n", info1);
    return outputStr;
}

char* outBuff2(){
    char *info2;
    char *thisCount = malloc(MAXBUFF);
    char *outputStr = malloc(MAXBUFF);

    if(front2 == -1){
        printf("Buffer 2 is empty\n");
        exit(1);
    }

    info2 = *buffItems2[front2];
    for(int j = 1; info2[j] != '\0'; j++){
        thisCount[j-1] = info2[j];
    }
    sprintf(outputStr, "Process Type: %c, Process Count: %s, Consumption Number: %d\n", info2[0], thisCount, consumption2);

    if(front2 == rear2){
        front2 = -1;
        rear2 = -1;
    }
    else{
        front2 = (front2 +1) % MAXBUFF2;
    }
    printf("The element: %s has been returned and dequeued\n", info2);
    return outputStr;
}

void threadFunction(struct proInfo *info){
    //either type 1 or 2
    printf("The type here is %d\n", info->type);
    if(info->type == 1){
        ready = 0;
        //create the thread
        pthread_t thread_id1;
        //create the thread and it executes output sending in the info struct
        int rc = pthread_create(&thread_id1, NULL, output, (void *)&info);
        printf("%d\n", rc);
    }
    else if(info->type == 2){
        ready = 1;
        pthread_t thread_id2;
        int rc = pthread_create(&thread_id2, NULL, output, (void *)&info);
        printf("%d\n", rc);
    }
    else{
        printf("Error, incorrect type\n");
        exit(1);
    }
    //rest of the program waits for the thread to finish
    pthread_exit(NULL);
    }

void* output(void* p){
    //create the struct from the pointer
    struct proInfo *args = (struct proInfo *) p;

    printf("Tha type is %d\n", theType);
    //FILE* fptr;//file pointer
    char* info;
    //either type one, two or error

    pthread_mutex_lock(&lock);
    //pthread_cond_wait(&fill, &lock);

    if(theType == 1){
        printf("This is type one\n");
        info = outBuff1();
    }
    else if(theType == 2){
        printf("This is type two\n");
        info = outBuff2();
    }
    else{
        printf("Errorrr\n");
        exit(1);
    }

    size_t e = fwrite(info, 1, MAXBUFF, args->fptr);
    if(e == 0){
        printf("Error no fwrite\n");
    }
    fclose(args->fptr);

    pthread_cond_signal(&empty);
    pthread_mutex_unlock(&lock);

    return p;
}