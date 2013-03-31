#include <pololu/orangutan.h>
#include <stdlib.h>
#include <string.h>
#include "kcmd.h"
#include "kio.h"
#include "klinebuf.h"
#include "kserial.h"
#include "ktimers.h"
#include "kutils.h"

#define kRED 0
#define kYELLOW 1
#define kGREEN 2
#define kALL 3
#define kNCOLORS 3

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

uint32_t counter[kNCOLORS];
int period[kNCOLORS];

void redTask()
{
	static KIORegs* iop = 0;
	if (iop == 0)
	{
		static KIORegs ios;
		ios = getIORegs(RED_PIN);
		iop = &ios;
		setDataDir(iop, OUTPUT);
		setIOValue(iop, 0);
	}

	int value = 0;

	if (period[kRED] > 0)
		value = (int)(++counter[kRED] & 1);

	setIOValue(iop, value);
}

volatile uint32_t redInterruptCount;
uint32_t redInterruptsPerRelease;
uint32_t redNextRelease;

// This function is called by the timer interrupt routine
// that is setup by setup_CTC_timer, called by main().
//
static void redCallback(void* arg)
{
	++redInterruptCount;
}

static void yellowCallback(void* arg)
{
	static KIORegs* iop = 0;
	if (iop == 0)
	{
		static KIORegs ios;
		ios = getIORegs(YELLOW_PIN);
		iop = &ios;
		setDataDir(iop, OUTPUT);
		setIOValue(iop, 0);
	}

	int value = 0;

	if (period[kYELLOW] > 0)
		value = (int)(++counter[kYELLOW] & 1);

	setIOValue(iop, value);
}

static void greenCallback(void* arg)
{
	static KIORegs* iop = 0;
	if (iop == 0)
	{
		static KIORegs ios;
		ios = getIORegs(GREEN_PIN);
		iop = &ios;
		setDataDir(iop, OUTPUT);
		setIOValue(iop, 0);
	}

	int value = 0;

	if (period[kGREEN] > 0)
		value = (int)(++counter[kGREEN] & 1);

	setIOValue(iop, value);
}

extern int toggle_cmd(int argc, char** argv);
extern int zero_cmd(int argc, char** argv);
extern int print_cmd(int argc, char** argv);

const char* const colorNames[] = { "red", "yellow", "green", "all" };
int colorNameCount = sizeof(colorNames) / sizeof(*colorNames);

#define kTimer0Frequency 1000

void setRedPeriod(int msecPerToggle)
{
	redInterruptsPerRelease = (kTimer0Frequency * msecPerToggle) / 1000;
	redNextRelease = 0;
	redInterruptCount = 0;
}

void setPeriod(int color, int msecPeriod);

int main()
{
	//int blinkHz = 1;
	//int nsec = 5;
	//s_println("Busy blink yellow LED at %d Hz for %d seconds", blinkHz, nsec);
	//busy_blink(5, 1, GREEN_PIN);

	setup_CTC_timer0(1, redCallback, 0);

	setPeriod(kRED, 1000);
	setPeriod(kYELLOW, 500);
	setPeriod(kGREEN, 250);

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

		if (redInterruptCount >= redNextRelease)
		{
			redTask();
			redNextRelease += redInterruptsPerRelease;
		}
	}
}

int getColorIndex(const char* str)
{
	int result = -1;

	int i;
	for (i = 0; result < 0 && i < colorNameCount; ++i)
	{
		const char* name = colorNames[i];
		if (matchIgnoreCase(str, name, strlen(str)))
			result = i;
	}

	return result;
}

const char* getColorName(const char* str)
{
	const char* result = 0;

	int i = getColorIndex(str);
	if (i >= 0)
		result = colorNames[i];

	return result;
}

typedef void (*ColorFunc)(int color, void* arg);

void do_for_color(const char* color, ColorFunc func, void* arg)
{
	int iBegin = -1;
	int iEnd = -1;
	int i = getColorIndex(color);
	if (0 <= i && i < kNCOLORS)
	{
		iBegin = i;
		iEnd = iBegin + 1;
	}
	else if (i == kNCOLORS)
	{
		iBegin = 0;
		iEnd = kNCOLORS;
	}

	if (iBegin >= iEnd)
		s_println("%s: unknown color", color);
	else
	{
		for (i = iBegin; i < iEnd; ++i)
			func(i, arg);
	}
}

void toggleFunc(int color, void* arg);
void zeroFunc(int color, void* arg);
void printFunc(int color, void* arg);

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
		int time = atoi(time_str);

		if (time < 0 || time > 1000)
			s_println("time must be in (0,1000]");
		else
		{
			do_for_color(color_str, toggleFunc, &time);
			result = 0;
		}
	}

	return result;
}

void setPeriod(int color, int msecPerToggle)
{
	int freq;

	s_println("setPeriod(%d, %d)", color, msecPerToggle);

	period[color] = msecPerToggle;

	switch (color)
	{
		case kRED:
			redInterruptsPerRelease = (kTimer0Frequency * msecPerToggle) / 1000;
			redInterruptCount = 0;
			redNextRelease = 0;
			break;

		case kYELLOW:
			setup_CTC_timer3(msecPerToggle, yellowCallback, 0);
			break;

		case kGREEN:
			setup_PWM_timer1(msecPerToggle, greenCallback, 0);
			break;
	}
}

void toggleFunc(int color, void* arg)
{
	int msecPerToggle = *(int*)arg;
	setPeriod(color, msecPerToggle);
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
		do_for_color(color_str, zeroFunc, 0);
		result = 0;
	}

	return result;
}

void zeroFunc(int color, void* arg)
{
	counter[color] = 0;
	printFunc(color, arg);
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
		do_for_color(color_str, printFunc, 0);
		result = 0;
	}

	return result;
}

void printFunc(int color, void* arg)
{
	s_println("%s count=%ld", colorNames[color], (long)counter[color]);
}
