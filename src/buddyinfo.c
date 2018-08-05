#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mmzone.h>
#include <linux/slab.h>
#include <linux/seq_file.h>

#include "../include/buddyinfo.h"

/* these functions are not exported, copied from kernel source code */
/* to use while traversing nodes in online */
struct pglist_data *first_online_pgdat(void)
{
	return NODE_DATA(first_online_node);
}
struct pglist_data *next_online_pgdat(struct pglist_data *pgdat)
{
	int nid = next_online_node(pgdat->node_id);

	if (nid == MAX_NUMNODES)
		return NULL;
	return NODE_DATA(nid);
}

node_info_list_t* init_buddy_info(void)
{
	unsigned int size;
	node_info_t* prev,*tmp;
	bool first;
	pg_data_t* pgdat;
	node_info_list_t* array;
	pr_info("hw2: initiating buddy info...");
	array = (node_info_list_t*) kmalloc(sizeof(node_info_list_t),GFP_KERNEL);
	if(array==NULL)
		return NULL;
	prev=NULL;
	tmp=NULL;
	first=true;
	size=0;
	/* finding nodes in online */
	for_each_online_pgdat(pgdat)
	{
		size++;
		tmp = (node_info_t*) kmalloc(sizeof(node_info_t),GFP_KERNEL);
		if(!first)
		{
            prev->next=tmp;
			prev=tmp;
		}
		else
		{
			array->head=tmp;
			prev=tmp;
			first=false;
		}
	}
	prev->next=NULL;
	array->size=size;

	return array;
}

void clear_buddy_inft(node_info_list_t* array)
{
	node_info_t* curr, *tmp;
	int size;
	if(array==NULL)
		return;
	pr_info("exiting buddy_info size: %d",array->size);
	curr = array->head;
	size = array->size;
	while(curr!=NULL)
	{
		tmp=curr;
		curr=curr->next;
		kfree(tmp);
		size--;
	}
	kfree(array);
	/* to check if there is any memory leak */
	pr_info("exiting buddy_info done -> size: %d",size);
}

void update_buddy_info(node_info_list_t* array)
{
	node_info_t* curr_node;
	int i,j,k;
	pg_data_t* pgdat;
	struct zone* zone;
	i=0; j=0; k=0;
	curr_node = array->head;
	pr_info("updating buddy info");
	for_each_online_pgdat(pgdat)
	{
		curr_node->node_id = pgdat->node_id;
		j=0;
		for(zone=pgdat->node_zones;
			zone < pgdat->node_zones + MAX_NR_ZONES;
			zone++)
			{
				for(k=0;k<MAX_ORDER;++k)
				{
					curr_node->zone[j].nr_free[k]
						= zone->free_area[k].nr_free;
				}
				curr_node->zone[j].type=(char*)zone->name;
				curr_node->zone[j].initialized=zone->initialized;
				++j;
			}
		++i;
		curr_node = curr_node->next;
	}
}

void print_node_info(struct seq_file* s,node_info_list_t* array)
{
	int j,k;
	node_info_t* curr_node;
	curr_node = array->head;
	pr_info("printing buddy info");
	while(curr_node!=NULL)
	{
		for(j=0;j<MAX_NR_ZONES;++j)
		{
			{
				seq_printf(s,"Node %d Zone %8s", 
					curr_node->node_id,
					curr_node->zone[j].type);
				for(k=0;k<MAX_ORDER;++k)
				{
					seq_printf(s,"%6lu",
						curr_node->zone[j].nr_free[k]);
				}
				seq_printf(s,"\n");
			}
		}
		curr_node = curr_node->next;
	}
}
