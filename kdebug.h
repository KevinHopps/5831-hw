#ifndef _kdebug_h_
#define _kdebug_h_

// This file contains some useful debugging utilities.
//

void die(const char* msg); // displays a message and stops
void beep(); // makes a short beep noise

// This tests the condition and, if false, displays a message and stops.
//
#define KASSERT(X) do { if (!(X)) die(#X); } while(0)

// These functions may be used during interrupt handlers to queue
// up messages to be printed outside the context of an interrupt
// handler. The dbg_print functions accumulate characters into a
// limited space buffer. The dbg_flush function prints the contents
// of the buffer, if any, and resets it.
//
#define DEBUG_BUFSIZE 256
int dbg_printf(const char* fmt, ...); // appends to a debug buffer
int dbg_println(const char* fmt, ...); // appends to a debug buffer
void dbg_flush(); // prints debug buffer

#endif // #ifndef _kdebug_h_
