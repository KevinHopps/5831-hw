#include "kio.h"
#include "kmotor.h"
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

void main()
{
	CommandIO cio;
	CIOReset(&cio);
	InitCommands(&cio);
	
	setLED(RED_PIN, 0);
	setLED(YELLOW_PIN, 0);
	setLED(GREEN_PIN, 0);
	
	sei();
	
	while (true)
	{
		if (CIOCheckForCommand(&cio))
			CIORunCommand(&cio);
	}
}
