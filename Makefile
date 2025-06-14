# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I/usr/local/include -I/opt/homebrew/include -O2
LDFLAGS = -L/usr/local/lib -L/opt/homebrew/lib -lraylib -framework OpenGL -framework OpenAL -framework Cocoa -lsqlite3 -lgsl -lgslcblas -lm

# Targets
TARGET = sensor_simulator
VISUALIZER = sensor_visualizer
GSL_VISUALIZER = sensor_gsl_visualizer

# Source files
SIMULATOR_SRC = sensor_simulator.c
VISUALIZER_SRC = sensor_visualizer.c
GSL_VISUALIZER_SRC = sensor_gsl_visualizer.c

# Default target
all: $(TARGET) $(VISUALIZER) $(GSL_VISUALIZER)

# Build rules
$(TARGET): $(SIMULATOR_SRC)
	$(CC) $(CFLAGS) -o $@ $< -lsqlite3 -lm

$(VISUALIZER): $(VISUALIZER_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(GSL_VISUALIZER): $(GSL_VISUALIZER_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Clean rule
clean:
	rm -f $(TARGET) $(VISUALIZER) $(GSL_VISUALIZER)

# Run targets
run_sim: $(TARGET)
	./$(TARGET)

run_visual: $(VISUALIZER)
	./$(VISUALIZER)

run_gsl_visual: $(GSL_VISUALIZER)
	./$(GSL_VISUALIZER)

# Combined run with GSL visualizer
run: all
	@echo "Starting sensor simulator and GSL visualizer..."
	@echo "- Press Ctrl+C in this window to stop both applications"
	@echo "- Visualizer will open in a new window"
	@echo "- Simulator output will be shown in this terminal"
	@(./$(TARGET) &) && (./$(GSL_VISUALIZER))

# Individual build targets
sim: $(TARGET)
visual: $(VISUALIZER)
gsl_visual: $(GSL_VISUALIZER)

# Run with GSL visualizer
gsl: all run_gsl_visual

.PHONY: all clean run_sim run_visual run_gsl_visual run sim visual gsl_visual gsl
