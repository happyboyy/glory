#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
	int NO;
	struct watchpoint *next;
	char exp[32];
	uint32_t value;

	/* TODO: Add more members if necessary */


} WP;
WP * new_wp(char *exp);
void free_wp(WP* wp);
int test_change();
void delete_wp(int no);
void print_wp();
#endif
