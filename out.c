#include "out.h"
#include <string.h>  // for strlen() and strcat()
#include <stdlib.h>  // for itoa()
#include <stdarg.h>  // for va_start(), va_end() and va_arg()
#include <stdint.h>


void terminal_write(const char *str, int len, TerminalColor color) {
    const char *color_code;

    // Determine the color code based on the TerminalColor enum
    switch (color) {
        case COLOR_RED:
            color_code = "\033[31m";
            break;
        case COLOR_YELLOW:
            color_code = "\033[1;33m";
            break;
        default:
            color_code = "\033[0m"; // Default to reset if color is unknown
            break;
    }

    // Set the color
    for (int i = 0; i < strlen(color_code); i++) {
        *(char*)(0x10000000UL) = color_code[i];
    }

    // Write the actual string
    for (int i = 0; i < len; i++) {
        *(char*)(0x10000000UL) = str[i];
    }

    // Reset color back to default
    const char reset[] = "\033[0m";
    for (int i = 0; i < sizeof(reset) - 1; i++) {
        *(char*)(0x10000000UL) = reset[i];
    }
}




void format_to_str(char* out, const char* fmt, va_list args) {
    for(out[0] = 0; *fmt != '\0'; fmt++) {
        if (*fmt != '%') {
            strncat(out, fmt, 1);
        } else {
            fmt++;
            switch (*fmt)
            {
                case 's':
                    strcat(out, va_arg(args, char*));
                    break;
                case 'd':
                    itoa(va_arg(args, int), out + strlen(out), 10);
                    break;
                case 'c':
                    char to_append = (char)va_arg(args, int);  
                    strncat(out, &to_append, 1); 
                    break;
                case 'x':
                    itoa(va_arg(args, int), out + strlen(out), 16);
                    break;
                case 'u': {
                    unsigned int num = va_arg(args, unsigned int);
                    char buf[16];  // Enough for 32-bit unsigned (10 digits max)
                    char *p = buf + sizeof(buf) - 1;
                    *p = '\0';
                    do {
                        *--p = '0' + (num % 10);
                        num /= 10;
                    } while (num);
                    strcat(out, p);
                    break;

                }
                case 'p':
                    void* ptr = va_arg(args, void*);  
                    itoa((uintptr_t)ptr, out + strlen(out), 16);  
                    break;
                case 'l':
                    if (*(fmt+1) == 'u') {
                        unsigned long long num = va_arg(args, unsigned long long);
                        char buf[32]; 
                        char *p = buf + sizeof(buf) - 1;
                        *p = '\0';
                        do {
                            *--p = '0' + (num % 10);
                            num /= 10;
                        } while (num);
                        strcat(out, p);
                        fmt++;  // Skip the 'u'
                    }
                default:
                    break;
            }
        }
    }
}



int printf(const char* format, ...) {
    char buf[512] = "";
    va_list args;
    va_start(args, format);
    format_to_str(buf, format, args);
    va_end(args);
    terminal_write(buf, strlen(buf), COLOR_DEFAULT);
    return 0;

}



void log_d(const char* message, ...) {
    char buf[512] = "";
  
    // Format the actual message
    char msg_buf[256] = "";
    va_list args;
    va_start(args, message);
    format_to_str(msg_buf, message, args);
    va_end(args);
    
    strcat(buf, msg_buf);
    strcat(buf, "\n\r");
    
    terminal_write(buf, strlen(buf), COLOR_YELLOW);
}

void log_e(const char* message, ...) {
    char buf[512] = "[ERROR]"; 
    
    // Format the actual message
    char msg_buf[256] = "";
    va_list args;
    va_start(args, message);
    format_to_str(msg_buf, message, args);
    va_end(args);
    
    strcat(buf, msg_buf);
    strcat(buf, "\n\r");
    
    terminal_write(buf, strlen(buf), COLOR_RED);
}