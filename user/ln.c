#include <inc/lib.h>

void
umain(int argc, char **argv) {
	if(argc != 3 && argc != 4) {
		printf("Usage: ln <file> <link>\n");
		return;
	}
	/*int res;
	if (argv[1][1] == 's' && argv[1][1] == '-') {
		res = link(argv[2], argv[3]);
	}
	res = link(argv[1], argv[2]);
	if (res < 0) {
		printf("Link %s to %s failed\n", argv[1], argv[2]);
	}*/
}
