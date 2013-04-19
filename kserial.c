#include <pololu/orangutan.h>
#include <stdarg.h>
#include <stdio.h>
#include "kserial.h"
#include "kdebug.h"

void empty_sendbuf()
{
	do
	{
		serial_check();
	} while (!serial_send_buffer_empty(USB_COMM));
}

void s_write(const char* buf, int len)
{
	empty_sendbuf();
	serial_send(USB_COMM, (char*)buf, len);
	empty_sendbuf();
}

static int s_printPlus(const char* extra, const char* fmt, va_list ap)
{
	char buf[256];
	char* bp = buf;
	
	bp += vsprintf(bp, fmt, ap);
	KASSERT(bp < buf + sizeof(buf));
	
	char c;
	while ((c = *extra++) != 0)
		*bp++ = c;
		
	int result = bp - buf;
	s_write(buf, result);
	
	return result;
}

int s_printf(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	int result = s_printPlus("", fmt, ap);

	va_end(ap);

	return result;
}

const char s_eol[] = "\r\n";

int s_println(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	int result = s_printPlus(s_eol, fmt, ap);

	va_end(ap);

	return result;
}

#define kBufSize 128 // must be a power of 2
#define kSizeMask (kBufSize-1)

int s_read(char* buf, int want, int msecTimeout)
{
	static int ringNext = -1; // -1 means uninitialized
	static char ringBuffer[kBufSize]; // size must fit in unsigned char

	if (want <= 0 || buf == 0)
		return 0;

	int maxRead = kBufSize - 1;
	if (want > maxRead)
		want = maxRead;

	if (ringNext < 0)
	{
		serial_receive_ring(USB_COMM, ringBuffer, kBufSize);
		ringNext = 0;
	}

	int avail = 0;
	int done = 0;
	while (!done)
	{
		serial_check();
		int nbytes = serial_get_received_bytes(USB_COMM);
		avail = (nbytes - ringNext) & kSizeMask;
		if (avail > 0 || msecTimeout <= 0)
			done = 1;
		else
		{
			delay_ms(1);
			--msecTimeout;
		}
	}

	if (want > avail)
		want = avail;

	int got = 0;
	while (got < want)
	{
		buf[got++] = ringBuffer[ringNext++];
		ringNext &= kSizeMask;
	}

	return got;
}

#if USE_FLOATS

char* s_ftosbp(char** bpp, double f, int places)
{
	char* bp = *bpp;

	if (f < 0.0)
	{
		*bp++ = '-';
		f = -f;
	}
	
	long n = (long)f;
	bp += sprintf(bp, "%ld", n);

	f -= n;
	
	char fmt[8];
	sprintf(fmt, ".%c0%dld", '%', places);
	while (--places >= 0)
		f *= 10.0;
	n = (long)(f + 0.5);
	bp += sprintf(bp, fmt, n);

	char* result = *bpp;
	*bpp = bp;
	
	return result;
}

char* s_ftosb(char* buf, double f, int places)
{
	char* result = buf;
	s_ftosbp(&buf, f, places);
	return result;
}

char* s_ftos(double f, int places)
{
	static char buf[32];
	return s_ftosb(buf, f, places);
}

int s_printflt(double f, int places)
{
	char buf[32];
	return s_printf("%s", s_ftosb(buf, f, places));
}

#endif // #if USE_FLOATS
