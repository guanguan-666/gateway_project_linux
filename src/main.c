/* 必须放在第一行 */
#define _DEFAULT_SOURCE 
#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <signal.h> 
#include "linux-hal.h"
#include "SX1278.h"

#define PIN_RESET  119 
#define PIN_DIO0   116 

volatile int g_running = 1;
uint32_t g_total_recv = 0;
uint32_t g_total_lost = 0;
uint32_t g_last_seq = 0;
int      g_is_first = 1;

void sig_handler(int signo) { if (signo == SIGINT) g_running = 0; }

int main() {
    signal(SIGINT, sig_handler);

    printf("\n=== LoRa Gateway (Manual Parsing Mode) ===\n");
    printf("Expected Protocol: [AA] [ID] [T1 T2 T3 T4] [S1 S2 S3 S4] (10 Bytes)\n");
    printf("------------------------------------------------------------\n");

    SX1278_hw_t sx1278_hw;
    SX1278_t sx1278;

    sx1278_hw.spi_fd = -1;
    sx1278_hw.reset_pin = PIN_RESET;
    sx1278_hw.dio0_pin = PIN_DIO0;
    sx1278.hw = &sx1278_hw;

    SX1278_init(&sx1278, 433000000, SX1278_POWER_17DBM, SX1278_LORA_SF_9,
                SX1278_LORA_BW_125KHZ, SX1278_LORA_CR_4_5, SX1278_LORA_CRC_EN, 10);

    if (SX1278_SPIRead(&sx1278, 0x42) == 0x12) {
        printf("✅ LoRa Init OK. Waiting for packets...\n");
        SX1278_LoRaEntryRx(&sx1278, 10, 0);

        while(g_running) {
            if (SX1278_hw_GetDIO0(&sx1278_hw)) {
                uint8_t buf[64] = {0};
                int len = SX1278_LoRaRxPacket(&sx1278);
                SX1278_read(&sx1278, buf, len);

                // 1. 基础校验
                if (len == 10 && buf[0] == 0xAA) {
                    // --- 手动解析 (与 STM32 发送顺序严格对应) ---
                    // ID: buf[1]
                    // Temp: buf[2]..buf[5]
                    // Seq:  buf[6]..buf[9]
                    
                    float temp;
                    memcpy(&temp, &buf[2], 4); 

                    uint32_t seq = (uint32_t)buf[6] | 
                                   ((uint32_t)buf[7] << 8) | 
                                   ((uint32_t)buf[8] << 16) | 
                                   ((uint32_t)buf[9] << 24);

                    // --- 统计逻辑 ---
                    if (g_is_first) {
                        g_is_first = 0;
                        g_last_seq = seq;
                    } else {
                        // 正常情况：seq 应该比 last_seq 大 1
                        if (seq > g_last_seq + 1) {
                            int lost = seq - g_last_seq - 1;
                            g_total_lost += lost;
                            printf("\n[WARN] Lost %d pkts (Seq %d -> %d)\n", lost, g_last_seq, seq);
                        } 
                        // 重启情况
                        else if (seq < g_last_seq) {
                            printf("\n[INFO] Device Restarted. Stats Reset.\n");
                            g_total_lost = 0;
                            g_total_recv = 0;
                        }
                        g_last_seq = seq;
                    }
                    g_total_recv++;

                    // --- 仪表盘显示 ---
                    float rate = 0;
                    if(g_total_recv + g_total_lost > 0)
                        rate = (float)g_total_lost / (g_total_recv + g_total_lost) * 100.0f;

                    // 打印数据，并在末尾附带原始 HEX 供查错
                    printf("\rSeq:%-5d | Temp:%-6.2f | Loss:%-3d (%.1f%%) | RAW: %02X %02X %02X %02X...  ", 
                           seq, temp, g_total_lost, rate, buf[6], buf[7], buf[8], buf[9]);
                    fflush(stdout);

                } else {
                    // 异常数据打印
                    printf("\n[ERR] Invalid Packet. Len:%d, Head:0x%02X\n", len, buf[0]);
                    printf("      Hex: ");
                    for(int i=0; i<len; i++) printf("%02X ", buf[i]);
                    printf("\n");
                }
                
                SX1278_LoRaEntryRx(&sx1278, 10, 0);
            }
            usleep(2000); 
        }
    } 
    return 0;
}
