#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/gfp.h>
#include "main.h"
#include "klib.h"
#include <stdlib.h>

MODULE_LICENSE("GPL");
struct idt_dsc current_idt;


extern void syscall_handler(int id, void* arg);

/* Allocates a new IDT via imp_copy_idt.
 * Index 15 of IDT is used for
 * custom system call handler. Copies
 * the entry corresponding to the current
 * system call handler (index 128) to 
 * index 15 in the new IDT. Sets the lower16 
 * and higher16 fields at index 15 to the 
 * lower 16 bits and higher 16 bits
 * of syscall_handler respectively. Loads
 * the new IDT using imp_load_idt. Saves
 * the original IDT in a global variable.
 */
static void register_syscall(void)
{
	unsigned long syscall_addr = (unsigned long)syscall_handler;
	unsigned long lower_16 = syscall_addr & 0xFFFF0000;
	unsigned long higher_16 = syscall_addr & 0xFFFF0000;
	struct idt_entry new_idt_entry = malloc(sizeof(struct idt_entry));
	struct idt_dsc ds = malloc(sizeof(struct idt_dsc));
	imp_copy_idt(ds);
	original_idt = ds;
	struct idt_entry *index_15 = ds->base + (64*15);
	struct idt_entry *index_128 = ds->base + (64*128);
	memcpy(index_15, index_128, 64);
	index_15->lower16 = lower_16;
	index_15->higher16 = higher_16;
	current_idt = ds;
}

/* If the current IDT is not the original one
 * (can be checked using the global variable),
 * loads the original IDT using imp_load_idt.
 * Frees the previous IDT using imp_free_desc.
 */
static void unregister_syscall(void)
{
	struct idt_dsc original_idt = malloc(sizeof(struct idt_dsc));
	imp_copy_idt(current_idt, original_idt);
	if(current_idt->base != original_idt->base){
		imp_load_idt(current_idt, original_idt);
	}
	imp_free_desc(current_idt);
}

static long device_ioctl(struct file *file, unsigned int ioctl, unsigned long arg)
{
	switch (ioctl)
	{
		case REGISTER_SYSCALL:
			register_syscall();
			break;
		case UNREGISTER_SYSCALL:
			unregister_syscall();
			break;
		default:
			printk("NOT a valid ioctl\n");
			return 1;
	}
	return 0;
}

static struct file_operations fops = 
{
	.unlocked_ioctl = device_ioctl,
};

int start_module(void)
{
	if (register_chrdev(MAJOR_NUM, MODULE_NAME, &fops) < 0)
	{
		printk("PANIC: module loading failed\n");
		return 1;
	}
	printk("module loaded successfully\n");
	return 0;
}

void exit_module(void)
{
	unregister_chrdev(MAJOR_NUM, MODULE_NAME);
	printk("module unloaded successfully\n");
}

module_init(start_module);
module_exit(exit_module);
