#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int parent_fd[2];
    int child_fd[2];
    if (pipe(parent_fd) < 0 || pipe(child_fd) < 0) {
        fprintf(2, "create  pipe filed \n");
        exit();
    }
    char buf[1];
    if (fork()) {
        // parent procecss
        write(parent_fd[1], buf, 1);
        read(child_fd[0], buf, 1);
        fprintf(1, "%d: received pong\n", getpid());
    } else {
        // child process
        read(parent_fd[0], buf, 1);
        fprintf(1, "%d: received ping\n", getpid());
        write(child_fd[1], buf, 1);
    }
    exit();
}

