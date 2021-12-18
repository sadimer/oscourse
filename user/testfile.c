#include <inc/lib.h>

const char *msg = "This is the NEW message of the day!\n\n";

#define FVA ((struct Fd *)0xA000000)

static int
xopen(const char *path, int mode) {
    extern union Fsipc fsipcbuf;
    envid_t fsenv;

    strcpy(fsipcbuf.open.req_path, path);
    fsipcbuf.open.req_omode = mode;

    fsenv = ipc_find_env(ENV_TYPE_FS);
    size_t sz = PAGE_SIZE;
    ipc_send(fsenv, FSREQ_OPEN, &fsipcbuf, sz, PROT_RW);
    return ipc_recv(NULL, FVA, &sz, NULL);
}

void
umain(int argc, char **argv) {
    int64_t r, f;
    struct Fd *fd;
    struct Fd fdcopy;
    struct Stat st;
    char buf[512];

    /* We open files manually first, to avoid the FD layer */
    if ((r = xopen("/not-found", O_RDONLY)) < 0 && r != -E_NOT_FOUND)
        panic("serve_open /not-found: %ld", (long)r);
    else if (r >= 0)
        panic("serve_open /not-found succeeded!");

    if ((r = xopen("/newmotd", O_RDONLY)) < 0)
        panic("serve_open /newmotd: %ld", (long)r);
    if (FVA->fd_dev_id != 'f' || FVA->fd_offset != 0 || FVA->fd_omode != O_RDONLY)
        panic("serve_open did not fill struct Fd correctly\n");
    cprintf("serve_open is good\n");

    if ((r = devfile.dev_stat(FVA, &st)) < 0)
        panic("file_stat: %ld", (long)r);
    if (strlen(msg) != st.st_size)
        panic("file_stat returned size %ld wanted %zd\n", (long)st.st_size, strlen(msg));
    cprintf("file_stat is good\n");

    memset(buf, 0, sizeof buf);
    if ((r = devfile.dev_read(FVA, buf, sizeof buf)) < 0)
        panic("file_read: %ld", (long)r);
    if (strcmp(buf, msg) != 0)
        panic("file_read returned wrong data");
    cprintf("file_read is good\n");

    if ((r = devfile.dev_close(FVA)) < 0)
        panic("file_close: %ld", (long)r);
    cprintf("file_close is good\n");

    /* We're about to unmap the FD, but still need a way to get
     * the stale filenum to serve_read, so we make a local copy.
     * The file server won't think it's stale until we unmap the
     * FD page. */
    fdcopy = *FVA;
    sys_unmap_region(0, FVA, PAGE_SIZE);

    if ((r = devfile.dev_read(&fdcopy, buf, sizeof buf)) != -E_INVAL)
        panic("serve_read does not handle stale fileids correctly: %ld", (long)r);
    cprintf("stale fileid is good\n");

    /* Try writing */
    if ((r = xopen("/new-file", O_RDWR | O_CREAT)) < 0)
        panic("serve_open /new-file: %ld", (long)r);

    if ((r = devfile.dev_write(FVA, msg, strlen(msg))) != strlen(msg))
        panic("file_write: %ld", (long)r);
    cprintf("file_write is good\n");

    FVA->fd_offset = 0;
    memset(buf, 0, sizeof buf);
    if ((r = devfile.dev_read(FVA, buf, sizeof buf)) < 0)
        panic("file_read after file_write: %ld", (long)r);
    if (r != strlen(msg))
        panic("file_read after file_write returned wrong length: %ld", (long)r);
    if (strcmp(buf, msg) != 0)
        panic("file_read after file_write returned wrong data");
    cprintf("file_read after file_write is good\n");

    /* Now we'll try out open */
    if ((r = open("/not-found", O_RDONLY)) < 0 && r != -E_NOT_FOUND)
        panic("open /not-found: %ld", (long)r);
    else if (r >= 0)
        panic("open /not-found succeeded!");

    if ((r = open("/newmotd", O_RDONLY)) < 0)
        panic("open /newmotd: %ld", (long)r);
    fd = (struct Fd *)(0xD0000000 + r * PAGE_SIZE);
    if (fd->fd_dev_id != 'f' || fd->fd_offset != 0 || fd->fd_omode != O_RDONLY)
        panic("open did not fill struct Fd correctly\n");
    cprintf("open is good\n");

    /* Try files with indirect blocks */
    if ((f = open("/big", O_WRONLY | O_CREAT)) < 0)
        panic("creat /big: %ld", (long)f);
    memset(buf, 0, sizeof(buf));
    for (int64_t i = 0; i < (NDIRECT * 3) * BLKSIZE; i += sizeof(buf)) {
        *(int *)buf = i;
        if ((r = write(f, buf, sizeof(buf))) < 0)
            panic("write /big@%ld: %ld", (long)i, (long)r);
    }
    close(f);

    if ((f = open("/big", O_RDONLY)) < 0)
        panic("open /big: %ld", (long)f);
    for (int64_t i = 0; i < (NDIRECT * 3) * BLKSIZE; i += sizeof(buf)) {
        *(int *)buf = i;
        if ((r = readn(f, buf, sizeof(buf))) < 0)
            panic("read /big@%ld: %ld", (long)i, (long)r);
        if (r != sizeof(buf))
            panic("read /big from %ld returned %ld < %d bytes",
                  (long)i, (long)r, (uint32_t)sizeof(buf));
        if (*(int *)buf != i)
            panic("read /big from %ld returned bad data %d",
                  (long)i, *(int *)buf);
    }
    close(f);
    cprintf("large file is good\n");
    
    /* Simple test - create dir, check that we can't exec, read and write to dir, add file to dir, 
       read and write to this file, remove dir, and check removing file */
    cprintf("creating /dir\n");
    if ((f = mkdir("/dir")) < 0)  {
		panic("creat /dir: %ld", (long)f);
	}
	if ((f = open("/dir", O_RDWR)) >= 0) {
		close(f);
        panic("open /dir on write: %ld", (long)f);
	}
	if ((f = open("/dir", O_WRONLY)) >= 0) {
		close(f);
        panic("open /dir on write: %ld", (long)f);
	}
	if ((f = open("/dir", O_RDONLY | O_SPAWN)) >= 0) {
		close(f);
        panic("open /dir to exec: %ld", (long)f);
	}
	cprintf("creating /dir is good\n");
	if ((f = open("/dir/file", O_RDWR | O_CREAT)) < 0) {
        panic("open /dir/file: %ld", (long)f);
	}
	memset(buf, 0, sizeof(buf));
	if ((r = write(f, buf, sizeof(buf))) < 0) {
		panic("write /dir/file %ld", (long)r);
	}
	if ((r = read(f, buf, sizeof(buf))) < 0) {
		panic("read /dir/file %ld", (long)r);
	}
	close(f);
	cprintf("operations with files in /dir is good\n");
	cprintf("removing /dir\n");
	if ((f = remove("/dir")) < 0)  {
		panic("remove /dir: %ld", (long)f);
	}
	if ((f = open("/dir", O_RDONLY) >= 0)) {
		close(f);
        panic("open removed /dir: %ld", (long)f);
	}
	close(f);
	if ((f = open("/dir/file", O_RDONLY) >= 0)) {
		close(f);
        panic("open removed /dir/file: %ld", (long)f);
	}
	close(f);
	cprintf("removing /dir is good\n");
    cprintf("dir simple test is good\n");
}	
