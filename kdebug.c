#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "kdebug.h"
#include "kserial.h"
#include "kutils.h"

void beep()
{
	static const char beep_button_top[] PROGMEM = "!c32";
	play_from_program_space(beep_button_top);
}

void die(const char* msg)
{
	beep();
	set_motors(0, 0);
	s_println("** KASSERT failed: %s", msg);
	int beep_delay = 250;
	int beep_time = 1;
	int nbeeps = beep_time * 1000 / beep_delay;
	while (--nbeeps >= 0)
	{
		beep();
		delay_ms(beep_delay);
	}
	while (1)
		continue;
}

char dbg_buf[256];
char* dbg_bp = dbg_buf;

static int dbg_printPlus(const char* extra, const char* fmt, va_list ap)
{
	int result = 0;
	
	int minSpace = strlen(fmt) + 10;
	
	BEGIN_ATOMIC
		int space = dbg_buf + sizeof(dbg_buf) - dbg_bp;
		if (space < minSpace)
			sprintf(dbg_buf, "*OVR*");
		else
		{
			char* old_bp = dbg_bp;
			dbg_bp += vsprintf(dbg_bp, fmt, ap);
			while ((*dbg_bp = *extra++) != 0)
				++dbg_bp;
			result = dbg_bp - old_bp;
		}
	END_ATOMIC
	
	return result;
}

int dbg_printf(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	int result = dbg_printPlus("", fmt, ap);
	
	va_end(ap);
	
	return result;
}

int dbg_println(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	int result = dbg_printPlus(s_eol, fmt, ap);
	
	va_end(ap);
	
	return result;
}

void dbg_flush()
{
	BEGIN_ATOMIC
		if (dbg_bp > dbg_buf)
		{
			s_printf("%s", dbg_buf);
			dbg_bp = dbg_buf;
		}
	END_ATOMIC
}
