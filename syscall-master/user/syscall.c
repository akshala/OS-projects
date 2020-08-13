#define _GNU_SOURCE

#include "../kernel/main.h"
#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

struct user_buffer
{
	char *buf;
	int len;
};

extern void syscall_u(int syscall_id, void *ptr) __attribute__((stdcall));

int main(int argc, char **argv)
{
	char name[100];
	int fd;
	int syscall_id = PRINT_KERNEL_SYS;
	char buf[128];
	const char *result = "syscall done.";
	struct user_buffer ubuf;
	int ret;

	ret = snprintf(name, sizeof(name), "/dev/%s", MODULE_NAME);
	name[ret] = '\0';

	fd = open(name, O_RDONLY);
	if (fd < 0)
	{
		perror("unable to open dev");
		return 0;
	}

	/* enables custom syscall handler. */
	if (ioctl(fd, REGISTER_SYSCALL, NULL) != 0)
	{
		perror("ioctl REGISTER_SYACALL error");
		abort();
	}

	ret = snprintf(buf, 128, "Hey, checkout my new system call!");
	buf[ret] = '\0';

	ubuf.buf = buf;
	ubuf.len = ret;

	/* syscall_u takes syscall_id and a
	 * user buffer as argument. syscall passes the 
	 * syscall_id and buffer to the syscall_handler
	 * present at index 15 in the IDT.
	 */
	syscall_u(syscall_id, &ubuf);

	/* PRINT_KERNEL system call handler stores 
	 * the string "syscall done." in the user's buffer
	 * passed to it.
	 */
	if (!memcmp(buf, result, ubuf.len))
	{
		printf("syscall done!\n");
	}
	else {
		printf("syscall failed!\n");
	}

   /* restores the original IDT */
	if (ioctl(fd, UNREGISTER_SYSCALL, NULL) != 0)
	{
		perror("ioctl REGISTER_SYACALL error");
		abort();
	}

	close(fd);

	return 0;
}
