#include <stdio.h>
#include <string.h>
#include <pololu/orangutan.h>
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

void setLED(int16_t pin, uint8_t state); // state: 0=OFF, 0xff=TOGGLE, else=ON

// This is the user interface task. It is invoked from the main
// loop when no interrupt handlers (higher priority tasks) are
// running.
//
void ConsoleTask(CommandIO* ciop)
{
	// CIOCheckForCommand polls serial input and returns true
	// when the user has entered a valid command. Then
	// CIORunCommand() is invoked to run the command.
	//
	if (CIOCheckForCommand(ciop))
		CIORunCommand(ciop);

	// If logging is enabled, we will print out the results of
	// the PDController's last torque calculation. We keep track
	// of the last one reported, so that we only issue a report
	// if the values are different.
	//
	Context* ctx = (Context*)ciop->m_context;
	if (ctx->m_logging)
	{
		static TorqueCalc lastTorqueCalc; // last one reported
		TorqueCalc tc; // new one
		PDControlGetTorqueCalc(ctx->m_pdc, &tc); // get new one
		if (!EqualTorqueCalc(&tc, &lastTorqueCalc))
		{
			// If the calcluation has changed, we issue a report.
			//
			lastTorqueCalc = tc;
			
			// Minimally, we print out Pr, Pm, and T
			//
			s_printf("Pr=%ld, Pm=%ld, T=%d", tc.m_Pr, tc.m_Pm, tc.m_torqueUsed);
			if (tc.m_torqueUsed != tc.m_torqueCalculated)
			{
				// Sometimes the calculation generates a torque that is
				// unusable for some reason. This explains why.
				// This is the calculated torque and the Kp value, as
				// a float.
				//
				s_printf(", TC=%ld, %s", tc.m_torqueCalculated, s_ftos(tc.m_Kp, 2));
				
				// This is the error (Pr - Pm) and the Kd value, as a float.
				//
				s_printf("*%ld + %s", (long)(tc.m_Pr - tc.m_Pm), s_ftos(tc.m_Kd, 2));
				
				// This is the velocity, as a float.
				//
				s_printf("*%s", s_ftos(tc.m_velocity, 2));
				
				// The PDController caps the change in torque that is allowed,
				// in order to prevent jerky motion. This reports if that capping
				// was done.
				//
				if (tc.m_torqueChangeTooHigh)
					s_printf(", rapid acc");
					
				// If the forumla calculated a torque that was higher in
				// magnitude (positive or negative) than is allowed, this
				// is reported here.
				// 
				if (tc.m_torqueMagnitudeTooHigh)
					s_printf(", exceeds max");
					
				// If the formula calculated a torque that is too small
				// in magnitude to produce motion, the PDController will
				// increase its magnitude to the minimum that does move.
				// That is reported here.
				if (tc.m_torqueMagnitudeTooLow)
					s_printf(", no movement");
			}
			s_println("");
		}
	}
}

#define PDCONTROLLER_PERIOD_MSEC 100 // max 3355
#define TRAJECTORY_PERIOD_MSEC 10 // max 13

int main()
{
	setInterruptsEnabled(false);
	
	setLED(RED_PIN, 0);
	setLED(YELLOW_PIN, 0);
	setLED(GREEN_PIN, 0);
	
	// Initialize the Motor structure.
	//
	Motor motor;
	MotorInit(&motor);
	
	// This initializes the PDControl task, including setting
	// up CTC timer 3. Since it talks to the motor, we pass it
	// the address of the Motor structure.
	//
	PDControl pdControl;
	PDControlInit(&pdControl, &motor, PDCONTROLLER_PERIOD_MSEC);
	
	// This initializes the Trajectory task, including setting
	// up CTC timer 0. Since it talks to the PDController, we
	// pass it the address of the PDControl structure.
	//
	Trajectory trajectory;
	TrajectoryInit(&trajectory, &pdControl, TRAJECTORY_PERIOD_MSEC);

	// This initializes the context that is used by the command
	// handling facility. Since the commands may access any of the
	// data structures, we pass all of them.
	//
	Context commandContext;
	ContextInit(&commandContext, &trajectory, &pdControl, &motor);
	
	// This initializes the command handling facility. This
	// polls for serial input and executes commands when the
	// enter/return key is hit. We pass it the address of the
	// Context, since any command may use it.
	//
	CommandIO cio;
	CIOReset(&cio, &commandContext);
	InitCommands(&cio);
	
	// Print a welcome message and a hint at how to get help.
	//
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

void setLED(int16_t pin, uint8_t state)
{
	KIORegs io = getIORegs(pin);
	setDataDir(&io, OUTPUT);
	setIOValue(&io, state != 0);
}
