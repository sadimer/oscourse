#include <inc/lib.h>

void 
umain(int argc, char **argv) {
    if (argc != 2 && argc != 3) {
        printf("usage: mkdir <-p> [directory name or path]\n");
        return;
    }
    if (argc == 2) {
		int res = mkdir((const char *)argv[1]);
		if (res < 0) {
			printf("error on creation dir with %s: %d\n", argv[0], res);
		}
		return;
	}
	if (argc == 3 && !strcmp(argv[1], "-p")) {
		char cur_path[MAXPATHLEN] = {0};
		char new[MAXPATHLEN] = {0};
		char tmp[MAXPATHLEN] = {0};
		if (argv[2][0] != '/') {
			getcwd(cur_path, MAXPATHLEN);
			strcat(cur_path, argv[2]);
		} else {
			strcat(cur_path, argv[2]);
		}
		beauty_path(new, cur_path);
		skip_doubledots(tmp, new);
		strcpy(cur_path, tmp);
		int len = strlen(cur_path);
		for (int i = 1; i < len; i++) {
			if (cur_path[i] == '/') {
				cur_path[i] = '\0';
				mkdir(cur_path);
				cur_path[i] = '/';
			} else if (i == len - 1) {
				mkdir(cur_path);
			}
		}
	} else {
		printf("usage: mkdir <-p> [directory name or path]\n");
        return;
	}
}
