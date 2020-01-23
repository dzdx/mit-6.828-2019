#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"


int main(int argc, char *argv[]) {
    char *xargs[MAXARG];
    char *buf = malloc(128 * sizeof(char));
    int count = 0;
    int i = 0;
    char tmp[1];
    while (1) {
        int n = read(0, tmp, 1);
        if (n < 0) {
            fprintf(2, "read from stdin failed");
            exit();
        }
        if (n == 0) {
            break;
        }
        if (tmp[0] == '\n' || tmp[0] == ' ' || tmp[0] == '\t') {
            i = 0;
            xargs[count] = buf;
            buf = malloc(128 * sizeof(char));
            count++;
            continue;
        }
        buf[i] = tmp[0];
        i++;
    }
    if (argc - 2 + count > MAXARG) {
        fprintf(2, "too many arguments");
        exit();
    }
    if (fork()) {
        wait();
    } else {
        char **args = malloc((argc - 1 + count) * sizeof(char *));
        for (i = 1; i < argc; i++) {
            args[i - 1] = argv[i];
        }
        for (int j = 0; j < count; j++) {
            args[argc - 1 + j] = xargs[j];
        }
        exec(argv[1], args);
    }
    exit();
}
