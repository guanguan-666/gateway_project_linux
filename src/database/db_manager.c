#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sqlite3.h"
#include "db_manager.h"

static sqlite3 *g_db = NULL;

// 初始化数据库
// 在 src/database/db_manager.c 中

int db_init(const char *db_path) {
    int rc = sqlite3_open(db_path, &g_db);
    if (rc) {
        printf("[DB] Can't open database: %s\n", sqlite3_errmsg(g_db));
        return -1;
    }
    printf("[DB] Opened database successfully\n");

    // --- 【新增】自动创建表 ---
    const char *sql_create_table = 
        "CREATE TABLE IF NOT EXISTS sensor_data (" \
        "id INTEGER PRIMARY KEY AUTOINCREMENT," \
        "dev_id INTEGER," \
        "temperature REAL," \
        "humidity REAL," \
        "pid_out REAL," \
        "timestamp INTEGER);";

    if (db_exec(sql_create_table) != 0) {
        printf("[DB] Create table failed\n");
        return -1;
    }
    printf("[DB] Table 'sensor_data' ready.\n");
    // -------------------------

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

// 执行通用的 SQL 语句 (无返回结果集)
int db_exec(const char *sql) {
    char *errMsg = NULL;
    if (g_db == NULL) {
        printf("[DB] Error: Database not opened\n");
        return -1;
    }
    
    // 调用 SQLite3 执行接口
    int rc = sqlite3_exec(g_db, sql, 0, 0, &errMsg);
    
    if (rc != SQLITE_OK) {
        printf("[DB] SQL Error: %s\n", errMsg);
        sqlite3_free(errMsg);
        return -1;
    }
    return 0;
}

