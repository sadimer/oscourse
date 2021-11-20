/* implement fork from user space */

#include <inc/string.h>
#include <inc/lib.h>

/* User-level fork with copy-on-write.
 * Create a child.
 * Lazily copy our address space and page fault handler setup to the child.
 * Then mark the child as runnable and return.
 *
 * Returns: child's envid to the parent, 0 to the child, < 0 on error.
 * It is also OK to panic on error.
 *
 * Hint:
 *   Use sys_map_region, it can perform address space copying in one call
 *   Remember to fix "thisenv" in the child process.
 */
envid_t
fork(void) {
    // LAB 9: Your code here
	envid_t envid = sys_exofork();
    if (envid < 0)
        return envid;
    if (envid == 0) {
        thisenv = &envs[ENVX(sys_getenvid())];
        return 0;
    }
    if (sys_map_region(0, NULL, envid, NULL, MAX_USER_ADDRESS, PROT_ALL | PROT_LAZY | PROT_COMBINE) < 0) {
		return -1;
	}
    if (sys_env_set_pgfault_upcall(envid, thisenv->env_pgfault_upcall) < 0) {
		return -1;
	}
    if (sys_env_set_status(envid, ENV_RUNNABLE) < 0) {
		return -1;
	}
    return envid;
}

envid_t
sfork() {
    panic("sfork() is not implemented");
}
