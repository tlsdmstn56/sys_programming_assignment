#ifndef BUDDYINFO_H_
#define BUDDYINFO_H_

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/seq_file.h>


/* buddy information */
typedef struct buddy_info_zone{
	unsigned long nr_free[MAX_ORDER];
	char* type;
} zone_info_t;

typedef struct buddy_info_node{
	zone_info_t zone[MAX_NR_ZONES];
	int node_id;
	struct buddy_info_node* next;
} node_info_t;

typedef struct buddy_info_node_array{
	node_info_t* head;
	unsigned int size;
} node_info_list_t;

node_info_list_t* init_buddy_info(void);
void clear_buddy_info(node_info_list_t* array);

void update_buddy_info(node_info_list_t* array);
void print_node_info(struct seq_file* s,node_info_list_t* array);

#endif