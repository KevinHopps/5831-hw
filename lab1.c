#include <pololu/orangutan.h>
#include "kserial.h"
#include "ktimers.h"

void callback_function(void* arg)
{
	long* n = (long*)arg;
	++(*n);
}

int main()
{
	s_println("main");
	int counter = 0;
	long nextBreak = 1000;
	volatile long callback_count = 0;

	int whichTimer = 0;
	int frequency = 1000;
	Callback func = callback_function;
	void* arg = &callback_count;

	s_println("calling setup_CTC_timer(%d, %d, 0x%x, 0x%x)",
		whichTimer, frequency, func, arg);

	setup_CTC_timer(whichTimer, frequency, func, arg);

	delay_ms(5000);
	s_println("sei()");

	sei();	// enable interrupts

	while (1)
	{
		while (callback_count < nextBreak)
			continue;
		nextBreak += 1000;
		s_println("callback_count %ld", callback_count);
	}
}
