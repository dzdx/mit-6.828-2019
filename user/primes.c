#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int reader;
    int right[2];
    if (pipe(right) < 0) {
        fprintf(2, "create pipe filed \n");
        exit();
    }
    int buf1[1];
    int buf2[1];
    if (fork()) {
        for (int i = 2; i <= 35; i++) {
            buf1[0] = i;
            write(right[1], buf1, sizeof(buf1));
        }
        exit();
    } else {
        while (1) {
            close(right[1]); // close parent write fd in child process
            reader = right[0];
            if (read(reader, buf1, sizeof(buf1)) == 0) {
                exit();
            }
            if (buf1[0] >= 35) {
                exit();
            }
            if (pipe(right) < 0) {
                fprintf(2, "%d create pipe filed \n", getpid());
                exit();
            }
            if (fork()) {
                fprintf(1, "%d: prime %d\n", getpid(), buf1[0]);
                while (1) {
                    if (read(reader, buf2, sizeof(buf2)) == 0) {
                        exit();
                    }
                    if (buf2[0] % buf1[0]) {
                        write(right[1], buf2, sizeof(buf2));
                    }
                }
            }
        }
    }
}
