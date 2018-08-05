#ifndef VIRTMEMINFO_H_
#define VIRTMEMINFO_H_

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h> /* task_struct */

/**
 *  value in the pud,pmd.pte use 20 MSB for base address
 */
#define PFN_SHIFT 12
#define BASE_ADDR_MASK 0xfffffffffffff000ull
#define get_base_addr(entry_val) ((unsigned long long)(entry_val)&BASE_ADDR_MASK)

#define VIRT_ADDR_64_SHIFT 48
#define VIRT_ADDR_64_MASK (1ul << VIRT_ADDR_64_SHIFT)

/**
 * macro function to get take out index
 * 
 * linux use 48 bit for virtual address
 * 9 bit for pgd index
 * 9 bit for pud index
 * 9 bit for pmd index
 * 9 bit for pte index
 * 12bit for page index
 */
#define get_pgd_offset(virt) (((virt) & ~VIRT_ADDR_64_MASK) >> PGDIR_SHIFT)
#define get_pud_offset(virt) (((virt) & ~PGDIR_MASK) >> PUD_SHIFT)
#define get_pmd_offset(virt) (((virt) & ~PUD_MASK) >> PMD_SHIFT)
#define get_pte_offset(virt) (((virt) & 0x1ff000llu) >> 12)
#define get_page_index(virt) ((virt)&0xfffllu)

typedef struct pgd_info
{
    unsigned long long pgd_base_addr;
    unsigned long long pgd_addr;
    unsigned long long pgd_val;
    unsigned long long pfn_addr;
    unsigned int pgd_flags;
} pgd_info_t;

#define get_page_size_pgd(pgd_flags)         ((pgd_flags) & 0x80)
#define get_access_bit_pgd(pgd_flags)        ((pgd_flags) & 0x20)
#define get_cache_disable_bit_pgd(pgd_flags) ((pgd_flags) & 0x10)
#define get_write_through_pgd(pgd_flags)     ((pgd_flags) & 0x08)
#define get_mode_bit_pgd(pgd_flags)          ((pgd_flags) & 0x04)
#define get_rw_bit_pgd(pgd_flags)            ((pgd_flags) & 0x02)
#define get_page_present_pgd(pgd_flags)      ((pgd_flags) & 0x01)

typedef struct pud_info
{
    unsigned long long pud_addr;
    unsigned long long pud_val;
    unsigned long long pfn_addr;
} pud_info_t;

typedef struct pmd_info
{
    unsigned long long pmd_addr;
    unsigned long long pmd_val;
    unsigned long long pfn_addr;
} pmd_info_t;

typedef struct pte_info
{
    unsigned long long pte_addr;
    unsigned long long pte_val;
    unsigned long long pg_base_addr;
    unsigned int pte_flags;
    unsigned long long phys_addr;
} pte_info_t;

#define get_dirty_bit_pte(pte_flags)         ((pte_flags) & 0x80)
#define get_access_bit_pte(pte_flags)        ((pte_flags) & 0x20)
#define get_cache_disable_bit_pte(pte_flags) ((pte_flags) & 0x10)
#define get_pg_write_through_pte(pte_flags)  ((pte_flags) & 0x08)
#define get_mode_bit_pte(pte_flags)          ((pte_flags) & 0x04)
#define get_rw_bit_pte(pte_flags)            ((pte_flags) & 0x02)
#define get_page_present_pte(pte_flags)      ((pte_flags) & 0x01)

typedef struct virt_addr_info{
    struct task_struct* p;
    unsigned long start_code, end_code, pages_code; 
    unsigned long start_data, end_data, pages_data;
	unsigned long start_heap, end_heap, pages_heap; 
    unsigned long start_stack, end_stack, pages_stack;
    unsigned long start_shared, end_shared, pages_shared;
    unsigned long start_bss, end_bss, pages_bss;
    
	pgd_info_t* pgd;
    pud_info_t* pud;
    pmd_info_t* pmd;
    pte_info_t* pte;
} virt_addr_info_t;

virt_addr_info_t* init_virt_addr_info(void);
void clear_virt_addr_info(virt_addr_info_t* v);
void update_virt_addr_info(virt_addr_info_t* v);

#endif