#include <pololu/orangutan.h>
#include "kio.h"
#include "kserial.h"
#include "ktimers.h"
#include "ktypes.h"
#include "kdebug.h"

#define WGM1_NORMAL 0 // TOP=0xffff OCRnx=immediate TOVn=MAX
#define WGM1_PWM_PHASE_CORRECT_8_BIT 1 // TOP=0x00ff, OCRnx=TOP, TOVn=BOTTOM
#define WGM1_PWM_PHASE_CORRECT_9_BIT 2 // TOP=0x01ff, OCRnx=TOP, TOVn=BOTTOM
#define WGM1_PWM_PHASE_CORRECT_10_BIT 3 // TOP=0x03ff, OCRnx=TOP, TOVn=BOTTOM
#define WGM1_CTC_TOP_OCRnA 4 // TOP=OCRnA, OCRnx=immediate, TOVn=MAX
#define WGM1_FAST_PWM_8_BIT 5 // TOP=0x00ff, OCRnx=BOTTOM, TOVn=TOP
#define WGM1_FAST_PWM_9_BIT 6 // TOP=0x01ff, OCRnx=BOTTOM, TOVn=TOP
#define WGM1_FAST_PWM_10_BIT 7 // TOP=0x03ff, OCRnx=BOTTOM, TOVn=TOP
#define WGM1_PWM_PHASE_AND_FREQ_CORRECT_TOP_ICRn 8 // TOP=ICRn, OCRnx=BOTTOM, TOVn=BOTTOM
#define WGM1_PWM_PHASE_AND_FREQ_CORRECT_TOP_OCRnA 9 // TOP=OCRnA, OCRnx=BOTTOM, TOVn=BOTTOM
#define WGM1_PWM_PHASE_CORRECT_TOP_ICRn 10 // TOP=ICRn, OCRnx=TOP, TOVn=BOTTOM
#define WGM1_PWM_PHASE_CORRECT_TOP_OCRnA 11 // TOP=OCRnA, OCRnx=TOP, TOVn=BOTTOM
#define WGM1_CTC_TOP_ICRn 12 // TOP=ICRn, OCRnx=immediate, TOVn=MAX
#define WGM1_RESERVED 13
#define WGM1_FAST_PWM_TOP_ICRn 14 // TOP=ICRn, OCRnx=BOTTOM, TOVn=TOP
#define WGM1_FAST_PWM_TOP_OCRnA 15 // TOP=OCRnA, OCRnx=BOTTOM, TOVn=TOP

#define WGM2_NORMAL 0
#define WGM2_PWM_PHASE_CORRECT_TOP_FF 1 // TOP=0xff
#define WGM2_PWM_PHASE_CORRECT_TOP_OCRA 5 // TOP=OCRA

typedef struct CallbackInfo_
{
	Callback m_func;
	void* m_arg;
} CallbackInfo;

CallbackInfo timer0CallbackInfo;
CallbackInfo timer3CallbackInfo;
CallbackInfo timer2MatchCallbackInfo;
CallbackInfo timer2PeriodCallbackInfo;

static const int kMinPrescaleIndex = 1;
static const int kPrescaleFrequency[] = { 0, 1, 8, 64, 256, 1024 };
static const int kMaxPrescaleIndex = sizeof(kPrescaleFrequency) / sizeof(*kPrescaleFrequency) - 1;
static const uint32_t kClockFrequency = 20L * 1000L * 1000L;

struct TimerSetup_
{
	uint8_t m_cs;
	uint16_t m_top;
};
typedef struct TimerSetup_ TimerSetup;

static TimerSetup calcTimerSetup(int msecPeriod, uint16_t maxTop)
{
	TimerSetup result = { 0, 0 };

	uint32_t ltop = 0;
	result.m_cs = kMinPrescaleIndex;
	int done = 0;
	uint32_t periodTimesClockFreq = msecPeriod * (kClockFrequency / 1000);
	while (!done)
	{
		int prescale = kPrescaleFrequency[result.m_cs];
		ltop = periodTimesClockFreq / prescale;
		if (result.m_cs >= kMaxPrescaleIndex || ltop <= maxTop)
			done = 1;
		else
			++result.m_cs;
	}

	result.m_top = (int)ltop;

	int derivedPeriod = (1000L * result.m_top * kPrescaleFrequency[result.m_cs]) / kClockFrequency;
	s_println("calcTimerSetup: msecPeriod=%d, (1000L * top=%u * prescale[%d]=%d / kClockFrequency) = %d",
		msecPeriod, result.m_top, result.m_cs, kPrescaleFrequency[result.m_cs], derivedPeriod);

	return result;
}

void setup_CTC_timer0(uint16_t msecPeriod, Callback func, void* arg)
{
	TimerSetup ts = { 0, 0 };

	if (msecPeriod > 0)
	{
		uint16_t maxTop = 0xff; // timer0 uses 8-bit registers
		ts = calcTimerSetup(msecPeriod, maxTop);
	}

	s_println("setup_CTC_timer0: cs=%d, top=%u", ts.m_cs, ts.m_top);

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

	int ocieNa = (msecPeriod > 0);
	int ocieNb = 0;
	int toieN = 0;
	TIMSK0 = ((ocieNa & 1) << OCIE0A) | ((ocieNb & 1) << OCIE0B) | ((toieN& 1) << TOIE0);

	timer0CallbackInfo.m_func = func;
	timer0CallbackInfo.m_arg = arg;
}

void set_bits(uint8_t* lvalue, int highest, int lowest, int value)
{
	int nbits = highest - lowest + 1;
	int mask = ~(~0 << nbits);
	value &= mask;
	mask <<= lowest;
	value <<= lowest;
	*lvalue = (*lvalue & ~mask) | value;
}

void setup_PWM_timer2(float dutyCycle)
{
	int useA = 0;
	int useB = 1;
	int wgm = WGM2_PWM_PHASE_CORRECT_TOP_FF;
	uint8_t tccr2a = 0;
	uint8_t tccr2b = 0;
	
	tccr2a = 0;
	set_bits(&tccr2a, 7, 6, 2*useA);
	set_bits(&tccr2a, 5, 4, 2*useB);
	set_bits(&tccr2a, 1, 0, wgm);

	tccr2b = 0;
	wgm >>= 2;
	set_bits(&tccr2b, 3, 3, wgm);
	int cs = 5;
	set_bits(&tccr2b, 2, 0, cs);
	
	TCCR2A = tccr2a;
	TCCR2B = tccr2b;
	
	uint8_t matchCount = (uint8_t)(255 * dutyCycle + 0.5);
	OCR2A = 0;
	OCR2B = 0;
	if (useA)
		OCR2A = matchCount;
	else if (useB)
		OCR2B = matchCount;

	TIMSK2 = 0;
}

void setup_CTC_timer3(uint16_t msecPeriod, Callback func, void* arg)
{
	TimerSetup ts = {0, 0};
	int com0a = 0; // 0 for "normal" mode
	int com0b = 0; // 0 for "normal" mode
	int wgm = 4; // OCR3 for TOP
	int icnc = 0; // input capture noise canceller
	int ices = 0; // image capture edge select
	unsigned maxTop = 0xffff; // timer3 uses 16-bit registers

	if (msecPeriod > 0)
	{
		ts = calcTimerSetup(msecPeriod, maxTop);
		s_println("setup_CTC_timer3 cs=%d, top=%u", ts.m_cs, ts.m_top);
	}

	int a = ((com0a & 3) << 6) | ((com0b & 3) << 4) | (wgm & 3);
	int b = ((icnc & 1) << 7) | ((ices & 1) << 6) | (((wgm >> 2) & 3) << 3) | (ts.m_cs & 7);
	TCCR3A = a;
	TCCR3B = b;

	OCR3A = ts.m_top;

	int icie3 = 0;
	int ocie3b = 0;
	int ocie3a = (msecPeriod > 0); // enable Timer/Counter3 Output Compare A Match interrupt
	int toie3 = 0;
	a = ((icie3 & 1) << 5) | ((ocie3b & 1) << 2) | ((ocie3a & 1) << 1) | (toie3 & 1);
	TIMSK3 = a;

	timer3CallbackInfo.m_func = func;
	timer3CallbackInfo.m_arg = arg;
}

ISR(TIMER0_COMPA_vect)
{
	Callback func = timer0CallbackInfo.m_func;
	void* arg = timer0CallbackInfo.m_arg;
	func(arg);
}

/*
ISR(TIMER2_COMPA_vect)
{
	Callback func = timer2MatchCallbackInfo.m_func;
	void* arg = timer2MatchCallbackInfo.m_arg;
	if (func != 0)
		func(arg);
}

ISR(TIMER2_OVF_vect)
{
	Callback func = timer2PeriodCallbackInfo.m_func;
	void* arg = timer2PeriodCallbackInfo.m_arg;
	if (func != 0)
		func(arg);
}
*/

ISR(TIMER3_COMPA_vect)
{
	Callback func = timer3CallbackInfo.m_func;
	void* arg = timer3CallbackInfo.m_arg;
	if (func != 0)
		func(arg);
}
