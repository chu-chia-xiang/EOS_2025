#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>

// 使用mutex來保護 delivery_persons 資料
// 確保多執行緒同時存取時不會發生衝突
pthread_mutex_t delivery_mutex = PTHREAD_MUTEX_INITIALIZER;

struct food {
    char name[20];
    int  price;
};
typedef struct food Food;

struct shop {
    char name[20];
    int  distance;
    Food items[2];
};
typedef struct shop Shop;

/*
  定義外送員結構：
  - Person：外送員編號
  - current_remain_time：這位外送員當前還需花費的秒數 (正在處理的外送)
*/
struct deliveryPerson {
    int Person;
    int current_remain_time;
};
typedef struct deliveryPerson DeliveryPerson;

// 測試資料：三家店，每家店有兩道菜
Shop shops[3] = {
    {"Dessert shop",  3, {{"cookie", 60}, {"cake", 80}}},
    {"Beverage shop", 5, {{"tea", 40}, {"boba", 70}}},
    {"Diner",         8, {{"fried-rice", 120}, {"Egg-drop-soup", 50}}}
};

int shops_num = sizeof(shops) / sizeof(Shop);

/*
  同時有兩名外送員，分別初始化為 Person=1, remain=0 與 Person=2, remain=0
  remain=0 表示尚無工作量
*/
DeliveryPerson delivery_persons[2] = {
    {1, 0},
    {2, 0}
};

int server_fd;

// SIGINT 信號處理函數
void sigint_handler(int signum)
{
    close(server_fd);
    fprintf(stderr, "Server closed.\n");
    exit(0);
}

int get_food_count(Shop shop)
{
    return sizeof(shop.items) / sizeof(Food);
}

// 查找哪一家店賣這個 food，回傳店家 index；找不到就回 -1
int find_shop_by_food(Shop *shops, int shops_num, char *order_food)
{
    for (int i = 0; i < shops_num; i++) {
        int food_num = get_food_count(shops[i]);
        for (int j = 0; j < food_num; j++) {
            if (strcmp(order_food, shops[i].items[j].name) == 0) {
                return i;
            }
        }
    }
    return -1;
}

// 查找 food 在該店中的 index
int find_food_index(Shop order_shop, int food_num, char *order_food)
{
    for (int i = 0; i < food_num; i++) {
        if (strcmp(order_food, order_shop.items[i].name) == 0) {
            return i;
        }
    }
    return -1;
}

/*
  get_restaurant：根據餐點名稱判斷所屬餐廳代號
  - return 1: Dessert shop
  - return 2: Beverage shop
  - return 3: Diner
  - return 0: 無效餐點
*/
int get_restaurant(const char *dishes)
{
    // 透過 find_shop_by_food() 找到 shops[] 裡賣這道菜的店家索引
    int idx = find_shop_by_food(shops, shops_num, (char *)dishes);
    if (idx == -1) {
        // 表示這個餐點不在三家店裡
        return 0;
    }
    return idx + 1;
}

/*
  get_price：傳回該餐點的價格若無效餐點回傳 0
*/
int get_price(const char *dishes)
{
    // 1. 找出這道菜屬於哪一家店
    int shop_idx = find_shop_by_food(shops, shops_num, (char *)dishes);
    if (shop_idx == -1) {
        // 表示這道菜不在三家店裡，回傳0表示找不到
        return 0;
    }

    // 2. 在該店裡面，再找出該菜的索引
    Shop current_shop = shops[shop_idx];
    int  count        = get_food_count(current_shop);
    int  food_idx     = find_food_index(current_shop, count, (char *)dishes);

    if (food_idx == -1) {
        // 理論上不太會到這，因為前一步已保證該店有這道菜
        return 0;
    }

    // 3. 回傳 items[food_idx] 的 price
    return current_shop.items[food_idx].price;
}

/*
  get_distance：根據餐廳編號取得距離
  - 1 => Dessert shop 距離 3 km
  - 2 => Beverage shop 距離 5 km
  - 3 => Diner 距離 8 km
  - 其它 => 0
*/
int get_distance(int restaurant)
{
    // restaurant = 1,2,3 => shops[0], shops[1], shops[2]
    if (restaurant >= 1 && restaurant <= 3) {
        return shops[restaurant - 1].distance;
    }
    return 0; // 無效餐廳代號
}

/*
  order_point_to：將一筆外送工作指派給目前剩餘時間較少的外送員
  - delivery_time：此筆外送需要花費的秒數 (例如距離幾 km => 幾秒)
  - 會回傳外送員完成這筆訂單所需等候的總秒數 (累加後)
*/
int order_point_to(int delivery_time)
{
    pthread_mutex_lock(&delivery_mutex);

    // 比較兩名外送員的 current_remain_time
    // 選擇較小者 (表示可以更快處理)
    int selected;
    if (delivery_persons[0].current_remain_time <=
        delivery_persons[1].current_remain_time) {
        selected = 0;
    } else {
        selected = 1;
    }

    // 為該外送員累加新的外送秒數
    int wait_time = delivery_persons[selected].current_remain_time + delivery_time;
    delivery_persons[selected].current_remain_time = wait_time;

    printf("[%d]: %d delivery person assigned, remaining time: %d\n",
           gettid(), selected, wait_time);

    // 解鎖後返回
    pthread_mutex_unlock(&delivery_mutex);

    return wait_time;
}

/*
  update_logic：後台執行緒，用於每秒扣減外送員 current_remain_time
  假設外送員需要 15 秒，則每秒扣 1，直到歸零
*/
// 參考
void *update_logic(void *arg)
{
    (void)arg;

    while (1) {
        sleep(1);
        pthread_mutex_lock(&delivery_mutex);

        for (int i = 0; i < 2; i++) {
            if (delivery_persons[i].current_remain_time > 0) {
                delivery_persons[i].current_remain_time--;
            }
        }

        // 顯示外送員目前剩餘時間
        if (delivery_persons[0].current_remain_time != 0 ||
            delivery_persons[1].current_remain_time != 0) {
            printf("[%d]: Person 1: %d, Person 2: %d\n",
                   gettid(),
                   delivery_persons[0].current_remain_time,
                   delivery_persons[1].current_remain_time);
        }

        pthread_mutex_unlock(&delivery_mutex);
    }

    return NULL;
}

/*
  handle_client：負責處理每個客戶端連線
  透過 recv(...) 讀取客戶端指令 (shop list / order / confirm / cancel ...)
  並根據指令內容回覆相應結果
*/
void *handle_client(void *arg)
{
    int clientSocket = *(int *)arg;
    free(arg);

    int  chosenRest      = 0;
    char orderList[256]  = "\n";
    int  totalCost       = 0;
    int  distanceWait    = 0;
    int  cntCookie       = 0, cntCake    = 0;
    int  cntTea          = 0, cntBoba    = 0;
    int  cntFried        = 0, cntEggSoup = 0;

    // 用來接收指令的緩衝
    char inputBuf[256] = {0};
    int  rBytes        = 0;

    // 不斷接收客戶端指令
    while ((rBytes = recv(clientSocket, inputBuf, 256, 0)) > 0) {
        inputBuf[rBytes] = '\0';
        inputBuf[strcspn(inputBuf, "\r\n")] = 0;

        char debugMsg[512] = {0};
        snprintf(debugMsg, sizeof(debugMsg), "[%d]: %s\n", gettid(), inputBuf);
        printf("%s", debugMsg);

        // 判斷指令種類
        if (!strcmp(inputBuf, "shop list")) {
            // 回傳店家/菜單資訊
            char shop_list[256];
            snprintf(shop_list, sizeof(shop_list),
                     "Dessert shop:3km\n- cookie:$60|cake:$80\n"
                     "Beverage shop:5km\n- tea:$40|boba:$70\n"
                     "Diner:8km\n- fried-rice:$120|Egg-drop-soup:$50\n");
            send(clientSocket, shop_list, strlen(shop_list), 0);
        }
        else if (strncmp(inputBuf, "order", 5) == 0) {
            /*
              解析 order 指令
              格式：order <dishes> <quantity>
            */
            char dishes[30] = {0};
            int  quantity   = 0;

            if (sscanf(inputBuf, "order %s %d", dishes, &quantity) == 2) {
                // 確認餐點是哪家店
                int current_restaurant = get_restaurant(dishes);

                // 若是第一次點餐 => 設定該餐廳為本次訂單
                if (chosenRest == 0) {
                    chosenRest   = current_restaurant;
                    distanceWait = get_distance(chosenRest);
                }
                else if (chosenRest != current_restaurant) {
                    // 無法跨店點餐
                    char msg[256] = "Cannot order from different restaurant\n";
                    send(clientSocket, msg, 256, 0);
                    continue;
                }

                // 根據餐廳更新數量
                if (current_restaurant == 1) {
                    if (strcmp(dishes, "cookie") == 0) {
                        cntCookie += quantity;
                    }
                    else if (strcmp(dishes, "cake") == 0) {
                        cntCake += quantity;
                    }
                }
                else if (current_restaurant == 2) {
                    if (strcmp(dishes, "tea") == 0) {
                        cntTea += quantity;
                    }
                    else if (strcmp(dishes, "boba") == 0) {
                        cntBoba += quantity;
                    }
                }
                else if (current_restaurant == 3) {
                    if (strcmp(dishes, "fried-rice") == 0) {
                        cntFried += quantity;
                    }
                    else if (strcmp(dishes, "Egg-drop-soup") == 0) {
                        cntEggSoup += quantity;
                    }
                }
                else {
                    // 不做任何事或做預設行為
                }

                // 計算價格
                int price = get_price(dishes) * quantity;
                totalCost += price;

                // 更新並回傳當前的點餐清單
                char updatedOrder[256] = {0};
                char tmpVal[64]        = {0};

                if (current_restaurant == 1) {
                    if (cntCookie > 0) {
                        snprintf(tmpVal, sizeof(tmpVal), "cookie %d", cntCookie);
                        strcat(updatedOrder, tmpVal);
                        memset(tmpVal, 0, sizeof(tmpVal));
                    }
                    if (cntCookie > 0 && cntCake > 0) {
                        strcat(updatedOrder, "|");
                    }
                    if (cntCake > 0) {
                        snprintf(tmpVal, sizeof(tmpVal), "cake %d", cntCake);
                        strcat(updatedOrder, tmpVal);
                        memset(tmpVal, 0, sizeof(tmpVal));
                    }
                }
                else if (current_restaurant == 2) {
                    if (cntTea > 0) {
                        snprintf(tmpVal, sizeof(tmpVal), "tea %d", cntTea);
                        strcat(updatedOrder, tmpVal);
                        memset(tmpVal, 0, sizeof(tmpVal));
                    }
                    if (cntTea > 0 && cntBoba > 0) {
                        strcat(updatedOrder, "|");
                    }
                    if (cntBoba > 0) {
                        snprintf(tmpVal, sizeof(tmpVal), "boba %d", cntBoba);
                        strcat(updatedOrder, tmpVal);
                        memset(tmpVal, 0, sizeof(tmpVal));
                    }
                }
                else if (current_restaurant == 3) {
                    if (cntFried > 0) {
                        snprintf(tmpVal, sizeof(tmpVal),
                                 "fried-rice %d", cntFried);
                        strcat(updatedOrder, tmpVal);
                        memset(tmpVal, 0, sizeof(tmpVal));
                    }
                    if (cntFried > 0 && cntEggSoup > 0) {
                        strcat(updatedOrder, "|");
                    }
                    if (cntEggSoup > 0) {
                        snprintf(tmpVal, sizeof(tmpVal),
                                 "Egg-drop-soup %d", cntEggSoup);
                        strcat(updatedOrder, tmpVal);
                        memset(tmpVal, 0, sizeof(tmpVal));
                    }
                }
                else {
                }

                strcat(updatedOrder, "\n");
                strncpy(orderList, updatedOrder, sizeof(orderList) - 1);

                // 傳送目前訂單內容給 client
                send(clientSocket, orderList, 256, 0);
            }
            else {
                // 指令格式錯誤
                char msg[256] = "[Debug]Invalid order command\n";
                send(clientSocket, msg, 256, 0);
            }
        }
        else if (!strcmp(inputBuf, "confirm")) {
            // 確認訂單
            if (totalCost == 0) {
                // 尚未點任何餐
                char msg[256] = "Please order some meals\n";
                send(clientSocket, msg, 256, 0);
            }
            else {
                // 根據餐廳距離加上外送員剩餘時間
                int base_t = distanceWait;
                pthread_mutex_lock(&delivery_mutex);

                int selected;
                if (delivery_persons[0].current_remain_time <=
                    delivery_persons[1].current_remain_time) {
                    selected = 0;
                }
                else {
                    selected = 1;
                }

                int wait_time =
                    delivery_persons[selected].current_remain_time + base_t;
                pthread_mutex_unlock(&delivery_mutex);

                // 若時間超過 30 秒，詢問客戶是否要等
                if (wait_time >= 30) {
                    char prompt[256] =
                        "Your delivery will take a long time, do you want to wait?\n";
                    send(clientSocket, prompt, 256, 0);

                    printf("[%d]: Your delivery will take a long time, do you want to wait?\n",
                           gettid());

                    memset(inputBuf, 0, sizeof(inputBuf));
                    rBytes = recv(clientSocket, inputBuf, 256, 0);
                    if (rBytes <= 0) {
                        printf("[%d]: Connection closed\n", gettid());
                        break;
                    }
                    inputBuf[rBytes] = '\0';
                    inputBuf[strcspn(inputBuf, "\r\n")] = 0;

                    printf("[%d]: %s\n", gettid(), inputBuf);

                    // 若回 No => 取消訂單
                    if (!strcmp(inputBuf, "Yes")) {
                        // do nothing
                    }
                    else if (!strcmp(inputBuf, "No")) {
                        char msg[256] = "Order canceled\n";
                        send(clientSocket, msg, 256, 0);
                        break;
                    }
                    else {
                    }
                }

                // 指派外送，並送出等待提示
                wait_time = order_point_to(base_t);

                char msg[256] = "Please wait a few minutes...\n";
                send(clientSocket, msg, 256, 0);

                printf("[%d]: Please wait a few minutes...\n", gettid());

                // 等待外送時間
                sleep(wait_time);

                // 告知客戶外送抵達，並付款
                char delivery_msg[256];
                snprintf(delivery_msg, sizeof(delivery_msg),
                         "Delivery has arrived and you need to pay %d$\n",
                         totalCost);
                send(clientSocket, delivery_msg, strlen(delivery_msg), 0);

                break;
            }
        }
        else if (!strcmp(inputBuf, "cancel")) {
            // 取消訂單
            char msg[256] = "Order canceled\n";
            send(clientSocket, msg, strlen(msg), 0);
            break;
        }
        else {
            // 無效指令
            char msg[256] = "[Debug]Invalid command\n";
            send(clientSocket, msg, strlen(msg), 0);
        }

        memset(inputBuf, 0, sizeof(inputBuf));
    }

    close(clientSocket);
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    signal(SIGINT, sigint_handler);

    printf("Server running on port %s...\n", argv[1]);
    fflush(stdout);

    // 建立 socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(1);
    }

    int yes = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family      = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port        = htons(atoi(argv[1]));

    if (bind(server_fd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        perror("bind");
        close(server_fd);
        exit(1);
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        close(server_fd);
        exit(1);
    }

    pthread_t tid;
    if (pthread_create(&tid, NULL, update_logic, NULL) != 0) {
        perror("pthread_create");
        close(server_fd);
        exit(1);
    }

    while (1) {
        struct sockaddr_in caddr;
        socklen_t          clen = sizeof(caddr);
        int                c    = accept(server_fd, (struct sockaddr *)&caddr, &clen);

        if (c < 0) {
            perror("accept");
            continue;
        }

        pthread_t cid;
        int      *pc = (int *)malloc(sizeof(int));
        *pc = c;
        if (pthread_create(&cid, NULL, handle_client, pc) != 0) {
            perror("pthread_create");
            free(pc);
            close(c);
            continue;
        }
    }

    close(server_fd);
    return 0;
}

