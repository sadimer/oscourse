#include <inc/lib.h>

void
umain(int argc, char **argv) {
	if(argc != 3) {
		printf("Usage: ln <file> <link>\n");
		return;
	}
	int res = symlink(argv[2], argv[1]);
	if (res < 0) {
		printf("Link %s to %s failed\n", argv[2], argv[1]);
	}
}
