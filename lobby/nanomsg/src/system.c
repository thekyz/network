#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

int exec_cmd(char *argv[])
{
    pid_t pid;
    int rc;

    pid = fork();
    if (pid == 0) {
        // child
        rc = execvp(argv[0], argv);
        if (rc < 0) {
            fprintf(stderr, "error: child %d exec failed (%s)\n", pid, strerror(errno));
            exit(-1);
        }

        // never reached ...
    } else if (pid < 0) {
        fprintf(stderr, "error: child fork failed (%s)\n", strerror(errno));
        return -1;
    }

    return 0;
}
