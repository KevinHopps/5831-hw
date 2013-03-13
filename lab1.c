#include <pololu/orangutan.h>
#include "kserial.h"
#include "ktimers.h"

// This function is called by the timer interrupt routine
// that is setup by setup_CTC_timer, called by main().
//
void callback_function(void* arg)
{
	long* n = (long*)arg;
	++(*n);
}

int main()
{
	long nextBreak = 1000;
	volatile long callback_count = 0;

	int whichTimer = 0;
	int frequency = 1000;
	Callback func = callback_function;
	void* arg = &callback_count;

	s_println("calling setup_CTC_timer(%d, %d, 0x%x, 0x%x)",
		whichTimer, frequency, func, arg);

	setup_CTC_timer(whichTimer, frequency, func, arg);

	s_println("sei()");

	sei();	// enable interrupts

	int counter = 0;
	for (counter = 0; ; ++counter)
	{
		while (callback_count < nextBreak)
			continue;
		nextBreak += 1000;
		s_println("%d: callback_count %ld", counter, callback_count);
	}
}
