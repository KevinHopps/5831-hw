#ifndef _ktimers_h_
#define _ktimers_h_

typedef void (*Callback)(void* arg);

void setup_CTC_timer0(uint16_t msecPeriod, Callback func, void* arg);
void setup_CTC_timer3(uint16_t msecPeriod, Callback func, void* arg);

// This will set up a phase-and-frequency-correct signal on OC2A with the given
// dutyCycle. The dutyCycle shall be in [0,1].
//
void setup_PWM_timer2(float dutyCycle);

#endif // #ifndef _ktimers_h_
