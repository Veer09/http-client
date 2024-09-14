# Define variables
CC = gcc
CFLAGS = -Wall -Iinclude
TARGET = http_client
SRC_DIR = src
OBJ_DIR = obj
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)


# Default rule to build the target
all: $(TARGET)

# Rule to build the target executable
$(TARGET): $(OBJ_DIR) $(OBJS)
	$(CC) -o $@ $(OBJS)

# Rule to build object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create object directory if it doesn't exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Rule to clean up build artifacts
clean:
	rm -f $(TARGET)
	rm -rf $(OBJ_DIR)

# Declare phony targets
.PHONY: all clean
