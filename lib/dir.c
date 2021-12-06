#include <inc/string.h>
#include <inc/lib.h>

char *getcwd(char *buffer, int maxlen) {
	if (!buffer || maxlen < 0) {
		return (char *)thisenv->workpath;
	}
	return strncpy((char *)buffer, (const char *)thisenv->workpath, maxlen);
}

int chdir(const char *path) {
	struct Stat st;
	int res = stat(path, &st);
	if (res < 0) {
		return res;
	}
	if (!st.st_isdir) {
		return -E_INVAL;
	}
	if (path[strlen(path) - 1] != '/') {
		strcat((char *)path, "/");
	}
	return sys_env_set_workpath(thisenv->env_id, path);
}


int mkdir(const char *dirname) {
	char cur_path[MAXPATH];
	getcwd(cur_path, MAXPATH);
	strcat(cur_path, dirname);
	int res = open(cur_path, O_MKDIR);
	if (res < 0) {
		return res;
	}
	close(res);
	return 0;
}
