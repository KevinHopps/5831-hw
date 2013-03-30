#ifndef _ktimers_h_
#define _ktimers_h_

typedef void (*Callback)(void* arg);

// This will setup a timer with an interrupt handler that will invoke
//     func(arg)
// frequency times per second.
//
// NOTE: Currently, whichTimer must be 0. This is because there are different
// tables to use for different timers.
//
void setup_CTC_timer(int whichTimer, int frequency, Callback func, void* arg);

void setup_CTC_timer3(int frequency, Callback func, void* arg);

void set_TCCRn(int n, int com0a, int com0b, int wgm, int foc0a, int foc0b, int cs);
void set_OCRnA(int n, int val);
void set_TIMSKn(int n, int ocieNa, int ocieNb, int toieN);

#endif // #ifndef _ktimers_h_
