#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

#define MAX_PATH 256
#define CMD_FILE "/tmp/treasure_hunt_monitor_cmd"

volatile sig_atomic_t monitor_got_signal = 0;

void signal_handler(int signum) {
    monitor_got_signal = signum;
}

void got_signal(int signum) {
    switch (signum) {
        case SIGUSR1:
            // read command from file
            int fd = open(CMD_FILE, O_RDONLY);
            if (fd != -1) {
                char command[MAX_PATH];
                ssize_t bytes_read = read(fd, command, sizeof(command) - 1);
                if (bytes_read > 0) {
                    command[bytes_read] = '\0';
                    command[strcspn(command, "\n")] = 0;

                    int ret = system(command);
                    if (ret != 0) {
                        printf("monitor: command returned code %d", ret);
                    }
                } else {
                    printf("monitor: error reading command");
                }
                close(fd);
            } else {
                printf("monitor: error opening command file");
            }
            break;
        case SIGTERM:
            // exit gracefully
            printf("monitor: received SIGTERM, stopping...\n");
            unlink(CMD_FILE);
            exit(0);
        default:
            break;
    }
}

int monitor_main() {
    // Set up the signal handler
    struct sigaction sa;
    sa.sa_flags = 0;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);

    while(1) {
        // Wait for signals
        pause();

        // Check if we got command
        if (monitor_got_signal) {
            got_signal(monitor_got_signal);
        }
    }
}