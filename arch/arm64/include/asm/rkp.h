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

//定义功能对应的smc指令id
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
#define TEESMC_OPTEED_FUNCID_RKP_MEM_SET 27
#define TEESMC_OPTEED_RKP_MEM_SET \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_RKP_MEM_SET)

#define TEESMC_OPTEED_FUNCID_RKP_SET_ROADDR 40
#define TEESMC_OPTEED_RKP_SET_ROADDR \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_RKP_SET_ROADDR)
#define TEESMC_OPTEED_FUNCID_RKP_SET_FORBID_FLAG 41
#define TEESMC_OPTEED_RKP_SET_FORBID_FLAG \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_RKP_SET_FORBID_FLAG)
#define TEESMC_OPTEED_FUNCID_RKP_SET_PXN 42
#define TEESMC_OPTEED_RKP_SET_PXN \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_RKP_SET_PXN)
#define TEESMC_OPTEED_FUNCID_PKM_PROTECT_KEY_CODE 51
#define TEESMC_OPTEED_PKM_PROTECT_KEY_CODE \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_PKM_PROTECT_KEY_CODE)
#define TEESMC_OPTEED_FUNCID_PKM_SELINUX 52
#define TEESMC_OPTEED_PKM_SELINUX \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_PKM_SELINUX)

#define TEESMC_OPTEED_FUNCID_KILL_HOOK 80
#define TEESMC_OPTEED_KILL_HOOK \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_KILL_HOOK)
#define TEESMC_OPTEED_FUNCID_PUSH_TASKADDR 81
#define TEESMC_OPTEED_PUSH_TASKADDR \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_PUSH_TASKADDR)
#define TEESMC_OPTEED_FUNCID_SET_PUSH_TASKADDR_FLAG 82
#define TEESMC_OPTEED_SET_PUSH_TASKADDR_FLAG \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_SET_PUSH_TASKADDR_FLAG)

#define TEESMC_OPTEED_FUNCID_SET_PID_AND_STACK 90
#define TEESMC_OPTEED_SET_PID_AND_STACK \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_SET_PID_AND_STACK)
#define TEESMC_OPTEED_FUNCID_FREE_PID_AND_STACK 91
#define TEESMC_OPTEED_FREE_PID_AND_STACK \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_FREE_PID_AND_STACK)
#define TEESMC_OPTEED_FUNCID_INIT_PID_AND_STACK 92
#define TEESMC_OPTEED_INIT_PID_AND_STACK \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_INIT_PID_AND_STACK)
#define TEESMC_OPTEED_FUNCID_SET_STACK_HASH 93
#define TEESMC_OPTEED_SET_STACK_HASH \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_SET_STACK_HASH)
#define TEESMC_OPTEED_FUNCID_SWITCH_STACK 94
#define TEESMC_OPTEED_SWITCH_STACK \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_SWITCH_STACK)
#define TEESMC_OPTEED_FUNCID_CHECK_PID_AND_STACK 95
#define TEESMC_OPTEED_CHECK_PID_AND_STACK \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_CHECK_PID_AND_STACK)

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

void* rkp_mem_set(void *, int, unsigned long);

int find_task_addr(unsigned long long task_addr);

int push_task_addr(unsigned long long task_addr);

int push_pid(short now_pid);

int find_pid(short now_pid);

int set_push_flag(void);

int init_pid_and_stack(void);

int set_pid_and_stack(short pid, unsigned long stack);

int free_pid_and_stack(short pid);

int switch_pid_and_stack(short prev_pid, unsigned long prev_stack, short next_pid, unsigned long next_stack);
typedef struct
{
    char state;
    unsigned long stack;
    // BYTE hash[SHA256_BLOCK_SIZE];
}STACK_STRUCT;
#define PID_SIZE 2000

#endif
