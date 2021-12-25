#include <inc/lib.h>

int flag[256];

void lsdir(const char *, const char *);
void ls1(const char *, bool, off_t, const char *, uint8_t, bool);

void
ls(const char *path, const char *prefix) {
    int r;
    struct Stat st;
	char new[MAXPATHLEN] = {0};
	char new_prefix[MAXPATHLEN] = {0};
	beauty_path(new, path);
	beauty_path(new_prefix, prefix);
    if ((r = stat(new, &st)) < 0)
        printf("stat %s: %i\n", new, r);
    if (st.st_isdir && !flag['d'])
        lsdir(new, new_prefix);
    else
        ls1(0, st.st_isdir, st.st_size, new, st.st_perm, st.st_issym);
}

void
lsdir(const char *path, const char *prefix) {
    int fd, n;
    struct File f;

    if ((fd = open(path, O_RDWR)) < 0)
        printf("open %s: %i\n", path, fd);
    while ((n = readn(fd, &f, sizeof f)) == sizeof f)
        if (f.f_name[0])
            ls1(prefix, f.f_type == FTYPE_DIR, f.f_size, f.f_name, f.f_perm, f.f_type == FTYPE_LINK);
    if (n > 0)
        printf("short read in directory %s\n", path);
    if (n < 0)
        printf("error reading directory %s: %i\n", path, n);
}


void
ls1(const char *prefix, bool isdir, off_t size, const char *name, uint8_t perm, bool issym) {
    const char *sep;

    if (flag['l'])
        printf("%11d %c%c %c%c%c ", size, isdir ? 'd' : '-', issym ? 's' : '-', perm & 4 ? 'r' : '-', perm & 2 ? 'w' : '-', perm & 1 ? 'x' : '-');
    if (prefix) {
        if (prefix[0] && prefix[strlen(prefix) - 1] != '/')
            sep = "/";
        else
            sep = "";
        printf("%s%s", prefix, sep);
    }
    printf("%s", name);
    if (flag['F'] && isdir)
        printf("/");
    printf("\n");
}

void
usage(void) {
    printf("usage: ls [-dFl] [file...]\n");
    exit();
}

void
umain(int argc, char **argv) {
    int i;
    struct Argstate args;

    argstart(&argc, argv, &args);
    while ((i = argnext(&args)) >= 0)
        switch (i) {
        case 'd':
        case 'F':
        case 'l':
            flag[i]++;
            break;
        default:
            usage();
        }

    if (argc == 1) {
		char path[MAXPATHLEN];
		getcwd(path, MAXPATHLEN);
        ls(path, "");
	}
    else {
        for (i = 1; i < argc; i++)
            ls(argv[i], argv[i]);
    }
}
