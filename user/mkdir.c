#include <inc/lib.h>

void umain(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: mkdir <directory name or path>\n");
        return;
    }
    int res = mkdir((const char *)argv[1]);
    if (res < 0) {
        printf("Error on creation dir with %s: %d\n", argv[0], res);
	}
}
