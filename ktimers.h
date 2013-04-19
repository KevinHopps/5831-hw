#ifndef _ktimers_h_
#define _ktimers_h_

#define CLOCK_TICKS_PER_SEC (20L * 1000L * 1000L) // 20 MHz
#define CLOCK_TICKS_PER_MSEC (CLOCK_TICKS_PER_SEC / 1000L)
#define MAX_TOP_TIMER0 255
#define MAX_TOP_TIMER3 65535
#define MAX_PRESCALE 1024
#define MAX_PERIOD_MSEC_TIMER0 (((long)MAX_TOP_TIMER0 * (long)MAX_PRESCALE) / CLOCK_TICKS_PER_MSEC) // 13
#define MAX_PERIOD_MSEC_TIMER3 (((long)MAX_TOP_TIMER3 * (long)MAX_PRESCALE) / CLOCK_TICKS_PER_MSEC) // 3355

typedef void (*Callback)(void* arg);

void setup_CTC_timer0(uint16_t msecPeriod, Callback func, void* arg);
void setup_CTC_timer3(uint16_t msecPeriod, Callback func, void* arg);

// This will set up a phase-and-frequency-correct signal on OC2A with the given
// dutyCycle. A dutyCycle of 0 means full off and 255 means full on.
//
void setup_PWM_timer2(uint8_t dutyCycle);

#endif // #ifndef _ktimers_h_
