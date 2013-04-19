#include <pololu/orangutan.h>
#include <stdio.h>
#include "kdebug.h"
#include "kserial.h"
#include "kutils.h"
#include "PDControl.h"
#include "kmotor.h"

void TorqueCalcInit(TorqueCalc* tcp)
{
	tcp->m_Pr = 0;
	tcp->m_Pm = 0;
	tcp->m_Kp = 0.0;
	tcp->m_Kd = 0.0;
	tcp->m_velocity = 0.0;
	tcp->m_torqueCalculated = 0;
	tcp->m_torqueUsed = 0;
	tcp->m_torqueChangeTooHigh = false;
	tcp->m_torqueMagnitudeTooHigh = false;
	tcp->m_torqueMagnitudeTooLow = false;
}

// This compares two TorqueCalc structures and
// returns true if they are equal.
//
bool EqualTorqueCalc(const TorqueCalc* tcp1, const TorqueCalc* tcp2)
{
	return true
		&& tcp1->m_Pr == tcp2->m_Pr
		&& tcp1->m_Pm == tcp2->m_Pm
		&& tcp1->m_Kp == tcp2->m_Kp
		&& tcp1->m_Kd == tcp2->m_Kd
		&& tcp1->m_velocity == tcp2->m_velocity
		&& tcp1->m_torqueCalculated == tcp2->m_torqueCalculated
		&& tcp1->m_torqueUsed == tcp2->m_torqueUsed
		&& tcp1->m_torqueChangeTooHigh == tcp2->m_torqueChangeTooHigh
		&& tcp1->m_torqueMagnitudeTooHigh == tcp2->m_torqueMagnitudeTooHigh
		&& tcp1->m_torqueMagnitudeTooLow == tcp2->m_torqueMagnitudeTooLow
		;
}

// This initializes the PDControl structure and initializes CTC timer 0.
// The PDControlTask function will be called by the interrupt handler
// for timer 0, and the PDControl* will be passed to it.
//
void PDControlInit(PDControl* pdc, Motor* motor, uint16_t periodMSec)
{
	pdc->m_enabled = false;
	pdc->m_targetAngleSet = false;
	pdc->m_ready = false;
	pdc->m_lastAngle = 0;
	pdc->m_targetAngle = 0;
	pdc->m_kp = 6;
	pdc->m_kd = -6;
	pdc->m_lastMSec = 0;
	pdc->m_motor = motor;
	pdc->m_calcIndex = 0;
	TorqueCalcInit(&pdc->m_calc);
	
	setup_CTC_timer0(periodMSec, PDControlTask, pdc);
}

// Set the Kp value for the torque calculation function.
//
void PDControlSetKp(PDControl* pdc, float kp)
{
	pdc->m_kp = kp;
}

// Get the Kp value used in the torque calculation function.
//
float PDControlGetKp(PDControl* pdc)
{
	return pdc->m_kp;
}

// Set the Kd value for the torque calculation function.
//
void PDControlSetKd(PDControl* pdc, float kd)
{
	pdc->m_kd = kd;
}

// Get the Kd value used in the torque calculation function.
//
float PDControlGetKd(PDControl* pdc)
{
	return pdc->m_kd;
}

// This sets the target angle for the PDControl task.
// The PDControl task will notice the change and adapt.
//
void PDControlSetTargetAngle(PDControl* pdc, MotorAngle angle)
{
	pdc->m_targetAngle = angle;
	pdc->m_targetAngleSet = true;
}

// This returns the current motor position.
//
MotorAngle PDControlGetCurrentAngle(PDControl* pdc)
{
	return MotorGetCurrentAngle(pdc->m_motor);
}

// This resets the current motor position, treating
// the current location as zero.
//
void PDControlResetCurrentAngle(PDControl* pdc)
{
	MotorResetCurrentAngle(pdc->m_motor);
}

void PDControlSetEnabled(PDControl* pdc, bool enabled)
{
	if (pdc->m_enabled != enabled)
	{
		pdc->m_enabled = enabled;
		pdc->m_ready = false; // must now do work to allow velocity calculation
		pdc->m_targetAngleSet = false; // must now set a new target angle
	}
}

bool PDControlIsEnabled(PDControl* pdc)
{
	return pdc->m_enabled;
}

#define MAX_ERROR 1 // what is considered "close enough"
#define MAX_DELTA_TORQUE (MOTOR_MAX_TORQUE / 8) // limits acceleration

// This is called from the timer0 interrupt vector. This *is* the
// PDController task.
//
// The new torque is calculated based on the formula
//     t = Kp*errorAngle + Kd*velocity
//
// Now errorAngle is the difference between the current angle and
// the target angle, and velocity is the rate of change of the angle.
//
// Because of the component of velocity, we must measure the change
// in angle since the last time this function was called, and calculate
// the elapsed time. Because of this, we cannot calculate velocity the
// first time this function is called. The m_ready flag is used for
// this.
//
void PDControlTask(void* arg)
{
	PDControl* pdc = (PDControl*)arg;
	
	if (!pdc->m_enabled)
		return;
		
	MotorAngle currentAngle = MotorGetCurrentAngle(pdc->m_motor);
	uint32_t currentMSec = get_ms();
	
	if (!pdc->m_ready)
		pdc->m_ready = true;
	else if (pdc->m_targetAngleSet)
	{
		int16_t lastTorque = MotorGetTorque(pdc->m_motor);
		MotorAngle errorAngle = pdc->m_targetAngle - currentAngle;
		MotorAngle deltaAngle = currentAngle - pdc->m_lastAngle;
		uint32_t elapsed = currentMSec - pdc->m_lastMSec;
		if (elapsed > 0)
		{
			// m_calc records this calculation so it may be recalled
			// by the logging task, invoked by the main loop.
			//
			++pdc->m_calcIndex;
			TorqueCalcInit(&pdc->m_calc);
			
			pdc->m_calc.m_Pr = pdc->m_targetAngle;
			pdc->m_calc.m_Pm = currentAngle;
			pdc->m_calc.m_Kp = pdc->m_kp;
			pdc->m_calc.m_Kd = pdc->m_kd;
		
			int32_t torque = 0;
			
			if (ABS(errorAngle) > MAX_ERROR)
			{
				pdc->m_calc.m_velocity = (float)deltaAngle / (float)elapsed;
				torque = pdc->m_kp * errorAngle - pdc->m_kd * pdc->m_calc.m_velocity;
				pdc->m_calc.m_torqueCalculated = torque;
				
				pdc->m_calc.m_torqueChangeTooHigh = true; // assume the worse, fix below if ok
				int16_t minTorque = lastTorque - MAX_DELTA_TORQUE;
				if (torque < minTorque)
					torque = minTorque; // acceleration too high, limit it
				else
				{
					int16_t maxTorque = lastTorque + MAX_DELTA_TORQUE;
					if (torque > maxTorque)
						torque = maxTorque; // acceleration too high, limit it
					else
						pdc->m_calc.m_torqueChangeTooHigh = false; // acceleration ok after all
				}
				
				// If the torque magnitude is too small to produce movement,
				// set the torque magnitude to the minimum that does move.
				//
				if (-MOTOR_MIN_TORQUE < torque && torque < MOTOR_MIN_TORQUE)
				{
					pdc->m_calc.m_torqueMagnitudeTooLow = true; // assume the worse, fix below if ok
					if (torque < 0)
						torque = -MOTOR_MIN_TORQUE;
					else if (torque > 0)
						torque = MOTOR_MIN_TORQUE;
					else
						pdc->m_calc.m_torqueMagnitudeTooLow = false; // magnitude ok after all
				}
			}

			pdc->m_calc.m_torqueMagnitudeTooHigh = true; // assume the worst, fix below if ok
			if (torque < -MOTOR_MAX_TORQUE)
				torque = -MOTOR_MAX_TORQUE;
			else if (torque > MOTOR_MAX_TORQUE)
				torque = MOTOR_MAX_TORQUE;
			else
				pdc->m_calc.m_torqueMagnitudeTooHigh = false; // magnitude ok after all
			
			pdc->m_calc.m_torqueUsed = (int16_t)torque;
			MotorSetTorque(pdc->m_motor, (int16_t)torque);
		}
	}
	
	pdc->m_lastMSec = currentMSec;
	pdc->m_lastAngle = currentAngle;
}

// Return the last torque calculation performed.
//
uint32_t PDControlGetTorqueCalc(PDControl* pdc, TorqueCalc* tcp)
{
	uint32_t result = 0;
	
	// Just to make sure we don't start performing a new calulcation
	// in the middle of copying this one, we turn interrupts off.
	//
	BEGIN_ATOMIC
		*tcp = pdc->m_calc;
		result = pdc->m_calcIndex;
	END_ATOMIC
	
	return result;
}
