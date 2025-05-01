#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_PATH 256
#define MAX_ARGC 32


bool monitor_started = false;


int cmd_start_monitor(int argc, char *argv[]) {
    if (!monitor_started) {
        monitor_started = true;
        // start the monitor
        printf("NOT IMPLEMENTED!\n");
        printf("Monitor has been started!\n");
    } else {
        printf("Monitor already started!\n");
    }

    return 0;
}

int cmd_list_hunts(int argc, char *argv[]) {
    printf("NOT IMPLEMENTED!\n");

    return 0;
}

int cmd_list_treasures(int argc, char *argv[]) {
    printf("NOT IMPLEMENTED!\n");

    return 0;
}

int cmd_view_treasure(int argc, char *argv[]) {
    printf("NOT IMPLEMENTED!\n");

    printf("I got %d arguments:\n", argc);
    for (int i=0; i < argc; i++) {
        printf("argv[%d]: %s\n", i, argv[i]);
    }

    return 0;
}

int cmd_stop_monitor(int argc, char *argv[]) {
    if (monitor_started) {
        monitor_started = false;
        // stop the monitor
        printf("NOT IMPLEMENTED!\n");
        printf("Monitor stopped!\n");
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

        if (strcmp(argv[0], "start_monitor") == 0) {
            cmd_start_monitor(argc, argv);
        } else if (strcmp(argv[0], "list_hunts") == 0) {
            cmd_list_hunts(argc, argv);
        } else if (strcmp(argv[0], "list_treasures") == 0) {
            cmd_list_treasures(argc, argv);
        } else if (strcmp(argv[0], "view_treasure") == 0) {
            cmd_view_treasure(argc, argv);
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
    return parse_interactive_command();
}