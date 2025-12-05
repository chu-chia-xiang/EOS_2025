#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/time.h>

typedef struct {
    int guess;
    char result[8];
} data;

data *shared_data;
int lower_bound = 1, upper_bound;
pid_t game_pid;

void timer_handler(int sig) 
{
    if (strcmp(shared_data->result, "bingo") == 0) 
    {
        exit(0);
    } 
    else if (strcmp(shared_data->result, "smaller") == 0) 
    {
        upper_bound = shared_data->guess - 1;
    } 
    else if (strcmp(shared_data->result, "bigger") == 0) 
    {
        lower_bound = shared_data->guess + 1;
    }

    shared_data->guess = (lower_bound + upper_bound) / 2;
    printf("[guess] Guess %d\n", shared_data->guess);

    kill(game_pid, SIGUSR1);
}

int main(int argc, char *argv[]) 
{
    if (argc != 4) 
    {
        fprintf(stderr, "Usage: %s <key> <upper_bound> <pid>\n", argv[0]);
        exit(1);
    }

    key_t key = atoi(argv[1]);
    upper_bound = atoi(argv[2]);
    game_pid = atoi(argv[3]);

    int shmid = shmget(key, sizeof(data), 0666);

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

    struct sigaction sa;
    struct itimerval timer;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &timer_handler;
    sigaction(SIGALRM, &sa, NULL);

    timer.it_value.tv_sec = 1;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 1;
    timer.it_interval.tv_usec = 0;

    setitimer(ITIMER_REAL, &timer, NULL);

    shared_data->guess = (lower_bound + upper_bound) / 2;
    printf("[guess] Guess: %d\n", shared_data->guess);

    kill(game_pid, SIGUSR1);

    while (1) 
    {
        pause(); // Wait for timer signal
    }

    return 0;
}
