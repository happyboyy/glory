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
	bool  success=success;
	p->value=expr(p->exp,&success);

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

void delete_wp(int no)
{
	WP *p=head;
	while(p)
	{
		if(p->NO==no)
		free_wp(p);
	}
}

void print_wp()
{
	WP *p=head;
	while(p)
	{
		printf("%d\t",p->NO);
		printf("%s\t",p->exp);
		printf("%d\t",p->value);
		printf("\n");
		p=p->next;
	}
}

int test_change()
{
	WP *p=head;
	int count=0;
	while(p)
	{
		count++;
		p=p->next;
	}
	int i;
	bool success;
	WP *q=head;
	for(i=0;i<count;i++)
	{
		if(q->value!=expr(q->exp,&success))
		return 1;
	}
	return 0;
}
/* TODO: Implement the functionality of watchpoint */


