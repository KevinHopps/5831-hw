#include <pololu/orangutan.h>
#include "kio.h"
#include "kserial.h"
#include "ktimers.h"

#define RED_PIN IO_C1
#define YELLOW_PIN IO_A0
#define GREEN_PIN IO_D5

// PD5 is OC1A? Need to set DDRD pin 5 output1
// CTC = clear timer on compare match
// PWM = Pulse Width Modulation
// Setup a 16-bit timer counter with PWM to toggle an LED without using software.
// See table page 132-134.
//
void spin(volatile int32_t n)
{
	while (--n >= 0)
		continue;
}

static const uint32_t kLoopsPerSec = 2L * 424852L; // by experiment -- see spinCount.c

void kdelay_ms(int ms)
{
	spin((kLoopsPerSec * ms) / 1000);
}

void busy_blink(int nsec, int blinkHz, int pin)
{
	KIORegs io = getIORegs(pin);
	setDataDir(&io, OUTPUT);
	int value = 1;
	nsec *= 2;
	int msec = 500 * blinkHz;
	while (--nsec >= 0)
	{
		setIOValue(&io, value);
		value ^= 1;
		kdelay_ms(msec);
	}
	set_digital_output_value(&io, 0);
}

void redTask()
{
	static KIORegs* iop = 0;
	static int count = 0;
	if (iop == 0)
	{
		static KIORegs ios;
		s_println("getIORegs(%d)", RED_PIN);
		ios = getIORegs(RED_PIN);
		iop = &ios;
		setDataDir(iop, OUTPUT);
		setIOValue(iop, 0);
	}

	setIOValue(iop, ++count & 1);
}

// This function is called by the timer interrupt routine
// that is setup by setup_CTC_timer, called by main().
//
static void timerCallback(void* arg)
{
	uint32_t* n = (uint32_t*)arg;
	++(*n);
}

int main()
{
	char cmdbuf[32];
	char* cmd_next = cmdbuf;
	char* cmd_eob = cmdbuf + sizeof(cmdbuf);
	int blinkHz = 1;

	int nsec = 5;
	s_println("Busy blink yellow LED at %d Hz for %d seconds", blinkHz, nsec);
	busy_blink(nsec, blinkHz, YELLOW_PIN);

	s_println("Setup CTC timer");

	int whichTimer = 0;
	int timerHz = 1000;
	volatile uint32_t timerCount = 0;
	setup_CTC_timer(whichTimer, timerHz, timerCallback, &timerCount);

	int redReleaseInterval = timerHz / (2 * blinkHz);
	uint32_t redNextRelease = 0;

	sei(); // enable interrupts

	while (1)
	{
		if (timerCount >= redNextRelease)
		{
			redTask();
			redNextRelease += redReleaseInterval;
		}
	}
}
