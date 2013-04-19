#ifndef _kdebug_h_
#define _kdebug_h_

#include <pololu/orangutan.h>

extern void die(const char* msg);
extern void beep();

#define KASSERT(X) do { if (!(X)) die(#X); } while(0)

int dbg_printf(const char* fmt, ...); // appends to a debug buffer
int dbg_println(const char* fmt, ...); // appends to a debug buffer
void dbg_flush(); // prints debug buffer

#endif // #ifndef _kdebug_h_
