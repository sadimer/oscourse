/* User virtual page table helpers */

#include <inc/lib.h>
#include <inc/mmu.h>

extern volatile pte_t uvpt[];     /* VA of "virtual page table" */
extern volatile pde_t uvpd[];     /* VA of current page directory */
extern volatile pdpe_t uvpdp[];   /* VA of current page directory pointer */
extern volatile pml4e_t uvpml4[]; /* VA of current page map level 4 */

pte_t
get_uvpt_entry(void *va) {
    if (!(uvpml4[VPML4(va)] & PTE_P)) return uvpml4[VPML4(va)];
    if (!(uvpdp[VPDP(va)] & PTE_P) || (uvpdp[VPDP(va)] & PTE_PS)) return uvpdp[VPDP(va)];
    if (!(uvpd[VPD(va)] & PTE_P) || (uvpd[VPD(va)] & PTE_PS)) return uvpd[VPD(va)];
    return uvpt[VPT(va)];
}

int
get_prot(void *va) {
    pte_t pte = get_uvpt_entry(va);
    int prot = pte & PTE_AVAIL & ~PTE_SHARE;
    if (pte & PTE_P) prot |= PROT_R;
    if (pte & PTE_W) prot |= PROT_W;
    if (!(pte & PTE_NX)) prot |= PROT_X;
    if (pte & PTE_SHARE) prot |= PROT_SHARE;
    return prot;
}

bool
is_page_dirty(void *va) {
    pte_t pte = get_uvpt_entry(va);
    return pte & PTE_D;
}

bool
is_page_present(void *va) {
    return get_uvpt_entry(va) & PTE_P;
}

int
foreach_shared_region(int (*fun)(void *start, void *end, void *arg), void *arg) {
    /* Calls fun() for every shared region */
    // LAB 11: Your code here
    for (uintptr_t i = 0; i < MAX_USER_ADDRESS; i += (1LL << PML4_SHIFT)) {
		if (uvpml4[VPML4(i)] & PTE_P) {
			for (uintptr_t j = i; j < i + (1LL << PML4_SHIFT); j += (1LL << PDP_SHIFT)) {
				if (uvpdp[VPDP(j)] & PTE_P) {
					for (uintptr_t k = j; k < j + (1LL << PDP_SHIFT); k += (1LL << PD_SHIFT)) {
						if (uvpd[VPD(k)] & PTE_P) {
							for (uintptr_t addr = k; addr < k + (1LL << PD_SHIFT); addr += (1LL << PT_SHIFT)) {
								if (uvpt[VPT(addr)] & PTE_P && uvpt[VPT(addr)] & PTE_SHARE) {
									fun((void*)addr, (void *)(addr + PAGE_SIZE), arg);
								}
							}
						}
					}
				}
			}
		}
	}
    return 0;
}
