#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sqlite3.h"
#include "db_manager.h"

static sqlite3 *g_db = NULL;

// 初始化数据库
int db_init(const char *db_path) {
    int rc = sqlite3_open(db_path, &g_db);
    if (rc) {
        fprintf(stderr, "[DB] Can't open database: %s\n", sqlite3_errmsg(g_db));
        return -1;
    } else {
        printf("[DB] Opened database successfully\n");
    }

    // 创建表 SQL 语句 (如果表不存在则创建)
    // 包含字段: ID (主键), DeviceID, Temperature, Sequence, LostCount, Timestamp
    char *sql = "CREATE TABLE IF NOT EXISTS sensor_log (" \
                "id INTEGER PRIMARY KEY AUTOINCREMENT," \
                "device_id INTEGER NOT NULL," \
                "temperature REAL NOT NULL," \
                "sequence INTEGER NOT NULL," \
                "lost_count INTEGER NOT NULL," \
                "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP);";

    char *zErrMsg = 0;
    rc = sqlite3_exec(g_db, sql, 0, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[DB] SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return -1;
    } else {
        printf("[DB] Table created/checked successfully\n");
    }
    return 0;
}

// 插入数据
int db_insert_data(uint8_t dev_id, float temp, uint32_t seq, uint32_t lost) {
    if (g_db == NULL) return -1;

    char sql[256];
    char *zErrMsg = 0;

    // 格式化插入语句
    snprintf(sql, sizeof(sql), 
             "INSERT INTO sensor_log (device_id, temperature, sequence, lost_count) "
             "VALUES (%d, %.2f, %u, %u);", 
             dev_id, temp, seq, lost);

    int rc = sqlite3_exec(g_db, sql, 0, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[DB] Insert error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return -1;
    }
    // printf("[DB] Insert OK\n"); // 调试时可开启
    return 0;
}

// 关闭数据库
void db_close(void) {
    if (g_db) {
        sqlite3_close(g_db);
        printf("[DB] Database closed\n");
        g_db = NULL;
    }
}
