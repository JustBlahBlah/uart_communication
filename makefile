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

install: $(TARGET) uartecho.service
	@echo "Copying binary to /usr/local/bin/ on TARGET..."
	scp $(TARGET) $(PI_USER)@$(PI_HOST):/tmp/$(TARGET)
	ssh -t $(PI_USER)@$(PI_HOST) "sudo mv /tmp/$(TARGET) /usr/local/bin/$(TARGET) && sudo chmod +x /usr/local/bin/$(TARGET)"
	
	@echo "Copying systemd unit file to /etc/systemd/system/..."
	scp uartecho.service $(PI_USER)@$(PI_HOST):/tmp/uartecho.service
	ssh -t $(PI_USER)@$(PI_HOST) "sudo mv /tmp/uartecho.service /etc/systemd/system/uartecho.service && sudo chown root:root /etc/systemd/system/uartecho.service"
	
	@echo "Reloading systemd, enabling and starting the service..."
	ssh -t $(PI_USER)@$(PI_HOST) "sudo systemctl daemon-reload && sudo systemctl enable uartecho.service && sudo systemctl restart uartecho.service"
	@echo "Installation completed successfully!"
