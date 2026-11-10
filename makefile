CROSS_COMPILE ?= 
CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Wextra -O2 -std=c17

TARGET = mycom
SRC = main.c

PI_USER ?= pi
PI_HOST ?= 192.168.0.106
PI_DIR ?= /home/pi/UART_communication

.PHONY: all clean deploy

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)
	@echo "Build complete: $(TARGET)"

clean:
	rm -f $(TARGET) *.o uart_log_*.txt
	@echo "Cleaned up project files."

deploy: $(TARGET)
	@echo "Deploying $(TARGET) to $(PI_USER)@$(PI_HOST):$(PI_DIR) ..."
	ssh $(PI_USER)@$(PI_HOST) "mkdir -p $(PI_DIR)"
	@echo "Deploying $(TARGET) to $(PI_USER)@$(PI_HOST):$(PI_DIR) ..."
	scp $(TARGET) $(PI_USER)@$(PI_HOST):$(PI_DIR)/$(TARGET)
	@echo "Deployment complete!"
