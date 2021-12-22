#include <inc/lib.h>


void 
umain(int argc, char **argv) {
    if (argc != 2) {
        printf("usage: cd [directory name]\n");
        return;
    }
	int res = chdir(argv[1], 0);
	if (res < 0) {
		printf("error on chdir of %s: %d\n", argv[0], res);
	}
}
