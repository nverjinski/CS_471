CS_471
======

OS-161 System Call Implementations

These files are the the result of a joint effort by Nathan Verjinski and Kunal Sarkhel. These files originated from OS-161 and have been edited by the afore mentioned to include implementations of basic system calls for the operating system OS-161. 

These files are not intended to be portrayed as a complete solution for the implementations of system calls in OS-161. These files are here as an example of our current abilities and knowledge of relatively low level programming.

Known Problems: 
- Some system calls fail when run from the shell but not when called directly from the OS. 
- When the system calls are tested with the forktest program, the the test fails, however, when fork is run from     simpleforktest, provided in the repository and created by Kunal Sarkhel, the sys_fork() passes. 
