#ifndef _ktimers_h_
#define _ktimers_h_

typedef void (*Callback)(void* arg);

typedef struct CallbackInfo_
{
	Callback m_func;
	void* m_arg;
} CallbackInfo;

// This will setup a timer with an interrupt handler that will invoke
//     func(arg)
// frequency times per second.
//
CallbackInfo setup_CTC_timer(int whichTimer, int frequency, Callback func, void* arg);

void set_TCCRn(int n, int com0a, int com0b, int wgm, int foc0a, int foc0b, int cs);
void set_OCRnA(int n, int val);
void set_TIMSKn(int n, int ocieNa, int ocieNb, int toieN);

#endif // #ifndef _ktimers_h_
