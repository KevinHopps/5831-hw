#ifndef _kserial_h_
#define _kserial_h_

#include <pololu/orangutan.h>

extern void empty_sendbuf();

extern void s_write(char* buf, int len);

extern int s_printf(const char* fmt, ...);

extern const char s_eol[];

extern int s_println(const char* fmt, ...); // s_printf plus s_eol

#endif // #ifndef _kserial_h_
