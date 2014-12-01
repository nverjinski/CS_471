/* 	File originally from OS-161. This file has been largely edited and 
	implemented by Nathan Verjinski and Kunal Sarkhel 		*/


#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <machine/pcb.h>
#include <machine/spl.h>
#include <machine/trapframe.h>
#include <kern/callno.h>
#include <synch.h>
#include <syscall.h>
#include <thread.h>
#include <curthread.h>
#include <addrspace.h>
#include <kern/unistd.h>
#include <vfs.h>
#include <kern/limits.h>
#include <thread.h>

/*
 * System call handler.
 *
 * A pointer to the trapframe created during exception entry (in
 * exception.S) is passed in.
 *
 * The calling conventions for syscalls are as follows: Like ordinary
 * function calls, the first 4 32-bit arguments are passed in the 4
 * argument registers a0-a3. In addition, the system call number is
 * passed in the v0 register.
 *
 * On successful return, the return value is passed back in the v0
 * register, like an ordinary function call, and the a3 register is
 * also set to 0 to indicate success.
 *
 * On an error return, the error code is passed back in the v0
 * register, and the a3 register is set to 1 to indicate failure.
 * (Userlevel code takes care of storing the error code in errno and
 * returning the value -1 from the actual userlevel syscall function.
 * See src/lib/libc/syscalls.S and related files.)
 *
 * Upon syscall return the program counter stored in the trapframe
 * must be incremented by one instruction; otherwise the exception
 * return code will restart the "syscall" instruction and the system
 * call will repeat forever.
 *
 * Since none of the OS/161 system calls have more than 4 arguments,
 * there should be no need to fetch additional arguments from the
 * user-level stack.
 *
 * Watch out: if you make system calls that have 64-bit quantities as
 * arguments, they will get passed in pairs of registers, and not
 * necessarily in the way you expect. We recommend you don't do it.
 * (In fact, we recommend you don't use 64-bit quantities at all. See
 * arch/mips/include/types.h.)
 */

struct fork_struct{
	struct semaphore * sem;
	struct thread * parent_thread;
	struct trapframe * tf;
	struct addrspace * addrspace;
	pid_t child_pid;
};

void
mips_syscall(struct trapframe *tf)
{
	int callno;
	int32_t retval;
	int err;

	assert(curspl==0);

	callno = tf->tf_v0;

	/*
	 * Initialize retval to 0. Many of the system calls don't
	 * really return a value, just 0 for success and -1 on
	 * error. Since retval is the value returned on success,
	 * initialize it to 0 by default; thus it's not necessary to
	 * deal with it except for calls that return other values, 
	 * like write.
	 */

	retval = 0;

	switch (callno) {
	    kprintf("Handling syscall %d\n", callno);
	    case SYS_reboot:
		err = sys_reboot(tf->tf_a0);
		break;

	    /* Add stuff here */
	    case SYS_getpid:
		err = sys_getpid(&retval);
		break;
	    case SYS_fork:
		retval = sys_fork(tf);
		err = tf->tf_a3;
		break;
	    case SYS_read:
		retval = sys_read(tf->tf_a0, (char *)tf->tf_a1, tf->tf_a2);
		err = (retval < 0 ? retval : 0);
		break;
	   case SYS_write:
		retval = sys_write(tf->tf_a0, (char *)tf->tf_a1, tf->tf_a2);
		err = (retval < 0 ? retval : 0);
		break; 
	   case SYS_waitpid:
		err = sys_waitpid((pid_t)tf->tf_a0, (u_int32_t *)tf->tf_a1, tf->tf_a2, &retval);
		break;
	   case SYS__exit:
		err = sys__exit(tf->tf_a0);
		break;
	   case SYS_execv:
		err = sys_execv((char *)tf->tf_a0, (char **)tf->tf_a1);
		break;
	   case SYS_chdir:
		err = sys_chdir((char *)tf->tf_a0, &retval);
		break;
 
	    default:
		kprintf("Unknown syscall %d\n", callno);
		err = ENOSYS;
		break;
	}


	if (err) {
		/*
		 * Return the error code. This gets converted at
		 * userlevel to a return value of -1 and the error
		 * code in errno.
		 */
		tf->tf_v0 = err;
		tf->tf_a3 = 1;      /* signal an error */
	}
	else {
		/* Success. */
		tf->tf_v0 = retval;
		tf->tf_a3 = 0;      /* signal no error */
	}
	
	/*
	 * Now, advance the program counter, to avoid restarting
	 * the syscall over and over again.
	 */
	
	tf->tf_epc += 4;

	/* Make sure the syscall code didn't forget to lower spl */
	assert(curspl==0);
}

void
md_forkentry(struct trapframe *data1, unsigned long data2)
{
	/*
	 * This function is provided as a reminder. You need to write
	 * both it and the code that calls it.
	 *
	 * Thus, you can trash it and do things another way if you prefer.
	 */
	// new fork entry function
	// cast fork_info input back to pointer to fork_info struct
	struct fork_struct * fork_info = (struct fork_struct *) data2;
	data1 -> tf_v0 = 0;
	data1 -> tf_a3 = 0;
	data1 -> tf_epc += 4;

	as_copy(fork_info -> addrspace, &(curthread -> t_vmspace));
	as_activate(curthread -> t_vmspace);// activate the address space in the current thread

	struct trapframe tf = *data1;
	//finished with all the important stuff
	curthread -> p_pid = fork_info -> parent_thread -> t_pid; //sets the childs parent pid field
	fork_info -> child_pid = curthread -> t_pid;

	V(fork_info -> sem);// allow parent to start running again
	mips_usermode(&tf);
	kprintf("leaving md_forkentry");
}

int
sys_getpid(pid_t * retval) {
	*retval = curthread->t_pid;	
	return 0;
}

/*
 * Simple workaround for system call read(int, char *, int).
 * This is not how a system call should be implemented. 
 * But, for now, it works.
 */
int
sys_read(int filehandle, char *buf, size_t size)
{
	(void) filehandle;
	(void) size;
	
	char ch = getch();	
	if(ch == '\r') {
		ch = '\n';
	}
	buf[0] = ch;
	return 1;
}

/*
 * Implementation of system call write(int, char *, int).
 * This is not how a system call should be implemented. 
 * But, for now, it works.
 */
 
int
sys_write(int filehandle, char *buf, size_t size)
{
	(void) filehandle;
	
	size_t i;	
	for (i = 0; i < size; ++i) {
		putch((int)buf[i]);
	}	
	return i;
}

int sys_fork(struct trapframe * tf) {
	struct fork_struct* fork_info = (struct fork_struct*)kmalloc(sizeof(struct fork_struct));
	struct thread * child;
	fork_info -> tf = (struct trapframe*)kmalloc(sizeof(struct trapframe));
	fork_info -> parent_thread = curthread;
	fork_info -> sem = sem_create("semaphore", 0);
	if(fork_info -> sem == NULL){ //check to see if there is enough memory to create a semaphore
		return ENOMEM;
	}
	if (tf == NULL) {// no need to free fork_info -> tf because there is not enough memory to free
		return ENOMEM;
	}

	assert(curthread->t_vmspace != NULL);
	as_copy(curthread->t_vmspace, &(fork_info -> addrspace));

	*(fork_info -> tf) = *tf; // copy parent trapframe to kernel heap

	int result;// will be 0 if thread_fork worked correctly, will be -1 or error otherwise
	result = thread_fork(curthread->t_name, fork_info -> tf, (unsigned long)fork_info,(void (*)(void *, unsigned long) )md_forkentry, &child);
	P(fork_info -> sem);// stop the parent from running until the child has everything in place

	if (result == 0) { //our fork was successful
		tf->tf_v0 = fork_info -> child_pid;
		tf -> tf_a3 = 0;
		kfree(fork_info -> tf);// frees the copy of the trapframe that we put on the kernel heap
		return tf->tf_v0;
	} else{ //our fork was not successful, return codes accordingly
		tf->tf_v0 = result;
		tf -> tf_a3 = 1;
		return tf->tf_v0;
	}

	return 0; //obligatory return statement
}

int sys_waitpid(pid_t pid, u_int32_t *status, int options, pid_t *retval) {
	if (status == NULL) {
		return EFAULT;
	}
	if (pid < 0 || pid > PID_MAX) {
		return EINVAL;
	}
	if (options != 0 || pid_list[pid] != 1 || thread_list[pid]->p_pid != curthread->t_pid) {
		return EAGAIN;
	}
	struct thread * proc = thread_list[(int) pid];
	
	if (proc->status != THREAD_EXITED) { // wait until sys__exit signals exit_sem
		P(proc->exit_sem);
	}
	*retval = pid;
	*status = proc->status;
	return 0;
}

int sys__exit(int status) {
	curthread->status = THREAD_EXITED;
	curthread->exit_code = status;
	V(curthread->exit_sem);
	thread_exit();
	return 0;
}

int sys_execv(char * program, char ** argv) {
	kprintf("Entered sys_execv\n");
	/* Pretty similar to runprogram */
	struct vnode * v;
	vaddr_t entrypoint, stackptr;
	int result, argc, i, len;
	char ** kargv = (char **)kmalloc(sizeof(char *));
	size_t got; // does not matter; copyoutstr wants it

	/* Iterate through argv until NULL to find argc. */
	for(argc=0; argv[argc] != NULL; argc++) {}

	/* Copy all args to kernel stack and NULL terminate it */
	for(i=0; i<argc; i++) {
		kargv[i] = kstrdup(argv[i]);	
	}
	kargv[argc] = NULL;

	/* Open the file. */
	result = vfs_open(program, O_RDONLY, &v);
	if (result) {
		return result;
	}

	/* We should be a new thread. */
	// This won't work unless we fork 
	//assert(curthread->t_vmspace == NULL);

	/* Create a new address space. */
	// TODO: probably need to free old address space or something...
	curthread->t_vmspace = as_create();
	if (curthread->t_vmspace==NULL) {
		vfs_close(v);
		return ENOMEM;
	}

	/* Activate it. */
	as_activate(curthread->t_vmspace);

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* thread_exit destroys curthread->t_vmspace */
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(curthread->t_vmspace, &stackptr);
	if (result) {
		/* thread_exit destroys curthread->t_vmspace */
		return result;
	}

	/* Copy to stack and NULL terminate for argtest */
	u_int32_t ustack[argc+1];
	for(i=argc-1; i>-1; i--){
		len = strlen(kargv[i]);
		len += (len%4) == 0 ? 4: len%4;
		stackptr -= len;
		copyoutstr(kargv[i], (userptr_t)stackptr, len, &got);
		ustack[i] = stackptr;
	}
	ustack[argc] = (int)NULL;

	for(i=argc-1; i>-1; i--){
		stackptr -= 4;
		copyout(&ustack[i], (userptr_t)stackptr, sizeof(ustack[i]));
	}

	/* Warp to user mode. */
	md_usermode(argc, (userptr_t)stackptr, stackptr, entrypoint);
	
	/* md_usermode does not return */
	panic("md_usermode returned\n");
	return EINVAL;
}

int sys_chdir(char * filename, int * retval) {
	if (filename == NULL) {
		*retval = EFAULT;
		return EFAULT;
	}
	size_t got;
	char * safename = (char *)kmalloc(sizeof(char) * PATH_MAX);
	copyinstr(safename, (userptr_t)filename, PATH_MAX, &got);
	//char * safename = kstrdup(filename);

	/* Disable interrupts */
	int spl = splhigh();
	int result = vfs_chdir(safename); // this makes this somewhat trivial...
	*retval = result;
	kfree(safename);
	splx(spl);
	return result;
}
