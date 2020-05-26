#ifndef __ASM_RKP_H
#define __ASM_RKP_H

#define SMC_TYPE_FAST			1ULL
#define FUNCID_TYPE_SHIFT		31U
#define FUNCID_CC_SHIFT			30U
#define SMC_32				0U
#define FUNCID_OEN_SHIFT		24U
#define FUNCID_NUM_MASK			0xffffU
#define TEESMC_OPTEED_RV(func_num) \
		((SMC_TYPE_FAST << FUNCID_TYPE_SHIFT) | \
		 ((SMC_32) << FUNCID_CC_SHIFT) | \
		 (62 << FUNCID_OEN_SHIFT) | \
		 ((func_num) & FUNCID_NUM_MASK))

#define RKP_PTE 0
#define RKP_PMD 1
#define RKP_PUD 2
#define RKP_PGD 3
#define RKP_WHATEVER 4
#define POOLSIZE 8192
typedef unsigned long long phys_addr_t;
extern phys_addr_t POOLSTART;
extern phys_addr_t POOLEND;

#define rkp_paIsManaged(pa) ((phys_addr_t)pa > POOLSTART-1 && (phys_addr_t)pa < POOLEND)
#define PTVALUE2UL(a) *(unsigned long*)&a 
#define PTPTR2ULPTR(a) *(unsigned long **)&a
#define TEESMC_OPTEED_FUNCID_TZC400_SET_READONLY 11
#define TEESMC_OPTEED_TZC400_SET_READONLY \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_TZC400_SET_READONLY)

#define TEESMC_OPTEED_FUNCID_RKP_PTM_INIT 20
#define TEESMC_OPTEED_RKP_PTM_INIT \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_RKP_PTM_INIT)
#define TEESMC_OPTEED_FUNCID_RKP_PTM_GETAPT 21
#define TEESMC_OPTEED_RKP_PTM_GETAPT \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_RKP_PTM_GETAPT)
#define TEESMC_OPTEED_FUNCID_RKP_PTM_RELEASEAPT 22
#define TEESMC_OPTEED_RKP_PTM_RELEASEAPT \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_RKP_PTM_RELEASEAPT)
#define TEESMC_OPTEED_FUNCID_RKP_SET_PAGETABLE 23
#define TEESMC_OPTEED_RKP_SET_PAGETABLE \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_RKP_SET_PAGETABLE)
#define TEESMC_OPTEED_FUNCID_RKP_INSTR_SIMULATION 24
#define TEESMC_OPTEED_RKP_INSTR_SIMULATION \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_RKP_INSTR_SIMULATION)
#define TEESMC_OPTEED_FUNCID_RKP_CLEAR_PAGE 25
#define TEESMC_OPTEED_RKP_CLEAR_PAGE \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_RKP_CLEAR_PAGE)
#define TEESMC_OPTEED_FUNCID_RKP_COPY_PAG 26
#define TEESMC_OPTEED_RKP_COPY_PAGE \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_RKP_COPY_PAG)





phys_addr_t rkp_allocPageTable(void);
void rkp_releasePageTable(phys_addr_t target);

void rkp_setPageTableElementWithPa(int pageTableType,phys_addr_t pa, unsigned long content);

void rkp_setPageTableElement(int pageTableType,unsigned long * va, unsigned long content);

void rkp_instruction_simulation(unsigned long instr_mark,unsigned long param2,
                                unsigned long param3,unsigned long param4);

unsigned long rkp_xchg_relaxed(int ptType,unsigned long* ptePtr, unsigned long newVal);
unsigned long rkp_cmpxchg_relaxed(int ptType,unsigned long* ptePtr, unsigned long oldVal, unsigned long newVal);

extern int PTMAPED;
int rkp_iscross_with_ptm(phys_addr_t start, phys_addr_t end);
#define RKP_ISCROSS(a,b) (PTMAPED == 0 ? rkp_iscross_with_ptm(a,b) : 0)
void rkp_set_PTMAPED(void);

void rkp_clear_page(void * kaddr);
unsigned long rkp_copy_page(void *kto,const void* kfrom, unsigned long n);

int rkp_pa_is_managed(phys_addr_t pa);







#endif
