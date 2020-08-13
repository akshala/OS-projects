#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/gfp.h>
#include "main.h"
#include "klib.h"

MODULE_LICENSE("GPL");

struct user_buffer {
	char __user *buf;
	int len;
};

/* prints the user buffer and writes,
 * "syscall done." to the user buffer.
 */
void print_kernel(void __user *arg)
{
	struct user_buffer b;
	const char *result = "syscall done.";
	char buf[128];
	int len;
	int ret;
	
	ret = copy_from_user(&b, arg, sizeof(b));

	if (ret != 0) {
		printk("unable to copy buffer from user1\n");
		return;
	}
	len = b.len;
	if (len <= 0 || len >= 128) {
		printk("invalid length\n");
		return;
	}
	ret = copy_from_user(buf, b.buf, len);
	buf[len] = '\0';
	if (ret != 0) {
		printk("unable to copy buffer from user2\n");
		return;
	}
	printk("user says: %s\n", buf);
	len = strlen(result);
	copy_to_user(b.buf, result, len);
	b.len = len;
	copy_to_user(arg, &b, sizeof(b));
}

/* custom system call handler. */
asmlinkage void syscall_handler_k(int id, void *arg)
{
	if (id == PRINT_KERNEL_SYS) {
		print_kernel(arg);
	}
}

/* allocates memory for a new IDT, copies the
 * contents of the current IDT to the
 * new IDT. Takes a pointer to idt_desc (ds)
 * as input. Sets the base and size in ds to
 * the address of the new IDT and the size of
 * the old IDT. ds can be used to load the new
 * IDT into IDTR register. You can call
 * imp_load_idt for loading an IDT in the
 * IDTR.
 */
int imp_copy_idt(struct idt_desc *ds)
{
	struct idt_entry *newbase, *base;

	asm volatile("sidt %0":"=m" ((*ds)));

	newbase = (struct idt_entry*)__get_free_pages(GFP_KERNEL, 0);
	if (newbase == NULL)
		return 0;
	base = ds->base;
	memcpy(newbase, base, ds->size);
	ds->base = newbase;
	BUG_ON(ds->size >= PAGE_SIZE);
	return 1;
}

/* Takes the idt_desc of new IDT as input.
 * Loads the base and size of new IDT in the
 * IDTR register. Returns the base
 * and size of the previous IDT via old.
 */
void imp_load_idt(struct idt_desc *new, struct idt_desc *old)
{
	asm volatile("sidt %0":"=m" (*old));
	asm volatile("lidt %0":"=m" (*new));
}

/* Frees the IDT allocated in imp_copy_idt */
void imp_free_desc(struct idt_desc *desc)
{
	free_pages((unsigned long)desc->base, 0);
}
