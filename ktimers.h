#ifndef _ktimers_h_
#define _ktimers_h_

typedef void (*Callback)(void* arg);

void setup_CTC_timer0(int msecPeriod, Callback func, void* arg);
void setup_PWM_timer1(int msecPeriod, Callback func, void* arg);
void setup_CTC_timer3(int msecPeriod, Callback func, void* arg);

#endif // #ifndef _ktimers_h_
