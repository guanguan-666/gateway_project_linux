#include <stdio.h>
#include <unistd.h>
#include "linux-hal.h"  // 引入我们写的 HAL 头文件
#include <string.h>
#include "SX1278.h"     // 引入官方驱动头文件

// 定义引脚 (请根据你的设备树和接线修改!)
// 如果用软件模拟SPI，这里要填模拟的引脚；如果用硬件SPI，只填普通GPIO
// 假设: Reset -> GPIO4_23 (119), DIO0 -> GPIO4_24 (120)
#define PIN_RESET  119 
#define PIN_DIO0   116 

int main() {
    printf("Starting LoRa Gateway...\n");

    // 1. 定义驱动结构体
    SX1278_hw_t sx1278_hw;
    SX1278_t sx1278;

    // 2. 填充硬件信息
    sx1278_hw.spi_fd = -1;       // 初始状态，由 init 函数去打开
    sx1278_hw.reset_pin = PIN_RESET;
    sx1278_hw.dio0_pin = PIN_DIO0;
    
    // 3. 绑定 HAL
    sx1278.hw = &sx1278_hw;

    // 4. 初始化 LoRa 模块
    // 参数: 频率433MHz, 功率17dBm, SF7, BW125k, CR4/5, CRC开启, 包长10
    printf("Initializing SX1278...\n");
    SX1278_init(&sx1278, 433000000, SX1278_POWER_17DBM, SX1278_LORA_SF_9,
                SX1278_LORA_BW_125KHZ, SX1278_LORA_CR_4_5, SX1278_LORA_CRC_EN, 10);

    // 5. 验证是否初始化成功 (再次读取版本号)
    // 注意：这里我们直接调用驱动提供的 Read 函数，验证移植层是否通了
    uint8_t version = SX1278_SPIRead(&sx1278, 0x42);
    printf("LoRa Module Version: 0x%02X\n", version);

    if (version == 0x12) {
        printf("✅ Driver Porting SUCCESS!\n");
        
        // 进入接收模式
        SX1278_LoRaEntryRx(&sx1278, 10, 2000);
        printf("Entered RX Mode. Waiting for packets...\n");
        
        while(1) {
// 检查是否收到数据
    if (SX1278_hw_GetDIO0(&sx1278_hw)) {
        printf("Packet Received!\n");
        
        // 1. 读取数据
        uint8_t rx_buf[255];
        uint8_t len = SX1278_LoRaRxPacket(&sx1278);
        SX1278_read(&sx1278, rx_buf, len);
        printf("Data: %s, RSSI: %d\n", rx_buf, SX1278_RSSI_LoRa(&sx1278));

        // 2. 发送回复 (Ping-Pong 测试)
	printf("Sending Reply...\n");
        char *reply_str = "Ack from Gateway";
        
        // 发送数据 (超时时间 1000ms)
        SX1278_transmit(&sx1278, (uint8_t *)reply_str, (uint8_t)strlen(reply_str), 1000);
        
        // 3. 发送完后，必须切回接收模式，否则无法接收下一条
        // 注意：修改了驱动后，这里的 timeout 参数不再起阻塞作用，给 0 即可
        SX1278_LoRaEntryRx(&sx1278, 10, 0); 
        printf("Back to RX Mode.\n");
    }
    usleep(10000); // 10ms
        }
    } else {
        printf("❌ Driver Porting FAILED. Check SPI/GPIO.\n");
    }

    return 0;
}
