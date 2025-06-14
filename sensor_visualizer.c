#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <time.h>
#include <math.h>
#include <raylib.h>

#define MAX_READINGS 100
#define WINDOW_WIDTH  1000
#define WINDOW_HEIGHT 800
#define GRAPH_HEIGHT 220
#define GRAPH_MARGIN 20
#define GRAPH_LEFT_MARGIN 30    // Increased to make room for Y-axis labels
#define GRAPH_TOP_MARGIN 80
#define GRAPH_BOTTOM_MARGIN 60  // Increased to make room for time labels
#define TITLE_OFFSET 25
#define TIME_LABEL_OFFSET 25    // Space below graph for time labels
#define Y_LABEL_WIDTH 20        // Width for Y-axis labels

typedef struct {
    double timestamp;
    float temperature;
    float humidity;
    float illuminance;
} SensorReading;

SensorReading readings[MAX_READINGS];
int reading_count = 0;
double last_reading_timestamp = 0;

void load_sensor_data(sqlite3 *db) {
    sqlite3_stmt *stmt;
    const char *sql;
    int rc;
    int new_readings = 0;
    
    // First, get the latest timestamp from the database
    double latest_db_timestamp = 0;
    const char *latest_ts_sql = "SELECT strftime('%s', MAX(timestamp)) FROM sensor_readings;";
    
    rc = sqlite3_prepare_v2(db, latest_ts_sql, -1, &stmt, 0);
    if (rc == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW) {
        latest_db_timestamp = sqlite3_column_double(stmt, 0);
    }
    sqlite3_finalize(stmt);
    
    // If no new data, return early
    if (latest_db_timestamp <= last_reading_timestamp) {
        return;
    }
    
    // If we have previous readings, only fetch new ones
    if (reading_count > 0) {
        sql = "SELECT strftime('%s', timestamp) as ts, temperature, humidity, illuminance "
              "FROM sensor_readings WHERE timestamp > datetime(?, 'unixepoch') "
              "ORDER BY timestamp ASC";
    } else {
        sql = "SELECT strftime('%s', timestamp) as ts, temperature, humidity, illuminance "
              "FROM sensor_readings ORDER BY timestamp DESC LIMIT ?";
    }
    
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return;
    }
    
    if (reading_count > 0) {
        sqlite3_bind_double(stmt, 1, last_reading_timestamp);
    } else {
        sqlite3_bind_int(stmt, 1, MAX_READINGS);
    }
    
    // If we're adding to existing readings, shift old readings to make room for new ones
    if (reading_count > 0) {
        // Remove unused variable
        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            double ts = sqlite3_column_double(stmt, 0);
            if (ts <= last_reading_timestamp) continue;
            
            // If buffer is full, remove oldest reading
            if (reading_count >= MAX_READINGS) {
                for (int i = 1; i < reading_count; i++) {
                    readings[i-1] = readings[i];
                }
                reading_count--;
            }
            
            // Add new reading at the end
            readings[reading_count].timestamp = ts;
            readings[reading_count].temperature = sqlite3_column_double(stmt, 1);
            readings[reading_count].humidity = sqlite3_column_double(stmt, 2);
            readings[reading_count].illuminance = sqlite3_column_double(stmt, 3);
            
            // Log the new reading
            time_t t = (time_t)ts;
            struct tm *timeinfo = localtime(&t);
            char time_str[20];
            strftime(time_str, sizeof(time_str), "%H:%M:%S", timeinfo);
            
            printf("[%s] New reading: %.1f°C, %.1f%%, %.0f lux\n", 
                   time_str,
                   readings[reading_count].temperature,
                   readings[reading_count].humidity,
                   readings[reading_count].illuminance);
            
            reading_count++;
            new_readings++;
        }
    } else {
        // Initial load
        reading_count = 0;
        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW && reading_count < MAX_READINGS) {
            readings[reading_count].timestamp = sqlite3_column_double(stmt, 0);
            readings[reading_count].temperature = sqlite3_column_double(stmt, 1);
            readings[reading_count].humidity = sqlite3_column_double(stmt, 2);
            readings[reading_count].illuminance = sqlite3_column_double(stmt, 3);
            reading_count++;
        }
        printf("Initial load: %d readings.\n", reading_count);
    }
    
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Error during query execution: %s\n", sqlite3_errmsg(db));
    } else if (new_readings > 0) {
        printf("Added %d new readings. Total: %d\n", new_readings, reading_count);
    }
    
    // Update the last known timestamp
    if (reading_count > 0) {
        last_reading_timestamp = readings[reading_count-1].timestamp;
    }
    
    sqlite3_finalize(stmt);
}

void draw_graph(float *values, int count, int graph_index, float min_val, float max_val, Color color, const char* title) {
    // Calculate graph position
    float graph_x = GRAPH_LEFT_MARGIN + Y_LABEL_WIDTH;  // Add space for Y labels
    float graph_y = GRAPH_TOP_MARGIN + graph_index * (GRAPH_HEIGHT + GRAPH_MARGIN);
    float graph_width = WINDOW_WIDTH - GRAPH_LEFT_MARGIN - GRAPH_MARGIN - Y_LABEL_WIDTH;
    float graph_height = GRAPH_HEIGHT - GRAPH_BOTTOM_MARGIN;
    
    // Draw title above the graph box (left-aligned and using graph line color)
    DrawText(title, graph_x + 5, graph_y - TITLE_OFFSET, 16, color);
    
    // Draw background
    DrawRectangle(graph_x, graph_y, graph_width, graph_height, (Color){ 240, 240, 240, 255 });
    
    // Draw border
    DrawRectangleLines(graph_x, graph_y, graph_width, graph_height, LIGHTGRAY);
    
    if (count < 2) {
        DrawText("Not enough data points", graph_x + 20, graph_y + 40, 14, GRAY);
        return;
    }
    
    // Calculate scales
    float x_scale = (graph_width - 20) / (count - 1);
    float y_scale = (graph_height - 20) / (max_val - min_val);
    
    // Draw grid lines and Y-axis labels
    for (int i = 0; i <= 5; i++) {
        float value = min_val + (max_val - min_val) * (1.0f - (float)i / 5);
        float y = graph_y + 10 + (graph_height - 30) * (i / 5.0f);
        
        // Grid line
        DrawLine(graph_x + 10, y, graph_x + graph_width - 10, y, Fade(LIGHTGRAY, 0.5f));
        
        // Y-axis label - moved to left of graph
        char value_text[16];
        snprintf(value_text, sizeof(value_text), "%.1f", value);
        int text_width = MeasureText(value_text, 12);
        DrawText(value_text, GRAPH_LEFT_MARGIN + Y_LABEL_WIDTH - text_width - 5, y - 8, 12, DARKGRAY);
    }
    
    // Draw X-axis labels (time)
    if (count > 1) {
        // X-axis label (time) - moved below graph, adjusted to KST (+9 hours)
        time_t first_time = (time_t)readings[0].timestamp + (9 * 3600);  // Add 9 hours for KST
        struct tm *tm_info = gmtime(&first_time);  // Use gmtime since we've already adjusted the time
        char time_buf[20];
        strftime(time_buf, sizeof(time_buf), "%H:%M:%S", tm_info);
        DrawText(time_buf, graph_x + 5, graph_y + graph_height + 5, 12, DARKGRAY);
        
        // Last point time
        time_t last_time = (time_t)readings[count-1].timestamp + (9 * 3600);  // Add 9 hours for KST
        tm_info = gmtime(&last_time);
        strftime(time_buf, sizeof(time_buf), "%H:%M:%S", tm_info);
        int text_width = MeasureText(time_buf, 12);
        DrawText(time_buf, graph_x + graph_width - text_width - 5, graph_y + graph_height + 5, 12, DARKGRAY);
        
        // Optional: Add a middle time point for better reference
        if (count > 2) {
            int mid = count / 2;
            time_t mid_time = (time_t)readings[mid].timestamp + (9 * 3600);  // Add 9 hours for KST
            tm_info = gmtime(&mid_time);
            strftime(time_buf, sizeof(time_buf), "%H:%M:%S", tm_info);
            text_width = MeasureText(time_buf, 12);
            DrawText(time_buf, graph_x + (graph_width - text_width) / 2, graph_y + graph_height + 5, 12, DARKGRAY);
        }
    }
    
    // Draw graph line
    for (int i = 0; i < count - 1; i++) {
        float x1 = graph_x + 10 + i * x_scale;
        float y1 = graph_y + graph_height - 10 - (values[i] - min_val) * y_scale;
        float x2 = graph_x + 10 + (i + 1) * x_scale;
        float y2 = graph_y + graph_height - 10 - (values[i + 1] - min_val) * y_scale;
        
        // Clamp y values to graph bounds
        y1 = (y1 < graph_y + 10) ? graph_y + 10 : (y1 > graph_y + graph_height - 10) ? graph_y + graph_height - 10 : y1;
        y2 = (y2 < graph_y + 10) ? graph_y + 10 : (y2 > graph_y + graph_height - 10) ? graph_y + graph_height - 10 : y2;
        
        // Draw line segment
        DrawLineEx((Vector2){x1, y1}, (Vector2){x2, y2}, 2.0f, color);
        
        // Draw data point
        DrawCircle(x1, y1, 2.0f, color);
    }
    
    // Draw the last point
    if (count > 0) {
        float x = graph_x + 10 + (count - 1) * x_scale;
        float y = graph_y + graph_height - 10 - (values[count - 1] - min_val) * y_scale;
        y = (y < graph_y + 10) ? graph_y + 10 : (y > graph_y + graph_height - 10) ? graph_y + graph_height - 10 : y;
        DrawCircle(x, y, 2.0f, color);
    }
}

int main() {
    sqlite3 *db;
    // Try to open the database
    printf("Attempting to open database...\n");
    int flags = SQLITE_OPEN_READWRITE;
    int rc = sqlite3_open_v2("sensor_data.db", &db, flags, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        if (db) sqlite3_close(db);
        return 1;
    }
    printf("Database opened successfully.\n");
    
    // Check if the table exists
    sqlite3_stmt *stmt;
    const char *check_table = "SELECT name FROM sqlite_master WHERE type='table' AND name='sensor_readings';";
    rc = sqlite3_prepare_v2(db, check_table, -1, &stmt, 0);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare table check statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }
    
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        fprintf(stderr, "sensor_readings table not found. Please run the simulator first.\n");
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }
    sqlite3_finalize(stmt);
    printf("sensor_readings table found.\n");
    
    // Count the number of rows
    const char *count_sql = "SELECT COUNT(*) FROM sensor_readings;";
    rc = sqlite3_prepare_v2(db, count_sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to count rows: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int row_count = sqlite3_column_int(stmt, 0);
        printf("Found %d rows in sensor_readings table.\n", row_count);
    }
    sqlite3_finalize(stmt);
    
    // Enable WAL mode for better concurrency
    char *err_msg = 0;
    rc = sqlite3_exec(db, "PRAGMA journal_mode=WAL;", 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to set WAL mode: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
    
    // Initialize window
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Sensor Data Visualizer");
    SetTargetFPS(60);
    
    // Find min/max values for scaling
    float min_temp = 15, max_temp = 35;      // Typical temperature range
    float min_humidity = 20, max_humidity = 80;  // Typical humidity range
    float min_lux = 0, max_lux = 1000;       // Typical illuminance range
    
    // Load initial data
    load_sensor_data(db);
    
    // Main game loop
    while (!WindowShouldClose()) {
        // Reload data every second
        static double lastUpdate = 0;
        double currentTime = GetTime();
        if (currentTime - lastUpdate >= 1.0) {
            load_sensor_data(db);
            lastUpdate = currentTime;
        }
        
        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        // Draw temperature graph (graph_index = 0)
        if (reading_count > 0) {
            float temp_values[MAX_READINGS];
            for (int i = 0; i < reading_count; i++) {
                temp_values[i] = readings[i].temperature;
            }
            draw_graph(temp_values, reading_count, 0, min_temp, max_temp, RED, "Temperature (°C)");
        }
        
        // Draw humidity graph (graph_index = 1)
        if (reading_count > 0) {
            float hum_values[MAX_READINGS];
            for (int i = 0; i < reading_count; i++) {
                hum_values[i] = readings[i].humidity;
            }
            draw_graph(hum_values, reading_count, 1, min_humidity, max_humidity, BLUE, "Humidity (%)");
        }
        
        // Draw illuminance graph (graph_index = 2)
        if (reading_count > 0) {
            float lux_values[MAX_READINGS];
            for (int i = 0; i < reading_count; i++) {
                lux_values[i] = readings[i].illuminance;
            }
            draw_graph(lux_values, reading_count, 2, min_lux, max_lux, DARKGREEN, "Illuminance (lux)");
        }
        
        // Draw FPS in top-right corner
        DrawFPS(WINDOW_WIDTH - 100, 10);
        
        // Draw current values
        if (reading_count > 0) {
            char text[128];
            int latest = reading_count - 1;
            
            sprintf(text, "Latest: Temp: %.1f°C, Hum: %.1f%%, Lux: %.0f",
                   readings[latest].temperature, 
                   readings[latest].humidity, 
                   readings[latest].illuminance);
            DrawText(text, 10, 10, 18, DARKGRAY);
        }
        
        EndDrawing();
    }
    
    CloseWindow();
    sqlite3_close(db);
    
    return 0;
}
