#include <inc/lib.h>
#define MAXPATH 1000

void umain(int argc, char **argv) {
    char path[MAXPATH];
    if(argc > 1) {
        printf("%s : too many arguments\n",argv[0]);
	} else {
        printf("%s\n", getcwd(path, MAXPATH));
	}
}
