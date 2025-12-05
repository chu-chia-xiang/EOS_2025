#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

/* pid_t 用來表示 process ID；儲存 fork() 的返回值以區分 parent/child */
pid_t childpid;

/* 伺服器 socket（全域） */
int server_fd;

/* 清除 zombie process */
void handler_sigchld(int signum) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

/* Ctrl+C 時關閉伺服器 socket */
void handler_sigint(int signum) {
    close(server_fd);
}

void childfunc(int client_fd) {
    /* 子行程不需要監聽 socket */
    close(server_fd);

    /* 顯示子行程 PID（此行輸出到目前 stdout） */
    printf("Train ID: %d\n", getpid());

    /* 將標準輸出導向 client socket */
    dup2(client_fd, STDOUT_FILENO);
    close(client_fd);

    /* 執行 sl 動畫（小火車） */
    execlp("sl", "sl", "-l", NULL);

    /* 若 exec 失敗 */
    perror("execlp failed");
    exit(EXIT_FAILURE);
}

void parentfunc(int client_fd) {
    /* 親行程關閉與客戶端的連線 socket */
    close(client_fd);
}

int main(int argc, char *argv[]) {
    int client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* 安裝訊號處理器 */
    signal(SIGCHLD, handler_sigchld);
    signal(SIGINT,  handler_sigint);

    /* 建立伺服器 socket（IPv4, TCP） */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    /* 允許重複使用位址 */
    int yes = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    /* 綁定位址與埠 */
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(atoi(argv[1]));
    bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    /* 進入監聽狀態 */
    listen(server_fd, 5);

    /* 主迴圈：接受連線並 fork */
    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd == -1) {
            perror("accept failed");
            continue;
        }

        childpid = fork();
        if (childpid >= 0) {
            if (childpid == 0) {
                /* 子行程 */
                childfunc(client_fd);
            } else {
                /* 親行程 */
                parentfunc(client_fd);
            }
        } else {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }
    }

    close(server_fd);
    return 0;
}

