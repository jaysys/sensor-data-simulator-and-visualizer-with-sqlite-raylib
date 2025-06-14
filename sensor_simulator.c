#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h> 
#include <time.h>
#include <unistd.h>
#include <math.h>

float random_float(float min, float max) {
    return min + ((float)rand() / RAND_MAX) * (max - min);
}

void get_current_timestamp(char *timestamp, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(timestamp, size, "%Y-%m-%d %H:%M:%S", t);
}

int main() {
    sqlite3 *db;
    char *err_msg = 0;
    int rc;
    char sql[256];
    
    // First, ensure the database file exists and is writable
    FILE *f = fopen("sensor_data.db", "a+");
    if (!f) {
        perror("Failed to create/open database file");
        return 1;
    }
    fclose(f);
    
    // Open database with WAL mode for better concurrency
    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
    rc = sqlite3_open_v2("sensor_data.db", &db, flags, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        if (db) sqlite3_close(db);
        return 1;
    }
    
    // Enable WAL mode for better concurrency
    rc = sqlite3_exec(db, "PRAGMA journal_mode=WAL;", 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to set WAL mode: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
    
    rc = sqlite3_exec(db, "PRAGMA synchronous=NORMAL;", 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to set synchronous mode: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
    
    const char *create_table_sql = 
        "CREATE TABLE IF NOT EXISTS sensor_readings ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "timestamp DATETIME NOT NULL,"
        "temperature FLOAT NOT NULL,"
        "humidity FLOAT NOT NULL,"
        "illuminance FLOAT NOT NULL);";
    
    rc = sqlite3_exec(db, create_table_sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }
    
    printf("Starting sensor data simulation...\n");
    printf("Press Ctrl+C to stop\n");
    
    srand(time(NULL));
    
    while (1) {
        char timestamp[20];
        float temperature, humidity, illuminance;
        
        get_current_timestamp(timestamp, sizeof(timestamp));
        
        temperature = 20.0f + random_float(-5.0f, 5.0f);
        humidity = 50.0f + random_float(-10.0f, 10.0f);
        illuminance = 500.0f + random_float(-200.0f, 200.0f);
        
        snprintf(sql, sizeof(sql),
                 "INSERT INTO sensor_readings (timestamp, temperature, humidity, illuminance) "
                 "VALUES ('%s', %.2f, %.2f, %.2f);",
                 timestamp, temperature, humidity, illuminance);
        
        rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", err_msg);
            sqlite3_free(err_msg);
        } else {
            printf("Data recorded: %s - Temp: %.1f°C, Hum: %.1f%%, Lux: %.0f\n", 
                   timestamp, temperature, humidity, illuminance);
        }
        
        sleep(10);
    }
    
    sqlite3_close(db);
    return 0;
}
