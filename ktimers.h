#ifndef _ktimers_h_
#define _ktimers_h_

#include "ktypes.h"

// This will set up a phase-and-frequency-correct signal on OC2A with the given
// dutyCycle. A dutyCycle of 0 means full off and 255 means full on.
//
void setup_PWM_timer2(uint8_t dutyCycle);

// This is the signature of a callback function. Some
// timers may have a callback associated with them, which will
// be called from their respective ISRs. When you call the timer
// setup function, you specify a callback function and an arg
// that will be passed to it.
//
typedef void (*Callback)(void* arg);

// These specify the max period in microseconds for an 8- and 16-bit
// CTC timer. Note that the calculation adds 1 to the TOP, because
// it is every time the counter reaches TOP that the ISR will fire.
// That will occur every TOP+1 times, because the counter resets
// to zero, not one. For example, if TOP=1, the counter will repeat
// 0, 1, 0, 1, and the ISR will fire every two times.
//
#define CLOCK_TICKS_PER_USEC 20 // 20 MHz clock (not 20*1024*1024)
#define MAX_PRESCALE 1024
#define MAX_PERIOD_USEC(TOP) ((((uint32_t)(TOP)+1) * (uint32_t)MAX_PRESCALE) / CLOCK_TICKS_PER_USEC)
#define MAX_PERIOD_USEC_8BIT MAX_PERIOD_USEC(0xff) // 13107 = 13.1 milliseconds
#define MAX_PERIOD_USEC_16BIT MAX_PERIOD_USEC(0xffff) // 3355443 = 3.36 seconds

// This sets up a timer (0, 1, or 3) as a CTC timer with the specified
// period, in microseconds. Timer 0 is an 8 bit timer, and the period
// must be in the range [1,13107]. Timers 1 and 3 are 16 bit timers,
// and their periods must be in the range [1,3355443].
//
// The callback function is called from the ISR, with interrupts disabled.
// The arg is not used, but is passed to the Callback func. The period
// may not be exactly matchable, and the actual period is returned.
//

uint32_t setup_CTC_timer(uint8_t whichTimer, uint32_t periodUSec, Callback func, void* arg);

void setupMSecTimer(uint8_t whichTimer); // sets up timer 1 or 3 as a millisecond timer
void resetMSecTimer(); // resets the count
uint32_t getMSec(); // gets the count since last {setup,reset}MSecTimer() call.


#endif // #ifndef _ktimers_h_
