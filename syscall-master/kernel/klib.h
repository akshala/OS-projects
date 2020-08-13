#ifndef _KLIB_H_
#define _KLIB_H_

/* You may refer to Intel Sofware Manual for detailed description. */
struct idt_entry
{
	unsigned short lower16; /*lower 16 bits of interrupt handler */
	unsigned short sel;
	unsigned short flags:13;
	unsigned short dpl:2;
	unsigned short present:1;
	unsigned short higher16; /*higher 16 bits of interrupt handler */
} __attribute__((packed));

struct idt_desc
{
	unsigned short size;   /* size of idt table */
	struct idt_entry *base; /* base address of idt table */
} __attribute__((packed));


void print_kernel(void __user *arg);
void imp_load_idt(struct idt_desc *new, struct idt_desc *old);
int imp_copy_idt(struct idt_desc *ds);
void imp_free_desc(struct idt_desc *desc);
asmlinkage void syscall_handler_k(int id, void *arg);

#endif
