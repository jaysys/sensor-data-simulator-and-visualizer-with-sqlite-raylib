#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <time.h>
#include <math.h>
#include <gsl/gsl_statistics_float.h>
#include <gsl/gsl_fit.h>
#include <gsl/gsl_sort.h>
#include <gsl/gsl_math.h>
#include <raylib.h>

// Configuration
#define MAX_READINGS 500
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 900
#define GRAPH_HEIGHT 250
#define GRAPH_MARGIN 30
#define GRAPH_LEFT_MARGIN 30
#define GRAPH_TOP_MARGIN 80
#define GRAPH_BOTTOM_MARGIN 60
#define TITLE_OFFSET 25
#define Y_LABEL_WIDTH 20
#define MOVING_AVG_WINDOW 7
#define TREND_POLY_DEGREE 2

typedef struct {
    double timestamp;
    float temperature;
    float humidity;
    float illuminance;
} SensorReading;

// Global variables
SensorReading readings[MAX_READINGS];
int reading_count = 0;
double last_reading_timestamp = 0;
sqlite3 *db = NULL;

// Function prototypes
void load_sensor_data();
void draw_graph(const char* title, float *values, int count, int graph_index, 
                float min_val, float max_val, Color color);
void draw_statistics(float x, float y, float mean, float median, float sd, float min, float max, Color color);

int main() {
    // Initialize window
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Sensor Data Visualizer with GSL Analysis");
    SetTargetFPS(30);

    // Open database
    int rc = sqlite3_open_v2("sensor_data.db", &db, SQLITE_OPEN_READONLY | SQLITE_OPEN_FULLMUTEX, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    // Main game loop
    while (!WindowShouldClose()) {
        // Update data
        load_sensor_data();
        
        // Begin drawing
        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        // Draw title
        DrawText("SENSOR DATA VISUALIZATION WITH GSL ANALYSIS", 
                WINDOW_WIDTH/2 - MeasureText("SENSOR DATA VISUALIZATION WITH GSL ANALYSIS", 24)/2, 
                20, 24, DARKGRAY);
        
        // Draw graphs if we have data
        if (reading_count > 1) {
            // Prepare data arrays
            float temps[MAX_READINGS], hums[MAX_READINGS], lums[MAX_READINGS];
            float min_temp = 1000, max_temp = -1000;
            float min_hum = 1000, max_hum = -1000;
            float min_lum = 1000000, max_lum = -1;
            
            for (int i = 0; i < reading_count; i++) {
                temps[i] = readings[i].temperature;
                hums[i] = readings[i].humidity;
                lums[i] = readings[i].illuminance;
                
                if (temps[i] < min_temp) min_temp = temps[i];
                if (temps[i] > max_temp) max_temp = temps[i];
                if (hums[i] < min_hum) min_hum = hums[i];
                if (hums[i] > max_hum) max_hum = hums[i];
                if (lums[i] < min_lum) min_lum = lums[i];
                if (lums[i] > max_lum) max_lum = lums[i];
            }
            
            // Add some padding to the Y-axis
            float y_padding = (max_temp - min_temp) * 0.1;
            min_temp -= y_padding;
            max_temp += y_padding;
            
            y_padding = (max_hum - min_hum) * 0.1;
            min_hum = fmax(0, min_hum - y_padding);
            max_hum = fmin(100, max_hum + y_padding);
            
            y_padding = (max_lum - min_lum) * 0.1;
            min_lum = fmax(0, min_lum - y_padding);
            max_lum *= 1.1;
            
            // Draw graphs
            draw_graph("Temperature (°C)", temps, reading_count, 0, min_temp, max_temp, RED);
            draw_graph("Humidity (%)", hums, reading_count, 1, min_hum, max_hum, BLUE);
            draw_graph("Illuminance (lux)", lums, reading_count, 2, min_lum, max_lum, DARKGREEN);
        } else {
            DrawText("Waiting for sensor data...", WINDOW_WIDTH/2 - 100, WINDOW_HEIGHT/2, 20, GRAY);
        }
        
        // Draw FPS
        DrawFPS(10, 10);
        
        EndDrawing();
    }
    
    // Cleanup
    sqlite3_close(db);
    CloseWindow();
    return 0;
}

void load_sensor_data() {
    sqlite3_stmt *stmt;
    const char *sql;
    int rc;
    
    // Get the latest timestamp from the database
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
    
    // Prepare SQL query
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
    
    // Process results
    int new_readings = 0;
    if (reading_count > 0) {
        // Append new readings
        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW && reading_count < MAX_READINGS) {
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
            
            reading_count++;
            new_readings++;
            last_reading_timestamp = ts;
        }
    } else {
        // Initial load
        reading_count = 0;
        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW && reading_count < MAX_READINGS) {
            readings[reading_count].timestamp = sqlite3_column_double(stmt, 0);
            readings[reading_count].temperature = sqlite3_column_double(stmt, 1);
            readings[reading_count].humidity = sqlite3_column_double(stmt, 2);
            readings[reading_count].illuminance = sqlite3_column_double(stmt, 3);
            
            if (readings[reading_count].timestamp > last_reading_timestamp) {
                last_reading_timestamp = readings[reading_count].timestamp;
            }
            
            reading_count++;
        }
        
        // If we loaded data in reverse order (DESC), reverse the array
        if (reading_count > 0) {
            for (int i = 0; i < reading_count / 2; i++) {
                SensorReading temp = readings[i];
                readings[i] = readings[reading_count - 1 - i];
                readings[reading_count - 1 - i] = temp;
            }
        }
        
        printf("Initial load: %d readings.\n", reading_count);
    }
    
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Error during query execution: %s\n", sqlite3_errmsg(db));
    } else if (new_readings > 0) {
        printf("Added %d new readings. Total: %d\n", new_readings, reading_count);
    }
    
    sqlite3_finalize(stmt);
}

void draw_statistics(float x, float y, float mean, float median, float sd, float min, float max, Color color) {
    char text[128];
    
    // Draw statistics box
    DrawRectangle(x, y, 200, 80, Fade(LIGHTGRAY, 0.7f));
    DrawRectangleLines(x, y, 200, 80, Fade(color, 0.5f));
    
    // Draw statistics text
    snprintf(text, sizeof(text), "Mean: %.2f", mean);
    DrawText(text, x + 5, y + 5, 14, color);
    
    snprintf(text, sizeof(text), "Median: %.2f", median);
    DrawText(text, x + 5, y + 25, 14, color);
    
    snprintf(text, sizeof(text), "SD: %.2f", sd);
    DrawText(text, x + 5, y + 45, 14, color);
    
    snprintf(text, sizeof(text), "Min/Max: %.1f/%.1f", min, max);
    DrawText(text, x + 100, y + 5, 14, color);
}

void draw_graph(const char* title, float *values, int count, int graph_index, 
                float min_val, float max_val, Color color) {
    if (count < 2) return;
    
    // Calculate graph position and dimensions
    float graph_x = GRAPH_LEFT_MARGIN + Y_LABEL_WIDTH;
    float graph_y = GRAPH_TOP_MARGIN + graph_index * (GRAPH_HEIGHT + GRAPH_MARGIN);
    float graph_width = WINDOW_WIDTH - GRAPH_LEFT_MARGIN - GRAPH_MARGIN - Y_LABEL_WIDTH;
    float graph_height = GRAPH_HEIGHT - GRAPH_BOTTOM_MARGIN;
    
    // Draw title (left-aligned)
    DrawText(title, graph_x + 5, graph_y - TITLE_OFFSET, 20, color);
    
    // Draw background and border
    DrawRectangle(graph_x, graph_y, graph_width, graph_height, Fade(RAYWHITE, 0.8f));
    DrawRectangleLines(graph_x, graph_y, graph_width, graph_height, Fade(color, 0.3f));
    
    // Calculate scales
    float x_scale = (graph_width - 20) / (count - 1);
    float y_scale = (graph_height - 20) / (max_val - min_val);
    
    // Draw grid lines and Y-axis labels
    for (int i = 0; i <= 5; i++) {
        float value = min_val + (max_val - min_val) * (1.0f - (float)i / 5);
        float y = graph_y + 10 + (graph_height - 30) * (i / 5.0f);
        
        // Grid line
        DrawLine(graph_x + 10, y, graph_x + graph_width - 10, y, Fade(LIGHTGRAY, 0.5f));
        
        // Y-axis label
        char value_text[16];
        snprintf(value_text, sizeof(value_text), "%.1f", value);
        int text_width = MeasureText(value_text, 12);
        DrawText(value_text, graph_x - text_width - 5, y - 6, 12, DARKGRAY);
    }
    
    // Calculate moving average
    float moving_avg[MAX_READINGS] = {0};
    if (count >= MOVING_AVG_WINDOW) {
        for (int i = MOVING_AVG_WINDOW/2; i < count - MOVING_AVG_WINDOW/2; i++) {
            float sum = 0;
            for (int j = -MOVING_AVG_WINDOW/2; j <= MOVING_AVG_WINDOW/2; j++) {
                sum += values[i + j];
            }
            moving_avg[i] = sum / MOVING_AVG_WINDOW;
        }
    }
    
    // Calculate statistics using GSL
    float mean = gsl_stats_float_mean(values, 1, count);
    float sd = gsl_stats_float_sd(values, 1, count);
    float min = gsl_stats_float_min(values, 1, count);
    float max = gsl_stats_float_max(values, 1, count);
    
    // Sort for median calculation
    float *sorted_values = (float *)malloc(count * sizeof(float));
    for (int i = 0; i < count; i++) sorted_values[i] = values[i];
    gsl_sort_float(sorted_values, 1, count);
    float median = gsl_stats_float_median_from_sorted_data(sorted_values, 1, count);
    free(sorted_values);
    
    // Draw statistics
    draw_statistics(graph_x + graph_width - 210, graph_y + 10, mean, median, sd, min, max, color);
    
    // Draw data points and lines
    Vector2 prev_point = {0};
    Vector2 prev_avg_point = {0};
    
    for (int i = 0; i < count; i++) {
        float x = graph_x + 10 + i * x_scale;
        float y = graph_y + 10 + (max_val - values[i]) * y_scale;
        
        // Draw data point
        DrawCircle(x, y, 2, Fade(color, 0.7f));
        
        // Draw line to previous point
        if (i > 0) {
            DrawLine(prev_point.x, prev_point.y, x, y, Fade(color, 0.3f));
        }
        
        // Draw moving average line
        if (i >= MOVING_AVG_WINDOW/2 && i < count - MOVING_AVG_WINDOW/2) {
            float avg_y = graph_y + 10 + (max_val - moving_avg[i]) * y_scale;
            
            if (i > MOVING_AVG_WINDOW/2) {
                DrawLine(prev_avg_point.x, prev_avg_point.y, x, avg_y, Fade(MAROON, 0.8f));
            }
            
            prev_avg_point = (Vector2){x, avg_y};
        }
        
        prev_point = (Vector2){x, y};
    }
    
    // Draw mean line
    float mean_y = graph_y + 10 + (max_val - mean) * y_scale;
    DrawLine(graph_x + 10, mean_y, graph_x + graph_width - 10, mean_y, 
             Fade(GOLD, 0.7f));
    
    // Draw time labels on X-axis
    if (count > 1) {
        // First point time (KST = UTC+9)
        time_t first_time = (time_t)readings[0].timestamp + (9 * 3600);
        struct tm *tm_info = gmtime(&first_time);
        char time_buf[20];
        strftime(time_buf, sizeof(time_buf), "%H:%M:%S", tm_info);
        DrawText(time_buf, graph_x + 10, graph_y + graph_height + 5, 12, DARKGRAY);
        
        // Last point time
        time_t last_time = (time_t)readings[count-1].timestamp + (9 * 3600);
        tm_info = gmtime(&last_time);
        strftime(time_buf, sizeof(time_buf), "%H:%M:%S", tm_info);
        int text_width = MeasureText(time_buf, 12);
        DrawText(time_buf, graph_x + graph_width - text_width - 10, graph_y + graph_height + 5, 12, DARKGRAY);
    }
}
