# Makefile - 編譯 hw1_writer 和 hw1_driver 並產生可執行檔

# 通用參數
CROSS_COMPILE = aarch64-linux-gnu-
ARCH = arm64
KDIR = /home/stu/linux
PWD  := $(shell pwd)

# Writer 參數
WRITER_SRC = HW1_writer.c
WRITER_BIN = HW1_writer_arm64

# Driver 參數
obj-m += HW1_driver.o

all: driver writer

driver:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

writer: $(WRITER_SRC)
	$(CROSS_COMPILE)gcc -Wall $< -o $(WRITER_BIN)

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean
	rm -f *.o *.ko *.mod.* *.symvers *.order $(WRITER_BIN)

