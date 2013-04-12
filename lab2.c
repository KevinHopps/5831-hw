#include <stdio.h>
#include "kio.h"
#include "kmotor.h"
#include "kserial.h"
#include "lab2cmds.h"

// Tasks
// encoder 2 ISRs to calculate encoder values
// pd controller (derive and send motor command)
// trajectory interpolator (derive reference position)
// user interface (monitor serial comm and set user reference position)

#define RED_PIN IO_A3
#define YELLOW_PIN IO_A0
#define GREEN_PIN IO_D5

void setLED(int16_t pin, bool state);

typedef struct Interpolator_ Interpolator;
struct Interpolator_
{
	uint8_t m_readiness;
	int16_t m_lastAngle;
	uint32_t m_lastMS;
	float m_lastTorque;
};
void InterpolatorInit(Interpolator* interp);
void InterpolatorIterate(Interpolator* interp, Motor* motor, uint32_t now);

#define INTERPOLATE 0

int main()
{	
	setLED(RED_PIN, 0);
	setLED(YELLOW_PIN, 0);
	setLED(GREEN_PIN, 0);

	Context commandContext;
	ContextInit(&commandContext);
	
	CommandIO cio;
	CIOReset(&cio, &commandContext);
	InitCommands(&cio);
	
	sei();
	
	Interpolator interpolator;
	InterpolatorInit(&interpolator);
	
	const uint32_t kPositionCheckInterval = 1;
	time_reset();
	uint32_t lastPositionCheck = get_ms();
	InterpolatorIterate(&interpolator, &commandContext.m_motor, lastPositionCheck); // 1st call initializes
	
	while (true)
	{
#if INTERPOLATE
		unsigned long now = get_ms();
		if (now - lastPositionCheck >= kPositionCheckInterval)
		{
			lastPositionCheck = now;
			InterpolatorIterate(&interpolator, &commandContext.m_motor, now);
		}
		else
#endif
			if (CIOCheckForCommand(&cio))
				CIORunCommand(&cio);
	}
}

void InterpolatorInit(Interpolator* interp)
{
	interp->m_readiness = 0;
	interp->m_lastAngle = 0;
	interp->m_lastMS = 0;
}

#define ABS(X) ((X) < 0 ? -(X) : (X))
#define SGN(X) ((X) < 0 ? -1 : ((X) > 0 ? 1 : 0))
#define MAX_ERROR (1)
#define MAX_DELTA_TORQUE (1.0)

void InterpolatorIterate(Interpolator* interp, Motor* motor, uint32_t now)
{
	static int32_t ncalls;
	
	if (ncalls++ % 1000 == 0)
		s_println("InterpolatorIterate");
	int16_t currentAngle = MotorGetCurrentAngle(motor);
	
	if (interp->m_readiness < 1)
		++interp->m_readiness;
	else
	{
		float lastTorque = MotorGetTorque(motor);
		int16_t desiredAngle = MotorGetTargetAngle(motor);
		int16_t errorAngle = desiredAngle - currentAngle;
		int16_t deltaAngle = currentAngle - interp->m_lastAngle;
		float elapsed = now - interp->m_lastMS;
		if (elapsed > 0.0)
		{
			if (ABS(errorAngle) > MAX_ERROR)
			{
				float velocity = deltaAngle / elapsed;
				if (interp->m_readiness < 2)
					++interp->m_readiness;
				else
				{
					float torque = MotorGetKp(motor) * errorAngle - MotorGetKd(motor) * velocity;
					if (torque < lastTorque - MAX_DELTA_TORQUE)
						torque = lastTorque - MAX_DELTA_TORQUE;
					else
					{
						if (torque > lastTorque + MAX_DELTA_TORQUE)
							torque = lastTorque + MAX_DELTA_TORQUE;
					}
					
					float minTorque = MotorGetMinTorque(motor);
					if (-minTorque < torque && torque < minTorque)
					{
						if (torque < 0.0)
							torque = -minTorque;
						else if (torque > 0.0)
							torque = minTorque;
					}
					
					s_println("setting torque=%s", s_ftos(torque, 2));
					MotorSetTorque(motor, torque);
				}
			}
		}
	}
	
	interp->m_lastMS = now;
	interp->m_lastAngle = currentAngle;
}

void setLED(int16_t pin, bool state)
{
	KIORegs io = getIORegs(pin);
	setDataDir(&io, OUTPUT);
	setIOValue(&io, state != 0);
}
