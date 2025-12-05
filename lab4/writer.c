#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

// 驅動程式與user space交互的裝置文件路徑
#define DEVICE "/dev/mydev"

int main(int argc, char *argv[]) {
    int fd;     
    char buffer[2];  // 用來存放字母和結束符
    buffer[1] = '\0';  // 字符串結束符

    // 檢查是否提供了必要的參數
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <string>\n", argv[0]);
        return 1;
    }

    // 開啟裝置文件,O_RDWR為允許讀和寫操作
    fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("Failed to open the device");
        return 1;
    }

    // 取得傳入的字串參數
    char *input_string = argv[1];
    int length = strlen(input_string);

    // 每秒寫入字串中的每個字母
    // buffer[0] 用來存放字母
    for (int i = 0; i < length; i++) {
        buffer[0] = input_string[i];  // 設置當前要寫入的字母
        write(fd, buffer, strlen(buffer));  // 寫入字母到驅動
        printf("Wrote %c to the device\n", buffer[0]);

        sleep(1);  // 等待 1 秒
    }

    close(fd);

    return 0;
}
