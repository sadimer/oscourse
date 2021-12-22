#include <inc/lib.h>

void 
umain(int argc, char **argv) {
    char path[MAXPATHLEN];
    if (argc > 1) {
        printf("usage: pwd\n");
	} else {
        printf("%s\n", getcwd(path, MAXPATHLEN));
	}
}
