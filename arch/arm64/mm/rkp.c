#include <asm/rkp.h>

static int INITED = 0;
phys_addr_t POOLSTART = 0;
phys_addr_t POOLEND = 0;

phys_addr_t rkp_allocPageTable(void){
    struct arm_smccc_res res;
	
    if(INITED == 0){
        memset(&res, 0, sizeof(res));
        POOLSTART = memblock_phys_alloc(POOLSIZE*PAGE_SIZE+POOLSIZE*sizeof(unsigned int), PAGE_SIZE);
        arm_smccc_smc(TEESMC_OPTEED_RKP_PTM_INIT, POOLSTART, POOLSIZE, 0, 0, 0, 0, 0, &res);
        if(res.a0 != 0){
            pr_err("TEESMC_OPTEED_RKP_PTM_INIT failed");
            return 0;
        }
        POOLEND = POOLSTART + POOLSIZE*PAGE_SIZE;
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
    memset(&res, 0, sizeof(res));
    arm_smccc_smc(TEESMC_OPTEED_RKP_PTM_RELEASEAPT, target, 0, 0, 0, 0, 0, 0, &res);
    if(res.a0 != 0){
        pr_err("TEESMC_OPTEED_RKP_PTM_RELEASEAPT failed");
        return;
    }
    return;
}

void rkp_setPageTableElementWithPa(int pageTableType,phys_addr_t pa, unsigned long content){
    struct arm_smccc_res res;
    memset(&res, 0, sizeof(res));
    arm_smccc_smc(TEESMC_OPTEED_RKP_SET_PAGETABLE, pageTableType, pa, content, 0, 0, 0, 0, &res);
    if(res.a0 != 0){
        pr_err("TEESMC_OPTEED_RKP_SET_PAGETABLE failed");
        return;
    }
    return;
}

void rkp_setPageTableElement(int pageTableType,unsigned long * va, unsigned long content){
    struct arm_smccc_res res;
    unsigned long long pa = virt_to_phys(va);
    if(!rkp_paIsManaged(pa)){
        *va = content;
        return;
    }
    memset(&res, 0, sizeof(res));
    arm_smccc_smc(TEESMC_OPTEED_RKP_SET_PAGETABLE, pageTableType, pa, content, 0, 0, 0, 0, &res);
    if(res.a0 != 0){
        pr_err("TEESMC_OPTEED_RKP_SET_PAGETABLE failed");
        return;
    }
    return;
}

void rkp_instruction_simulation(unsigned long instr_mark,unsigned long param2,
                                unsigned long param3,unsigned long param4){
    struct arm_smccc_res res;
    memset(&res, 0, sizeof(res));
    arm_smccc_smc(TEESMC_OPTEED_RKP_INSTR_SIMULATION, instr_mark, param2, param3, param4, 0, 0, 0, &res);
    if(res.a0 != 0){
        pr_err("TEESMC_OPTEED_RKP_INSTR_SIMULATION failed");
        return;
    }
    return;    

}