/* hw1_writer.c - 外送管理系統 互動菜單 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

void send_order_to_driver(int distance, int total_price) {
    int fd = open("/dev/HW1_dev", O_WRONLY);
    if (fd < 0) {
        perror("無法開啟 /dev/HW1_dev");
        return;
    }

    char buffer[32];
    sprintf(buffer, "%d,%d", distance, total_price);
    write(fd, buffer, strlen(buffer));
    close(fd);
}

void show_main_menu() {
    printf("請選擇服務:\n");
    printf("1. shop list\n");
    printf("2. order\n");
}

void show_shop_list() {
    printf("1. Dessert shop: 3km\n");
    printf("2. Beverage shop: 5km\n");
    printf("3. Diner: 8km\n");
}

void show_dessert_menu() {
    printf("1. cookie: $60\n");
    printf("2. cake: $80\n");
    printf("3. confirm\n");
    printf("4. cancel\n");
}

void show_beverage_menu() {
    printf("1. tea: $40\n");
    printf("2. boba: $70\n");
    printf("3. confirm\n");
    printf("4. cancel\n");
}

void show_diner_menu() {
    printf("1. fried rice: $120\n");
    printf("2. egg-drop soup: $50\n");
    printf("3. confirm\n");
    printf("4. cancel\n");
}

void process_order() {
    int shop, item, qty1 = 0, qty2 = 0, total = 0, distance = 0;

    printf("請選擇餐廳 (1~3): ");
    scanf("%d", &shop);

    switch (shop) {
    case 1: /* Dessert */
        distance = 3; qty1 = qty2 = total = 0;
        while (1) {
            show_dessert_menu();
            printf("請選擇商品 (1~4): ");
            scanf("%d", &item);

            if (item == 1) {
                int n = 0; printf("輸入 cookie 數量: "); scanf("%d", &n);
                if (n > 0) qty1 += n;
            } else if (item == 2) {
                int n = 0; printf("輸入 cake 數量: "); scanf("%d", &n);
                if (n > 0) qty2 += n;
            } else if (item == 3) {
                total = qty1 * 60 + qty2 * 80;
                printf("送出訂單，金額: $%d\n", total);
                send_order_to_driver(distance, total);
                printf("please pick up your meal\n");
                break;
            } else if (item == 4) {
                printf("訂單已取消，回到主選單\n");
                break;
            } else {
                printf("無效選項\n");
            }
        }
        break;

    case 2: /* Beverage */
        distance = 5; qty1 = qty2 = total = 0;
        while (1) {
            show_beverage_menu();
            printf("請選擇商品 (1~4): ");
            scanf("%d", &item);

            if (item == 1) {
                int n = 0; printf("輸入 tea 數量: "); scanf("%d", &n);
                if (n > 0) qty1 += n;
            } else if (item == 2) {
                int n = 0; printf("輸入 boba 數量: "); scanf("%d", &n);
                if (n > 0) qty2 += n;
            } else if (item == 3) {
                total = qty1 * 40 + qty2 * 70;
                printf("送出訂單，金額: $%d\n", total);
                send_order_to_driver(distance, total);
                printf("please pick up your meal\n");
                break;
            } else if (item == 4) {
                printf("訂單已取消，回到主選單\n");
                break;
            } else {
                printf("無效選項\n");
            }
        }
        break;

    case 3: /* Diner */
        distance = 8; qty1 = qty2 = total = 0;
        while (1) {
            show_diner_menu();
            printf("請選擇商品 (1~4): ");
            scanf("%d", &item);

            if (item == 1) {
                int n = 0; printf("輸入 fried rice 數量: "); scanf("%d", &n);
                if (n > 0) qty1 += n;
            } else if (item == 2) {
                int n = 0; printf("輸入 egg-drop soup 數量: "); scanf("%d", &n);
                if (n > 0) qty2 += n;
            } else if (item == 3) {
                total = qty1 * 120 + qty2 * 50;
                printf("送出訂單，金額: $%d\n", total);
                send_order_to_driver(distance, total);
                printf("please pick up your meal\n");
                break;
            } else if (item == 4) {
                printf("訂單已取消，回到主選單\n");
                break;
            } else {
                printf("無效選項\n");
            }
        }
        break;

    default:
        printf("尚未實作該餐廳\n");
        break;
    }
}

int main() {
    int choice;
    while (1) {
        show_main_menu();
        printf("輸入選項: ");
        scanf("%d", &choice);

        if (choice == 1) {
            show_shop_list();
        } else if (choice == 2) {
            process_order();
        } else {
            printf("無效選項，請重新輸入\n");
        }
    }
    return 0;
}

