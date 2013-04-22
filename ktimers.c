#include <pololu/orangutan.h>
#include "kdebug.h"
#include "kio.h"
#include "kserial.h"
#include "ktimers.h"
#include "kutils.h"
#include "ktypes.h"

// The following functions implement a millisecond timer
// that is accurate to within 0.03%. The values of
// gNumerator and gDenominator were determined by experiment
// using the "clock" command in lab2cmds.c.
//
volatile uint32_t gTimeCounter;
volatile uint16_t gNumerator = 453;
volatile uint16_t gDenominator = 195;

void TimerCallback(void* arg)
{
	uint32_t* counter = (uint32_t*)arg;
	++(*counter);
}

void setupMSecTimer(uint8_t whichTimer)
{
	KASSERT(whichTimer == 1 || whichTimer == 3);
	BEGIN_ATOMIC
		setup_CTC_timer(whichTimer, 100, TimerCallback, (void*)&gTimeCounter);
	END_ATOMIC
}

uint32_t getMSec()
{
	uint32_t now;
	
	BEGIN_ATOMIC
		now = gTimeCounter;
	END_ATOMIC
	
	return (now * gNumerator) / gDenominator;
}

void resetMSecTimer()
{
	BEGIN_ATOMIC
		gTimeCounter = 0;
	END_ATOMIC
}

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

CallbackInfo timerCallbackInfo[4];

static const int kMinPrescaleIndex = 1;
static const int kPrescaleFrequency[] = { 0, 1, 8, 64, 256, 1024 };
static const int kMaxPrescaleIndex = sizeof(kPrescaleFrequency) / sizeof(*kPrescaleFrequency) - 1;
static const uint16_t kClockFrequencyMHz = 20;

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
static uint32_t calcTimerSetup(TimerSetup* tsp, uint32_t usecPeriod, uint16_t maxTop)
{
	uint32_t calculatedTop = 0;
	tsp->m_cs = kMinPrescaleIndex;
	int done = 0;
	uint32_t periodTimesClockFreq = usecPeriod * kClockFrequencyMHz;
	while (!done)
	{
		int prescale = kPrescaleFrequency[tsp->m_cs];
		calculatedTop = periodTimesClockFreq / prescale - 1;
		if (tsp->m_cs >= kMaxPrescaleIndex || calculatedTop <= maxTop)
			done = 1;
		else
			++tsp->m_cs;
	}

	KASSERT(calculatedTop <= maxTop);

	// To calculate the actual period, we add 1 to calculatedTop, because
	// the ISR will fire ever time the counter reaches TOP, and the counter
	// starts at zero. For example, if TOP=2, the counter will repeat
	// 0, 1, 2, 0, 1, 2 and the ISR will fire every 3 times.
	//
	usecPeriod = ((calculatedTop + 1) * kPrescaleFrequency[tsp->m_cs]) / kClockFrequencyMHz;
	tsp->m_top = (uint16_t)calculatedTop;
	
	KASSERT(usecPeriod > 0);

	return usecPeriod;
}

// Sets up a CTC using timer 0. The callback func will be invoked
// by the ISR. This function will fail if the period is too large.
//
static uint32_t setup_CTC_timer0(uint32_t usecPeriod, Callback func, void* arg)
{
	KASSERT(usecPeriod <= MAX_PERIOD_USEC_8BIT);
	
	TimerSetup ts;

	if (usecPeriod > 0)
	{
		uint16_t maxTop = 0xff; // timer0 uses 8-bit registers
		usecPeriod = calcTimerSetup(&ts, usecPeriod, maxTop);
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

	int ocieNa = (usecPeriod > 0);
	int ocieNb = 0;
	int toieN = 0;
	TIMSK0 = ((ocieNa & 1) << OCIE0A) | ((ocieNb & 1) << OCIE0B) | ((toieN& 1) << TOIE0);

	CallbackInfo *cbi = &timerCallbackInfo[0];
	cbi->m_func = func;
	cbi->m_arg = arg;
	
	return usecPeriod;
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

static uint32_t setup_16bit_CTC_timer(uint8_t whichTimer /* 1 or 3 */, uint32_t usecPeriod, Callback func, void* arg)
{
	KASSERT(usecPeriod <= MAX_PERIOD_USEC_16BIT);
	
	TimerSetup ts;
	uint8_t com0a = 0; // 0 for "normal" mode
	uint8_t com0b = 0; // 0 for "normal" mode
	uint8_t wgm = 4; // OCRn for TOP
	uint8_t icnc = 0; // input capture noise canceller
	uint8_t ices = 0; // image capture edge select

	if (usecPeriod > 0)
	{
		uint16_t maxTop = 0xffff; // timers 1 & 3 use 16-bit registers
		usecPeriod = calcTimerSetup(&ts, usecPeriod, maxTop);
	}

	uint8_t tccrNa = ((com0a & 3) << 6) | ((com0b & 3) << 4) | (wgm & 3);
	uint8_t tccrNb = ((icnc & 1) << 7) | ((ices & 1) << 6) | (((wgm >> 2) & 3) << 3) | (ts.m_cs & 7);

	uint8_t icie3 = 0;
	uint8_t ocie3b = 0;
	uint8_t ocie3a = (usecPeriod > 0); // enable Timer/CounterN Output Compare A Match interrupt
	uint8_t toie3 = 0;
	uint8_t timskN = ((icie3 & 1) << 5) | ((ocie3b & 1) << 2) | ((ocie3a & 1) << 1) | (toie3 & 1);
	
	KASSERT(whichTimer == 1 || whichTimer == 3);
	
	if (whichTimer == 1)
	{
		TCCR1A = tccrNa;
		TCCR1B = tccrNb;
		OCR1A = ts.m_top;
		TIMSK1 = timskN;
	}
	else
	{
		TCCR3A = tccrNa;
		TCCR3B = tccrNb;
		OCR3A = ts.m_top;
		TIMSK3 = timskN;
	}

	CallbackInfo *cbi = &timerCallbackInfo[whichTimer];
	cbi->m_func = func;
	cbi->m_arg = arg;
	
	return usecPeriod;
}

uint32_t setup_CTC_timer(uint8_t whichTimer, uint32_t usecPeriod, Callback func, void* arg)
{
	KASSERT(whichTimer == 0 || whichTimer == 1 || whichTimer == 3);
	
	uint32_t result;
	
	if (whichTimer == 0)
		result = setup_CTC_timer0(usecPeriod, func, arg);
	else
		result = setup_16bit_CTC_timer(whichTimer, usecPeriod, func, arg);
		
	return result;
}

ISR(TIMER0_COMPA_vect)
{
	CallbackInfo *cbi = &timerCallbackInfo[0];
	Callback func = cbi->m_func;
	void* arg = cbi->m_arg;
	if (func != 0)
		func(arg);
}

ISR(TIMER1_COMPA_vect)
{
	CallbackInfo *cbi = &timerCallbackInfo[1];
	Callback func = cbi->m_func;
	void* arg = cbi->m_arg;
	if (func != 0)
		func(arg);
}

ISR(TIMER3_COMPA_vect)
{
	CallbackInfo *cbi = &timerCallbackInfo[3];
	Callback func = cbi->m_func;
	void* arg = cbi->m_arg;
	if (func != 0)
		func(arg);
}
