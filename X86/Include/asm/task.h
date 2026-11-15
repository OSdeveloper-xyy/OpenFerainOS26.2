#ifndef      TASK_H
#define      TASK_H

#include <stdint.h>
#include <stdio.h>
#include <device/DEVICERW.h>
#include <asm/paging.h>
#include <asm/descriptor.h>

#define TASK_DATA_BASE       0x00005000
#define TASK_LDT_BASE        0x00015000
#define MEM_MAP_BASE         0x00020000
#define TEMP_PT_BASE         0x00048000
#define TASK_MAP_BASE        0x00049000
//Task State Define
#define TASK_WAITING         1
#define TASK_SLEEPING        2
#define TASK_STOPPED         3
#define TASK_RUNNING         4
struct task_state_segment{
    uint16_t Previous_Task_Link;uint16_t Reserved0;
    uint32_t ESP0;
    uint16_t SS0;uint16_t Reserved1;
    uint32_t ESP1;
    uint16_t SS1;uint16_t Reserved2;
    uint32_t ESP2;
    uint16_t SS2;uint16_t Reserved3;
    uint32_t CR3;
    uint32_t EIP;
    uint32_t EFLAGS;
    uint32_t EAX;
    uint32_t ECX;
    uint32_t EDX;
    uint32_t EBX;
    uint32_t ESP;
    uint32_t EBP;
    uint32_t ESI;
    uint32_t EDI;
    uint16_t ES;uint16_t Reserved4;
    uint16_t CS;uint16_t Reserved5;
    uint16_t SS;uint16_t Reserved6;
    uint16_t DS;uint16_t Reserved7;
    uint16_t FS;uint16_t Reserved8;
    uint16_t GS;uint16_t Reserved9;
    uint16_t LDT_Segment_Selector;
    uint32_t Reserved10;
    uint16_t IO_Map_base_addr;
}__attribute__((packed));
typedef struct task_state_segment task_state_segment;
struct task_data_structure{
    uint8_t   state;
    uint8_t   Remaining_time;
    uint8_t   Priority;
    uint32_t  TaskBaseAddr;
    task_state_segment tss;   
}__attribute__((packed));
typedef struct task_data_structure task_data_structure;
struct AppFileHeader{
    uint32_t FerainAppID;
    uint16_t Priority;
    uint16_t AppName;
    uint32_t EntryOffset;
    uint32_t Reserved0;
    uint32_t Reserved1;
    uint32_t Reserved2;
}__attribute__((packed));
typedef struct AppFileHeader AppFileHeader;
static inline void task_num_write(uint32_t i){
    volatile uint8_t *task_read_pointer = (volatile uint8_t *)(TASK_MAP_BASE + (i / 8));
    uint8_t map_byte = 
        (1 << (i % 8))       ^
        *task_read_pointer;
    *task_read_pointer = map_byte;
    return;
}
static inline void mem_map_write(uint32_t i){
    volatile uint8_t *map_read_pointer = (volatile uint8_t *)(MEM_MAP_BASE + (i / 8));
    uint8_t map_byte = 
        (1 << (i % 8))       ^
        *map_read_pointer;
    *map_read_pointer = map_byte;
    return;
}
static inline uint32_t get_available_task_num(){
    uint32_t ret_val = 0xFFFFFFFF;
    for(uint32_t i = 0;i < 16;i++){
        volatile uint32_t *task_map_ptr = (volatile uint32_t *)(TASK_MAP_BASE + i * 4);
        uint32_t map_section = *task_map_ptr;
        for(uint32_t j = 0;j < 32;j++){
            uint32_t task_used_flag = (map_section >> j) & 0x00000001;
            if(task_used_flag == 0){
                ret_val = i * 32 + j;
            }
        }
    }
    return ret_val;
}
static inline uint32_t get_available_page(){
    uint32_t ret_val = 0xFFFFFFFF;
    for(uint32_t i = 0;i < 32768;i++){
        volatile uint32_t *mem_map_ptr = (volatile uint32_t *)(MEM_MAP_BASE + i * 4);
        uint32_t map_section = *mem_map_ptr;
        for(uint32_t j = 0;j < 32;j++){
            uint32_t page_used_flag = (map_section >> j) & 0x00000001;
            if(page_used_flag == 0){
                ret_val = i * 32 + j;
            }
        }
    }
    return ret_val;
}
static inline void creat_task(const char *path){
    uint32_t task_num = get_available_task_num();
    if(task_num == 0xFFFFFFFF){
        println("No available task number for task creation.");
        return;
    }
    uint32_t pd_base = get_available_page();
    if(pd_base == 0xFFFFFFFF){
        println("No available page for task creation.");
        return;
    }
    pd_base <<= 3;
    PTE_SET((pd_base % 0x400000)>> 3,TEMP_PT_BASE,pd_base,P_SYSTEM,P_READ_WRITE,P_PRESENT);
    uint32_t code_pt_base = get_available_page();
    if(code_pt_base == 0xFFFFFFFF){
        println("No available page for task creation.");
        return;
    }
    code_pt_base <<= 3;
    PDE_SET(0,KERNEL_PT_PHY_ADDR,pd_base,P_SYSTEM,P_READ_WRITE,P_PRESENT);
    uint32_t task_code_base = get_available_page();
    if(task_code_base == 0xFFFFFFFF){
        println("No available page for task creation.");
        return;
    }
    task_code_base <<= 3;
    PDE_SET(task_code_base / 0x400000,code_pt_base,pd_base,P_USER,P_ONLY_READ,P_PRESENT);
    PTE_SET((code_pt_base % 0X400000) >> 3,TEMP_PT_BASE,code_pt_base,P_SYSTEM,P_READ_WRITE,P_PRESENT);
    PTE_SET((task_code_base % 0x400000) >> 3,code_pt_base,task_code_base,P_USER,P_ONLY_READ,P_PRESENT);
    PTE_SET((task_code_base % 0x400000) >> 3,TEMP_PT_BASE,task_code_base,P_SYSTEM,P_READ_WRITE,P_PRESENT);
    //device read to task_code_base
    volatile task_data_structure *task_data_ptr = 
    (volatile task_data_structure *)(TASK_DATA_BASE + task_num * 128);
    volatile AppFileHeader *HeaderPtr = 
    (volatile AppFileHeader *)(HD_BF_BASE);
    task_data_ptr->state           =    TASK_RUNNING;
    task_data_ptr->Priority        =    HeaderPtr->Priority;
    task_data_ptr->Remaining_time  =    HeaderPtr->Priority;
    task_data_ptr->TaskBaseAddr    =    task_code_base;
    set_gd((4 + task_num*2),(TASK_LDT_BASE + task_num * 32),0x1F,GD_G_BYTE,GD_SYSTEM,GD_TYPE_LDT,PL_KERNEL);
    set_gd((5 + task_num*2),(uint32_t)&(task_data_ptr->tss.Previous_Task_Link),0x67,GD_G_BYTE,GD_SYSTEM,GD_TYPE_ATSS,PL_KERNEL);
    volatile uint32_t *LDT_CODE_LOW   = (volatile uint32_t *)(TASK_LDT_BASE + 8);
    volatile uint32_t *LDT_CODE_HIGH  = (volatile uint32_t *)(TASK_LDT_BASE + 12);
    volatile uint32_t *LDT_DATA_LOW   = (volatile uint32_t *)(TASK_LDT_BASE + 16);
    volatile uint32_t *LDT_DATA_HIGH  = (volatile uint32_t *)(TASK_LDT_BASE + 20);
    *LDT_CODE_LOW = 
        ((task_code_base & 0xFFFF) << 16)    |
        0x0000FFFF;
    *LDT_CODE_HIGH = 
        (task_code_base & 0xFF000000)        |
        0x00CFFA00                           |
        ((task_code_base & 0x00FF0000) >> 16);
    *LDT_DATA_LOW  =    0x0000FFFF;
    *LDT_DATA_HIGH =    0x00CFF200;
    task_data_ptr->tss.Previous_Task_Link     =    0;
    task_data_ptr->tss.ESP0                   =    0x3FFFF0;
    task_data_ptr->tss.SS0                    =    0x10;
    task_data_ptr->tss.ESP1                   =    0;
    task_data_ptr->tss.SS1                    =    0;
    task_data_ptr->tss.ESP2                   =    0;
    task_data_ptr->tss.SS2                    =    0;
    task_data_ptr->tss.CR3                    =    pd_base;
    task_data_ptr->tss.EIP                    =    0;
    task_data_ptr->tss.EFLAGS                 =    0x00000202;
    task_data_ptr->tss.EAX                    =    0;
    task_data_ptr->tss.ECX                    =    0;
    task_data_ptr->tss.EDX                    =    0;
    task_data_ptr->tss.EBX                    =    0;
    task_data_ptr->tss.ESP                    =    0xFFFFFFF0;
    task_data_ptr->tss.EBP                    =    0;
    task_data_ptr->tss.ESI                    =    0;
    task_data_ptr->tss.EDI                    =    0;
    task_data_ptr->tss.ES                     =    0x17;
    task_data_ptr->tss.CS                     =    0x0F;
    task_data_ptr->tss.SS                     =    0x17;
    task_data_ptr->tss.DS                     =    0x17;
    task_data_ptr->tss.FS                     =    0;
    task_data_ptr->tss.GS                     =    0;
    task_data_ptr->tss.LDT_Segment_Selector   =    ((4 + task_num*2) << 3);
    task_data_ptr->tss.IO_Map_base_addr       =    0x68;
    task_num_write(task_num);
    mem_map_write(pd_base);
    mem_map_write(code_pt_base);
    mem_map_write(task_code_base);
    return;
}

static inline void GotoTask(uint16_t tasknum){
    struct tss_switch_data_struct{
        uint32_t offset;
        uint16_t segment;
    }__attribute__((packed));
    struct tss_switch_data_struct tss_switch_data;
    tss_switch_data.offset = 0;
    tss_switch_data.segment = ((tasknum * 2 + 5) << 3);
    asm volatile (
        "ljmp *%[TSS]"
        : 
        : [TSS]"m"(tss_switch_data)
        : "memory"
    );
    return;
}
static inline uint16_t find_running_task(uint16_t NowTask,uint8_t NowPriority){
    uint16_t anstasknum = NowTask;
    uint8_t  anspriority = 0xFF;
    for(int i = 0;i < 512;i++){
        volatile task_data_structure *task_data_ptr = 
        (volatile task_data_structure *)(TASK_DATA_BASE + i * 128);
        if(task_data_ptr->state == TASK_RUNNING && task_data_ptr->Priority >= NowPriority && task_data_ptr->Priority <= anspriority){
             anstasknum = i;
             anspriority = task_data_ptr->Priority;
        }
    }
    if(anstasknum == NowTask){
        for(int i = 0;i < 512;i++){
            volatile task_data_structure *task_data_ptr = 
            (volatile task_data_structure *)(TASK_DATA_BASE + i * 128);
            if(task_data_ptr->state == TASK_RUNNING && task_data_ptr->Priority <= anspriority){
                anspriority = task_data_ptr->Priority;
                anstasknum = i;
            }
        }
    }
    return anstasknum;
}
__attribute__((noinline)) void ISR_PIT(uint32_t TR){
    uint16_t TaskNum = ((TR >> 3) - 5 ) / 2;
    volatile task_data_structure *task_data_ptr = 
    (volatile task_data_structure *)(TASK_DATA_BASE + TaskNum * 128);
    if(task_data_ptr->state == TASK_RUNNING){
        task_data_ptr->Remaining_time -= 1;
        if(task_data_ptr->Remaining_time == 0){
            task_data_ptr->Remaining_time = task_data_ptr->Priority;
            uint16_t tasknum = find_running_task(TaskNum,task_data_ptr->Priority);
            if(tasknum == TaskNum)return;
            GotoTask(tasknum);
        }
    }
	return;
}
#endif
