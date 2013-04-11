#include <stdio.h>
#include "kio.h"
#include "kmotor.h"
#include "kserial.h"
#include "lab2cmds.h"

#define RED_PIN IO_A3
#define YELLOW_PIN IO_A0
#define GREEN_PIN IO_D5

void setLED(int16_t pin, bool state)
{
	KIORegs io = getIORegs(pin);
	setDataDir(&io, OUTPUT);
	setIOValue(&io, state != 0);
}

extern Motor theMotor;

int main()
{
	CommandIO cio;
	CIOReset(&cio);
	InitCommands(&cio);
	
	setLED(RED_PIN, 0);
	setLED(YELLOW_PIN, 0);
	setLED(GREEN_PIN, 0);
	
#if USE_WHEEL_ENCODERS
	encoders_init(IO_D3, IO_D2, IO_D1, IO_D0);
	int m1 = encoders_get_counts_m1();
	int m2 = encoders_get_counts_m2();
#endif
	
	sei();
	
	int16_t oldAngle = 0;
	
	while (true)
	{
		if (CIOCheckForCommand(&cio))
			CIORunCommand(&cio);
			
		int16_t angle = MotorGetCurrentAngle(&theMotor);
	}
}
