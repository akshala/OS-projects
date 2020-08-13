#ifndef __THREAD_H__
#define __THREAD_H__

typedef void (*func_t)(void*);

void create_thread(func_t func, void *param);
void thread_yield();
void thread_exit();
void wait_for_all();


#endif
