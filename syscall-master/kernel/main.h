#ifndef MAIN_H
#define MAIN_H

#include <linux/ioctl.h>

#define MODULE_NAME "cse231"
#define MAJOR_NUM 100

#define REGISTER_SYSCALL     _IOR(MAJOR_NUM, 1, int)
#define UNREGISTER_SYSCALL   _IOR(MAJOR_NUM, 2, int)
#define PRINT_KERNEL_SYS     3

#endif
