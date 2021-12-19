#include <inc/lib.h>
#define MAXPATH 1000

int 
get_lastdir(const char *path) {
	int len = strlen(path);
	int max_ind = 0;
	for (int i = 0; i < len - 1; i++) {
		if (path[i] == '/') {
			max_ind = i;
		}
	}
	return max_ind;
}
void 
umain(int argc, char **argv) {
    if (argc != 2) {
        printf("usage: cd [directory name]\n");
        return;
    }
    char path[MAXPATH];
    if (argv[1][0] == '.' && (argv[1][1] != '.' || strlen(argv[1]) == 1)) {
		getcwd(path, MAXPATH);
		if (strlen(argv[1]) == 1) {
			strcat(path, "\0");
		} else {
			if (argv[1][1] == '/') {
				strcat(path, (const char *)&argv[1][2]);
				strcat(path, "/\0");
			} else {
				printf("error! Path without '/'!\n");
			}
		}
	} else if (argv[1][0] == '.' && argv[1][1] == '.') {
		getcwd(path, MAXPATH);
		int res = get_lastdir(path);
		path[res + 1] = '\0';
		if (strlen(argv[1]) != 2) {
			if (argv[1][2] == '/') {
				strcat(path, (const char *)&argv[1][3]);
				strcat(path, "/\0");
			} else {
				printf("error! Path without '/'!\n");
			}
		} else {
			strcat(path, "\0");
		}
	} else if (argv[1][0] != '/') {
		getcwd(path, MAXPATH);
		strcat(path, (const char *)argv[1]);
		strcat(path, "/\0");
	} else {
		strcat(path, (const char *)argv[1]);
		strcat(path, "/\0");
	}
	int res = chdir(path);
	if (res < 0) {
		printf("error on chdir of %s: %d\n", argv[0], res);
	}
}
