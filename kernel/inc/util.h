#pragma once

#if SHRK_DEBUG == 1
#define debg(msg) print_debug(__func__, msg)
#define debgf(msg, ...) print_debug(__func__, msg, __VA_ARGS__)
#else
#define debg(msg) asm("nop")
#define debgf(msg, ...) asm("nop")
#endif

void print_debug(const char *caller, char *msg, ...);