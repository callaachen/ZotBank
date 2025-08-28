# Calla Chen — ZotBank Project Makefile (src/ layout)
CXX = g++
CXXFLAGS = -std=c++98 -Wall -Wextra -I./src
TARGET = zotbank
SRC_DIR = src

# Object files in src directory
OBJS = $(SRC_DIR)/main.o \
       $(SRC_DIR)/banker.o \
       $(SRC_DIR)/logger.o \
       $(SRC_DIR)/command_handler.o \
       $(SRC_DIR)/validator.o \
       $(SRC_DIR)/log_global.o

# Build target
$(TARGET): $(OBJS)
	@echo "[BUILD] Linking executable..."
	@$(CXX) $(CXXFLAGS) -o $@ $(OBJS)
	@echo "[BUILD] Done: $(TARGET)"

# Rule for compiling source files
$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "[BUILD] Compiling $<..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean object files and binary
clean:
	@echo "[CLEAN] Removing compiled object files..."
	@rm -f $(SRC_DIR)/*.o

	@echo "[CLEAN] Removing executable binary..."
	@rm -f $(TARGET)

	@echo "[CLEAN] Removing log and session output files..."
	@rm -f logs/events.log logs/full_session.txt logs/report.csv logs/history.txt logs/save.txt
	@rm -f logs/customer_P*.txt logs/session_summary.txt logs/tmp.txt
	@rm -f logs/log_summary.csv logs/per_customer_log.csv logs/deadlock_log.csv logs/request_heatmap.csv
	@rm -f logs/*.png

	@echo "[CLEAN] Recreating blank log files..."
	@touch logs/events.log logs/full_session.txt logs/history.txt logs/session_summary.txt logs/report.csv
	@touch logs/log_summary.csv logs/per_customer_log.csv logs/deadlock_log.csv logs/request_heatmap.csv

	@echo "[CLEAN] Cleanup complete!"

# Run the simulation with tests input
run: $(TARGET)
	./$(TARGET) maximum.txt 10 5 7 8
