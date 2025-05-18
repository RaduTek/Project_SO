#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define HUNTS_DIR "./hunts"

int is_directory(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0)
        return 0;
    return S_ISDIR(statbuf.st_mode);
}

int calc_hunt_scores() {
    DIR *dir;
    struct dirent *entry;

    dir = opendir(HUNTS_DIR);
    if (!dir) {
        perror("hunt_scores: opendir");
        return 1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char path[512];
        snprintf(path, sizeof(path), "%s/%s", HUNTS_DIR, entry->d_name);

        if (!is_directory(path)) {
            continue;
        }
        
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("hunt_scores: pipe");
            continue;
        }

        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            // redirect stdout to the pipe
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);

            execl("./hunt_scores.sh", "./hunt_scores.sh", entry->d_name, (char *)NULL);

            perror("hunt_scores: exec");
            exit(1);
        }
    
        close(pipefd[1]);
        char buffer[1024];
        ssize_t bytes_read;

        while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer)-1)) > 0) {
            buffer[bytes_read] = '\0';
            printf("%s", buffer);
        }

        close(pipefd[0]);

        // wait for script to finish
        int status;
        waitpid(pid, &status, 0);
    }

    closedir(dir);
    return 0;
}
