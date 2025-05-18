#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "monitor.c"
#include "hunt_scores.c"

#define MAX_PATH 256
#define MAX_ARGC 32
#define CMD_FILE "/tmp/treasure_hunt_monitor_cmd"


bool monitor_started = false;
bool monitor_stopping = false;
pid_t monitor_pid = 0;
int monitor_pipefd[2];


void handle_sigchld(int signum) {
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (pid == monitor_pid) {
            monitor_started = false;
            monitor_stopping = false;
            printf("Monitor has exited\n");
        }
    }
}

void setup_signal() {
    struct sigaction sa;
    sa.sa_handler = handle_sigchld;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);
}


void read_monitor_output() {
    char buf[256];

    struct pollfd fds;
    fds.fd = monitor_pipefd[0];
    fds.events = POLLIN;

    // read from the pipe until done or it times out
    while (true) {
        int ret = poll(&fds, 1, 500);
        if (ret < 0) {
            perror("error: poll");
            break;
        } else if (ret == 0) {
            break;
        }

        if (fds.revents & POLLIN) {
            int n = read(monitor_pipefd[0], buf, sizeof(buf) - 1);
            if (n > 0) {
                buf[n] = '\0';
                printf("%s", buf);
            } else if (n == 0) {
                break;
            } else {
                perror("read");
                break;
            }
        } else if (fds.revents & (POLLERR | POLLHUP)) {
            // Error or hang-up
            break;
        }
    }
}

void send_command(char *command) {
    if (strlen(command) >= MAX_PATH) {
        printf("error: command exceeds maximum length of %d\n", MAX_PATH - 1);
        return;
    }

    int fd = open(CMD_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("error: open");
        return;
    }

    if (write(fd, command, strlen(command)) < 0) {
        perror("error: write");
        close(fd);
        return;
    }

    close(fd);

    if (kill(monitor_pid, SIGUSR1) < 0) {
        perror("error: kill");
    }

    // read command output from monitor
    read_monitor_output();
}


int cmd_start_monitor(int argc, char *argv[]) {
    if (!monitor_started) {
        // create pipe for the monitor
        if (pipe(monitor_pipefd) < 0) {
            perror("error: pipe");
            return 1;
        }

        // start the monitor
        monitor_pid = fork();
        if (monitor_pid == 0) {
            // child process
            // redirect stdout and stderr to the pipe
            close(monitor_pipefd[0]);
            dup2(monitor_pipefd[1], STDOUT_FILENO);
            dup2(monitor_pipefd[1], STDERR_FILENO);
            close(monitor_pipefd[1]);

            // run the monitor's main function
            exit(monitor_main());
        }

        close(monitor_pipefd[1]);

        monitor_started = true;
        monitor_stopping = false;
        printf("Monitor has been started!\n");
    } else {
        printf("Monitor already started!\n");
    }

    return 0;
}

int cmd_list_hunts(int argc, char *argv[]) {
    if (!monitor_started) {
        printf("Monitor is not started!\n");
        return 1;
    }

    char command[MAX_PATH];
    snprintf(command, sizeof(command), "ls ./hunts/");
    send_command(command);

    return 0;
}

int cmd_list_treasures(int argc, char *argv[]) {
    if (!monitor_started) {
        printf("Monitor is not started!\n");
        return 1;
    }

    if (argc < 2) {
        printf("Bad arguments!\n");
        return 1;
    }

    char command[MAX_PATH];
    snprintf(command, sizeof(command), "./treasure_manager --list %s", argv[1]);
    send_command(command);

    return 0;
}

int cmd_view_treasure(int argc, char *argv[]) {
    if (!monitor_started) {
        printf("Monitor is not started!\n");
        return 1;
    }

    if (argc < 3) {
        printf("Bad arguments!\n");
        return 1;
    }

    char command[MAX_PATH];
    snprintf(command, sizeof(command), "./treasure_manager --view %s %s", argv[1], argv[2]);
    send_command(command);

    return 0;
}

int cmd_calculate_score(int argc, char *argv[]) {

    calc_hunt_scores();

    return 0;
}

int cmd_stop_monitor(int argc, char *argv[]) {
    if (monitor_started) {
        // stop the monitor
        kill(monitor_pid, SIGTERM);
        monitor_started = false;
        monitor_stopping = true;
    } else {
        printf("Monitor is not started!\n");
    }

    return 0;
}

int cmd_help(int argc, char *argv[]) {
    printf("help: Available commands:\n");
    printf("  start_monitor: Starts the treasure monitor process\n");
    printf("  list_hunts: List all hunts\n");
    printf("  list_treasures: List all treasures\n");
    printf("  view_treasure: View a specific treasure\n");
    printf("  calculate_score: Calculate the score for each hunt\n");
    printf("  stop_monitor: Stop the monitor process\n");
    printf("  help: Show this help message\n");
    printf("  exit: Exit the program\n");

    return 0;
}

int cmd_exit(int argc, char *argv[]) {
    if (monitor_started) {
        printf("Monitor is still running. Please stop it before exiting.\n");
        return 1;
    }

    printf("Goodbye!\n");

    return 0;
}

int parse_interactive_command() {
    printf("Welcome to the Treasure Hub!\n");
    char buf[MAX_PATH];
    char *argv[MAX_ARGC];

    while (true) {
        printf("> ");
        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            if (feof(stdin)) {
                break;
            }
            
            if (ferror(stdin) && errno == EINTR) {
                // Interrupted by signal, retry
                clearerr(stdin);
                continue;
            }

            printf("error: read input\n");
            continue;
        }

        buf[strcspn(buf, "\n")] = 0;

        int argc = 0;
        char *arg = strtok(buf, " ");
        while (arg != NULL && argc < MAX_ARGC) {
            argv[argc++] = arg;
            arg = strtok(NULL, " ");
        }

        if (argc == 0) continue;

        if (monitor_stopping) {
            printf("error: monitor is stopping\n");
        }

        if (strcmp(argv[0], "start_monitor") == 0) {
            cmd_start_monitor(argc, argv);
        } else if (strcmp(argv[0], "list_hunts") == 0) {
            cmd_list_hunts(argc, argv);
        } else if (strcmp(argv[0], "list_treasures") == 0) {
            cmd_list_treasures(argc, argv);
        } else if (strcmp(argv[0], "view_treasure") == 0) {
            cmd_view_treasure(argc, argv);
        } else if (strcmp(argv[0], "calculate_score") == 0) {
            cmd_calculate_score(argc, argv);
        } else if (strcmp(argv[0], "stop_monitor") == 0) {
            cmd_stop_monitor(argc, argv);
        } else if (strcmp(argv[0], "help") == 0) {
            cmd_help(argc, argv);
        } else if (strcmp(argv[0], "exit") == 0) {
            if (cmd_exit(argc, argv) == 0) {
                break;
            }
        } else {
            printf("treasure_hub: Command not recognized \"%s\"\n", argv[0]);
        }
    }

    return 0;
}

int main() {
    setup_signal();

    return parse_interactive_command();
}