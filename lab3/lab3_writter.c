#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    const char *default_id = "313611104";
    const char *id = (argc >= 2 && argv[1] && argv[1][0]) ? argv[1] : default_id;
    ssize_t len = (ssize_t)strlen(id);

    int fd = open("/dev/lab3_dev", O_WRONLY);
    if (fd < 0) { perror("open write"); return 1; }
    if (write(fd, id, len) != len) { perror("write id"); close(fd); return 1; }
    close(fd);

    fd = open("/dev/lab3_dev", O_RDONLY);
    if (fd < 0) { perror("open read"); return 1; }

    while (1) {
        char ch;
        ssize_t n = read(fd, &ch, 1);
        if (n < 0) { if (errno == EINTR) continue; perror("read"); break; }
        if (n == 0) { continue; }   // 無資料則重試
        printf("Display: %c\n", ch);
        sleep(1);                   // 每秒切換一碼
    }
    close(fd);
    return 0;
}
