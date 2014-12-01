/* 	File originally from OS-161. This file has been largely edited and 
	implemented by Nathan Verjinski and Kunal Sarkhel 		*/


#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <types.h>

/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

int sys_reboot(int code);
int sys_getpid(pid_t * retval);
int sys_read(int filehandle, char *buf, size_t size);
int sys_write(int filehandle, char *buf, size_t size);
int sys_execv(char * program, char ** args);
int sys_fork(struct trapframe * tf);
int sys_waitpid(pid_t pid, u_int32_t *status, int options, pid_t *retval);
int sys__exit(int result);
int sys_chdir(char * filename, int * retval);


#endif /* _SYSCALL_H_ */
