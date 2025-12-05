#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr,
                "Usage: %s <ip> <port> <deposit/withdraw> <amount> <times>\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }

    // 把命令列參數轉成程式內要用的變數
    const char *server_ip = argv[1];
    int         port      = atoi(argv[2]);
    const char *operation = argv[3];
    int         amount    = atoi(argv[4]);
    int         times     = atoi(argv[5]);
    //建立 TCP socket 並連線到 server
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    //準備要連線的 server 位址結構 server_addr
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    if (connect(client_socket,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];

    for (int i = 0; i < times; ++i) {
        // 構造請求："deposit 1" / "withdraw 2" 之類
        snprintf(buffer, BUFFER_SIZE, "%s %d", operation, amount);

        // 將請求發送到伺服器
        send(client_socket, buffer, strlen(buffer), 0);

        // 清空 buffer 並接收伺服器回應
        memset(buffer, 0, BUFFER_SIZE);
        recv(client_socket, buffer, BUFFER_SIZE, 0);
    }

    close(client_socket);
    return 0;
}

