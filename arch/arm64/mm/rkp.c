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
static int INITED = 0;
static DEFINE_SPINLOCK(xchg_spin_lock);
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
        pr_err("PTM start 0x%016llx end 0x%016llx",POOLSTART,POOLEND);
        INITED = 1;
    }

    memset(&res, 0, sizeof(res));
    arm_smccc_smc(TEESMC_OPTEED_RKP_PTM_GETAPT, 0, 0, 0, 0, 0, 0, 0, &res);
    if(res.a0 != 0){
        pr_err("TEESMC_OPTEED_RKP_PTM_GETAPT failed");
        return 0;
    }
    return res.a1;
}
void rkp_releasePageTable(phys_addr_t target){
    struct arm_smccc_res res;
    // memset(&res, 0, sizeof(res));
    arm_smccc_smc(TEESMC_OPTEED_RKP_PTM_RELEASEAPT, target, 0, 0, 0, 0, 0, 0, &res);
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
    // if(res.a0 != 0){
    //     pr_err("TEESMC_OPTEED_RKP_SET_PAGETABLE failed");
    //     return;
    // }
    return;
}

void rkp_setPageTableElement(int pageTableType,unsigned long * va, unsigned long content){
    struct arm_smccc_res res;
    unsigned long long pa = virt_to_phys(va);
    if(!rkp_paIsManaged(pa)){
        WRITE_ONCE(*va,content);
        dsb(ishst);
        return;
    }
    //memset(&res, 0, sizeof(res));
    arm_smccc_smc(TEESMC_OPTEED_RKP_SET_PAGETABLE, pageTableType, pa, content, 0, 0, 0, 0, &res);
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