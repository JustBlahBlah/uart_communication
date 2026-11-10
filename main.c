#define _GNU_SOURCE
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define MAX_BUFFER_SIZE 1024
#define MSG_BUFFER_SIZE 2048

volatile sig_atomic_t keep_running = 1;
int uart_fd = -1;
FILE *log_file = NULL;

void signal_handler(int signum) {
  printf("\n[INFO] Received signal %d. Shutting down gracefully...\n", signum);
  keep_running = 0;
}

void get_timestamp(char *buf, size_t size, bool for_filename) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  if (for_filename) {
    strftime(buf, size, "%Y%m%d_%H%M%S", t);
  } else {
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", t);
  }
}

void log_print(const char *msg) {
  printf("%s\n", msg);
  if (log_file) {
    fprintf(log_file, "%s\n", msg);
    fflush(log_file);
  }
}

void invert_case(char *str) {
  for (int i = 0; str[i] != '\0'; i++) {
    if (islower(str[i])) {
      str[i] = toupper(str[i]);
    } else if (isupper(str[i])) {
      str[i] = tolower(str[i]);
    }
  }
}

bool configure_uart(int fd) {
  struct termios tty;
  if (tcgetattr(fd, &tty) != 0) {
    return false;
  }

  cfmakeraw(&tty);
  cfsetispeed(&tty, B115200);
  cfsetospeed(&tty, B115200);

  tty.c_cflag &= ~PARENB;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CSIZE;
  tty.c_cflag |= CS8;
  tty.c_cflag |= CREAD | CLOCAL;

  tty.c_cc[VMIN] = 0;
  tty.c_cc[VTIME] = 1;

  if (tcsetattr(fd, TCSANOW, &tty) != 0) {
    return false;
  }
  return true;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <uart_device> (e.g., /dev/ttyAMA0)\n", argv[0]);
    return EXIT_FAILURE;
  }
  const char *device = argv[1];

  struct sigaction action;
  memset(&action, 0, sizeof(struct sigaction));
  action.sa_handler = signal_handler;
  if (sigaction(SIGINT, &action, NULL) < 0 ||
      sigaction(SIGTERM, &action, NULL) < 0) {
    perror("[ERROR] Error setting sigaction");
    return EXIT_FAILURE;
  }

  char ts_buf[64];
  char msg_buf[MSG_BUFFER_SIZE];

  get_timestamp(ts_buf, sizeof(ts_buf), true);
  snprintf(msg_buf, sizeof(msg_buf), "echo_%s.log", ts_buf);
  log_file = fopen(msg_buf, "a");
  if (!log_file) {
    perror("[ERROR] Failed to create log file");
    return EXIT_FAILURE;
  }

  uart_fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);
  if (uart_fd < 0) {
    get_timestamp(ts_buf, sizeof(ts_buf), false);
    snprintf(msg_buf, sizeof(msg_buf), "[%s] [ERROR] Failed to open port %s",
             ts_buf, device);
    log_print(msg_buf);
    fclose(log_file);
    return EXIT_FAILURE;
  }

  if (!configure_uart(uart_fd)) {
    get_timestamp(ts_buf, sizeof(ts_buf), false);
    snprintf(msg_buf, sizeof(msg_buf),
             "[%s] [ERROR] Failed to configure terminal on %s", ts_buf, device);
    log_print(msg_buf);
    close(uart_fd);
    fclose(log_file);
    return EXIT_FAILURE;
  }

  get_timestamp(ts_buf, sizeof(ts_buf), false);
  snprintf(msg_buf, sizeof(msg_buf),
           "[%s] [INFO] ECHO server started on %s. Speed 115200 8N1", ts_buf,
           device);
  log_print(msg_buf);

  char rx_buffer[MAX_BUFFER_SIZE];
  int rx_index = 0;
  char rx_char;

  while (keep_running) {
    int n = read(uart_fd, &rx_char, 1);
    if (n > 0) {
      if (rx_index < MAX_BUFFER_SIZE - 3) {
        rx_buffer[rx_index++] = rx_char;
      }

      if (rx_char == '\n') {
        rx_buffer[rx_index] = '\0';

        int len = rx_index;
        if (len > 0 && rx_buffer[len - 1] == '\n')
          rx_buffer[--len] = '\0';
        if (len > 0 && rx_buffer[len - 1] == '\r')
          rx_buffer[--len] = '\0';

        get_timestamp(ts_buf, sizeof(ts_buf), false);
        snprintf(msg_buf, sizeof(msg_buf), "[%s] Received: %s", ts_buf,
                 rx_buffer);
        log_print(msg_buf);

        invert_case(rx_buffer);
        strcat(rx_buffer, "\r\n");

        if (write(uart_fd, rx_buffer, strlen(rx_buffer)) < 0) {
          perror("[ERROR] Failed to write to port");
        }

        rx_index = 0;
      }
    }
  }

  get_timestamp(ts_buf, sizeof(ts_buf), false);
  snprintf(msg_buf, sizeof(msg_buf),
           "[%s] [INFO] Closing port and exiting program.", ts_buf);
  log_print(msg_buf);

  close(uart_fd);
  fclose(log_file);
  return EXIT_SUCCESS;
}
