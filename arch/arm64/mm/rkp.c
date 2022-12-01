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
#include <linux/pid.h>
static int INITED = 0;
//声明自旋锁并初始化
static DEFINE_SPINLOCK(xchg_spin_lock);
static DEFINE_SPINLOCK(ptmanager_spin_lock);
static DEFINE_SPINLOCK(cfu_patch_spin_lock);
phys_addr_t POOLSTART = 0; //初始值保证rkp_paIsManaged宏在RKP未初始化前始终返回false
phys_addr_t POOLEND = 0;

phys_addr_t SSPOOLSTART = 0;
phys_addr_t SSPOOLEND = 0;
//分配页表
phys_addr_t rkp_allocPageTable(void){
    struct arm_smccc_res res;
    //如果安全内存区域还没有进行初始化，那么先进行初始化
    if(INITED == 0){
        // POOLSTART = memblock_phys_alloc(POOLSIZE*PAGE_SIZE+POOLSIZE*sizeof(unsigned int), PAGE_SIZE);
        // memblock_reserve(POOLSTART,POOLSIZE*PAGE_SIZE+POOLSIZE*sizeof(unsigned int));
        POOLSTART = memblock_phys_alloc(POOLSIZE*PAGE_SIZE+POOLSIZE*sizeof(unsigned int)+sizeof(STACK_STRUCT)*PID_SIZE, PAGE_SIZE);
        memblock_reserve(POOLSTART,POOLSIZE*PAGE_SIZE+POOLSIZE*sizeof(unsigned int)+sizeof(STACK_STRUCT)*PID_SIZE);

        int page_count = ((sizeof(STACK_STRUCT) + sizeof(CRED_STRUCT)) * PID_SIZE / PAGE_SIZE) + 1;
        int sspool_size = page_count * PAGE_SIZE;
        SSPOOLSTART = memblock_phys_alloc(sspool_size, PAGE_SIZE);
        memblock_reserve(SSPOOLSTART, sspool_size);
        // SSPOOLSTART = memblock_phys_alloc(sizeof(STACK_STRUCT)*PID_SIZE + sizeof(CRED_STRUCT)*PID_SIZE, PAGE_SIZE);
        // memblock_reserve(SSPOOLSTART,sizeof(STACK_STRUCT)*PID_SIZE + sizeof(CRED_STRUCT)*PID_SIZE);

        memset(&res, 0, sizeof(res));
        // arm_smccc_smc(TEESMC_OPTEED_RKP_PTM_INIT, POOLSTART, POOLSIZE, 0, 0, 0, 0, 0, &res);
        arm_smccc_smc(TEESMC_OPTEED_RKP_PTM_INIT, POOLSTART, POOLSIZE, SSPOOLSTART, 0, 0, 0, 0, &res);
        if(res.a0 != 0){
            pr_err("TEESMC_OPTEED_RKP_PTM_INIT failed");
            return 0;
        }

        POOLEND = POOLSTART + POOLSIZE*PAGE_SIZE;
        SSPOOLEND = SSPOOLSTART + sizeof(STACK_STRUCT)*PID_SIZE + sizeof(CRED_STRUCT)*PID_SIZE;

        pr_err("PTM start 0x%016llx end 0x%016llx",POOLSTART,POOLEND);
        pr_err("SSPOOL start 0x%016llx end 0x%016llx", SSPOOLSTART, SSPOOLEND);

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
//释放页表
void rkp_releasePageTable(phys_addr_t target){
    struct arm_smccc_res res;
    if(PTMAPED == 0){
        arm_smccc_smc(TEESMC_OPTEED_RKP_PTM_RELEASEAPT, target, 0, 0, 0, 0, 0, 0, &res);
    }else{
        spin_lock(&ptmanager_spin_lock);
        arm_smccc_smc(TEESMC_OPTEED_RKP_PTM_RELEASEAPT, target, 0, 0, 0, 0, 0, 0, &res);
        spin_unlock(&ptmanager_spin_lock);
    }
    return;
}
//设置页表项内容
void rkp_setPageTableElementWithPa(int pageTableType,phys_addr_t pa, unsigned long content){
    struct arm_smccc_res res;
    arm_smccc_smc(TEESMC_OPTEED_RKP_SET_PAGETABLE, pageTableType, pa, content, 0, 0, 0, 0, &res);
    dsb(ishst);
    return;
}
//设置页表项内容
void rkp_setPageTableElement(int pageTableType,unsigned long * va, unsigned long content){
    struct arm_smccc_res res;
    unsigned long long pa = virt_to_phys(va);//虚拟地址转换为物理地址
    //如果不是安全内存，直接处理
    if(!rkp_paIsManaged(pa)){
        WRITE_ONCE(*va,content);
        dsb(ishst);
        return;
    }
    arm_smccc_smc(TEESMC_OPTEED_RKP_SET_PAGETABLE, pageTableType, pa, content, 0, 0, 0, 0, &res);
    dsb(ishst);
    return;
}
//指令模拟
void rkp_instruction_simulation(unsigned long instruction,unsigned long param1,
                                unsigned long param2,unsigned long param3){
    struct arm_smccc_res res;
    arm_smccc_smc(TEESMC_OPTEED_RKP_INSTR_SIMULATION, instruction, param1, param2, param3, 0, 0, 0, &res);
    return;    
}
//模拟xchg_relaxed函数
unsigned long rkp_xchg_relaxed(int ptType,unsigned long* ptePtr, unsigned long newVal){
    unsigned long oldVal = 0;
    spin_lock(&xchg_spin_lock);
    oldVal = *ptePtr;
    rkp_setPageTableElement(ptType,ptePtr,newVal);
    spin_unlock(&xchg_spin_lock);
    return oldVal;
}
//模拟cmpxchg_relaxed函数
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
}
//清除页
void rkp_clear_page(void * kaddr){
    struct arm_smccc_res res;
    unsigned long long pa = virt_to_phys(kaddr);
    arm_smccc_smc(TEESMC_OPTEED_RKP_CLEAR_PAGE, pa, 0, 0, 0, 0, 0, 0, &res);
    return;
}
//复制页
unsigned long rkp_copy_page(void *kto,const void*kfrom, unsigned long n){
    struct arm_smccc_res res;
    unsigned long long pa_to;
    unsigned long long pa_from;
    pa_to = virt_to_phys(kto);
    pa_from = virt_to_phys(kfrom);
    arm_smccc_smc(TEESMC_OPTEED_RKP_COPY_PAGE, pa_to, pa_from, n, 0, 0, 0, 0, &res);
    return res.a1;
}
//判断地址是否属于安全内存
int rkp_pa_is_managed(phys_addr_t pa)
{
	return ((phys_addr_t)pa > POOLSTART-1 && (phys_addr_t)pa < POOLEND);
}
//内存设置
void* rkp_mem_set(void * buffer, int c, unsigned long n){
    struct arm_smccc_res res;
    unsigned long pa_buffer = (unsigned long) virt_to_phys(buffer);
    arm_smccc_smc(TEESMC_OPTEED_RKP_MEM_SET, pa_buffer, c, n, 0, 0, 0, 0, &res);
    return (void *)res.a1;
}

int find_task_addr(unsigned long long task_addr)
{
    struct arm_smccc_res res;
    arm_smccc_smc(TEESMC_OPTEED_KILL_HOOK, task_addr, 0, 0, 0, 0, 0, 0, &res);
    return (int)res.a0;
}

int push_task_addr(unsigned long long task_addr)
{
    pr_info("task_adrr:0x%016llx\n", task_addr);
    struct arm_smccc_res res;
    arm_smccc_smc(TEESMC_OPTEED_PUSH_TASKADDR, task_addr, 0, 0, 0, 0, 0, 0, &res);
    return (int)res.a0;
}

int push_pid(short now_pid)
{
    pr_info("pid:%d\n", now_pid);
    struct pid *pid = NULL;
	pid = find_vpid(now_pid);
    struct task_struct *p;
    p = pid_task(pid, PIDTYPE_PID);
    struct arm_smccc_res res;
    arm_smccc_smc(TEESMC_OPTEED_PUSH_TASKADDR, (unsigned long long)p, 0, 0, 0, 0, 0, 0, &res);
    return (int)res.a0;
}

int find_pid(short now_pid)
{
    struct pid *pid = NULL;
	pid = find_vpid(now_pid);
    struct task_struct *p;
    p = pid_task(pid, PIDTYPE_PID);
    pr_info("pid:%d task_adrr:0x%016llx", now_pid, (unsigned long long)p);
}

int set_push_flag(void)
{
    struct arm_smccc_res res;
    arm_smccc_smc(TEESMC_OPTEED_SET_PUSH_TASKADDR_FLAG, 0, 0, 0, 0, 0, 0, 0, &res);
    return (int)res.a0;
}

int init_pid_and_stack(void)
{
    pr_info("aaaaaaaa");
    struct arm_smccc_res res;
    arm_smccc_smc(TEESMC_OPTEED_INIT_PID_AND_STACK, 0, 0, 0, 0, 0, 0, 0, &res);
    return (int)res.a0;
}

int set_pid_and_stack(short pid, unsigned long stack)
{
    struct arm_smccc_res res;
    arm_smccc_smc(TEESMC_OPTEED_SET_PID_AND_STACK, pid, stack, 0, 0, 0, 0, 0, &res);
    return (int)res.a0;
}

int free_pid_and_stack(short pid)
{
    struct arm_smccc_res res;
    arm_smccc_smc(TEESMC_OPTEED_FREE_PID_AND_STACK, pid, 0, 0, 0, 0, 0, 0, &res);
    return (int)res.a0;
}

int switch_pid_and_stack(short prev_pid, unsigned long prev_stack, short next_pid, unsigned long next_stack)
{
    struct arm_smccc_res res;
    arm_smccc_smc(TEESMC_OPTEED_SWITCH_STACK, prev_pid, prev_stack, next_pid, next_stack, 0, 0, 0, &res);
    return (int)res.a0;
}

int visit_stack_struct()
{
    struct arm_smccc_res res;
    arm_smccc_smc(TEESMC_OPTEED_VISIT_STACK_STRUCT, 0, 0, 0, 0, 0, 0, 0, &res);
    return (int)res.a0;
}

int set_cred(short pid, unsigned long task_struct_addr, unsigned long cred_addr, unsigned long pgd_addr)
{
    struct arm_smccc_res res;
    arm_smccc_smc(TEESMC_OPTEED_SET_CRED, pid, task_struct_addr, cred_addr, pgd_addr, 0, 0, 0, &res);
    return (int)res.a0;
}

int check_cred(short pid, unsigned long task_struct_addr, unsigned long cred_addr, unsigned long pgd_addr)
{
    struct arm_smccc_res res;
    arm_smccc_smc(TEESMC_OPTEED_CHECK_CRED, pid, task_struct_addr, cred_addr, pgd_addr, 0, 0, 0, &res);
    return (int)res.a0;
}

int free_cred(short pid)
{
    struct arm_smccc_res res;
    arm_smccc_smc(TEESMC_OPTEED_FREE_CRED, pid, 0, 0, 0, 0, 0, 0, &res);
    return (int)res.a0;
}
EXPORT_SYMBOL(rkp_copy_page);
EXPORT_SYMBOL(rkp_pa_is_managed);
EXPORT_SYMBOL(rkp_mem_set);