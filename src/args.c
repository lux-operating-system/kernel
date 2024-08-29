/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <string.h>
#include <stdlib.h>
#include <kernel/boot.h>
#include <kernel/logger.h>

static char *argPosition(char *args, int index) {
    int count = 0;
    int i;
    for(i = 0; i < strlen(args); i++) {
        if(count == index) break;
        if(args[i] == ' ') count++;
    }

    if(count != index) return NULL;
    return args+i;
}

static size_t argLength(char *arg) {
    size_t len;
    for(len = 0; arg[len] && arg[len] != ' '; len++);

    return len;
}

static char *copyArg(char *dst, char *arg) {
    size_t len = argLength(arg);
    memcpy(dst, arg, len);
    dst[len] = 0;
    return dst;
}

/* parseBootArgs(): parses boot arguments into a series of C-like argmuents *
 * params: argv - pointer to store argv in
 * params: args - string containing arguments
 * returns: number of arguments "argc"
 */

int parseBootArgs(char ***argv, char *args) {
    // first count how many arguments
    char **v;
    int argc = 1;
    for(int i = 0; i < strlen(args); i++) {
        if(args[i] == ' ') argc++;
    }

    v = calloc(argc, sizeof(char *));
    if(!v) {
        KERROR("failed to allocate memory for kernel boot arguments\n");
        while(1);
    }

    // split the string by spaces
    for(int i = 0; i < argc; i++) {
        char *arg = argPosition(args, i);
        if(!arg) {
            KERROR("unable to retrieve boot argument %d\n", i);
            while(1);
        }

        v[i] = malloc(argLength(arg) + 1);
        if(!v[i]) {
            KERROR("failed to allocate memory for kernel boot arguments\n");
            while(1);
        }

        copyArg(v[i], arg);
    }

    argv[0] = v;
    return argc;
}
