#include <pololu/orangutan.h>
#include <stdarg.h>
#include <stdio.h>
#include "kserial.h"

void emptySendBuffer()
{
	do
	{
		serial_check();
	} while (!serial_send_buffer_empty(USB_COMM));
}

int spr(const char* fmt, ...)
{
	int result = 0;
	char buf[256];

	va_list ap;
	va_start(ap, fmt);

	result = vsprintf(buf, fmt, ap);

	va_end(ap);

	emptySendBuffer();

	serial_send(USB_COMM, buf, result);

	emptySendBuffer(); // or else buf[] goes out of scope during sending

	return result;
}
