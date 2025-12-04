# 编译器设置
CC = arm-linux-gnueabihf-gcc
CFLAGS = -Wall -g \
         -I./libs_install/include \
         -I./src/platform -I./src/radio -I./src/protocol -I./src/database -I./src/mqtt
# 库文件设置 (静态编译建议加 -static，但这里为了灵活性演示动态链接)
# 如果要静态编译，在 LDFLAGS 后加 -static，并注意库的顺序
LDFLAGS = -static -L./libs_install/lib \
          -lpaho-mqtt3c -lsqlite3 -lpthread -ldl -lm
# 自动寻找所有 .c 文件
SRCS = src/main.c \
       src/platform/linux-hal.c \
       src/radio/SX1278.c \
       #src/protocol/htdma.c \
       #src/database/db_manager.c \
       #src/mqtt/mqtt_client.c

# 生成目标文件列表 (.c -> .o)
OBJS = $(SRCS:.c=.o)

# 目标程序名
TARGET = gateway_app

# 默认任务
all: $(TARGET)

# 链接
$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)
	@echo "Build Success! -> $(TARGET)"

# 编译每个 .c 文件
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 清理
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
