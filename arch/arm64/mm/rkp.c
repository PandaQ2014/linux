#include <asm/rkp.h>
#include <linux/cache.h>
#include <linux/export.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/kexec.h>
#include <linux/libfdt.h>
#include <linux/mman.h>
#include <linux/nodemask.h>
#include <linux/memblock.h>
#include <linux/spinlock_types.h>
#include <linux/arm-smccc.h>
#include <linux/module.h>
#include <linux/mm_types.h>
#include <asm/pgtable-prot.h>
static int INITED = 0;
static DEFINE_SPINLOCK(xchg_spin_lock);
static DEFINE_SPINLOCK(ptmanager_spin_lock);
static DEFINE_SPINLOCK(cfu_patch_spin_lock);
phys_addr_t POOLSTART = 0; //初始值保证rkp_paIsManaged宏在RKP未初始化前始终返回false
phys_addr_t POOLEND = 0;

phys_addr_t rkp_allocPageTable(void){
    struct arm_smccc_res res;
    if(INITED == 0){
        POOLSTART = memblock_phys_alloc(POOLSIZE*PAGE_SIZE+POOLSIZE*sizeof(unsigned int), PAGE_SIZE);
        memblock_reserve(POOLSTART,POOLSIZE*PAGE_SIZE+POOLSIZE*sizeof(unsigned int));
        memset(&res, 0, sizeof(res));
        arm_smccc_smc(TEESMC_OPTEED_RKP_PTM_INIT, POOLSTART, POOLSIZE, 0, 0, 0, 0, 0, &res);
        if(res.a0 != 0){
            pr_err("TEESMC_OPTEED_RKP_PTM_INIT failed");
            return 0;
        }
        POOLEND = POOLSTART + POOLSIZE*PAGE_SIZE;
        pr_err("PTM start: 0x%016llx end: 0x%016llx, tzc end: 0x%016lx",POOLSTART,POOLEND, POOLEND+POOLSIZE*sizeof(unsigned int)-1);
        INITED = 1;
    }

    if(PTMAPED == 0){
        arm_smccc_smc(TEESMC_OPTEED_RKP_PTM_GETAPT, 0, 0, 0, 0, 0, 0, 0, &res);
    }else{
        spin_lock(&ptmanager_spin_lock);
        arm_smccc_smc(TEESMC_OPTEED_RKP_PTM_GETAPT, 0, 0, 0, 0, 0, 0, 0, &res);
        spin_unlock(&ptmanager_spin_lock);
    }
   
    if(res.a0 != 0){
        pr_err("TEESMC_OPTEED_RKP_PTM_GETAPT failed");
        return 0;
    }
    return res.a1;
}
void rkp_releasePageTable(phys_addr_t target){
    struct arm_smccc_res res;
    // memset(&res, 0, sizeof(res));
    if(PTMAPED == 0){
        arm_smccc_smc(TEESMC_OPTEED_RKP_PTM_RELEASEAPT, target, 0, 0, 0, 0, 0, 0, &res);
    }else{
        spin_lock(&ptmanager_spin_lock);
        arm_smccc_smc(TEESMC_OPTEED_RKP_PTM_RELEASEAPT, target, 0, 0, 0, 0, 0, 0, &res);
        spin_unlock(&ptmanager_spin_lock);
    }
    // if(res.a0 != 0){
    //     pr_err("TEESMC_OPTEED_RKP_PTM_RELEASEAPT failed");
    //     return;
    // }
    return;
}

void rkp_setPageTableElementWithPa(int pageTableType,phys_addr_t pa, unsigned long content){
    struct arm_smccc_res res;
    //memset(&res, 0, sizeof(res));
    //pr_err("set pt on 0x%016llx!!!!!!!",pa);
    arm_smccc_smc(TEESMC_OPTEED_RKP_SET_PAGETABLE, pageTableType, pa, content, 0, 0, 0, 0, &res);
    dsb(ishst);
    // if(res.a0 != 0){
    //     pr_err("TEESMC_OPTEED_RKP_SET_PAGETABLE failed");
    //     return;
    // }
    return;
}

void rkp_setPageTableElement(int pageTableType,unsigned long *va, unsigned long content){
    struct arm_smccc_res res;
    unsigned long long pa = virt_to_phys(va);
    // pr_info("rkp_setPageTableElement | va: 0x%016llx, *va: 0x%016llx,pa: 0x%016llx", va, *va, pa);
    if(!rkp_paIsManaged(pa)){
        WRITE_ONCE(*va,content);
        dsb(ishst);
        return;
    }
    //memset(&res, 0, sizeof(res));
    arm_smccc_smc(TEESMC_OPTEED_RKP_SET_PAGETABLE, pageTableType, pa, content, 0, 0, 0, 0, &res);
    dsb(ishst);
    // if(res.a0 != 0){
    //     pr_err("TEESMC_OPTEED_RKP_SET_PAGETABLE failed");
    //     return;
    // }
    return;
}

void rkp_instruction_simulation(unsigned long instr_mark,unsigned long param2,
                                unsigned long param3,unsigned long param4){
    struct arm_smccc_res res;
    // memset(&res, 0, sizeof(res));
    arm_smccc_smc(TEESMC_OPTEED_RKP_INSTR_SIMULATION, instr_mark, param2, param3, param4, 0, 0, 0, &res);
    // if(res.a0 != 0){
    //     pr_err("TEESMC_OPTEED_RKP_INSTR_SIMULATION failed");
    //     return;
    // }
    return;    

}
unsigned long rkp_xchg_relaxed(int ptType,unsigned long* ptePtr, unsigned long newVal){
    unsigned long oldVal = 0;
    spin_lock(&xchg_spin_lock);
    oldVal = *ptePtr;
    rkp_setPageTableElement(ptType,ptePtr,newVal);
    spin_unlock(&xchg_spin_lock);
    return oldVal;
}
unsigned long rkp_cmpxchg_relaxed(int ptType,unsigned long* ptePtr, unsigned long oldVal, unsigned long newVal){
    spin_lock(&xchg_spin_lock);
    if(oldVal == *ptePtr){
        rkp_setPageTableElement(ptType,ptePtr,newVal);
    }else{
        oldVal = *ptePtr;
    }
    spin_unlock(&xchg_spin_lock);
    return oldVal;
}
int PTMAPED = 0;
int rkp_iscross_with_ptm(phys_addr_t start, phys_addr_t end){
    phys_addr_t left = start > POOLSTART ? start : POOLSTART;
    phys_addr_t right = end < POOLEND ? end : POOLEND;
    return right>left;
}
void rkp_set_PTMAPED(void){
    unsigned long long * ptr = (phys_to_virt(0x00000008feffe000));
    PTMAPED = 1;
    pr_err("0x00000008feffe000 : 0x%016llx",*ptr);
}
void rkp_clear_page(void * kaddr){
    struct arm_smccc_res res;
    unsigned long long pa = virt_to_phys(kaddr);
    //memset(&res, 0, sizeof(res));
    arm_smccc_smc(TEESMC_OPTEED_RKP_CLEAR_PAGE, pa, 0, 0, 0, 0, 0, 0, &res);
    return;
}
unsigned long rkp_copy_page(void *kto,const void*kfrom, unsigned long n){
    struct arm_smccc_res res;
    unsigned long long pa_to;
    unsigned long long pa_from;
    pa_to = virt_to_phys(kto);
    pa_from = virt_to_phys(kfrom);
    //memset(&res, 0, sizeof(res));
    arm_smccc_smc(TEESMC_OPTEED_RKP_COPY_PAGE, pa_to, pa_from, n, 0, 0, 0, 0, &res);
    return res.a1;
}
int rkp_pa_is_managed(phys_addr_t pa)
{
	return ((phys_addr_t)pa > POOLSTART-1 && (phys_addr_t)pa < POOLEND);
}
void* rkp_mem_set(void * buffer, int c, unsigned long n){
    struct arm_smccc_res res;
    unsigned long pa_buffer = (unsigned long) virt_to_phys(buffer);
    arm_smccc_smc(TEESMC_OPTEED_RKP_MEM_SET, pa_buffer, c, n, 0, 0, 0, 0, &res);
    return (void *)res.a1;
}
void rkp_copy_from_user_patch_on(){
     struct arm_smccc_res res;
    spin_lock(&cfu_patch_spin_lock);
    arm_smccc_smc(TEESMC_OPTEED_RKP_CFU_PATCH, 1, 0, 0, 0, 0, 0, 0, &res);
    spin_unlock(&cfu_patch_spin_lock);
}

void rkp_copy_from_user_patch_off(){
    struct arm_smccc_res res;
    spin_lock(&cfu_patch_spin_lock);
    arm_smccc_smc(TEESMC_OPTEED_RKP_CFU_PATCH, 0, 0, 0, 0, 0, 0, 0, &res);
    spin_unlock(&cfu_patch_spin_lock);
}

// static int virt_dll_to_phys(void *p, paddr_t_dll *pa)
// {
// 	unsigned long va = (unsigned long)p;
// 	struct mm_struct *mm;
// 	pgd_t *pgd;
// 	p4d_t *p4d;
// 	pud_t *pud;
// 	pmd_t *pmd;
// 	pte_t *pte;

// 	//   user va: 0x0000 ~~~
// 	// kernel va: 0xffff ~~~
// 	if (va >> 48) {
// 		mm = &init_mm;
// 	} else {
// 		mm = current->mm;
// 	}

// 	pgd = pgd_offset(mm, va);
// 	if (pgd_none(*pgd)) {
// 		goto error;
// 	}

// 	p4d = p4d_offset(pgd, va);
// 	if (p4d_none(*p4d)) {
// 		goto error;
// 	}

// 	pud = pud_offset(p4d, va);
// 	if (pud_none(*pud)) {
// 		goto error;
// 	}

// 	pmd = pmd_offset(pud, va);
// 	if (pmd_none(*pmd)) {
// 		goto error;
// 	}

// 	pte = pte_offset_kernel(pmd, va);
// 	if (pte_none(*pte)) {
// 		goto error;
// 	}

// 	if (!pte_present(*pte)) {
// 		pr_dll_info("pte is not in RAM.\n");
// 		goto error;
// 	}

// 	*pa = (paddr_t_dll)(((pte_val(*pte) & PAGE_MASK) | (va & ~PAGE_MASK)) &
// 			    ARM64_DLL_PHYS_ADDR_MASK); // 0x0000ffff ffffffff
// 	return 0;

// error:
// 	pr_dll_info("error va: 0x%016lx\n", va);
// 	return -1;
// }

static int virt_dll_to_phys(void *p, unsigned long *pa, pte_t **pte_in)
{
	unsigned long va = (unsigned long)p;
	struct mm_struct *mm;
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;

	//   user va: 0x0000 ~~~
	// kernel va: 0xffff ~~~
	if (va >> 48) {
		mm = &init_mm;
	} else {
		mm = current->mm;
	}

	pgd = pgd_offset(mm, va);
	if (pgd_none(*pgd)) {
		goto error;
	}

	p4d = p4d_offset(pgd, va);
	if (p4d_none(*p4d)) {
		goto error;
	}

	pud = pud_offset(p4d, va);
	if (pud_none(*pud)) {
		goto error;
	}

	pmd = pmd_offset(pud, va);
	if (pmd_none(*pmd)) {
		goto error;
	}

	pte = pte_offset_kernel(pmd, va);
	if (pte_none(*pte)) {
		goto error;
	}
    // pr_info("virt_dll_to_phys | pte: 0x%016llx", pte);

	if (!pte_present(*pte)) {
		pr_err("pte is not in RAM.\n");
		goto error;
	}

	*pa = (unsigned long)(((pte_val(*pte) & PAGE_MASK) | (va & ~PAGE_MASK)) &
			    0x0000ffffffffffff); // 0x0000ffff ffffffff
    *pte_in = pte;
    // pr_info("virt_dll_to_phys | pte_in: 0x%016llx", pte_in);
	return 0;

error:
	pr_err("error va: 0x%016lx\n", va);
	return -1;
}

unsigned long rkp_patch_arch_copy_from_user_impl(unsigned long (*cfu)(void *to, const void __user *from, unsigned long n), void *to, const void __user *from, unsigned long n) {
    unsigned long res;
	unsigned long to_phy_addr;
    pte_t *to_pte_entry;
    pteval_t to_pte_val;

    unsigned long ton_phy_addr;
    pte_t *ton_pte_entry;

    bool isLeapPage = false;

    // pr_info("rkp_patch_arch_copy_from_user | 0");

    rkp_copy_from_user_patch_on();

    //to - to+n 跨了几页
    // pr_info("rkp_patch_arch_copy_from_user | 1");
    if(virt_dll_to_phys(to, &to_phy_addr, &to_pte_entry) == 0) {
        // pr_info("rkp_patch_arch_copy_from_user | 2");
        to_pte_val = pte_val(*to_pte_entry);
        pr_info("rkp_patch_arch_copy_from_user | to va: 0x%016lx, pa: 0x%016lx, entry:0x%016lx, pte: 0x%016lx n:%d", (unsigned long) to, to_phy_addr, to_pte_entry, to_pte_val,n);
        virt_dll_to_phys(to+n, &ton_phy_addr, &ton_pte_entry);
        pr_info("rkp_patch_arch_copy_from_user | to+n va: 0x%016lx, pa: 0x%016lx, entry:0x%016lx, n:%d", (unsigned long) (to+n), ton_phy_addr, ton_pte_entry, n);
        if(to_pte_entry == ton_pte_entry) {
            isLeapPage = false;
            pr_err("page still!");
        } else {
            isLeapPage = true;
            pr_err("leap page!");
        }
    } else {
        pr_err("rkp_patch_arch_copy_from_user | error va");
    }
    // pr_info("rkp_patch_arch_copy_from_user | 3");

    if (isLeapPage) {
        pte_t *ptep = to_pte_entry;
        int i = 0;
        while(ptep <= ton_pte_entry) {
            pr_info("rkp_patch_arch_copy_from_user | i: %d", i);
            rkp_setPageTableElement(RKP_PTE, (phys_addr_t) ptep, (pte_val(*to_pte_entry) & ~PTE_RDONLY) |  PTE_WRITE);
            pr_info("rkp_patch_arch_copy_from_user | modified pte: 0x%016lx", pte_val(*ptep));
            ptep++;
            i++;
        }
    } else {
        rkp_setPageTableElement(RKP_PTE, (phys_addr_t) to_pte_entry, (pte_val(*to_pte_entry) & ~PTE_RDONLY) |  PTE_WRITE);
        pr_info("rkp_patch_arch_copy_from_user | modified pte: 0x%016lx", pte_val(*to_pte_entry));
    }
    
    //查虚拟地址to的页表项，把只读标志位置位可写
    // rkp_setPageTableElement(RKP_PTE, (phys_addr_t) to_pte_entry, (pte_val(*to_pte_entry) & ~PTE_RDONLY) |  PTE_WRITE);
    // pr_info("rkp_patch_arch_copy_from_user | modified pte: 0x%016lx", pte_val(*to_pte_entry));
    // rkp_setPageTableElement(RKP_PTE, (phys_addr_t) to_pte_entry, *(unsigned long *)&pfn_pte(__phys_to_pfn(to_phy_addr), PAGE_KERNEL) );
    // set_pte(to_pte_entry,  pfn_pte(__phys_to_pfn(to_phy_addr), PAGE_KERNEL));
    // pr_info("rkp_patch_arch_copy_from_user | 4");

    // WRITE_ONCE(*(unsigned long *)to, 15UL);
    // unsigned long test;
    // test =  READ_ONCE(*(unsigned long *)to);
    // pr_info("rkp_patch_arch_copy_from_user | test: 0x%016lx", test);

	res = cfu((to), from, (n));
    pr_info("rkp_patch_arch_copy_from_user | res: %d", res);

    // pr_info("rkp_patch_arch_copy_from_user | 5");

	//恢复
    // set_pte(to_pte_entry,  pfn_pte(__phys_to_pfn(to_phy_addr), __pgprot((pgprot_val(PAGE_KERNEL)&~PTE_WRITE) | pgprot_val(PAGE_KERNEL_RO))));
    // rkp_setPageTableElement(RKP_PTE, (phys_addr_t) to_pte_entry, to_pte_val);
    // pr_info("rkp_patch_arch_copy_from_user | recovered pte: 0x%016lx", pte_val(*to_pte_entry));

    if (isLeapPage) {
        pte_t *ptep = to_pte_entry;
        int i = 0;
        while(ptep <= ton_pte_entry) {
            pr_info("rkp_patch_arch_copy_from_user | i: %d", i);
            rkp_setPageTableElement(RKP_PTE, (phys_addr_t) ptep, to_pte_val);
            pr_info("rkp_patch_arch_copy_from_user | recovered pte: 0x%016lx", pte_val(*ptep));
            ptep++;
            i++;
        }
    } else {
        rkp_setPageTableElement(RKP_PTE, (phys_addr_t) to_pte_entry, to_pte_val);
        pr_info("rkp_patch_arch_copy_from_user | recovered pte: 0x%016lx", pte_val(*to_pte_entry));
    }

	rkp_copy_from_user_patch_off();

    // WRITE_ONCE(*(unsigned long *)to, 15UL);
    // unsigned long test;
    // test =  READ_ONCE(*(unsigned long *)to);
    // pr_info("rkp_patch_arch_copy_from_user | test: 0x%016lx", test);

    // pr_info("rkp_patch_arch_copy_from_user | 6");

    return res;
}

unsigned long  rkp_patch_arch_copy_from_user(unsigned long (*cfu)(void *to, const void __user *from, unsigned long n), void *to, const void __user *from, unsigned long n) {
    spin_lock(&cfu_patch_spin_lock);
    unsigned long res = rkp_patch_arch_copy_from_user_impl(cfu, to, from ,n);
    spin_unlock(&cfu_patch_spin_lock);
    return res;
}

EXPORT_SYMBOL(rkp_copy_page);
EXPORT_SYMBOL(rkp_pa_is_managed);
EXPORT_SYMBOL(rkp_mem_set);
EXPORT_SYMBOL(rkp_copy_from_user_patch_on);
EXPORT_SYMBOL(rkp_copy_from_user_patch_off);
EXPORT_SYMBOL(rkp_patch_arch_copy_from_user);
EXPORT_SYMBOL(rkp_patch_arch_copy_from_user_impl);