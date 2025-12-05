/*
* shm_server.c -- creates the string and shared memory.
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#define SHMSZ 27

int main(int argc, char *argv[])
{
    char c;
    int shmid;
    key_t key;
    char *shm, *s;
    int retval;

    /* We'll name our shared memory segment "5678" */
    key = 5678;
    
    /* Create the segment */
    if ((shmid = shmget(key, SHMSZ, IPC_CREAT | 0666)) < 0) 
    {
        perror("shmget");
        exit(1);
    }

    /* Now we attach the segment to our data space */
    if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) 
    {
        perror("shmat");
        exit(1);
    }   

    printf("Server create and attach the share memory.\n");
    
    /* Now put some things into the memory for the other process to read */
    s = shm;
    
    printf("Server write a ~ z to share memory.\n");
    
    for (c = 'a'; c <= 'z'; c++)
    {
        *s++ = c;
    }

    *s = '\0';

    /*
    * Finally, we wait until the other process changes the first
    * character of our memory to '*', indicating that it has read
    * what we put there.
    */
    printf("Waiting other process read the share memory ...\n");

    while (*shm != '*')
        sleep(1);

    printf("Server read * from the share memory.\n");

    /* Detach the share memory segment */
    shmdt(shm);

    /* Destroy the share memory segment */
    printf("Server destroy the share memory.\n");

    retval = shmctl(shmid, IPC_RMID, NULL);

    if (retval < 0)
    {
        fprintf(stderr, "Server remove share memory failed\n");
        exit(1);
    }

    return 0;
}