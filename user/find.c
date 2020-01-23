#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"

char *pathjoin(const char *path1, const char *path2) {
    int len1 = strlen(path1);
    int len2 = strlen(path2);
    char *ret;
    int start2;

    if (path1[len1 - 1] == '/') {
        ret = malloc(sizeof(char) * (len1 + len2 + 1));
        strcpy(ret, path1);
        start2 = len1;
    } else {
        ret = malloc(sizeof(char) * (len1 + len2 + 2));
        strcpy(ret, path1);
        ret[len1] = '/';
        start2 = len1 + 1;
    }
    strcpy(ret + start2, path2);
    ret[start2 + len2] = '\0';
    return ret;
}

char *basename(char *path) {
    int len = strlen(path);
    int i;
    for (i = len - 1; path[i] != '/' && i >= 0; i--);
    return path + i + 1;
}

void find(char *path, char *pattern);

void find(char *path, char *pattern) {
    struct dirent de;
    struct stat st;
    int fd;
    if ((fd = open(path, 0)) < 0) {
        fprintf(2, "find: cannot open %s\n", path);
        exit();
    }
    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        exit();
    }
    switch (st.type) {
        case T_DIR:
            while (read(fd, &de, sizeof(de)) == sizeof(de)) {
                if (de.inum == 0)
                    continue;
                if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) {
                    continue;
                }
                char *subpath = pathjoin(path, de.name);
                find(subpath, pattern);
                free(subpath);
            }
            break;
        default:;
            char *filename = basename(path);
            if (strcmp(filename, pattern) == 0) {
                fprintf(1, "%s\n", path);
            }
            break;
    }
    close(fd);
}


int main(int argc, char *argv[]) {

    if (argc < 3) {
        fprintf(2, "usage: find path pattern\n");
        exit();
    }
    char *pattern = argv[2];
    char *path = argv[1];
    find(path, pattern);
    exit();
}


