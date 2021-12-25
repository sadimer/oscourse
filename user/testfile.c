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
    
    
    if ((f = remove("/big")) < 0)  {
        cprintf("remove /file: %ld", (long)f);
        return;
    }
    
    if ((f = remove("/new-file")) < 0)  {
        cprintf("remove /new-file: %ld", (long)f);
        return;
    }
    
    char dir[32] = "/dir";
	char file[32] = "/file";
	char copy[32] = "/copy";
	char src[32] = "/";
	char dir_file[32] = "/dir/file";
	
    /* Simple test - create dir, check that we can't exec, read and write to dir. Add file to dir.
       Read and write to this file, remove dir, and check removing file. */
       
    cprintf("creating /dir\n");
    if ((f = mkdir(dir)) < 0)  {
		cprintf("creat /dir: %ld\n", (long)f);
		return;
	}
	if ((f = open(dir, O_WRONLY)) >= 0) {
		close(f);
        cprintf("open /dir on write: %ld\n", (long)f);
        return;
	}
	if ((f = open(dir, O_RDONLY | O_SPAWN)) >= 0) {
		close(f);
        cprintf("open /dir to exec: %ld\n", (long)f);
        return;
	}
	cprintf("creating /dir is good\n");
	if ((f = open(dir_file, O_RDWR | O_CREAT)) < 0) {
        cprintf("open /dir/file: %ld\n", (long)f);
        return;
	}
	memset(buf, 0, sizeof(buf));
	if ((r = write(f, buf, sizeof(buf))) < 0) {
		cprintf("write /dir/file %ld\n", (long)r);
		return;
	}
	if ((r = read(f, buf, sizeof(buf))) < 0) {
		cprintf("read /dir/file %ld\n", (long)r);
		return;
	}
	close(f);
	cprintf("operations with files in /dir is good\n");
	
	/* Simple test - change dir. Check pwd. */
	cprintf("change directory check\n");
	if ((r = chdir(dir, 0)) < 0) {
        cprintf("cd /dir %ld", (long)r);
        return;
    }
    memset(buf, 0, sizeof(buf));
    getcwd(buf, 512);
    char dir1[32] = "/dir/";
    if (strcmp(buf, dir1)) {
        cprintf("fail to get pwd of /dir %ld\n", (long)r);
        return;
    }
    if ((r = chdir(src, 0)) < 0) {
        cprintf("cd / %ld\n", (long)r);
        return;
    }
    cprintf("change directory check is good\n");
    
	cprintf("removing /dir\n");
	if ((f = remove(dir)) < 0)  {
		cprintf("remove /dir: %ld\n", (long)f);
		return;
	}
	if ((f = open(dir, O_RDONLY) >= 0)) {
		close(f);
        cprintf("open removed /dir: %ld\n", (long)f);
        return;
	}
	close(f);
	if ((f = open(dir_file, O_RDONLY) >= 0)) {
		close(f);
        cprintf("open removed /dir/file: %ld\n", (long)f);
        return;
	}
	close(f);
	cprintf("removing /dir is good\n");
    cprintf("dir simple test is good\n");

    /* Simple test - create file, write something into it. Creat
       symlink. Read file from symlink. Write something into symlink.
	   Check the source file and compare it.
       After that delete source file. And open symlink. */
       
    if ((f = open(file, O_WRONLY | O_CREAT)) < 0) {
        cprintf("open /file: %ld\n", (long)f);
        return;
    }
    char buf2_copy[32], buf2[32] = "Hello world";
    if ((r = write(f, buf2, sizeof(buf))) < 0) {
        cprintf("write /file %ld\n", (long)r);
        return;
    }
    close(f);
    cprintf("operations with files is good\n");
    
    cprintf("creating symlink\n");
    if ((f = symlink(copy, file)) < 0)  {
        cprintf("creat symlink /copy: %ld\n", (long)f);
        return;
    }
    cprintf("reading from symlink\n");
    if ((f = open(copy, O_RDONLY)) < 0) {
        cprintf("open /copy: %ld\n", (long)f);
        return;
    }
    if ((r = read(f, buf2_copy, sizeof(buf2))) < 0) {
        cprintf("read /copy %ld\n", (long) r);
        return;
    }
    if (strcmp(buf2_copy, buf2)) {
        cprintf("file is different\n");
        return;
    }
    cprintf("read from symlink /copy is good\n");
    close(f);
    
    if ((f = open(copy, O_WRONLY)) < 0) {
        cprintf("open /copy: %ld\n", (long)f);
        return;
    }
	char buf3_copy[32], buf3[32] = "World Hello";
    if ((r = write(f, buf3, sizeof(buf3))) < 0) {
        cprintf("write /copy %ld\n", (long) r);
        return;
    }
    close(f);
    if ((f = open(file, O_RDONLY)) < 0) {
        cprintf("open /file: %ld\n", (long)f);
        return;
    }
    if ((r = read(f, buf3_copy, sizeof(buf3_copy))) < 0) {
        cprintf("read /file %ld\n", (long) r);
        return;
    }
    close(f);
    if (strcmp(buf3_copy, buf3)){
        cprintf("file is different\n");
        return;
    }
    cprintf("write into symlink /copy is good\n");

    cprintf("test situation when source file is deleted\n");
    if ((f = remove(file)) < 0)  {
        cprintf("remove /file: %ld\n", (long)f);
        return;
    }
    if ((f = open(copy, O_RDONLY)) != -15) {
        cprintf("open /copy: %ld\n", (long) f);
        return;
    }
    if ((f = remove(copy)) < 0)  {
        cprintf("remove /copy: %ld\n", (long)f);
        return;
    }
    cprintf("open not existed file from symlink /copy is failed\n");
    cprintf("symlink test is good\n");

    /* Open file dev/stdout, dev/stderr, dev/stdin (with dup). Write something into it. Check results. */
    
    char stderr[32] = "/dev/stderr";
    char stdout[32] = "/dev/stdout";
    char stdin[32] = "/dev/stdin";
    
    char buf7[32] = "stdout available\n";
    
    if ((r = write(1, buf7, sizeof(buf7))) >= 0) {
		char buf4[32] = "stdout is good\n";
		if ((f = open(stdout, O_WRONLY)) < 0) {
			cprintf("open /dev/stdout: %ld\n", (long)f);
			return;
		}
		if ((r = write(f, buf4, sizeof(buf4))) < 0) {
			cprintf("write /dev/stdout %ld\n", (long)r);
			return;
		}
		char buf5[32] = "stderr is good\n";
		if ((f = open(stderr, O_WRONLY)) < 0) {
			cprintf("open /dev/stdout: %ld\n", (long)f);
			return;
		}
		if ((r = write(f, buf5, sizeof(buf5))) < 0) {
			cprintf("write /dev/stdout %ld\n", (long)r);
			return;
		}
		int reserv = 3;
		dup(0, reserv);
		dup(1, 0);
		char buf6[32] = "stdin is good\n";
		if ((f = open(stdin, O_WRONLY)) < 0) {
			cprintf("open /dev/stdin: %ld\n", (long)f);
			return;
		}
		if ((r = write(f, buf6, sizeof(buf6))) < 0) {
			cprintf("write /dev/stdin %ld\n", (long)r);
			return;
		}
		dup(reserv, 0);
		close(reserv);
	}
    
    /* Create dir and add symlink to dir. Check that we can cd, and cant read/write/exec */
    cprintf("creating /dir\n");
    if ((f = mkdir(dir)) < 0)  {
		cprintf("creat /dir: %ld\n", (long)f);
		return;
	}
	cprintf("creating symlink to /dir\n");
    if ((f = symlink(copy, dir)) < 0)  {
        cprintf("creat symlink /copy: %ld\n", (long)f);
        return;
    }
	if ((r = chdir(copy, 0)) < 0) {
        cprintf("cd /dir %ld\n", (long)r);
        return;
    }
    if ((r = chdir(src, 0)) < 0) {
        cprintf("cd / %ld\n", (long)r);
        return;
    }
	if ((f = open(copy, O_WRONLY)) >= 0) {
		close(f);
        cprintf("open /dir on write: %ld\n", (long)f);
        return;
	}
	if ((f = open(copy, O_RDONLY | O_SPAWN)) >= 0) {
		close(f);
        cprintf("open /dir to exec: %ld\n", (long)f);
        return;
	}
    if ((f = remove(copy)) < 0)  {
        cprintf("remove /file: %ld\n", (long)f);
        return;
    }
    if ((f = remove(dir)) < 0)  {
        cprintf("remove /file: %ld\n", (long)f);
        return;
    }
    cprintf("change directory by symlink check is good\n");
    
    /* Create file. Call chmod on this file. Call stat and checkout st_perm.
       Is it correct? */
    if ((f = open(file, O_RDWR | O_CREAT)) < 0) {
        cprintf("open /file: %ld\n", (long)f);
        return;
    }
    if ((r = chmod(file, 3)) < 0) {
        cprintf("chmod /file: %ld\n", (long)r);
        return;
    }
    if ((r = fstat(f, &st)) < 0) {
		cprintf("stat /file: %ld\n", (long)r);
		return;
	}
	if (st.st_perm != 3) {
		cprintf("wrong permissions in stat: %ld\n", (long)f);
		return;
	}
    if ((r = chmod(file, 0)) < 0) {
        cprintf("chmod /file: %ld\n", (long)r);
        return;
    }
    if ((r = fstat(f, &st)) < 0) {
		cprintf("stat /file: %ld\n", (long)r);
		return;
	}
	if (st.st_perm != 0) {
		cprintf("wrong permissions in stat: %ld\n", (long)f);
		return;
	}
    if ((f = open(file, O_RDONLY)) >= 0) {
        close(f);
        cprintf("open /file on read: %ld\n", (long)f);
        return;
    }
    if ((f = open(file, O_WRONLY)) >= 0) {
        close(f);
        cprintf("open /file on write: %ld\n", (long)f);
        return;
    }
    if ((f = open(file, O_RDONLY | O_SPAWN)) >= 0) {
        close(f);
        cprintf("open /file to exec: %ld\n", (long)f);
        return;
    }
    if ((f = remove(file)) < 0)  {
        cprintf("remove /file: %ld\n", (long)f);
        return;
    }
    cprintf("chmod test is good\n");
    cprintf("all tests passed!\n");
}
