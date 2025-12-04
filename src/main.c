#include <stdio.h>
#include <unistd.h>
#include "linux-hal.h"  // 引入我们写的 HAL 头文件
#include <string.h>
#include "SX1278.h"     // 引入官方驱动头文件


// 1. 定义与 STM32 一致的结构体
#pragma pack(1)
typedef struct {
    uint8_t  head;
    uint8_t  dev_id;
    float    temp;
    uint32_t seq;
} LoraPacket_t;
#pragma pack()

// 定义引脚 (请根据你的设备树和接线修改!)
// 如果用软件模拟SPI，这里要填模拟的引脚；如果用硬件SPI，只填普通GPIO
// 假设: Reset -> GPIO4_23 (119), DIO0 -> GPIO4_24 (120)
#define PIN_RESET  119 
#define PIN_DIO0   116 


// 全局统计变量
uint32_t g_last_seq = 0;
uint32_t g_total_recv = 0;
uint32_t g_total_lost = 0;
int      g_is_first_pkt = 1;

void print_stats(LoraPacket_t *pkt, int rssi) {
    // 1. 计算丢包
    if (g_is_first_pkt) {
        g_is_first_pkt = 0;
        // 第一包不计算丢包，直接同步序号
    } else {
        // 如果当前序号 > 上次序号 + 1，说明中间有跳号
        if (pkt->seq > g_last_seq + 1) {
            uint32_t lost_count = pkt->seq - g_last_seq - 1;
            g_total_lost += lost_count;
            printf("\n[WARNING] Lost %d packets! (Last:%d, Curr:%d)\n\n", lost_count, g_last_seq, pkt->seq);
        }
        // 如果当前序号 < 上次序号，说明STM32重启了，重置统计
        else if (pkt->seq < g_last_seq) {
            printf("\n[INFO] Device Reset Detected. Statistics Cleared.\n\n");
            g_total_recv = 0;
            g_total_lost = 0;
        }
    }
    
    g_last_seq = pkt->seq;
    g_total_recv++;

    // 2. 计算丢包率
    float loss_rate = 0.0f;
    uint32_t total_expected = g_total_recv + g_total_lost;
    if (total_expected > 0) {
        loss_rate = (float)g_total_lost / (float)total_expected * 100.0f;
    }

    // 3. 格式化打印 (类似表格)
    // \r 可以让光标回到行首，实现单行刷新（如果不想刷屏的话）
    // 这里使用换行打印，方便你看历史记录
    printf("| Dev: %02d | Seq: %-5d | Temp: %-6.2f C | RSSI: %-4d | Loss: %-3d (%.1f%%) |\n", 
           pkt->dev_id, 
           pkt->seq, 
           pkt->temp, 
           rssi,
           g_total_lost,
           loss_rate);
           
    fflush(stdout); // 强制刷新缓冲区，确保日志立即显示
}


int main() {
    printf("Starting LoRa Gateway (Packet Loss Monitor)...\n");

    SX1278_hw_t sx1278_hw;
    SX1278_t sx1278;

    sx1278_hw.spi_fd = -1;
    sx1278_hw.reset_pin = PIN_RESET;
    sx1278_hw.dio0_pin = PIN_DIO0;
    sx1278.hw = &sx1278_hw;

    // 初始化：注意频率、扩频因子(SF)必须与STM32一致
    // 假设 STM32 是 SF9, BW125K
    SX1278_init(&sx1278, 433000000, SX1278_POWER_17DBM, SX1278_LORA_SF_9,
                SX1278_LORA_BW_125KHZ, SX1278_LORA_CR_4_5, SX1278_LORA_CRC_EN, 10);

    uint8_t version = SX1278_SPIRead(&sx1278, 0x42);
    if (version == 0x12) {
        printf("✅ LoRa Connected. Waiting for packets...\n");
        printf("----------------------------------------------------------------------\n");
        
        // 进入接收模式
        SX1278_LoRaEntryRx(&sx1278, sizeof(LoraPacket_t), 0);

        while(1) {
            if (SX1278_hw_GetDIO0(&sx1278_hw)) {
                // 1. 读取原始数据
                uint8_t rx_buf[64];
                uint8_t len = SX1278_LoRaRxPacket(&sx1278);
                SX1278_read(&sx1278, rx_buf, len);

                // 2. 校验长度和帧头
                if (len == sizeof(LoraPacket_t)) {
                    LoraPacket_t *pkt = (LoraPacket_t*)rx_buf;
                    
                    if (pkt->head == 0xAA) {
                        // 3. 处理并打印统计信息
                        print_stats(pkt, SX1278_RSSI_LoRa(&sx1278));
                    } else {
                        printf("[RX Error] Invalid Header: 0x%02X\n", pkt->head);
                    }
                } else {
                    printf("[RX Error] Size mismatch. Exp: %d, Got: %d\n", sizeof(LoraPacket_t), len);
                }

                // 4. 继续接收
                SX1278_LoRaEntryRx(&sx1278, sizeof(LoraPacket_t), 0); 
            }
            usleep(2000); // 2ms 轮询间隔
        }
    } else {
        printf("❌ LoRa Init Failed! Ver: 0x%02X\n", version);
    }

    return 0;
}
