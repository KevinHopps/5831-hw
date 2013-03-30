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

struct TimerSetup_
{
	uint8_t m_cs;
	uint16_t m_top;
};
typedef struct TimerSetup_ TimerSetup;

static TimerSetup calcTimerSetup(uint16_t frequency, uint16_t maxTop)
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

	int derivedFrequency = (kClockFrequency / (kPrescaleFrequency[result.m_cs]) / result.m_top);
	s_println("calcTimerSetup: frequency: %d, (clock=%ld/prescale[%d]=%d) / top=%d: %d",
				frequency, kClockFrequency, result.m_cs, kPrescaleFrequency[result.m_cs], result.m_top, derivedFrequency);

	return result;
}

void setup_CTC_timer0(int frequency, Callback func, void* arg)
{
	uint16_t maxTop = 0xff; // timer0 uses 8-bit registers
	TimerSetup ts = calcTimerSetup(frequency, maxTop);

	s_println("setup_CTC_timer0: cs=%d, top=%d", ts.m_cs, ts.m_top);

	int com0a = 0;
	int com0b = 0;
	int wgm = 2; // CTC mode
	int foc0a = 0;
	int foc0b = 0;

	uint8_t a = ((com0a & 3) << 6) | ((com0b & 3) << 4) | (wgm & 3);
	uint8_t b = ((foc0a & 1) << 7) | ((foc0b & 1) << 6) | ((wgm & 4) << 1) | (ts.m_cs & 7);
	TCCR0A = a;
	TCCR0B = b;

	OCR0A = ts.m_top;

	int ocieNa = 1;
	int ocieNb = 0;
	int toieN = 0;
	TIMSK0 = ((ocieNa & 1) << OCIE0A) | ((ocieNb & 1) << OCIE0B) | ((toieN& 1) << TOIE0);

	CallbackInfo* info = &callbackInfo[0];

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

	TimerSetup ts = calcTimerSetup(frequency, maxTop);

	s_println("setup_CTC_timer3 cs=%d, top=%d", ts.m_cs, ts.m_top);

	int a = ((com0a & 3) << 6) | ((com0b & 3) << 4) | (wgm & 3);
	int b = ((icnc & 1) << 7) | ((ices & 1) << 6) | (((wgm >> 2) & 3) << 3) | (ts.m_cs & 7);
	TCCR3A = a;
	TCCR3B = b;

	OCR3A = ts.m_top;

	int icie3 = 0;
	int ocie3b = 0;
	int ocie3a = 1; // enable Timer/Counter3 Output Compare A Match interrupt
	int toie3 = 0;
	a = ((icie3 & 1) << 5) | ((ocie3b & 1) << 2) | ((ocie3a & 1) << 1) | (toie3 & 1);
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
