#ifndef _kserial_h_
#define _kserial_h_

#include <pololu/orangutan.h>

void empty_sendbuf();

void s_write(const char* buf, int len);

// This function will read up to want bytes of data
// into buf and return the actual count read. If data
// is available, or if msecTimeout==0, this function
// will return immediately. Otherwise, it will block
// until either msecTimeout milliseconds elapses or
// until data is available, whichever occurs first.
//
int s_read(char* buf, int want, int msecTimeout);

int s_printf(const char* fmt, ...);

extern const char s_eol[];

int s_println(const char* fmt, ...); // s_printf plus s_eol

// Because %f does not seem to work, here are alternatives for printing floats.
// The will print as though you had specified %.*f, places.
//
char* s_ftosbp(char** buf, double f, int places); // e.g. char buf[16]; char* bp = buf; s_println("x=%s", s_ftosbp(&bp, x));
char* s_ftosb(char* buf, double f, int places); // e.g. char buf[16]; s_println("x=%s", s_ftosb(buf, x));
char* s_ftos(double f, int places); // a non-reentrant version that uses a static buffer
int s_printflt(double f, int places);

#endif // #ifndef _kserial_h_
