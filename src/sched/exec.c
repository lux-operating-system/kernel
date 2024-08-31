/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <kernel/sched.h>
#include <kernel/logger.h>

/* execveMemory(): executes a program from memory
 * params: ptr - pointer to the program in memory
 * params: argv - arguments to be passed to the program
 * params: envp - environmental variables to be passed
 * returns: should not return on success
 */

int execveMemory(const void *ptr, const char **argv, const char **envp) {
    return 0;    /* todo */
}

/* execve(): executes a program from a file
 * params: name - file name of the program
 * params: argv - arguments to be passed to the program
 * params: envp - environmental variables to be passed
 * returns: should not return on success
 */

int execve(const char *name, const char **argv, const char **envp) {
    return 0;    /* todo */
}
