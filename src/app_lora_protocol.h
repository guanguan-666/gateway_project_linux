#ifndef __APP_LORA_PROTOCOL_H__
#define __APP_LORA_PROTOCOL_H__

#include <stdint.h>

// --- 网络配置 ---
#define LORA_NET_ID         0x88        // 网络ID
#define LORA_BEACON_HEAD    0xBE        // 信标帧头
#define LORA_DATA_HEAD      0xDA        // 数据帧头

// --- H-TDMA 时序配置 (单位: ms) ---
#define TDMA_CYCLE_TIME     10000       // 总周期 10秒
#define CAP_PERIOD          2000        // 竞争期 2秒
#define SLOT_TIME           1000        // 时隙 1秒

// --- 数据包结构 ---
#pragma pack(1)

// 1. 网关广播的信标帧
typedef struct {
    uint8_t head;           // 0xBE
    uint8_t net_id;         // 网络ID
    uint32_t timestamp;     // 时间戳
    uint8_t padding[2];     // 补齐
} Beacon_Frame_t;

// 2. 节点上传的数据帧
typedef struct {
    uint8_t head;           // 0xDA
    uint8_t dev_id;         // 设备ID
    uint8_t type;           // 数据类型
    float   temperature;    // 温度
    float   humidity;       // 湿度
    float   pid_output;     // PID值
    uint8_t checksum;       // 校验和
} Node_Data_Frame_t;

#pragma pack()

#endif
