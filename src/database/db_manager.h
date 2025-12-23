#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include <stdint.h>

// 初始化数据库
int db_init(const char *db_path);

// 插入传感器数据
int db_insert_data(uint8_t dev_id, float temp, uint32_t seq, uint32_t lost);

// 关闭数据库
void db_close(void);

int db_exec(const char *sql);

#endif
