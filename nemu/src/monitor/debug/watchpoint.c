#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
	int i;
	for(i = 0; i < NR_WP; i ++) {
		wp_pool[i].NO = i;
		wp_pool[i].next = &wp_pool[i + 1];
	}
	wp_pool[NR_WP - 1].next = NULL;

	head = NULL;
	free_ = wp_pool;
}

WP * new_wp(char *exp)
{
	assert(free_!=NULL);
	WP *p=free_;
	p->next=NULL;
	free_=free_->next;
	strcpy(p->exp,exp);
	bool  success=false;
	p->value=expr(p->exp,&success);
	assert(success);
	if(head==NULL)
	{
		head=p;
	}
	else
	{
		WP *tail=head;
		while(tail->next)
		{
			tail=tail->next;
		}
		p->next=tail->next;
		tail->next=p;
	}
	return p;
}

void free_wp(WP* wp)
{
	if(wp==NULL)
	assert(0);
	if(wp==head)
	{
		head=head->next;
		wp->next=free_;
		free_=wp;
	}
	else
	{
       WP *p=head;
	   while(p->next!=wp)
	   p=p->next;
	   WP *q=wp->next;
	   p->next=q;
	   wp->next=free_;
	   free_=wp;
	}
}

/* TODO: Implement the functionality of watchpoint */


