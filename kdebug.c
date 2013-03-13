#include "kdebug.h"
#include "kserial.h"

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
