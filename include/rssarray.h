#ifndef RSSARRAY_H_
#define RSSARRAY_H_

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>       /* for task_struct */

#define RSS_ARRAY_MAX_SIZE 5

/* rss info data structures and API */
typedef struct _rss_elem 
{
	struct task_struct* p;
	unsigned long rss;
} rss_elem_t;
typedef struct _rss_array 
{
	rss_elem_t array[RSS_ARRAY_MAX_SIZE];
	bool is_full;
} rss_array_t;

rss_array_t* rss_array_init(rss_array_t* rss_array);
void rss_array_clear(rss_array_t* rss_array);
void rss_array_update(rss_array_t* rss_array);

#endif