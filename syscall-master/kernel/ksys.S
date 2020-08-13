#include <linux/linkage.h>

# syscall_handler_k routine in klib.c implements the actual system
# call handler. The arguments to syscall_handler_k are passed on the
# stack. Kernel's %fs segment register value is 216 that may be different
# from user applications. Kernel doesn't use %gs segment register.
# %ds and %es are the same for user and kernel.

#void syscall_handler_k(int id, void *arg);

# Save the caller-saved registers.
# Save the user's %fs register.
# set %fs to 216.
# pass user's arguments to syscall_handler_k
# restore %fs and caller-saved registers.
# return to user-mode.

#system call handler
ENTRY(syscall_handler)

# write some code here
push %eax
push %ecx
push %edx
push %fs

mov $216, %ax
mov %ax, %fs

push %ecx
push %eax

call syscall_handler_k

# write some code here
add $8 %esp

pop %fs
pop %edx
pop %ecx
add $4 %esp

iret
ENDPROC(syscall_handler)
