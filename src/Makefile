# 1. 指定编译器
CC = arm-linux-gnueabihf-gcc

# 2. 定义生成的程序名字
TARGET = gateway_app

# 3. 定义库的安装路径 (方便后面引用)
LIB_PATH = /home/xiaoguan/gateway_project/libs_install

# 4. 指定头文件路径 (-I)
INCLUDES = -I$(LIB_PATH)/include

# 5. 指定库文件路径和链接库 (-L, -l)
# 注意：静态编译需要 -static，且 -ldl -lpthread 必须放在最后
LIBS = -static -L$(LIB_PATH)/lib \
       -lpaho-mqtt3c -lsqlite3 -lpthread -ldl -lm

# 6. 编译规则
all:
	$(CC) main.c $(INCLUDES) $(LIBS) -o $(TARGET)
	@echo "Build Success! Generated: $(TARGET)"

clean:
	rm -f $(TARGET)
