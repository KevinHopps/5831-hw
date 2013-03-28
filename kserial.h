#ifndef _kserial_h_
#define _kserial_h_

#include <pololu/orangutan.h>

extern void empty_sendbuf();

extern void s_write(const char* buf, int len);

// This function will read up to want bytes of data
// into buf and return the actual count read. If data
// is available, or if msecTimeout==0, this function
// will return immediately. Otherwise, it will block
// until either msecTimeout milliseconds elapses or
// until data is available, whichever occurs first.
//
extern int s_read(char* buf, int want, int msecTimeout);

extern int s_printf(const char* fmt, ...);

extern const char s_eol[];

extern int s_println(const char* fmt, ...); // s_printf plus s_eol

#endif // #ifndef _kserial_h_
