CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I/usr/local/include -I/opt/homebrew/include
LDFLAGS = -L/usr/local/lib -L/opt/homebrew/lib -lraylib -framework OpenGL -framework OpenAL -framework Cocoa -lsqlite3 -lm

TARGET = sensor_simulator
VISUALIZER = sensor_visualizer
SIMULATOR_SRC = sensor_simulator.c
VISUALIZER_SRC = sensor_visualizer.c

all: $(TARGET) $(VISUALIZER)

$(TARGET): $(SIMULATOR_SRC)
	$(CC) $(CFLAGS) -o $@ $< -lsqlite3 -lm

$(VISUALIZER): $(VISUALIZER_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET) $(VISUALIZER)

run_sim: $(TARGET)
	./$(TARGET)

run_visual: $(VISUALIZER)
	./$(VISUALIZER)

run: all
	@echo "Starting sensor simulator and visualizer..."
	@echo "- Press Ctrl+C in this window to stop both applications"
	@echo "- Visualizer will open in a new window"
	@echo "- Simulator output will be shown in this terminal"
	@(./sensor_simulator &) && (./sensor_visualizer)

# Build only the simulator
sim: $(TARGET)

# Build only the visualizer
visual: $(VISUALIZER)

.PHONY: all clean run_simulator run_visualizer run sim visual
