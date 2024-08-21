/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Kernel-Level Partial Implementation of the C Library
 */

#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <kernel/tty.h>

int putchar(int c) {
    ttyPutc(c);
    return c;
}

int print(const char *s) {
    ttyPuts(s);
    return strlen(s);
}

int puts(const char *s) {
    int len = print(s);
    print("\n");
    return len+1;
}

int printf(const char *f, ...) {
    va_list args;
    va_start(args, f);
    int len = vprintf(f, args);
    va_end(args);
    return len;
}

int vprintf(const char *f, va_list args) {
    int l = 0;
    bool formatter = false;
    char format[16];
    char buffer[40];
    uint64_t number;
    int formatIndex;
    int numberLength, paddingLength;
    char paddingCharacter;
    char *str;

    while(*f) {
        if(!formatter && *f == '%') {
            formatter = true;
            formatIndex = 0;
            memset(format, 0, 16);
            f++;
        }

        if(formatter) {
            if(*f == '%') {
                formatter = false;
                putchar('%');
            } else {
                format[formatIndex] = *f;
                formatIndex++;

                if(*f >= 'A' && *f <= 'z') {
                    formatter = false;

                    // parse the format
                    memset(buffer, 0, 40);

                    if(format[0] == '0') {
                        paddingCharacter = '0';
                    } else {
                        paddingCharacter = ' ';     // default behavior for some reason
                    }

                    paddingLength = atoi(format);

                    switch(format[formatIndex-1]) {
                    case 'c':
                        number = va_arg(args, int);
                        putchar(number);
                        break;
                    case 's':
                        str = va_arg(args, char *);
                        print(str);
                        l += strlen(str);
                        break;
                    case 'd':
                    case 'i':
                    case 'u':
                        number = va_arg(args, uint64_t);
                        ltoa(number, buffer, DECIMAL);

                        numberLength = strlen(buffer);
                        if(numberLength < paddingLength) {
                            for(int i = 0; i < (paddingLength-numberLength); i++) {
                                putchar(paddingCharacter);
                                l++;
                            }
                        }
                        
                        print(buffer);
                        l += numberLength;
                        break;
                    case 'x':
                        number = va_arg(args, uint64_t);
                        ltoa(number, buffer, HEX);

                        numberLength = strlen(buffer);
                        if(numberLength < paddingLength) {
                            for(int i = 0; i < (paddingLength-numberLength); i++) {
                                putchar(paddingCharacter);
                                l++;
                            }
                        }
                        
                        print(buffer);
                        l += numberLength;
                        break;
                    case 'X':
                        number = va_arg(args, uint64_t);
                        ltoa(number, buffer, HEX);

                        numberLength = strlen(buffer);
                        if(numberLength < paddingLength) {
                            for(int i = 0; i < (paddingLength-numberLength); i++) {
                                putchar(paddingCharacter);
                                l++;
                            }
                        }
                        
                        for(int i = 0; i < numberLength; i++) {
                            if(buffer[i] >= 'a' && buffer[i] <= 'f') {
                                putchar(buffer[i] - 0x20);
                            } else {
                                putchar(buffer[i]);
                            }
                        }
                        l += numberLength;
                        break;
                    default:
                        print(format);
                    }
                }
            }
        } else {
            putchar(*f);
            l++;
        }
        f++;
    }

    return l;
}