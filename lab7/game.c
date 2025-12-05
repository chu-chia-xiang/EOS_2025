#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

typedef struct {
    int guess;
    char result[8];
}data;

data *shared_data;
int target_number;
int shmid;

void sigusr1_handler(int sig) 
{
    if (shared_data->guess == target_number) 
    {
        strcpy(shared_data->result, "bingo");
    } 
    else if (shared_data->guess > target_number) 
    {
        strcpy(shared_data->result, "smaller");
    } 
    else 
    {
        strcpy(shared_data->result, "bigger");
    }

    printf("[game] Guess %d, %s\n", shared_data->guess, shared_data->result);
}

void cleanup_shared_memory(int sig) 
{
    printf("Cleaning up shared memory...\n");
    shmdt(shared_data);
    shmctl(shmid, IPC_RMID, NULL);
    exit(0);
}

int main(int argc, char *argv[]) 
{
    if (argc != 3) 
    {
        fprintf(stderr, "Usage: %s <key> <guess>\n", argv[0]);
        exit(1);
    }

    key_t key = atoi(argv[1]);
    target_number = atoi(argv[2]);

    shmid = shmget(key, sizeof(data), IPC_CREAT | 0666);

    if (shmid < 0) 
    {
        perror("shmget");
        exit(1);
    }

    shared_data = (data *)shmat(shmid, NULL, 0);

    if (shared_data == (data *)-1) 
    {
        perror("shmat");
        exit(1);
    }

    printf("[game] Game PID: %d\n", getpid());

    signal(SIGUSR1, sigusr1_handler);
    signal(SIGINT, cleanup_shared_memory);

    while (1) 
    {
        pause(); // Wait for signal
    }

    return 0;
}
