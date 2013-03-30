#include <pololu/orangutan.h>
#include "kserial.h"
#include "ktimers.h"
#include "kdebug.h"

typedef struct CallbackInfo_
{
	Callback m_func;
	void* m_arg;
} CallbackInfo;

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

struct TimerSetup_
{
	uint8_t m_cs;
	uint16_t m_top;
};
typedef struct TimerSetup_ TimerSetup;

static TimerSetup calculate_prescale_and_top(uint16_t frequency, uint16_t maxTop)
{
	TimerSetup result = { 0, 0 };

	uint32_t freq = kClockFrequency;
	uint32_t ltop = 0;
	result.m_cs = kMinPrescaleIndex;
	int done = 0;
	while (!done)
	{
		int prescale = kPrescaleFrequency[result.m_cs];
		freq = kClockFrequency / prescale;
		ltop = freq / frequency;
		if (result.m_cs >= kMaxPrescaleIndex || ltop <= maxTop)
			done = 1;
		else
			++result.m_cs;
	}

	result.m_top = (int)ltop;

	int derivedFrequency = kClockFrequency / (kPrescaleFrequency[result.m_cs] * result.m_top);
	s_println("calculate_prescale_and_top: frequency: %d, clock=%ld/(prescale[%d]=%d * top=%d): %d",
				frequency, kClockFrequency, result.m_cs, kPrescaleFrequency[result.m_cs], result.m_top, derivedFrequency);

	return result;
}

void setup_CTC_timer(int whichTimer, int frequency, Callback func, void* arg)
{
	KASSERT(whichTimer == 0);

	uint16_t maxTop = 0xff; // timer0 uses 8-bit registers
	TimerSetup ts = calculate_prescale_and_top(frequency, maxTop);

	s_println("setup_CTC_timer: cs=%d, top=%d", ts.m_cs, ts.m_top);

	int com0a = 0;
	int com0b = 0;
	int wgm = 2; // CTC mode
	int foc0a = 0;
	int foc0b = 0;
	set_TCCRn(whichTimer, com0a, com0b, wgm, foc0a, foc0b, ts.m_cs);

	set_OCRnA(whichTimer, ts.m_top);

	int ocieNa = 1;
	int ocieNb = 0;
	int toieN = 0;
	set_TIMSKn(whichTimer, ocieNa, ocieNb, toieN);

	CallbackInfo* info = &callbackInfo[whichTimer];

	info->m_func = func;
	info->m_arg = arg;
}

void setup_CTC_timer3(int frequency, Callback func, void* arg)
{
	int com0a = 0; // 0 for "normal" mode
	int com0b = 0; // 0 for "normal" mode
	int wgm = 4; // OCR3 for TOP
	int icnc = 0; // input capture noise canceller
	int ices = 0; // image capture edge select
	unsigned maxTop = 0xffff; // timer3 uses 16-bit registers

	TimerSetup ts = calculate_prescale_and_top(frequency, maxTop);

	s_println("setup_CTC_timer3 cs=%d, top=%d", ts.m_cs, ts.m_top);

	int a = ((com0a & 3) << 6) | ((com0b & 3) << 4) | (wgm & 3);
	int b = ((icnc & 1) << 7) | ((ices & 1) << 6) | (((wgm >> 2) & 3) << 3) | (ts.m_cs & 7);
	s_println("TCCR3A=0x%02x, TCCR3B=0x%02x", a, b);
	TCCR3A = a;
	TCCR3B = b;

	s_println("OCR3A=0x%02x", ts.m_top);
	OCR3A = ts.m_top;

	int icie3 = 0;
	int ocie3b = 0;
	int ocie3a = 1; // enable Timer/Counter3 Output Compare A Match interrupt
	int toie3 = 0;
	a = ((icie3 & 1) << 5) | ((ocie3b & 1) << 2) | ((ocie3a & 1) << 1) | (toie3 & 1);
	s_println("TIMSK3=0x%02x", a);
	TIMSK3 = a;

	CallbackInfo* info = &callbackInfo[3];
	info->m_func = func;
}

ISR(TIMER0_COMPA_vect)
{
	Callback func = callbackInfo[0].m_func;
	void* arg = callbackInfo[0].m_arg;
	func(arg);
}

ISR(TIMER3_COMPA_vect)
{
	Callback func = callbackInfo[3].m_func;
	void* arg = callbackInfo[3].m_arg;
	func(arg);
}
