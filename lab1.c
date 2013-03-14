#include <pololu/orangutan.h>
#include "kserial.h"
#include "ktimers.h"

void spin(volatile uint32_t n)
{
	while (--n >= 0)
		continue;
}

static const uint32_t kLoopsPerSec = 2L * 424852L;

void kdelay_ms(int ms)
{
	spin((kLoopsPerSec * ms) / 1000);
}

void calcSpinCount()
{
	int state = 0;
	uint32_t loopCount = 423828;
	uint32_t incr = 2048;
	while (1)
	{
		red_led(++state & 1);
		spin(loopCount);

		int buttons = get_single_debounced_button_press(ANY_BUTTON);
		if (buttons & TOP_BUTTON)
		{
			loopCount -= incr;
			incr /= 2;
			s_println("faster: loopCount=%ld, incr=%ld", loopCount, incr);
		}
		else if (buttons & BOTTOM_BUTTON)
		{
			loopCount += incr;
			incr /= 2;
			s_println("slower: loopCount=%ld, incr=%ld", loopCount, incr);
		}
		else if (buttons & MIDDLE_BUTTON)
		{
			s_println("loopCount=%ld, incr=%ld", loopCount, incr);
		}
		get_single_debounced_button_release(ANY_BUTTON);
	}
}

void methodA(int nsec)
{
	while (--nsec >= 0)
	{
		red_led(1);
		kdelay_ms(500);
		red_led(0);
		kdelay_ms(500);
	}
}

// This function is called by the timer interrupt routine
// that is setup by setup_CTC_timer, called by main().
//
static void callback_function(void* arg)
{
	uint32_t* n = (uint32_t*)arg;
	++(*n);
}

void methodB(int nsec)
{
	volatile uint32_t callback_count = 0;

	int whichTimer = 0;
	int frequency = 1000;
	Callback func = callback_function;
	void* arg = &callback_count;

	s_println("calling setup_CTC_timer(%d, %d, 0x%x, 0x%x)",
		whichTimer, frequency, func, arg);

	setup_CTC_timer(whichTimer, frequency, func, arg);

	s_println("sei()");
	sei();	// enable interrupts

	uint32_t nextBreak = 500;
	int state = 0;
	nsec *= 2;
	while (--nsec >= 0)
	{
		green_led(++state & 1);
		while (callback_count < nextBreak)
			continue;
		nextBreak += 500;
	}

	cli(); // disable interrupts
}

int main()
{
	int nsec = 10;
	while (1)
	{
		methodB(nsec);
		methodA(nsec);
	}
}
