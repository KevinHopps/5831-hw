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

void s_write(char* buf, int len)
{
	empty_sendbuf();
	serial_send(USB_COMM, buf, len);
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

