#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <signal.h>
#include <errno.h>

#define BUFFER_SIZE 1024
#define SEM_MODE    0666   /* rw(owner)-rw(group)-rw(other) permission */
#define SEM_KEY     1122334455

int account_balance = 0;
int sem;                // semaphore 的 ID，用於保護共享資源
int total_requests = 0; // 記錄總請求數（目前未使用）
int server_socket;      // 儲存伺服器的 socket，用於清理

/* P() - returns 0 if OK; -1 if there was a problem
 * P() 實現 semaphore 的「lock」操作。
 *如果 semaphore 值 > 0 → 減 1，立刻回傳 0（取得鎖）
 *如果值 = 0 → 呼叫的 thread 被卡在 semop() 裡面等，直到有人做 V(+1) 之後才會醒來繼續，醒來後同樣會  把值減 1 再回傳 0。
 * 當 semaphore 的值為 0 時，呼叫的執行緒會被阻塞，直到其他執行緒釋放鎖。
 */
int P(int s) {
    struct sembuf sop;  /* the operation parameters */

    sop.sem_num = 0;    /* access the 1st (and only) sem in the array */
    sop.sem_op  = -1;   /* wait .. */
    sop.sem_flg = 0;    /* no special options needed */

    if (semop(s, &sop, 1) < 0) {
        fprintf(stderr, "P(): semop failed: %s\n", strerror(errno));
        return -1;
    } else {
        return 0;
    }
}

/* V() - returns 0 if OK; -1 if there was a problem
 *釋放semaphore
 * V() 實現 semaphore 的「unlock」操作，允許其他被阻塞的執行緒進入 critical section。
 */
int V(int s) {
    struct sembuf sop;  /* the operation parameters */

    sop.sem_num = 0;    /* the 1st (and only) sem in the array */
    sop.sem_op  = 1;    /* signal */
    sop.sem_flg = 0;    /* no special options needed */

    if (semop(s, &sop, 1) < 0) {
        fprintf(stderr, "V(): semop failed: %s\n", strerror(errno));
        return -1;
    } else {
        return 0;
    }
}

/* cleanup()：參考講義 race.c
 * 收到 SIGINT 時移除 semaphore 並關閉 server socket
 */
void cleanup(int signo) {
    (void)signo;

    // 移除 semaphore
    if (semctl(sem, 0, IPC_RMID, 0) < 0) {
        fprintf(stderr, "Unable to remove semaphore: %s\n",
                strerror(errno));
    } else {
        printf("Semaphore %d removed\n", SEM_KEY);
    }

    // 關閉伺服器 socket
    close(server_socket);

    exit(0);
}

/*不斷收 "deposit N" / "withdraw M" 的請求
 *用 P(sem) / V(sem) 確保只有自己在改 account_balance
 *更新餘額並印出結果
 *每筆操作完回 "ACK" 給 client
*client 關線就關閉 socket、結束 thread。
*/
void *handle_client(void *client_socket) {
    int sock = *(int *)client_socket;
    free(client_socket);  // 釋放分配的記憶體

    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_read = recv(sock, buffer, BUFFER_SIZE, 0);
        if (bytes_read <= 0)
            break;

        char operation[10];
        int amount;
        sscanf(buffer, "%s %d", operation, &amount);

        // 使用 P() 和 V() 保護 critical section
        P(sem);

        // 處理請求
        if (strcmp(operation, "deposit") == 0) {
            account_balance += amount;
            printf("After deposit: %d\n", account_balance);
        } else if (strcmp(operation, "withdraw") == 0) {
            if (account_balance >= amount) {
                account_balance -= amount;
                printf("After withdraw: %d\n", account_balance);
            } else {
                printf("Insufficient balance for withdraw! "
                       "Current balance: %d\n",
                       account_balance);
            }
        }

        V(sem);

        send(sock, "ACK", 3, 0);
    }

    close(sock);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // 設定 server 端按下 Ctrl+C 則執行清除 semaphore 的功能
    signal(SIGINT, cleanup);

    // 建立 semaphore，避免重複創建
    sem = semget(SEM_KEY, 1, IPC_CREAT | IPC_EXCL | SEM_MODE);
    if (sem < 0) {
        if (errno == EEXIST) {
            // 如果 semaphore 已存在，重新獲取
            sem = semget(SEM_KEY, 1, SEM_MODE);
            if (sem < 0) {
                fprintf(stderr,
                        "Failed to retrieve existing semaphore: %s\n",
                        strerror(errno));
                exit(EXIT_FAILURE);
            }
        } else {
            fprintf(stderr, "Sem creation failed: %s\n",
                    strerror(errno));
            exit(EXIT_FAILURE);
        }
    } else {
        // 初始化 semaphore
        if (semctl(sem, 0, SETVAL, 1) < 0) {
            fprintf(stderr, "Unable to initialize semaphore: %s\n",
                    strerror(errno));
            exit(EXIT_FAILURE);
        }
        printf("Semaphore %d created and initialized to 1\n", SEM_KEY);
    }

    // 建立 socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        cleanup(SIGINT);  // 確保資源清理
    }

    // 設定伺服器的地址資訊
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(atoi(argv[1]));

    if (bind(server_socket,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0) {
        perror("Bind failed");
        cleanup(SIGINT);
    }

    if (listen(server_socket, 5) < 0) {
        perror("Listen failed");
        cleanup(SIGINT);
    }

    printf("Server listening on port %s...\n", argv[1]);

    while (1) {
        int *client_socket = malloc(sizeof(int));
        if (!client_socket) {
            fprintf(stderr, "Memory allocation failed\n");
            continue;
        }

        *client_socket = accept(server_socket,
                                (struct sockaddr *)&client_addr,
                                &client_len);
        if (*client_socket < 0) {              // ← 這裡要檢查 *client_socket
            perror("Accept failed");
            free(client_socket);
            continue;
        }

        // 接受每個客戶端的連線，並為其創建一個新的執行緒執行
        pthread_t thread;
        pthread_create(&thread, NULL,
                       handle_client, client_socket);
        pthread_detach(thread);
    }

    return 0;
}

