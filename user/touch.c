#include <inc/lib.h>

void umain(int argc, char **argv)
{
    char *filename;
    char path[MAXPATHLEN];
    if (argc != 2) {
        printf("usage: touch [filename]\n");
        return;
    }
    filename = argv[1];
    if (*filename != '/') {
        getcwd(path, MAXPATHLEN);
	}
    strcat(path, filename);
    int res = open(path, O_CREAT);
    if (res < 0) {
        printf("error on creation file with %s: %d\n", argv[0], res);
	}
    close(res);
}
