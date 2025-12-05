# 定義編譯器和編譯選項
CC      = gcc
CFLAGS  = -pthread

# 定義目標檔案
TARGET_SERVER = server
TARGET_CLIENT = client

# 定義源碼檔案
SRC_SERVER = server.c
SRC_CLIENT = client.c

# 定義中間文件
OBJ_SERVER = server.o
OBJ_CLIENT = client.o

# 預設目標：編譯所有
all: $(TARGET_SERVER) $(TARGET_CLIENT)

# 編譯 server
$(TARGET_SERVER): $(OBJ_SERVER)
	$(CC) $(CFLAGS) -o $(TARGET_SERVER) $(OBJ_SERVER)

# 編譯 client
$(TARGET_CLIENT): $(OBJ_CLIENT)
	$(CC) $(CFLAGS) -o $(TARGET_CLIENT) $(OBJ_CLIENT)

# 編譯 server 的中間文件
$(OBJ_SERVER): $(SRC_SERVER)
	$(CC) $(CFLAGS) -c $(SRC_SERVER)

# 編譯 client 的中間文件
$(OBJ_CLIENT): $(SRC_CLIENT)
	$(CC) $(CFLAGS) -c $(SRC_CLIENT)

# 清除中間文件和可執行文件
clean:
	rm -f $(OBJ_SERVER) $(OBJ_CLIENT) $(TARGET_SERVER) $(TARGET_CLIENT)

.PHONY: all clean

