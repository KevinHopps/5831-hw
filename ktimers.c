#include <pololu/orangutan.h>
#include "kserial.h"
#include "ktimers.h"
#include "kdebug.h"

static const int kMinPrescaleIndex = 1;
static const int kPrescaleFrequency[] = { 0, 1, 8, 64, 256, 1024 };
static const int kMaxPrescaleIndex = sizeof(kPrescaleFrequency) / sizeof(*kPrescaleFrequency) - 1;
static const uint32_t kClockFrequency = 20L * 1000L * 1000L;

#define kNumTimers 4
static CallbackInfo callbackInfo[kNumTimers];

static int TCCRnA(int com0a, int com0b, int wgm)
{
	return ((com0a & 3) << 6) | ((com0b & 3) << 4) | (wgm & 3);
}

static int TCCRnB(int foc0a, int foc0b, int wgm, int cs)
{
	return ((foc0a & 1) << 7) | ((foc0b & 1) << 6) | ((wgm & 4) << 1) | (cs & 7);
}

void set_TCCRn(int n, int com0a, int com0b, int wgm, int foc0a, int foc0b, int cs)
{
	int a = TCCRnA(com0a, com0b, wgm);
	int b = TCCRnB(foc0a, foc0b, wgm, cs);

	switch (n)
	{
		case 0:
			TCCR0A = a;
			TCCR0B = b;
			break;

		case 1:
			TCCR1A = a;
			TCCR1B = b;
			break;

		case 2:
			TCCR2A = a;
			TCCR2B = b;
			break;

		case 3:
			TCCR3A = a;
			TCCR3B = b;
			break;

		default:
			KASSERT(0 <= n && n < kNumTimers);
			break;
	}
}

void set_OCRnA(int n, int val)
{
	switch (n)
	{
		case 0:
			OCR0A = val;
			break;

		case 1:
			OCR1A = val;
			break;

		case 2:
			OCR2A = val;
			break;

		case 3:
			OCR3A = val;
			break;

		default:
			KASSERT(0 <= n && n < kNumTimers);
			break;
	}
}

void set_TIMSKn(int n, int ocieNa, int ocieNb, int toieN)
{
	int val = 0;
	
	switch (n)
	{
		case 0:
			if (ocieNa)
				val |= 1 << OCIE0A;
			if (ocieNb)
				val |= 1 << OCIE0B;
			if (toieN)
				val |= 1 << TOIE0;
			TIMSK0 = val;
			break;

		case 1:
			if (ocieNa)
				val |= 1 << OCIE1A;
			if (ocieNb)
				val |= 1 << OCIE1B;
			if (toieN)
				val |= 1 << TOIE1;
			TIMSK1 = val;
			break;

		case 2:
			if (ocieNa)
				val |= 1 << OCIE2A;
			if (ocieNb)
				val |= 1 << OCIE2B;
			if (toieN)
				val |= 1 << TOIE2;
			TIMSK2 = val;
			break;

		case 3:
			if (ocieNa)
				val |= 1 << OCIE3A;
			if (ocieNb)
				val |= 1 << OCIE3B;
			if (toieN)
				val |= 1 << TOIE3;
			TIMSK3 = val;
			break;

		default:
			KASSERT(0 <= n && n < kNumTimers);
			break;
	}
}

CallbackInfo setup_CTC_timer(int whichTimer, int frequency, Callback func, void* arg)
{
	KASSERT(whichTimer == 0);

	uint32_t freq = kClockFrequency;
	uint32_t ltop = 0;
	int prescaleIndex = kMinPrescaleIndex;
	int done = 0;
	while (!done)
	{
		int prescale = kPrescaleFrequency[prescaleIndex];
		freq = kClockFrequency / prescale;
		ltop = freq / frequency;
		if (prescaleIndex >= kMaxPrescaleIndex || ltop < 256)
			done = 1;
		else
			++prescaleIndex;
	}

	int top = (int)ltop;

	int com0a = 0;
	int com0b = 0;
	int wgm = 2; // CTC mode
	int foc0a = 0;
	int foc0b = 0;
	int cs = prescaleIndex;
	set_TCCRn(whichTimer, com0a, com0b, wgm, foc0a, foc0b, cs);

	set_OCRnA(whichTimer, top);

	int ocieNa = 1;
	int ocieNb = 0;
	int toieN = 0;
	set_TIMSKn(whichTimer, ocieNa, ocieNb, toieN);

	CallbackInfo* info = &callbackInfo[whichTimer];

	CallbackInfo result = *info;

	info->m_func = func;
	info->m_arg = arg;

	return result;
}

void initTimers()
{
	// Put timer 0 in CTC mode
	//
	int wgm = 2; // CTC mode
	int mask = (1 << WGM01) | (1 << WGM00);
	TCCR0A = (TCCR0A & ~mask) | (wgm & mask);
	mask = (1 << WGM02);
	TCCR0B = (TCCR0B & ~mask) | (wgm & mask);

	// To get a frequency of 1000 Hz (1 ms), we take
	// the frequency of the 20MHz, divided by the
	// prescaler, divided by the TOP of OCRA.
	// Dividing the 20MHz by the prescaler, which
	// has choices of none, 1, 8, 64, 256, 1024.
	// We set this to 256, which brings us to
	// 20*1000*1000 / 256 = 78125
	//
	int clockSelect = 4; // setting for 256
	mask = 7;
	TCCR0B = (TCCR0B & ~mask) | clockSelect;

	// To get down to 1000Hz, we take
	// 78125/1000 = 78
	// so we need to set OCRA to 82
	OCR0A = 78;

	// Now enable the compare interrupt
	//
	mask = 1 << OCIE0A;
	TIMSK0 = (TIMSK0 & ~mask) | mask;
}

ISR(TIMER0_COMPA_vect)
{
	Callback func = callbackInfo[0].m_func;
	void* arg = callbackInfo[0].m_arg;
	func(arg);
}
