#include <stdio.h>
#include "kio.h"
#include "kmotor.h"
#include "kserial.h"
#include "lab2cmds.h"
#include "PDControl.h"
#include "Trajectory.h"

// Tasks
// encoder 2 ISRs to calculate encoder values
// pd controller (derive and send motor command)
// trajectory interpolator (derive reference position)
// user interface (monitor serial comm and set user reference position)

#define RED_PIN IO_A3
#define YELLOW_PIN IO_A0
#define GREEN_PIN IO_D5

void setLED(int16_t pin, bool state);

void taskUI(CommandIO* ciop)
{
	if (CIOCheckForCommand(ciop))
		CIORunCommand(ciop);
}

#define PDCONTROLLER_PERIOD_MSEC 10
#define TRAJECTORY_PERIOD_MSEC 100

int main()
{
	setLED(RED_PIN, 0);
	setLED(YELLOW_PIN, 0);
	setLED(GREEN_PIN, 0);
	
	Motor motor;
	MotorInit(&motor);
	
	PDControl pdControl;
	PDControlInit(&pdControl, &motor);
	
	Trajectory trajectory;
	TrajectoryInit(&trajectory, &pdControl);

	Context commandContext;
	ContextInit(&commandContext, &trajectory, &pdControl, &motor);
	
	CommandIO cio;
	CIOReset(&cio, &commandContext);
	InitCommands(&cio);

	time_reset();
	
	setup_CTC_timer0(PDCONTROLLER_PERIOD_MSEC, taskPDControl, &pdControl);
	setup_CTC_timer3(TRAJECTORY_PERIOD_MSEC, taskTrajectory, &trajectory);
	
	sei();

	s_println("Type 'go' to enable Trajectory and PDControl");
	s_println("Type '?' for help");
	
	while (true)
	{
		taskUI(&cio);
	}
}

void setLED(int16_t pin, bool state)
{
	KIORegs io = getIORegs(pin);
	setDataDir(&io, OUTPUT);
	setIOValue(&io, state != 0);
}
