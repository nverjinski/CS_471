/*
 * simpleforktest - test fork().
 *
 * This should work correctly when fork is implemented.
 *
 * It should also continue to work after subsequent assignments, most
 * notably after implementing the virtual memory system.
 * 
 * This test was created by Kunal Sarkhel and used to prove that our sys_fork syscall works as intended
 */

#include <stdio.h>
#include <unistd.h>

int main(){
	pid_t mypid = fork();
	if (mypid == 0) {
		printf("\nchild process (pid 0)\n");
	} else {
		printf("\nWaiting for the child %d to finish\n", mypid);
		int status;
		waitpid(mypid, &status, 0);
		printf("\nparent process (child process pid is %d)\n", mypid);
	}
	return 0;
}
