#include <inc/lib.h>

void 
umain(int argc, char **argv) {
	if (argc != 3) {
        printf("usage: chmod [permissions] [file name or path]\n");
        return;
    }
    int res = chmod((const char *)argv[2], strtol(argv[1], NULL, 10));
    if (res < 0) {
        printf("error on chmod with %s: %d\n", argv[0], res);
	}
}
