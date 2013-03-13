#ifndef _kdebug_h_
#define _kdebug_h_

#include <pololu/orangutan.h>

extern void die(const char* msg);
extern void beep();

#define KASSERT(X) do { if (!(X)) die(#X); } while(0)

#endif // #ifndef _kdebug_h_
