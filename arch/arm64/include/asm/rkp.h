#ifndef __ASM_RKP_H
#define __ASM_RKP_H

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
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>

#include <asm/barrier.h>
#include <asm/cputype.h>
#include <asm/fixmap.h>
#include <asm/kasan.h>
#include <asm/kernel-pgtable.h>
#include <asm/sections.h>
#include <asm/setup.h>
#include <asm/sizes.h>
#include <asm/tlb.h>
#include <asm/mmu_context.h>
#include <asm/ptdump.h>
#include <asm/tlbflush.h>

#include <linux/arm-smccc.h>
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

extern phys_addr_t POOLSTART;
extern phys_addr_t POOLEND;

#define rkp_paIsManaged(pa) (pa > POOLSTART-1 && pa < POOLEND)


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


phys_addr_t rkp_allocPageTable(void);

void rkp_releasePageTable(phys_addr_t target);

void rkp_setPageTableElementWithPa(int pageTableType,phys_addr_t pa, unsigned long content);

void rkp_setPageTableElement(int pageTableType,unsigned long * va, unsigned long content);

void rkp_instruction_simulation(unsigned long instr_mark,unsigned long param2,
                                unsigned long param3,unsigned long param4);

#endif
