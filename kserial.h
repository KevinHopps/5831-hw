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

struct LineBuf_
{
	char m_buf[32];
	uint8_t m_len;
	uint8_t m_frozen;
};
typedef struct LineBuf_ LineBuf;

extern void LBReset(LineBuf* lbuf);
extern int LBLength(const LineBuf* lbuf);
extern int LBCharAt(const LineBuf* lbuf, int index);

// This will possibly read characters into lbuf->m_buf each
// time it is called. When it receives a '\r' it will trim
// it and return lbuf->m_buf. Otherwise it will return 0.
//
extern const char* LBGetLine(LineBuf* lbuf);

#endif // #ifndef _kserial_h_
