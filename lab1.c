#include <pololu/orangutan.h>
#include <string.h>
#include "kcmd.h"
#include "kio.h"
#include "klinebuf.h"
#include "kserial.h"
#include "ktimers.h"
#include "kutils.h"

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

extern int toggle_cmd(int argc, char** argv);
extern int zero_cmd(int argc, char** argv);
extern int print_cmd(int argc, char** argv);

const char kRED[] = "red";
const char kGREEN[] = "green";
const char kYELLOW[] = "yellow";
const char kALL[] = "all";
const char* const colorNames[] = { kRED, kGREEN, kYELLOW, kALL };
int colorNameCount = sizeof(colorNames) / sizeof(*colorNames);

int main()
{
	int blinkHz = 1;

	int nsec = 5;
	s_println("Busy blink yellow LED at %d Hz for %d seconds", blinkHz, nsec);
	busy_blink(nsec, blinkHz, YELLOW_PIN);

	s_println("Setup CTC timer");

	int whichTimer = 0;
	int timerHz = 1000;
	volatile uint32_t timerCount = 0;
	setup_CTC_timer(whichTimer, timerHz, timerCallback, (void*)&timerCount);

	int redReleaseInterval = timerHz / (2 * blinkHz);
	uint32_t redNextRelease = 0;

	sei(); // enable interrupts

	CommandIO cio;
	CIOReset(&cio);

	CIORegisterCommand(&cio, "zero", zero_cmd);
	CIORegisterCommand(&cio, "toggle", toggle_cmd);
	CIORegisterCommand(&cio, "print", print_cmd);

	s_println("Entring while(1)");
	while (1)
	{
		if (CIOCheckForCommand(&cio))
			CIORunCommand(&cio);

		if (timerCount >= redNextRelease)
		{
			redTask();
			redNextRelease += redReleaseInterval;
		}
	}
}

const char* getColorName(const char* str)
{
	const char* result = 0;
	int i;
	for (i = 0; result == 0 && i < colorNameCount; ++i)
	{
		const char* name = colorNames[i];
		if (matchIgnoreCase(str, name, strlen(str)))
			result = name;
	}
	return result;
}

int toggle_cmd(int argc, char** argv)
{
	const char* usage = "usage: %s color msec";
	const char* cmd = "t";
	int result = -1;

	if (--argc >= 0)
		cmd = *argv++;

	if (argc != 2)
		s_println(usage, cmd);
	else
	{
		const char* color_str = *argv++;
		const char* time_str = *argv++;

		const char* colorName = getColorName(color_str);;
		int time = atoi(time_str);

		if (colorName == 0)
			s_println("%s: unknown color", color_str);
		else
		{
			s_println("toggle %s every %d ms", colorName, time);
			result = 0;
		}
	}

	return result;
}

int zero_cmd(int argc, char** argv)
{
	const char* usage = "usage: %s color";
	const char* cmd = "z";
	int result = -1;

	if (--argc >= 0)
		cmd = *argv++;

	if (argc != 1)
		s_println(usage, cmd);
	else
	{
		const char* color_str = *argv++;
		const char* colorName = getColorName(color_str);
		if (colorName == 0)
			s_println("%s: unknown color", color_str);
		else
		{
			s_println("zero counter for %s", colorName);
			result = 0;
		}
	}

	return result;
}

int print_cmd(int argc, char** argv)
{
	const char* usage = "usage: %s color";
	const char* cmd = "p";
	int result = -1;

	if (--argc >= 0)
		cmd = *argv++;

	if (argc != 1)
		s_println(usage, cmd);
	else
	{
		const char* color_str = *argv++;
		const char* colorName = getColorName(color_str);
		if (colorName == 0)
			s_println("%s: unknown color", color_str);
		else
		{
			s_println("print counter for %s", colorName);
			result = 0;
		}
	}

	return result;
}
