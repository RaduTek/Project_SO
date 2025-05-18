#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define MAX_PATH 256
#define USER_LEN 32
#define CLUE_LEN 128

typedef struct {
    int id;
    char user[USER_LEN];
    float lat;
    float lon;
    char clue[CLUE_LEN];
    int value;
} Treasure;

void log_general(const char *action) {
    mkdir("logs", 0777);
    int log_fd = open("logs/general_logs", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd < 0) {
        perror("log_general open");
        return;
    }

    if (write(log_fd, action, strlen(action)) < 0) {
        perror("log_general write");
    }

    close(log_fd);
}

void log_action(const char *hunt_id, const char *action) {
    char log_path[MAX_PATH];
    snprintf(log_path, MAX_PATH, "hunts/%s/logged_hunt", hunt_id);

    int log_fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd < 0) {
        perror("log_action open");
        return;
    }

    char log_entry[MAX_PATH];
    snprintf(log_entry, sizeof(log_entry), "[%ld] %s: %s\n", time(NULL), hunt_id, action);

    write(log_fd, log_entry, strlen(log_entry));
    close(log_fd);

    char symlink_name[MAX_PATH];
    snprintf(symlink_name, MAX_PATH, "/logs/logged_hunt-%s", hunt_id);

    // make link to log
    mkdir("logs", 0755); // endure logs folder exists
    unlink(symlink_name);
    symlink(log_path, symlink_name);

    // also log to general
    log_general(log_entry);
}

int add_treasure(const char *hunt_id) {
    // 755 = rwxr-xr-x

    char dir_path[MAX_PATH];
    snprintf(dir_path, MAX_PATH, "hunts/%s", hunt_id);
    mkdir("hunts", 0755);
    mkdir(dir_path, 0755);

    char file_path[MAX_PATH];
    snprintf(file_path, MAX_PATH, "hunts/%s/treasures", hunt_id);

    Treasure t;
    printf("Enter Treasure ID: ");
    if (scanf("%d", &t.id) != 1) {
        printf("Invalid ID\n");
        return 1;
    }
    printf("Enter Username: ");
    if (scanf("%s", t.user) != 1) {
        printf("Invalid Username\n");
        return 1;
    }
    printf("Enter Latitude: ");
    if (scanf("%f", &t.lat) != 1) {
        printf("Invalid Latitude\n");
        return 1;
    }
    printf("Enter Longitude: ");
    if (scanf("%f", &t.lon) != 1) {
        printf("Invalid Longitude\n");
        return 1;
    }

    // Clear the newline character left by scanf
    getchar();

    printf("Enter Clue: ");
    fgets(t.clue, CLUE_LEN, stdin);
    // strip newline
    t.clue[strcspn(t.clue, "\n")] = 0;

    printf("Enter Value: ");
    if (scanf("%d", &t.value) != 1) {
        printf("Invalid Value\n");
        return 1;
    }

    // 644 = rw-r--r--
    int fd = open(file_path, O_APPEND | O_CREAT | O_WRONLY, 0644);
    if (fd < 0) {
        perror("add_treasure open");
        return 1;
    }

    write(fd, &t, sizeof(Treasure));
    close(fd);

    char log_msg[MAX_PATH];
    snprintf(log_msg, sizeof(log_msg), "Added treasure with ID %d", t.id);
    log_action(hunt_id, log_msg);
    return 0;
}

int list_treasures(const char *hunt_id) {
    char file_path[MAX_PATH];
    snprintf(file_path, MAX_PATH, "hunts/%s/treasures", hunt_id);

    struct stat st;
    if (stat(file_path, &st) < 0) {
        perror("list_treasures stat");
        return 1;
    }

    printf("Hunt: %s\n", hunt_id);
    printf("File size: %ld bytes\n", st.st_size);
    printf("Last modified: %s", ctime(&st.st_mtime));

    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        perror("list_treasures open");
        return 1;
    }

    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        printf("Treasure ID: %d \t| User: %s \t| Coords: %.2f,%.2f \t| Value: %d\n", t.id, t.user, t.lat, t.lon, t.value);
    }
    close(fd);

    log_action(hunt_id, "Listed treasures");

    return 0;
}

int view_treasure(const char *hunt_id, int target_id) {
    char file_path[MAX_PATH];
    snprintf(file_path, MAX_PATH, "hunts/%s/treasures", hunt_id);

    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (t.id == target_id) {
            printf("ID: %d\nUser: %s\nCoords: %.2f, %.2f\nClue: %s\nValue: %d\n",
                   t.id, t.user, t.lat, t.lon, t.clue, t.value);
            close(fd);

            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "Viewed treasure ID %d", t.id);
            log_action(hunt_id, log_msg);
            close(fd);
            return 0;
        }
    }

    printf("Treasure ID %d not found.\n", target_id);
    close(fd);
    return 1;
}

int remove_treasure(const char *hunt_id, int target_id) {
    char file_path[MAX_PATH];
    snprintf(file_path, MAX_PATH, "hunts/%s/treasures", hunt_id);

    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        perror("remove_treasure open");
        return 1;
    }

    char tmp_path[MAX_PATH];
    snprintf(tmp_path, MAX_PATH, "hunts/%s/temp.dat", hunt_id);
    int tmp_fd = open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    Treasure t;
    int found = 0;

    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (t.id == target_id) {
            found = 1;

            // Log full treasure details to general log
            char general_log_msg[512];
            snprintf(general_log_msg, sizeof(general_log_msg),
                     "[%ld] %s: Removed treasure: ID=%d, User=%s, Coords=%.2f,%.2f, Clue=%s, Value=%d\n",
                     time(NULL), hunt_id, t.id, t.user, t.lat, t.lon, t.clue, t.value);
            log_general(general_log_msg);

            continue; // skip writing to new file
        }
        write(tmp_fd, &t, sizeof(Treasure));
    }

    close(fd);
    close(tmp_fd);
    rename(tmp_path, file_path);

    if (found) {
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "Removed treasure ID %d", target_id);
        log_action(hunt_id, log_msg);
    } else {
        printf("Treasure ID %d not found.\n", target_id);
        return 1;
    }

    return 0;
}

int remove_hunt(const char *hunt_id) {
    // remove treasure data file
    char file_path[MAX_PATH];
    snprintf(file_path, MAX_PATH, "hunts/%s/treasures", hunt_id);
    remove(file_path);

    // remove hunt log file
    char log_path[MAX_PATH];
    snprintf(log_path, MAX_PATH, "hunts/%s/logged_hunt", hunt_id);
    remove(log_path);

    // remove hunt directory
    char hunt_dir[MAX_PATH];
    snprintf(hunt_dir, MAX_PATH, "hunts/%s", hunt_id);
    rmdir(hunt_dir);

    // unlink log file from logs folder
    char symlink_name[MAX_PATH];
    snprintf(symlink_name, MAX_PATH, "logs/logged_hunt-%s", hunt_id);
    unlink(symlink_name);

    // log the remove action to general logs
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Removed hunt %s", hunt_id);
    log_general(log_msg);

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("usage: treasure_manager --<add|list|view|remove-treasure|remove-hunt> <hunt_id> [<id>]\n");
        return 1;
    }

    if (strcmp(argv[1], "--add") == 0) {
        return add_treasure(argv[2]);
    } else if (strcmp(argv[1], "--list") == 0) {
        return list_treasures(argv[2]);
    } else if (strcmp(argv[1], "--view") == 0 && argc == 4) {
        return view_treasure(argv[2], atoi(argv[3]));
    } else if (strcmp(argv[1], "--remove-treasure") == 0 && argc == 4) {
        return remove_treasure(argv[2], atoi(argv[3]));
    } else if (strcmp(argv[1], "--remove-hunt") == 0) {
        return remove_hunt(argv[2]);
    } else {
        printf("Invalid command or arguments.\n");
    }

    return 1;
}
