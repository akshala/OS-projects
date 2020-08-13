//apt-get install gcc-multilib

#include "thread.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>


// thread metadata
struct thread {
	void *esp;
	void *start_address_stack;
	struct thread *next;
	struct thread *prev;
};

struct thread *ready_list = NULL;     // ready list
struct thread *cur_thread = NULL;     // current thread
struct thread *garbage_thread = NULL;

struct lock_node{
	struct lock *lock_val;
	struct lock_node *next;
};

struct lock_node *lock_list = NULL;       // list of locks

// defined in context.s
void context_switch(struct thread *prev, struct thread *next);

// void print_ready_list_length(struct thread *ptr)
// {
// 	int count = 1;
// 	if (ptr==NULL){
// 		printf("Ready list has 0 length\n");
// 	}
// 	else
// 	{
// 		while(ptr->next != ready_list){
// 			ptr = ptr->next;
// 			count++;
// 		}
// 		printf("Ready list length = %d\n", count);
// 	}	
// }


// insert the input thread to the end of the ready list.
static void push_back(struct thread *t)
{
	// printf("Pushing ptr %p\n",t);
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
		// printf("push_back: Push done\n");
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
		struct thread * returned_thread = ready_list;
		ready_list = NULL;
		// printf("Popping: Ready list has only 1 element, returning %p\n", returned_thread);
		return returned_thread;
	}
	else{
		struct thread *returned_thread = ready_list;
		struct thread *ready_list_prev = ready_list->prev;
		struct thread* ready_list_next = ready_list->next;
		ready_list = ready_list_next;
		ready_list->prev = ready_list_prev;
		ready_list_prev->next = ready_list;
		// printf("Popping: From head of ready list, returning %p\n", returned_thread);
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
	if(cur_thread){
		context_switch(prev_thread, cur_thread);
	} 
	else{
		// printf("null readyList\n");
		exit(1);
	}
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
	struct thread* created_thread =  malloc(sizeof(struct thread));
	unsigned *stack = malloc(4096);
	created_thread->start_address_stack = stack;
	// printf("Allocated stack ------------------------- %p\n",stack);
	stack += 1024;
	stack -= 1;

	*(void**)(stack) = param;
	*(stack-1) = 0;
	*(func_t*)(stack-2) = func;
	*(stack-3) = 0;
	*(stack-4) = 0;
	*(stack-5) = 0;
	*(stack-6) = 0;

	created_thread->esp = stack-6;
	// printf("Created thread ######################## %p\n",created_thread);
	// printf("saving stack ++++++++++++++++++++++++++++ %p\n",created_thread->esp);
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
	// printf("free %p\n", garbage_thread);
	struct thread *exit_thread = garbage_thread;
	if (garbage_thread != NULL) {
		// printf("exit_thread --------------------------- %p\n", exit_thread);
		// printf("thread->esp --------------------------- %p\n", (exit_thread->esp+108));
		// printf("Freeing --------------------------- %p\n", (unsigned *)((exit_thread->esp)-4068+108));
		// free((unsigned *)((exit_thread->esp)-4068+108));
		free(exit_thread->start_address_stack);
		free(exit_thread);
	}
	garbage_thread = cur_thread;
	// printf("garbage_thread --------------------------- %p\n", garbage_thread);
	schedule();
}

// call schedule1 until ready_list is null
void wait_for_all()
{
	// printf("wait_for_all\n");
	while (1) {
		int lock_lists_empty = 1;
		while (ready_list != NULL){
			// printf("Yielding to ready-list thread\n");
			thread_yield();
		}
		struct lock_node *lock_ptr = lock_list;
		while(lock_ptr != NULL){
			if (lock_ptr->lock_val->wait_list != NULL){
				wakeup(lock_ptr->lock_val);
				// printf("Yielding to locked thread\n");
				thread_yield();
				lock_lists_empty = 0;
			}
			lock_ptr = lock_ptr->next;
		}
		if (ready_list == NULL && lock_lists_empty){
			return;
		}
	}
}

void add_lock_to_lock_list(struct lock_node *new_lock){
	// printf("Entering add_lock_to_lock_list\n");
	struct lock_node *lock_ptr = lock_list;
	struct lock_node *prev_lock_ptr = NULL;
	int flag = 1;
	while(lock_ptr != NULL){
		if(lock_ptr->lock_val == new_lock->lock_val){
			flag = 0;
			break;
		}
		prev_lock_ptr = lock_ptr;
		lock_ptr = lock_ptr->next;
	}
	if(flag){
		if (prev_lock_ptr!=NULL) {
			prev_lock_ptr->next = new_lock;
		} 
		else {
			lock_list = new_lock;
		}
	}
}

void sleep(struct lock *lock)
{
	struct lock_node *new_lock = malloc(sizeof(struct lock_node));
	new_lock->lock_val = lock;
	add_lock_to_lock_list(new_lock);

	if((struct thread*)(lock->wait_list) == NULL){
		// printf("Push: Adding to NULL wait list\n");
		lock->wait_list = (void*)cur_thread;
		cur_thread->next = (struct thread*)lock->wait_list;
		cur_thread->prev = (struct thread*)lock->wait_list;
	}
	else{
		struct thread *ptr = (struct thread*)(lock->wait_list);
		while(ptr->next != (struct thread*)lock->wait_list){
			// printf("Push: Iterating wait list\n");
			ptr = ptr->next;
		}
		ptr->next = cur_thread;
		cur_thread->prev = ptr;
		((struct thread*)(lock->wait_list))->prev = cur_thread;
		cur_thread->next = (struct thread*)(lock->wait_list);
		// printf("Push done\n");
	}
	schedule();
}

   void wakeup(struct lock *lock)
{
	// printf("wakeup\n");
	struct thread *returned_thread;
	if((struct thread*)lock->wait_list == NULL){
		// printf("Popping: NULL wait list\n");
		returned_thread = NULL;
	}
	else if(((struct thread*)(lock->wait_list))->next == (struct thread*)(lock->wait_list)){
		// printf("Popping: wait list has only 1 element\n");
		returned_thread = (struct thread*)(lock->wait_list);
		lock->wait_list = NULL;
		push_back(returned_thread);
	}
	else{
		returned_thread = (struct thread*)(lock->wait_list);
		struct thread *wait_list_prev = ((struct thread*)(lock->wait_list))->prev;
		struct thread *wait_list_next = ((struct thread*)(lock->wait_list))->next;
		// printf("Popping: From head of wait list\n");
		lock->wait_list = (void*)wait_list_next;
		((struct thread*)(lock->wait_list))->prev = wait_list_prev;
		wait_list_prev->next = (struct thread*)(lock->wait_list);
		push_back(returned_thread);
	}
}