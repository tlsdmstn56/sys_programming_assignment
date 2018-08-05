#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h> /* for module parameter */
#include <linux/interrupt.h>
#include <linux/proc_fs.h>     /* for proc IO */
#include <linux/seq_file.h>    /* for seq_file */
#include <linux/uaccess.h>     /* access macros for proc */
#include <linux/sched.h>       /* for task_struct */
#include <linux/timekeeping.h> /* ktime_* functions */
#include <linux/hrtimer.h>
#include <linux/types.h>

#include "../include/hw2.h"
#include "../include/buddyinfo.h"
#include "../include/virtmeminfo.h"
#include "../include/rssarray.h"

#define STUDENT_NAME "Eunsoo Sheen"
#define STUDENT_ID "2014122011"
/* Some exported kernel function need GPL license */
MODULE_LICENSE("GPL");
MODULE_AUTHOR(STUDENT_NAME);

/* Module Parameter: updating period in sec */
static uint period = 1;
module_param( period, uint, MODE_PARAM);
MODULE_PARM_DESC(period, "Updating period in seconds");

static unsigned long long last_update_time_ns = 0;
static node_info_list_t* buddy_info_array;
static rss_array_t* rss_array;
static virt_addr_info_t* virt_addr_info;
static struct tasklet_hrtimer hw2_tasklet;
/***************************************************/
/*               seq operation                     */ 
/***************************************************/
struct seq_operations hw2_seq_ops = {
    .start = hw2_seq_start,
    .next  = hw2_seq_next,
    .stop  = hw2_seq_stop,
    .show  = hw2_seq_show
};

void *hw2_seq_start(struct seq_file *s, loff_t *pos)
{
    // seq operation will be excuted only once
    if ( *pos == 0 )
    {	
        return &rss_array;
    }
    else
    {
        *pos = 0;
        return NULL;
    }
}

void *hw2_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
    (*pos)++;
    return NULL;
}

void hw2_seq_stop(struct seq_file *s, void *v)
{
    /* there is nothing to do when ending seq */
}

int hw2_seq_show(struct seq_file *s, void *v)
{
    print(s);
    return 0;
}
/***************************************************/
/*   file operation(in include/linux/proc_fs.h)    */ 
/***************************************************/
struct file_operations hw2_file_ops = {
    .owner   = THIS_MODULE,
    .open    = hw2_open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = seq_release
};
int hw2_open(struct inode *inode, struct file *file)
{
    return seq_open(file, &hw2_seq_ops);
}

void seq_printf_bar(struct seq_file *s)
{
	seq_printf(s,"**************************************************************************************");
	seq_printf(s,"\n");
}

void print(struct seq_file *s)
{
	int i;
	pr_info("hw2: printing...");
	seq_printf_bar(s);
	seq_printf(s,"Student ID: %s	Name: %s\n", STUDENT_ID, STUDENT_NAME);
	seq_printf(s,"Last update time %llu ms\n", last_update_time_ns/ns_to_ms);
	seq_printf_bar(s);

	seq_printf(s,"Buddy Information\n");
	seq_printf_bar(s);
	/* printing budd system info */
	print_node_info(s,buddy_info_array);
	seq_printf_bar(s);

	/* printing rss info */
	seq_printf(s,"RSS Information\n");
	seq_printf_bar(s);
	
	/* printing rss info */
	seq_printf(s,"%-4s","pid");
	seq_printf(s,"%10s","rss");
	seq_printf(s,"%20s\n","comm");
	for(i=RSS_ARRAY_MAX_SIZE-1; i>=0;--i) {
		seq_printf(s,"%-4d",rss_array->array[i].p->pid);
		seq_printf(s,"%10lu",rss_array->array[i].rss);
		seq_printf(s,"%20s\n",rss_array->array[i].p->comm);
	}
	seq_printf_bar(s);

	/* printing randomly selected process virt mem addr info */
	seq_printf(s,"Virtual Memory Address Information\n");
	seq_printf(s,"Process (%15s:%d)\n", virt_addr_info->p->comm, 
											virt_addr_info->p->pid);
	seq_printf_bar(s);	
	seq_printf(s,"0x%08lx - 0x%08lx : Code Area, %lu page(s)\n",
			virt_addr_info->start_code, virt_addr_info->end_code, 
			virt_addr_info->pages_code);
	seq_printf(s,"0x%08lx - 0x%08lx : Data Area, %lu page(s)\n",
			virt_addr_info->start_data, virt_addr_info->end_data, 
			virt_addr_info->pages_data);
	seq_printf(s,"0x%08lx - 0x%08lx : BSS Area, %lu page(s)\n",
			virt_addr_info->start_bss, virt_addr_info->end_bss, 
			virt_addr_info->pages_bss);
	seq_printf(s,"0x%08lx - 0x%08lx : Heap Area, %lu page(s)\n",
			virt_addr_info->start_heap, virt_addr_info->end_heap, 
			virt_addr_info->pages_heap);
	seq_printf(s,"0x%08lx - 0x%08lx : Shared Libraries Area, %lu page(s)\n",
			virt_addr_info->end_shared, virt_addr_info->start_shared, 
			virt_addr_info->pages_shared);
	seq_printf(s,"0x%08lx - 0x%08lx : Stack Area, %lu page(s)\n",
			virt_addr_info->end_stack, virt_addr_info->start_stack, 
			virt_addr_info->pages_stack);

	// 1 level paging (PGD Info)
	seq_printf_bar(s);
	seq_printf(s,"1 Level Paging: Page Directory Entry Information \n");
	seq_printf_bar(s);
	
	seq_printf(s,"PGD     Base Address            : 0x%08llx\n", virt_addr_info->pgd->pgd_base_addr);
	seq_printf(s,"code    PGD Address             : 0x%08llx\n", virt_addr_info->pgd->pgd_addr);
	seq_printf(s,"        PGD Value               : 0x%08llx\n", virt_addr_info->pgd->pgd_val);
	seq_printf(s,"        +PFN Address            : 0x%08llx\n", virt_addr_info->pgd->pfn_addr);
	print_pgd_flag(s,virt_addr_info->pgd->pgd_flags);
	
	// 2 level paging (PUD Info)
	seq_printf_bar(s);
	seq_printf(s,"2 Level Paging: Page Upper Directory Entry Information \n");
	seq_printf_bar(s);
	
	seq_printf(s,"code    PUD Address             : 0x%08llx\n", virt_addr_info->pud->pud_addr);
	seq_printf(s,"        PUD Value               : 0x%08llx\n", virt_addr_info->pud->pud_val);
	seq_printf(s,"        +PFN Address            : 0x%08llx\n", virt_addr_info->pud->pfn_addr);
	
	// 3 level paging (PMD Info)
	seq_printf_bar(s);
	seq_printf(s,"3 Level Paging: Page Middle Directory Entry Information \n");
	seq_printf_bar(s);

	seq_printf(s,"code    PMD Address             : 0x%08llx\n", virt_addr_info->pmd->pmd_addr);
	seq_printf(s,"        PMD Value               : 0x%08llx\n", virt_addr_info->pmd->pmd_val);
	seq_printf(s,"        +PFN Address            : 0x%08llx\n", virt_addr_info->pmd->pfn_addr);

	// 4 level paging (PTE Info)
	seq_printf_bar(s);
	seq_printf(s,"4 Level Paging: Page Table Entry Information \n");
	seq_printf_bar(s);

	seq_printf(s,"code    PTE Address             : 0x%08llx\n", virt_addr_info->pte->pte_addr);
	seq_printf(s,"        PTE Value               : 0x%08llx\n", virt_addr_info->pte->pte_val);
	seq_printf(s,"        +Page Base Address      : 0x%08llx\n", virt_addr_info->pte->pg_base_addr);	

	/* printing pyhsical address info */
	print_pte_flag(s,virt_addr_info->pte->pte_flags);

	seq_printf_bar(s);
	seq_printf(s,"Start of Physical Address       : 0x%08llx\n", virt_addr_info->pte->phys_addr);

	seq_printf_bar(s);
}

void print_pgd_flag(struct seq_file* s,unsigned int flags)
{
	// print pgd flag (pgd[0:7])
	seq_printf(s,"        +Page Size              : %s\n", 
		get_page_size_pgd(flags)?"4MB":"4KB");

	seq_printf(s,"        +Accessed Bit           : %s\n", 
		get_access_bit_pgd(flags)?"1":"0");

	seq_printf(s,"        +Cache Disable Bit      : %s\n", 
		get_cache_disable_bit_pgd(flags)?"true":"false");

	seq_printf(s,"        +Page Write-Through     : %s\n", 
		get_write_through_pgd(flags)?"write through":"write-back");

	seq_printf(s,"        +User/Supervisor Bit    : %s\n", 
		get_mode_bit_pgd(flags)?"user":"supervisor");

	seq_printf(s,"        +Read/Write Bit         : %s\n", 
		get_rw_bit_pgd(flags)?"read-write":"read-only");
		
	seq_printf(s,"        +Page Present Bit       : %s\n", 
		get_page_present_pgd(flags)?"1":"0");
}
void print_pte_flag(struct seq_file* s,unsigned int flags)
{
	// print pte flag (pte[0:7])
	seq_printf(s,"        +Dirty Bit              : %s\n", 
		get_dirty_bit_pte(flags)?"1":"0");
	seq_printf(s,"        +Accessed Bit           : %s\n", 
		get_access_bit_pte(flags)?"1":"0");
	seq_printf(s,"        +Cache Disable Bit      : %s\n", 
		get_cache_disable_bit_pte(flags)?"true":"false");
	seq_printf(s,"        +Page Write-Through     : %s\n", 
		get_pg_write_through_pte(flags)?"write-through":"write-back");
	seq_printf(s,"        +User/Supervisor        : %s\n", 
		get_mode_bit_pte(flags)?"user":"supervisor");
	seq_printf(s,"        +Read/Write Bit         : %s\n", 
		get_rw_bit_pte(flags)?"read-write":"read-only");
	seq_printf(s,"        +Page Present Bit       : %s\n", 
		get_page_present_pte(flags)?"1":"0");
}

/***************************************************/
/*           tasklet_hrtimer setting               */ 
/***************************************************/

enum hrtimer_restart hw2_tasklet_function(struct hrtimer* timer)
{
	ktime_t currtime , interval;
	currtime  = ktime_get();
	last_update_time_ns = ktime_to_ns(currtime);
	pr_info("hw2: tasklet was called %llus\n",
		last_update_time_ns/ns_to_sec);
	// update top 5 process which has the larget rss
	rss_array_update(rss_array);
	// update buddy system info
	update_buddy_info(buddy_info_array);
	// randomly select process and get address info
	update_virt_addr_info(virt_addr_info);
	// reset the timer
	interval = ktime_set(0,period*ns_to_sec); 
  	hrtimer_forward(timer, currtime , interval);
	return HRTIMER_RESTART;
}

/***************************************************/
/*           module load and clear                 */ 
/***************************************************/
static int __init hw2_init_module( void )
{ 
	/* create proc file */
	struct proc_dir_entry* entry;
	entry = proc_create(PROC_NAME, MODE_PROC, NULL, &hw2_file_ops);
    if (entry==NULL) 
    {
       pr_info("hw2: ERROR: fail at proc file creation");
        return -ENOMEM;
    }
	/* alloc rss_array data structure and init */
	rss_array = rss_array_init(rss_array);
	if (rss_array == NULL)
	{
		// if fail free all the resources
		remove_proc_entry(PROC_NAME, NULL);
		pr_info("hw2: mem alloc fail while init_rss_array\n");
		return -ENOMEM;
	}
	rss_array_update(rss_array);

	/* alloc buddy_info_array data structure and init */
	buddy_info_array = init_buddy_info();
	if (buddy_info_array == NULL)
	{
		remove_proc_entry(PROC_NAME, NULL);
		rss_array_clear(rss_array);
		pr_info("hw2: mem alloc fail while init buddy info\n");
		return -ENOMEM;
	}
	update_buddy_info(buddy_info_array);

	/* alloc virt_addr_info and init */
	virt_addr_info = init_virt_addr_info( virt_addr_info );
	if(virt_addr_info == NULL){
		remove_proc_entry(PROC_NAME, NULL);
		rss_array_clear(rss_array);
		clear_buddy_info(buddy_info_array);
		return -ENOMEM;
	}
	update_virt_addr_info(virt_addr_info);
	
	/* tasklet and hrtimer init and starting */
	tasklet_hrtimer_init(&hw2_tasklet,hw2_tasklet_function,
							CLOCK_MONOTONIC,HRTIMER_MODE_REL);
	tasklet_hrtimer_start(&hw2_tasklet,
							ktime_set(0,period*ns_to_sec),HRTIMER_MODE_REL);
	pr_info("Initiating module ...\n");
	return 0;
}



static void __exit hw2_cleanup_module( void )
{
    pr_info("Exiting module ...\n");
	// stop the timer
	tasklet_hrtimer_cancel(&hw2_tasklet);
	// free all the resources
	clear_virt_addr_info(virt_addr_info);
	rss_array_clear(rss_array);
    clear_buddy_info(buddy_info_array);
	// remove proc entry
	remove_proc_entry(PROC_NAME, NULL);
}

module_init(hw2_init_module);
module_exit(hw2_cleanup_module);
