#ifndef _ktimers_h_
#define _ktimers_h_

typedef void (*Callback)(void* arg);

// These will setup a timer with an interrupt handler that will invoke
//     func(arg)
// frequency times per second.
//
// Timer0 is an 8-bit timer.
// Timer3 is a 16-bit timer.
// They require different setting up.
//
void setup_CTC_timer0(int frequency, Callback func, void* arg);
void setup_PWM_timer1(int frequency);
void setup_CTC_timer3(int frequency, Callback func, void* arg);

#endif // #ifndef _ktimers_h_
