#include <inc/fs.h>
#include <inc/string.h>
#include <inc/lib.h>

union Fsipc fsipcbuf __attribute__((aligned(PAGE_SIZE)));


char tokens[MAXPATHLEN][MAXNAMELEN] = {0};
/* Send an inter-environment request to the file server, and wait for
 * a reply.  The request body should be in fsipcbuf, and parts of the
 * response may be written back to fsipcbuf.
 * type: request code, passed as the simple integer IPC value.
 * dstva: virtual address at which to receive reply page, 0 if none.
 * Returns result from the file server. */
static int
fsipc(unsigned type, void *dstva) {
    static envid_t fsenv;

    if (!fsenv) fsenv = ipc_find_env(ENV_TYPE_FS);

    static_assert(sizeof(fsipcbuf) == PAGE_SIZE, "Invalid fsipcbuf size");

    if (debug) {
        cprintf("[%08x] fsipc %d %08x\n",
                thisenv->env_id, type, *(uint32_t *)&fsipcbuf);
    }

    ipc_send(fsenv, type, &fsipcbuf, PAGE_SIZE, PROT_RW);
    size_t maxsz = PAGE_SIZE;
    return ipc_recv(NULL, dstva, &maxsz, NULL);
}

static int devfile_flush(struct Fd *fd);
static ssize_t devfile_read(struct Fd *fd, void *buf, size_t n);
static ssize_t devfile_write(struct Fd *fd, const void *buf, size_t n);
static int devfile_stat(struct Fd *fd, struct Stat *stat);
static int devfile_trunc(struct Fd *fd, off_t newsize);

struct Dev devfile = {
        .dev_id = 'f',
        .dev_name = "file",
        .dev_read = devfile_read,
        .dev_close = devfile_flush,
        .dev_stat = devfile_stat,
        .dev_write = devfile_write,
        .dev_trunc = devfile_trunc};

int
skip_dots(char *new, const char *path) {
	int len = strlen(path);
	for (int i = 0, j = 0; i < len; i++) {
		if (path[i] == '/' && path[i + 1] == '.' && i == len - 2) {
			return 0;
		}
		if (path[i] == '/' && path[i + 1] == '.' && path[i + 2] == '/') {
			new[j] = '/';
			j++;
			i = i + 1;
			continue;
		}
		new[j] = path[i];
		j++;
	}
	return 0;
}

int
skip_doubledots(char *new, const char *path) {
	int len = strlen(path);
	char tmp[MAXPATHLEN] = {0};
	strcpy(tmp, path);
	int skip = 0;
	for (int i = len - 1; i >= 0; i--) {
		if (tmp[i] == '.' && tmp[i - 1] == '.' && tmp[i - 2] == '/') {
			skip = 1;
			tmp[i] = '#';
			tmp[i - 1] = '#';
			i = i - 1;
			continue;
		}
		if (tmp[i] == '/' && tmp[i - 1] == '.' && tmp[i - 2] == '.' && tmp[i - 3] == '/') {
			skip++;
			tmp[i - 2] = '#';
			tmp[i - 1] = '#';
			i = i - 2;
			continue;
		}
		if (tmp[i] == '/' && skip > 0) {
			if (i == 0) {
				break;
			}
			i--;
			while (tmp[i] != '/') {
				tmp[i] = '#';
				i--;
			}
			i++;
			skip--;
		}
	}
	for (int i = 0, j = 0; i < len; i++) { 
		if (tmp[i] != '#') {
			new[j] = tmp[i];
			j++;
		}
	}
	memset(tmp, 0, MAXPATHLEN);
	beauty_path(tmp, new);
	strcpy(new, tmp);
	return 0;
}


int 
beauty_path(char *new, const char *path) {
	char tmp[MAXPATHLEN] = {0};
	skip_dots(tmp, path);
	int len = strlen(tmp);
	for (int i = 0, j = 0; i < len; i++) {
		if (tmp[i] == '/') {
			new[j] = tmp[i];
			j++;
		}
		while (tmp[i] == '/') {
			i++;
		}
		new[j] = tmp[i];
		j++;
	}
	return 0;
}

/* Open a file (or directory).
 *
 * Returns:
 *  The file descriptor index on success
 *  -E_BAD_PATH if the path is too long (>= MAXPATHLEN)
 *  < 0 for other errors. */
int
open(const char *path, int mode) {
    /* Find an unused file descriptor page using fd_alloc.
     * Then send a file-open request to the file server.
     * Include 'path' and 'omode' in request,
     * and map the returned file descriptor page
     * at the appropriate fd address.
     * FSREQ_OPEN returns 0 on success, < 0 on failure.
     *
     * (fd_alloc does not allocate a page, it just returns an
     * unused fd address.  Do you need to allocate a page?)
     *
     * Return the file descriptor index.
     * If any step after fd_alloc fails, use fd_close to free the
     * file descriptor. */

    int res;
    struct Fd *fd;

    if (strlen(path) >= MAXPATHLEN) {
        return -E_BAD_PATH;
	}
	
	if (*strfind(path, '#') != '\0') {
		cprintf("please don't use # in filenames\n");
		return -E_BAD_PATH;
	}
	
    if ((res = fd_alloc(&fd)) < 0) return res;
	
	char cur_path[MAXPATHLEN] = {0};
	char new[MAXPATHLEN] = {0};
    char tmp[MAXPATHLEN] = {0};
    
    if (path[0] != '/') {
		getcwd(cur_path, MAXPATHLEN);
		strcat(cur_path, path);
	} else {
		strcat(cur_path, path);
	}
	
	beauty_path(new, cur_path);
	
	skip_doubledots(tmp, new);
	strcpy(new, tmp);
	
    strcpy(fsipcbuf.open.req_path, new);
    fsipcbuf.open.req_omode = mode;
    
    if ((res = fsipc(FSREQ_OPEN, fd)) < 0) {
        fd_close(fd, 0);
        return res;
    }
    if (!strcmp(new, "/dev/stdin")) {
		if (mode & O_SPAWN) {
			cprintf("don't try to exec device files\n");
			return -E_INVAL;
		}
		fd_close(fd, 0);
		return 0;
	} else if (!strcmp(new, "/dev/stdout")) {
		if (mode & O_SPAWN) {
			cprintf("don't try to exec device files\n");
			return -E_INVAL;
		}
		fd_close(fd, 0);
		return 1;
	} else if (!strcmp(new, "/dev/stderr")) {
		if (mode & O_SPAWN) {
			cprintf("don't try to exec device files\n");
			return -E_INVAL;
		}
		fd_close(fd, 0);
		return 2;
	}
	
    return fd2num(fd);
}

/* Flush the file descriptor.  After this the fileid is invalid.
 *
 * This function is called by fd_close.  fd_close will take care of
 * unmapping the FD page from this environment.  Since the server uses
 * the reference counts on the FD pages to detect which files are
 * open, unmapping it is enough to free up server-side resources.
 * Other than that, we just have to make sure our changes are flushed
 * to disk. */
static int
devfile_flush(struct Fd *fd) {
    fsipcbuf.flush.req_fileid = fd->fd_file.id;
    return fsipc(FSREQ_FLUSH, NULL);
}

/* Read at most 'n' bytes from 'fd' at the current position into 'buf'.
 *
 * Returns:
 *  The number of bytes successfully read.
 *  < 0 on error. */
static ssize_t
devfile_read(struct Fd *fd, void *buf, size_t n) {
    /* Make an FSREQ_READ request to the file system server after
   * filling fsipcbuf.read with the request arguments.  The
   * bytes read will be written back to fsipcbuf by the file
   * system server. */
   
    // LAB 10: Your code here:
    if (!fd || !buf)
        return E_INVAL;

    size_t res0 = 0;
    while (n) {
        fsipcbuf.read.req_fileid = fd->fd_file.id;
        fsipcbuf.read.req_n      = n;

        int res = fsipc(FSREQ_READ, NULL);
        if (res <= 0)
            return res ? res : res0;
        memcpy(buf, fsipcbuf.readRet.ret_buf, res);

        buf += res;
        n -= res;
        res0 += res;
    }

    return res0;
}

/* Write at most 'n' bytes from 'buf' to 'fd' at the current seek position.
 *
 * Returns:
 *   The number of bytes successfully written.
 *   < 0 on error. */
static ssize_t
devfile_write(struct Fd *fd, const void *buf, size_t n) {
    /* Make an FSREQ_WRITE request to the file system server.  Be
	* careful: fsipcbuf.write.req_buf is only so large, but
	* remember that write is always allowed to write *fewer*
	* bytes than requested. */
	// LAB 10: Your code here:
    if (!fd || !buf)
        return E_INVAL;

    size_t res0 = 0;

    while (n) {
        size_t blk = MIN(n, sizeof(fsipcbuf.write.req_buf));

        memcpy(fsipcbuf.write.req_buf, buf, blk);
        fsipcbuf.write.req_fileid = fd->fd_file.id;
        fsipcbuf.write.req_n      = blk;
        int res = fsipc(FSREQ_WRITE, NULL);
        if (res < 0)
            return res;
        buf += res;
        n -= res;
        res0 += res;
    }

    return res0;
}

/* Get file information */
static int
devfile_stat(struct Fd *fd, struct Stat *st) {
    fsipcbuf.stat.req_fileid = fd->fd_file.id;
    int res = fsipc(FSREQ_STAT, NULL);
    if (res < 0) return res;

    strcpy(st->st_name, fsipcbuf.statRet.ret_name);
    st->st_size = fsipcbuf.statRet.ret_size;
    st->st_isdir = fsipcbuf.statRet.ret_isdir;
    st->st_perm = fsipcbuf.statRet.ret_perm;
    st->st_issym = fsipcbuf.statRet.ret_issym;

    return 0;
}

/* Truncate or extend an open file to 'size' bytes */
static int
devfile_trunc(struct Fd *fd, off_t newsize) {
    fsipcbuf.set_size.req_fileid = fd->fd_file.id;
    fsipcbuf.set_size.req_size = newsize;

    return fsipc(FSREQ_SET_SIZE, NULL);
}

/* Synchronize disk with buffer cache */
int
sync(void) {
    /* Ask the file server to update the disk
     * by writing any dirty blocks in the buffer cache. */

    return fsipc(FSREQ_SYNC, NULL);
}

int
chmod(const char *path, int perm) {
	char cur_path[MAXPATHLEN] = {0};
	if (path[0] != '/') {
		getcwd(cur_path, MAXPATHLEN);
		strcat(cur_path, path);
	} else {
		strcat(cur_path, path);
	}
	int res = open(cur_path, O_CHMOD | (perm << 0x4));
	if (res < 0) {
		return res;
	}
	close(res);
	return 0;
}

int
remove(const char *path) {
	char cur_path[MAXPATHLEN] = {0};
	if (path[0] != '/') {
		getcwd(cur_path, MAXPATHLEN);
		strcat(cur_path, path);
	} else {
		strcat(cur_path, path);
	}
	char new[MAXPATHLEN] = {0};
	char tmp[MAXPATHLEN] = {0};
	beauty_path(new, cur_path);
	skip_doubledots(tmp, new);
	strcpy(fsipcbuf.remove.req_path, tmp);
	int res = fsipc(FSREQ_REMOVE, NULL);
	if (res < 0) {
		return res;
	}
	return 0;
}

int 
symlink(const char *symlink_path, const char *path) {
	char cur_path[MAXPATHLEN] = {0};
	if (path[0] != '/') {
		getcwd(cur_path, MAXPATHLEN);
		strcat(cur_path, path);
	} else {
		strcat(cur_path, path);
	}
	char symlink_cur_path[MAXPATHLEN] = {0};
	if (symlink_path[0] != '/') {
		getcwd(symlink_cur_path, MAXPATHLEN);
		strcat(symlink_cur_path, symlink_path);
	} else {
		strcat(symlink_cur_path, symlink_path);
	}
	int fd = open(symlink_cur_path, O_MKLINK | O_WRONLY | O_SYSTEM | O_EXCL);
	if (fd < 0) {
		return fd;
	}
	int res = write(fd, cur_path, sizeof(cur_path));
	if (res != sizeof(cur_path)) {
		return res;
	}
	close(fd);
	return 0;
}
