#ifndef OUT_H
#define OUT_H
typedef enum {
    COLOR_RED,
    COLOR_YELLOW,
    COLOR_DEFAULT
} TerminalColor;
void terminal_write(const char *str, int len, TerminalColor color);
int printf(const char* format, ...);
void log_d(const char* format, ...);
void log_e(const char* format, ...);

#endif