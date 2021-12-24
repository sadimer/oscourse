#include <inc/string.h>
#include <inc/lib.h>

char 
*getcwd(char *buffer, int maxlen) {
	if (!buffer || maxlen < 0) {
		return (char *)thisenv->workpath;
	}
	return strncpy((char *)buffer, (const char *)thisenv->workpath, maxlen);
}

int 
chdir(const char *path, int mode) {
	char cur_path[MAXPATHLEN] = {0};
    if (path[0] != '/') {
		getcwd(cur_path, MAXPATHLEN);
		strcat(cur_path, path);
	} else {
		strcat(cur_path, path);
	}
	struct Stat st;
	int res = stat(cur_path, &st);
	if (res < 0) {
		return res;
	}
	if (!st.st_isdir) {
		return -E_INVAL;
	}
	char new[MAXPATHLEN] = {0};
	char tmp[MAXPATHLEN] = {0};
	beauty_path(new, cur_path);
	skip_doubledots(tmp, new);
	strcpy(cur_path, tmp);
	if (cur_path[strlen(cur_path) - 1] != '/') {
		strcat((char *)cur_path, "/");
	}
	return sys_env_set_workpath(thisenv->env_id, cur_path);
}


int 
mkdir(const char *dirname) {
	char cur_path[MAXPATHLEN] = {0};
	if (dirname[0] != '/') {
		getcwd(cur_path, MAXPATHLEN);
		strcat(cur_path, dirname);
	} else {
		strcat(cur_path, dirname);
	}
	int res = open(cur_path, O_MKDIR | O_SYSTEM);
	if (res < 0) {
		return res;
	}
	close(res);
	return 0;
}
