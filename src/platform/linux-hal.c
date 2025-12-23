#include "SX1278_hw.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <errno.h>

// --- 请务必确认设备路径 ---
#define SPI_DEVICE_PATH  "/dev/spidev0.0"  

static int spi_fd = -1;

// --- 硬件初始化 ---
void SX1278_hw_init(SX1278_hw_t *hw) {
    uint8_t mode = 0;
    uint8_t bits = 8;
    uint32_t speed = 500000; // 500KHz

    printf("[HAL] Opening SPI device: %s ...\n", SPI_DEVICE_PATH);

    if ((spi_fd = open(SPI_DEVICE_PATH, O_RDWR)) < 0) {
        perror("[HAL] Error opening SPI Device");
        exit(1);
    }

    // 配置 SPI
    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) < 0) {
        perror("[HAL] Error setting SPI mode");
        exit(1);
    }
    if (ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) {
        perror("[HAL] Error setting SPI bits");
        exit(1);
    }
    if (ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        perror("[HAL] Error setting SPI speed");
        exit(1);
    }

    printf("[HAL] SPI Initialized successfully (FD=%d)\n", spi_fd);
}

// --- 核心 SPI 传输函数 (修正参数类型) ---
// 错误修正：最后一个参数由 uint16_t 改为 int，以匹配头文件
void SX1278_hw_SPITransfer(SX1278_hw_t *hw, uint8_t *tx_buf, uint8_t *rx_buf, int length) {
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx_buf,
        .rx_buf = (unsigned long)rx_buf,
        .len = (uint32_t)length, // 强转一下给驱动用
        .delay_usecs = 0,
        .speed_hz = 500000,
        .bits_per_word = 8,
    };

    if (spi_fd < 0) return;

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
        // perror("[HAL] SPI Transfer Failed");
    }
}

// --- 硬件复位 ---
void SX1278_hw_Reset(SX1278_hw_t *hw) {
    usleep(100000); 
}

// --- 片选控制 ---
void SX1278_hw_SetNSS(SX1278_hw_t *hw, int value) {
    // Linux spidev 驱动自动控制 CS
}

// --- 毫秒延时 ---
// 保持之前的修正：参数为 uint32_t，无 hw 指针
void SX1278_hw_DelayMs(uint32_t ms) {
    usleep(ms * 1000);
}

// --- 读取 DIO0 状态 ---
// ----------------------------------------------------------
// 【超级补丁】通过 SPI 轮询寄存器，代替物理引脚检测
// ----------------------------------------------------------
int SX1278_hw_GetDIO0(SX1278_hw_t *hw) {
    if (spi_fd < 0) return 0;

    // 直接操作 SPI 读取 RegIrqFlags (地址 0x12)
    // 读寄存器时，地址位最高位需为 0
    uint8_t tx_buf[2] = { 0x12 & 0x7F, 0x00 }; 
    uint8_t rx_buf[2] = { 0 };
    
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx_buf,
        .rx_buf = (unsigned long)rx_buf,
        .len = 2,
        .delay_usecs = 0,
        .speed_hz = 500000,
        .bits_per_word = 8,
    };
    
    // 发起传输
    ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
    
    // rx_buf[1] 是读回来的寄存器值
    // Bit 6 = RxDone (接收完成)
    // Bit 3 = TxDone (发送完成)
    // 只要有任意一个置位，就告诉驱动“有情况”！
    if (rx_buf[1] & 0x48) {
        return 1;
    }
    return 0;
}

// --- 兼容旧接口 ---
void SX1278_hw_SPICommand(SX1278_hw_t *hw, uint8_t cmd) {
    uint8_t rx; 
    SX1278_hw_SPITransfer(hw, &cmd, &rx, 1);
}

uint8_t SX1278_hw_SPIReadByte(SX1278_hw_t *hw) {
    uint8_t tx = 0x00;
    uint8_t rx = 0x00;
    SX1278_hw_SPITransfer(hw, &tx, &rx, 1);
    return rx;
}

void SX1278_hw_SPIWriteByte(SX1278_hw_t *hw, uint8_t val) {
    uint8_t rx; 
    SX1278_hw_SPITransfer(hw, &val, &rx, 1);
}
