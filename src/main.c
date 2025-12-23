#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include "platform/linux-hal.h"
#include "radio/SX1278.h"
#include "database/db_manager.h"  // 确保你有这个文件，用于存数据库
#include "app_lora_protocol.h"

// --- 全局变量 ---
SX1278_hw_t sx1278_hw;
SX1278_t sx1278;

// 获取当前毫秒时间
uint32_t Get_Current_Time_Ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

// 处理接收到的数据
void Process_LoRa_Packet(uint8_t *buffer, int len) {
    if (len != sizeof(Node_Data_Frame_t)) return;

    Node_Data_Frame_t *data = (Node_Data_Frame_t *)buffer;

    // 1. 校验头和数据完整性
    if (data->head != LORA_DATA_HEAD) return;
    
    // 简单校验和检查 (可选)
    uint8_t sum = 0;
    uint8_t *p = (uint8_t*)data;
    for(int i=0; i<sizeof(Node_Data_Frame_t)-1; i++) {
        sum += p[i];
    }
    if (sum != data->checksum) {
        printf("[Warn] Checksum Error from ID %d\n", data->dev_id);
        return;
    }

    // 2. 打印接收到的数据 (这就是你要截图放到论文里的效果！)
    printf("\n[RX] 收到节点数据:\n");
    printf("   + 设备 ID: %d\n", data->dev_id);
    printf("   + 温    度: %.2f C\n", data->temperature);
    printf("   + 湿    度: %.2f %%\n", data->humidity);
    printf("   + PID 输出: %.2f\n", data->pid_output);

    // 3. 存入 SQLite 数据库
    // 假设你的 db_manager.c 里有这个函数，如果没有，需要去补充
    char sql[256];
    sprintf(sql, "INSERT INTO sensor_data (dev_id, temperature, humidity, pid_out, timestamp) VALUES (%d, %.2f, %.2f, %.2f, %ld);",
            data->dev_id, data->temperature, data->humidity, data->pid_output, time(NULL));
    
    if (db_exec(sql) == 0) {
        printf("   -> 数据已入库 (SQLite)\n");
    } else {
        printf("   -> [Error] 入库失败\n");
    }
}

int main(int argc, char *argv[]) {
    printf("==========================================\n");
    printf("   船舶冷箱 LoRa H-TDMA 网关启动 \n");
    printf("==========================================\n");

    // 1. 初始化数据库
    if (db_init("reefer_data.db") != 0) {
        printf("Error: DB Init failed\n");
        // return -1; // 暂时不退出，方便调试 LoRa
    }

    // 2. 初始化 LoRa
    // 注意：Linux 下 SPI 设备通常是 /dev/spidevX.X，在 SX1278_hw.c 里配置
    SX1278_hw_init(&sx1278_hw); 
SX1278_init(&sx1278, 433000000, SX1278_POWER_17DBM, 
                SX1278_LORA_SF_7, SX1278_LORA_BW_125KHZ, SX1278_LORA_CR_4_5, 
                SX1278_LORA_CRC_EN, 10);

    printf("[System] LoRa Init OK. Waiting for cycle...\n");

    uint32_t cycle_start = 0;
    uint8_t rx_buf[128];
    int rx_len;

    // --- 主循环 (TDMA 调度器) ---
    while (1) {
        cycle_start = Get_Current_Time_Ms();

        // ------------------------------------
        // 阶段 1: 广播 Beacon (同步所有节点)
        // ------------------------------------
        printf("\n[TDMA] >>> 新周期开始 (广播 Beacon) <<<\n");
        
        Beacon_Frame_t beacon;
        memset(&beacon, 0, sizeof(beacon));
        beacon.head = LORA_BEACON_HEAD;
        beacon.net_id = LORA_NET_ID;
        beacon.timestamp = (uint32_t)time(NULL); 

        SX1278_LoRaEntryTx(&sx1278, sizeof(beacon), 2000);
        SX1278_LoRaTxPacket(&sx1278, (uint8_t *)&beacon, sizeof(beacon), 2000);
        
        // ------------------------------------
        // 阶段 2: 接收窗口 (收集数据)
        // ------------------------------------
        printf("[TDMA] 进入接收模式 (持续 %d ms)...\n", TDMA_CYCLE_TIME);
        
        // 切换到接收
        SX1278_LoRaEntryRx(&sx1278, 16, 2000);

        // 在周期剩余时间内一直接收
        while (Get_Current_Time_Ms() - cycle_start < TDMA_CYCLE_TIME) {
            
            // 轮询检查是否有数据
            rx_len = SX1278_LoRaRxPacket(&sx1278);
            
            if (rx_len > 0) {
                SX1278_read(&sx1278, rx_buf, rx_len);
                Process_LoRa_Packet(rx_buf, rx_len);
                
                // 收完一个包，必须重新进入接收模式，准备收下一个
                SX1278_LoRaEntryRx(&sx1278, 16, 2000);
            }

            usleep(5000); // 休息 5ms，避免 CPU 100%
        }
    }

    return 0;
}
