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

int s_printf(const char* fmt, ...)
{
	char buf[256];

	va_list ap;
	va_start(ap, fmt);

	int result = vsprintf(buf, fmt, ap);
	KASSERT(result < sizeof(buf));

	va_end(ap);

	s_write(buf, result);

	return result;
}

const char s_eol[] = "\r\n";

int s_println(const char* fmt, ...)
{
	char buf[256];

	va_list ap;
	va_start(ap, fmt);

	char* bp = buf;
	bp += vsprintf(buf, fmt, ap);

	KASSERT(bp-buf+2 < sizeof(buf));

	va_end(ap);

	const char* cp = s_eol;
	while ((*bp++ = *cp++) != 0)
		continue;

	int result = bp - buf;

	s_write(buf, result);

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
