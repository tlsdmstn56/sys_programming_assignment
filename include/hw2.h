#ifndef HW2_H_
#define HW2_H_

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/seq_file.h>    /* seq operations */
#include <linux/uaccess.h>     /* access macros for proc */
#include <linux/proc_fs.h>     /* for proc IO */
#include <linux/hrtimer.h>     /* httimer */

#define ns_to_sec 1000000000ull
#define ns_to_ms 1000000ull
#define MODE_PARAM S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP

#define MODE_PROC S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH
#define PROC_NAME "hw2"

/* proc fs and seq_ops functions */
int hw2_open(struct inode *inode, struct file *file);
void* hw2_seq_start(struct seq_file *s, loff_t *pos);
void* hw2_seq_next(struct seq_file *s, void *v, loff_t *pos);
void hw2_seq_stop(struct seq_file *s, void *v);
int hw2_seq_show(struct seq_file *s, void *v);
void seq_printf_bar(struct seq_file *s);
void print(struct seq_file *s);
struct proc_dir_entry* init_proc_file(void);
void clear_proc_file(void);

/* printing functions */
void print_pgd_flag(struct seq_file* s,unsigned int flags);
void print_pte_flag(struct seq_file* s,unsigned int flags);

/* tasklet */
enum hrtimer_restart my_tasklet_function(struct hrtimer* timer);


#endif