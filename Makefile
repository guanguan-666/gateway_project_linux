# 1. 编译器设置
CC = arm-linux-gnueabihf-gcc
CFLAGS = -Wall -g \
         -I./libs_install/include \
         -I./src/platform -I./src/radio -I./src/protocol -I./src/database -I./src/mqtt

# 2. 库文件设置 
# -static: 静态编译
# 库链接顺序非常重要：依赖方在前，被依赖方在后
# gateway_app -> libsqlite3 -> libpthread/libdl
LDFLAGS = -static -L./libs_install/lib \
          -lpaho-mqtt3c -lsqlite3 -lpthread -ldl -lm

# 3. 源文件列表
# 注意：最后一行不要加反斜杠 "\"
SRCS = src/main.c \
       src/platform/linux-hal.c \
       src/radio/SX1278.c \
       src/database/db_manager.c

# 暂时不用的文件（请放在变量定义之外，避免续行符错误）
# src/mqtt/mqtt_client.c
# src/protocol/htdma.c

# 4. 生成目标文件列表 (.c -> .o)
OBJS = $(SRCS:.c=.o)

# 5. 目标程序名
TARGET = gateway_app

# 6. 默认任务
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
