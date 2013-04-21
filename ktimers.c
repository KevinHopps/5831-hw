#include <pololu/orangutan.h>
#include "kdebug.h"
#include "kio.h"
#include "kserial.h"
#include "ktimers.h"
#include "ktypes.h"

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

typedef struct CallbackInfo_ CallbackInfo;
struct CallbackInfo_
{
	Callback m_func;
	void* m_arg;
};

CallbackInfo timer0CallbackInfo;
CallbackInfo timer3CallbackInfo;
CallbackInfo timer2MatchCallbackInfo;
CallbackInfo timer2PeriodCallbackInfo;

static const int kMinPrescaleIndex = 1;
static const int kPrescaleFrequency[] = { 0, 1, 8, 64, 256, 1024 };
static const int kMaxPrescaleIndex = sizeof(kPrescaleFrequency) / sizeof(*kPrescaleFrequency) - 1;
static const uint32_t kClockFrequency = 20L * 1000L * 1000L;

typedef struct TimerSetup_ TimerSetup;
struct TimerSetup_
{
	uint8_t m_cs;
	uint16_t m_top;
};

// This calculates a timer setup to achieve the specified period as closely as
// possible. This is done by making the prescale as small as possible such that
// the top value is <= maxTop. This is called for both 8-bit and 16-bit timers,
// so maxTop may be 0xff or 0xffff. This will fail if the period is too long
// given the contraints of maxTop and max prescale.
//
static uint16_t calcTimerSetup(TimerSetup* tsp, uint16_t msecPeriod, uint16_t maxTop)
{
	uint32_t ltop = 0;
	tsp->m_cs = kMinPrescaleIndex;
	int done = 0;
	uint32_t periodTimesClockFreq = msecPeriod * (kClockFrequency / 1000);
	while (!done)
	{
		int prescale = kPrescaleFrequency[tsp->m_cs];
		ltop = periodTimesClockFreq / prescale;
		if (tsp->m_cs >= kMaxPrescaleIndex || ltop <= maxTop)
			done = 1;
		else
			++tsp->m_cs;
	}

	KASSERT(ltop <= maxTop);
	
	uint16_t derivedPeriod = (1000L * tsp->m_top * kPrescaleFrequency[tsp->m_cs]) / kClockFrequency;
	tsp->m_top = (uint16_t)ltop;

	return derivedPeriod;
}

// Sets up a CTC using timer 0. The callback func will be invoked
// by the ISR. This function will fail if the period is too large.
//
void setup_CTC_timer0(uint16_t msecPeriod, Callback func, void* arg)
{
	TimerSetup ts;

	if (msecPeriod > 0)
	{
		uint16_t maxTop = 0xff; // timer0 uses 8-bit registers
		calcTimerSetup(&ts, msecPeriod, maxTop);
	}

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

// This sets the bits from [lowest,highest] in *lvalue, leaving
// other bits unchanged. For example:
//     uint8_t x = 0xff; // 11111111
//     uint8_t y = 0x00; // 00000000
//     set_bits(&x, 5, 2, 6); // 11.0110.11
//     set_bits(&y, 5, 2, 6); // 00.0110.00
//     KASSERT(x == 0xdb && y == 0x18);
//
void set_bits(uint8_t* lvalue, int highest, int lowest, int value)
{
	int nbits = highest - lowest + 1;
	int mask = ~(~0 << nbits);
	value &= mask;
	mask <<= lowest;
	value <<= lowest;
	*lvalue = (*lvalue & ~mask) | value;
}

void setup_PWM_timer2(uint8_t dutyCycle)
{
	uint8_t useA = 0; // one of useA/useB should be one and the other zero
	uint8_t useB = 1; // they specify which counter to use, A or B.
	uint8_t wgm = WGM2_PWM_PHASE_CORRECT_TOP_FF;
	
	uint8_t tccr2a = 0;
	set_bits(&tccr2a, 7, 6, 2*useA);
	set_bits(&tccr2a, 5, 4, 2*useB);
	set_bits(&tccr2a, 1, 0, wgm);

	uint8_t tccr2b = 0;
	wgm >>= 2;
	set_bits(&tccr2b, 3, 3, wgm);
	int cs = 5;
	set_bits(&tccr2b, 2, 0, cs);
	
	TCCR2A = tccr2a;
	TCCR2B = tccr2b;
	
	uint8_t matchCount = dutyCycle;
	OCR2A = 0;
	OCR2B = 0;
	if (useA)
		OCR2A = matchCount;
	else if (useB)
		OCR2B = matchCount;

	TIMSK2 = 0;
}

// Sets up a CTC using timer 3. The callback func will be invoked
// by the ISR. This function will fail if the period is too large.
//
void setup_CTC_timer3(uint16_t msecPeriod, Callback func, void* arg)
{
	TimerSetup ts;
	uint8_t com0a = 0; // 0 for "normal" mode
	uint8_t com0b = 0; // 0 for "normal" mode
	uint8_t wgm = 4; // OCR3 for TOP
	uint8_t icnc = 0; // input capture noise canceller
	uint8_t ices = 0; // image capture edge select

	if (msecPeriod > 0)
	{
		uint16_t maxTop = 0xffff; // timer3 uses 16-bit registers
		calcTimerSetup(&ts, msecPeriod, maxTop);
	}

	uint8_t a = ((com0a & 3) << 6) | ((com0b & 3) << 4) | (wgm & 3);
	uint8_t b = ((icnc & 1) << 7) | ((ices & 1) << 6) | (((wgm >> 2) & 3) << 3) | (ts.m_cs & 7);
	TCCR3A = a;
	TCCR3B = b;

	OCR3A = ts.m_top;
	
	s_println("TCCR3A=0x%02x, TCCR3B=0x%02x, top=%d, cs=%d", a, b, ts.m_top, ts.m_cs);

	uint8_t icie3 = 0;
	uint8_t ocie3b = 0;
	uint8_t ocie3a = (msecPeriod > 0); // enable Timer/Counter3 Output Compare A Match uint8_terrupt
	uint8_t toie3 = 0;
	a = ((icie3 & 1) << 5) | ((ocie3b & 1) << 2) | ((ocie3a & 1) << 1) | (toie3 & 1);
	s_println("TIMSK3=0x%02x", a);
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

ISR(TIMER3_COMPA_vect)
{
	Callback func = timer3CallbackInfo.m_func;
	void* arg = timer3CallbackInfo.m_arg;
	if (func != 0)
		func(arg);
}
