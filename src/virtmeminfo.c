#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/random.h> /* for random number */
#include <linux/vmalloc.h> /* to use vmalloc */
#include <linux/mm_types.h> /* mm_struct */

#include "../include/virtmeminfo.h"

/* forward declaration */
void update_pgd_info(struct task_struct* p,pgd_info_t* pgd_info);
void update_pud_info(struct task_struct* p,pgd_info_t* pgd_info,pud_info_t* pud_info);
void update_pmd_info(struct task_struct* p,pud_info_t* pud_info,pmd_info_t* pmd_info);
void update_pte_info(struct task_struct* p,pmd_info_t* pmd_info, pte_info_t* pte_info);
void update_virt_addr(struct task_struct *p,virt_addr_info_t* v);


virt_addr_info_t* init_virt_addr_info(virt_addr_info_t* v){
    pr_info("hw2: initiating virt_addr_info...");
    v = (virt_addr_info_t*)vmalloc(sizeof(virt_addr_info_t));
    if(v==NULL)
        goto out1;
    
    v->pgd = (pgd_info_t*)vmalloc(sizeof(pgd_info_t));
    if(v->pgd==NULL)
        goto out2;

    v->pud = (pud_info_t*)vmalloc(sizeof(pud_info_t));
    if(v->pud==NULL)
        goto out3;

    v->pmd = (pmd_info_t*)vmalloc(sizeof(pmd_info_t));
    if(v->pmd==NULL)
        goto out4;
    
    v->pte = (pte_info_t*)vmalloc(sizeof(pte_info_t));
    if(v->pte==NULL)
        goto out5;
    
    return v;
    /* when allocation fail, free the resources*/
out5: vfree(v->pmd);
out4: vfree(v->pud);
out3: vfree(v->pgd);
out2: vfree(v);
out1: return NULL;
}

void clear_virt_addr_info(virt_addr_info_t* v)
{
    pr_info("hw2: clearing virt_addr_info...");
    vfree(v->pte);
    vfree(v->pmd);
    vfree(v->pud);
    vfree(v->pgd);
    vfree(v);
}

void update_virt_addr_info(virt_addr_info_t* v)
{
    struct task_struct* p;
    struct mm_struct* mm;
    unsigned int rand;
    pr_info("hw2: updating... virt_addr_info\n");
select_again:
    p=NULL;
    /* randomly select process */
    for_each_process(p)
    {
        get_random_bytes(&rand, sizeof(rand));
        if(rand%100<2 && p->mm!=NULL){
            pr_info("pid: %d, %s is selected\n",p->pid,p->comm);
            break;
        }
        /* for test */
        // if(!strcmp("evolution-calen",p->comm))
        //     break;
    }
    if(p->mm==NULL)
        goto select_again;
    v->p = p;
    mm = p->mm;
    v->start_code = mm->start_code;
    v->end_code   = mm->end_code;
    v->pages_code   = ((mm->end_code-mm->start_code)>>12)+1;
    v->start_data = mm->start_data;
    v->end_data   = mm->end_data;
    v->pages_data   = ((mm->end_data-mm->start_data)>>12)+1;
    v->start_heap = mm->start_brk;
    v->end_heap   = mm->brk;
    v->pages_heap   = ((mm->brk-mm->start_brk)>>12)+1;
    v->start_stack = mm->start_stack;
    v->end_stack = 0;
    v->pages_stack   = 0;
    v->start_shared=mm->mmap_base;
    v->end_shared=0;
    v->pages_shared   = 0;
    v->start_bss=0;
    v->end_bss=0;
    v->pages_bss   = 0;
    /* update virt info of given process */ 
    update_virt_addr(v->p,v); // update remaining field initialized as 0  
    update_pgd_info(v->p, v->pgd);
    update_pud_info(v->p, v->pgd, v->pud);
    update_pmd_info(v->p, v->pud, v->pmd);
    update_pte_info(v->p, v->pmd, v->pte);
}

void update_virt_addr(struct task_struct *p,virt_addr_info_t* v)
{
    struct vm_area_struct *vma = 0;
    bool bss=true, shared=true,stack=true;
    if (p->mm && p->mm->mmap)
    {
        for (vma = p->mm->mmap; vma; vma = vma->vm_next)
        {
            /* finding BSS area */
            if(p->mm->end_data<=vma->vm_start && 
                vma->vm_end< (p->mm->start_brk))
            {
                if(bss)
                {
                    bss=false;
                    v->start_bss = vma->vm_start;
                }
                v->pages_bss +=(vma->vm_end-vma->vm_start)/PAGE_SIZE;
                v->end_bss=vma->vm_end;
            }
            /* finding shared library area */
            else if(vma->vm_start>p->mm->brk&&vma->vm_end <= p->mm->mmap_base)
            {
                if(shared)
                {
                    shared=false;
                    v->end_shared=vma->vm_start;
                }
                v->pages_shared+=((vma->vm_end-vma->vm_start)/PAGE_SIZE);
            }
            /* finding stack area */
            else if( (p->mm->start_stack>vma->vm_end && vma->vm_start>p->mm->mmap_base) ||
                    (vma->vm_start<=p->mm->start_stack && p->mm->start_stack<=vma->vm_end) )
            {
                if(stack)
                {
                    stack=false;
                    v->end_stack = vma->vm_start;
                }
                v->pages_stack+=((vma->vm_end-vma->vm_start)/PAGE_SIZE);
                if(vma->vm_next==NULL)
                    v->start_stack = vma->vm_end;
            }

        }
    }
}

void update_pgd_info(struct task_struct* p,pgd_info_t* pgd_info)
{
    unsigned long pgd_offset;
    pgd_t* pgd_base_addr;
    pr_info("updating... pgd_info");
    if(p==NULL || p->mm==NULL)
        return;
    pgd_base_addr = p->mm->pgd; // get base address from mm->pgd
    pgd_offset = get_pgd_offset(p->mm->start_code); // bit 47-39
    pgd_info->pgd_base_addr = (unsigned long long) pgd_base_addr;
    pgd_info->pgd_addr  = (unsigned long long)(pgd_base_addr+pgd_offset);
    pgd_info->pgd_val   = (pgd_base_addr+pgd_offset)->pgd;
    pgd_info->pfn_addr  = pgd_info->pgd_val>>PFN_SHIFT; // >>12
    pgd_info->pgd_flags = pgd_info->pgd_val & 0xFF; // 0-7 bit
}

void update_pud_info(struct task_struct* p,
                        pgd_info_t* pgd_info, pud_info_t* pud_info)
{
    unsigned long long pud_offset;
    pud_t* pud_base_addr;
    pr_info("updating... pud_info");
    if(p==NULL || p->mm==NULL)
        return;
    /* base address is 20 MSB */
    pud_base_addr = (pud_t*)phys_to_virt( 
                        get_base_addr(pgd_info->pgd_val) );
    /* bit 38-30 */
    pud_offset = get_pud_offset(p->mm->start_code);
    pud_info->pud_addr = (unsigned long long)(pud_base_addr+pud_offset);
    pud_info->pud_val  = (pud_base_addr+pud_offset)->pud;
    /* 40 MSB of pud_val */
    pud_info->pfn_addr = pud_info->pud_val >> PFN_SHIFT;
}

void update_pmd_info(struct task_struct* p,pud_info_t* pud_info,pmd_info_t* pmd_info)
{
    
    pmd_t* pmd_base_addr;
    unsigned long long pmd_offset;
    pr_info("updating... pmd_info");
    if(p==NULL || p->mm==NULL)
        return;
    /* base address is 20 MSB */
    pmd_base_addr = (pmd_t*)phys_to_virt(get_base_addr(pud_info->pud_val));
    /* bit 29-21 */
    pmd_offset = get_pmd_offset(p->mm->start_code);
    pmd_info->pmd_addr = (unsigned long long)(pmd_base_addr+pmd_offset);
    pmd_info->pmd_val  = (pmd_base_addr+pmd_offset)->pmd;
    /* 40 MSB of pmd_val */
    pmd_info->pfn_addr = pmd_info->pmd_val >> PFN_SHIFT;
}

void update_pte_info(struct task_struct* p,pmd_info_t* pmd_info, pte_info_t* pte_info)
{
    pte_t* pte_base_addr; 
    unsigned long pte_offset;
    pr_info("updating... pte_info");
    if(p==NULL || p->mm==NULL)
        return;
    /* base address is 20 MSB */
    pte_base_addr = (pte_t*)phys_to_virt(get_base_addr(pmd_info->pmd_val));
    /* bit 20-12 */
    pte_offset = get_pte_offset(p->mm->start_code);
    pte_info->pte_addr  = (unsigned long long)(pte_base_addr+pte_offset);
    pte_info->pte_val   = (pte_base_addr+pte_offset)->pte;
    /* 40 MSB of pte_val */
    pte_info->pg_base_addr  = pte_info->pte_val>>PFN_SHIFT;
    pte_info->pte_flags = pte_info->pte_val & 0xFF;
    /* 40bit(pg_basd_addr) + 12 bit( in virt addr ) */
    pte_info->phys_addr = (pte_info->pg_base_addr<<12) | 
                               get_page_index(p->mm->start_code);
}