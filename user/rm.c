#include <inc/lib.h>

void umain(int argc, char **argv) {
	if (argc != 2) {
		printf("usage: rm [file name or path]\n");
        return;
    }
	int res = remove(argv[1]);
	if (res < 0) {
        printf("error on remove file with %s: %d\n", argv[0], res);
	}
}
