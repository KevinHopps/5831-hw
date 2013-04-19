#include <stdio.h>
#include <string.h>
#include "kdebug.h"
#include "kio.h"
#include "kmotor.h"
#include "kserial.h"
#include "kutils.h"
#include "lab2cmds.h"
#include "PDControl.h"
#include "Trajectory.h"

// This program is divided into four tasks.
//
// The Encoder task is interrupt-driven, monitors encoders to determine
// the motor position.
//
// The PDController adjusts motor torque to bring it to a given
// position smoothly. This is used for short distance runs. It
// operates at a higher frequency than the Trajectory Interporator.
//
// The Trajectory Interpolator takes high-level motor position
// instructions and uses the PDController repeatedly, if necessary,
// to bring the motor to the desired position. It operates at a lower
// frequency than the PDController.
//
// The user interface uses serial I/O to communicate with the user.
// It is the lowest priority task, running in the main loop when
// no other tasks are operating.
//

#define RED_PIN IO_A3
#define YELLOW_PIN IO_A0
#define GREEN_PIN IO_D5

void setLED(int16_t pin, bool state);

void ConsoleTask(CommandIO* ciop)
{
	if (CIOCheckForCommand(ciop))
		CIORunCommand(ciop);
		
	Context* ctx = (Context*)ciop->m_context;
	if (ctx->m_logging)
	{
		static TorqueCalc lastTorqueCalc;
		TorqueCalc tc;
		PDControlGetTorqueCalc(ctx->m_pdc, &tc);
		if (!EqualTorqueCalc(&tc, &lastTorqueCalc))
		{
			lastTorqueCalc = tc;
			s_printf("Pr=%ld, Pm=%ld, T=%d", tc.m_Pr, tc.m_Pm, tc.m_torqueUsed);
			if (tc.m_torqueUsed != tc.m_torqueCalculated)
			{
				s_printf(", TC=%ld, %s", tc.m_torqueCalculated, s_ftos(tc.m_Kp, 2));
				s_printf("*%ld + %s", (long)(tc.m_Pr - tc.m_Pm), s_ftos(tc.m_Kd, 2));
				s_printf("*%s", s_ftos(tc.m_velocity, 2));
				if (tc.m_torqueChangeTooHigh)
					s_printf(", rapid acc");
				if (tc.m_torqueMagnitudeTooHigh)
					s_printf(", exceeds max");
				if (tc.m_torqueMagnitudeTooLow)
					s_printf(", no movement");
			}
			s_println("");
		}
	}
}

#define PDCONTROLLER_PERIOD_MSEC 10 // max 13
#define TRAJECTORY_PERIOD_MSEC 100 // max 3355

int main()
{
	setInterruptsEnabled(false);
	
	setLED(RED_PIN, 0);
	setLED(YELLOW_PIN, 0);
	setLED(GREEN_PIN, 0);
	
	Motor motor;
	MotorInit(&motor);
	
	// This initializes the PDControl task, including setting
	// up CTC timer 0.
	//
	PDControl pdControl;
	PDControlInit(&pdControl, &motor, PDCONTROLLER_PERIOD_MSEC);
	
	// This initializes the Trajectory task, including setting
	// up CTC timer 3.
	//
	Trajectory trajectory;
	TrajectoryInit(&trajectory, &pdControl, TRAJECTORY_PERIOD_MSEC);

	// This initializes the context that is used by the command
	// handling facility.
	Context commandContext;
	ContextInit(&commandContext, &trajectory, &pdControl, &motor);
	
	// This initializes the command handling facility. This
	// polls for serial input and executes commands when the
	// enter/return key is hit.
	//
	CommandIO cio;
	CIOReset(&cio, &commandContext);
	InitCommands(&cio);
	
	s_println("Type 'go' to enable Trajectory and PDControl");
	s_println("Type '?' for help");

	// We're all set, turn on the interrupt handlers. This enables
	// the PDControl task and Trajectory task to run from their
	// respective interrupt handlers. But those tasks will not do
	// anything until the "go" command is received.
	//
	setInterruptsEnabled	(true);
	
	while (true)
	{
		ConsoleTask(&cio); // poll for human commands on serial input
		
		// In interrupt handlers, serial output sometimes doesn't
		// work, so dbg_printf is used to store the characters into
		// a buffer. This prints them to the serial output, if there
		// are any.
		//
		dbg_flush();
	}
}

void setLED(int16_t pin, bool state)
{
	KIORegs io = getIORegs(pin);
	setDataDir(&io, OUTPUT);
	setIOValue(&io, state != 0);
}
