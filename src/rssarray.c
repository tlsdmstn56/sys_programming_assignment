#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#include "../include/rssarray.h"

/* forward declaration */
void rss_array_insert(rss_array_t* rss_array, struct task_struct* p);

rss_array_t* init_rss_array(rss_array_t* rss_array)
{
	int i;
	pr_info("hw2: init rss_array\n");
	rss_array = (rss_array_t*)vmalloc(sizeof(rss_array_t));
	if(rss_array==NULL)
		return NULL;
	for(i=0; i<RSS_ARRAY_MAX_SIZE; ++i)
	{
		rss_array->array[i].rss=0;
		rss_array->array[i].p=NULL;
	}
	rss_array->is_full=false;
	return rss_array;
}

void clear_rss_array(rss_array_t* rss_array)
{
	pr_info("hw2: clearing rss_array...");
	vfree(rss_array);
}

void rss_array_insert(rss_array_t* rss_array, struct task_struct* p)
{
	/* some mm_struct(i.e. kernel thread) is null */
	if(p->mm==NULL)
		return; 
	/* reading rss info from mm_struct */
	unsigned long file_pages = (unsigned long)atomic_long_read(&p->mm->rss_stat.count[MM_FILEPAGES]); 
	unsigned long anon_pages = (unsigned long)atomic_long_read(&p->mm->rss_stat.count[MM_ANONPAGES]); 
	unsigned long shmem_pages = (unsigned long)atomic_long_read(&p->mm->rss_stat.count[MM_SHMEMPAGES]); 
	unsigned long rss = file_pages+anon_pages+shmem_pages;
	/* when rss_array is full (it has 5 process already) */
	if(rss_array->is_full && rss>rss_array->array[0].rss)
	{
		int i;
		int new_pos=-1;
		for (i=1; i<RSS_ARRAY_MAX_SIZE; ++i)
		{
			if(rss>rss_array->array[i].rss)
				new_pos=i;
		}
		if(new_pos==-1)
		{
			new_pos=0;
		}
		else{
			int j;
			for ( j=0; j<new_pos; ++j)
			{
				rss_array->array[j]=rss_array->array[j+1];
			}
		}
		rss_array->array[new_pos].rss=rss;
		rss_array->array[new_pos].p=p;
	}
	else /* when rss_array is NOT full (it has less than 5 processes) */
	{
		int i;
		int new_pos=-1;
		for (i=0; i<RSS_ARRAY_MAX_SIZE; ++i)
		{
			if(rss_array->array[i].p==NULL)
				break;
			if(rss>rss_array->array[i].rss)
				new_pos=i+1;
		}
		if(new_pos==-1)
			new_pos=0;
		if(i==4)
			rss_array->is_full = true;
		int j;
		for ( j=i-1; j>=new_pos; --j)
		{
			rss_array->array[j+1]=rss_array->array[j];
		}
		rss_array->array[new_pos].rss=rss;
		rss_array->array[new_pos].p=p;
	}
	
}

void rss_array_update(rss_array_t* rss_array){
	int i;
	struct task_struct* p;
	pr_info("hw2: updating rss_array\n");
	/* init rss_array */
	for(i=0; i<RSS_ARRAY_MAX_SIZE; ++i)
	{
		rss_array->array[i].rss=0;
		rss_array->array[i].p=NULL;
	}
	rss_array->is_full=false;
	/* inserting process into rss_array */
	for_each_process(p)
	{
		rss_array_insert(rss_array,p);
	}
}