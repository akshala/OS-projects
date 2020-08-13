// apt-get install gcc-multilib

#include "thread.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

// thread metadata
struct thread {
	void *esp;
	struct thread *next;
	struct thread *prev;
};

struct thread *ready_list = NULL;     // ready list
struct thread *cur_thread = NULL;     // current thread

// defined in context.s
void context_switch(struct thread *prev, struct thread *next);

// void print_ready_list_length(struct thread *ptr)
// {
// 	int count = 1;
// 	if (ptr==NULL){
// 		// printf("Ready list has 0 length\n");
// 	}
// 	else
// 	{
// 		while(ptr->next != ready_list){
// 			ptr = ptr->next;
// 			count++;
// 		}
// 		// printf("Ready list length = %d\n", count);
// 	}	
// }


// insert the input thread to the end of the ready list.
static void push_back(struct thread *t)
{
	if(ready_list == NULL){
		// printf("Push: Adding to NULL ready list\n");
		ready_list = t;
		t->next = ready_list;
		t->prev = ready_list;
	}
	else{
		struct thread *ptr = ready_list;
		while(ptr->next != ready_list){
			// printf("Push: Iterating ready list\n");
			ptr = ptr->next;
		}
		ptr->next = t;
		t->prev = ptr;
		ready_list->prev = t;
		t->next = ready_list;
		// printf("Push done\n");
	}
	// print_ready_list_length(ready_list);
}

// remove the first thread from the ready list and return to caller.
static struct thread *pop_front()
{
	if(ready_list == NULL){
		// printf("Popping: NULL ready list\n");
		return NULL;
	}
	else if(ready_list->next == ready_list){
		// printf("Popping: Ready list has only 1 element\n");
		struct thread * returned_thread = ready_list;
		ready_list = NULL;
		return returned_thread;
	}
	else{
		struct thread *returned_thread = ready_list;
		struct thread *ready_list_prev = ready_list->prev;
		struct thread* ready_list_next = ready_list->next;
		// printf("Popping: From head of ready list\n");
		ready_list = ready_list_next;
		ready_list->prev = ready_list_prev;
		ready_list_prev->next = ready_list;
		return returned_thread;
	}
}

// the next thread to schedule is the first thread in the ready list.
// obtain the next thread from the ready list and call context_switch.
static void schedule()
{
	struct thread *prev_thread = cur_thread;
	// printf("Schedule: Calling pop\n");
	cur_thread =  pop_front();
	context_switch(prev_thread, cur_thread);
}

// push the cur_thread to the end of the ready list and call schedule
// if cur_thread is null, allocate struct thread for cur_thread
static void schedule1()
{
	if(cur_thread == NULL){
		cur_thread =  malloc(sizeof(struct thread));
	}
	// printf("Schedule1: Calling push\n");
	push_back(cur_thread);
	// printf("Schedule1: Calling schedule\n");
	schedule();
}

// allocate stack and struct thread for new thread
// save the callee-saved registers and parameters on the stack
// set the return address to the target thread
// save the stack pointer in struct thread
// push the current thread to the end of the ready list
void create_thread(func_t func, void *param)
{
	unsigned *stack = malloc(4096);
	stack += 1024;
	struct thread* created_thread =  malloc(sizeof(struct thread));
	stack -= 1;

	*(void**)(stack) = param;
	*(stack-1) = 0;
	*(func_t*)(stack-2) = func;
	*(stack-3) = 0;
	*(stack-4) = 0;
	*(stack-5) = 0;
	*(stack-6) = 0;

	created_thread->esp = stack-6;
	// printf("Create: Calling push\n");
	push_back(created_thread);	
}

// call schedule1
void thread_yield()
{
	schedule1();
}

// call schedule
void thread_exit()
{
	schedule();
}

// call schedule1 until ready_list is null
void wait_for_all()
{
	while(ready_list != NULL){
		schedule1();
		// ready_list = ready_list->next;
	}
}
