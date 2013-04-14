#include <pololu/orangutan.h>
#include "kserial.h"
#include "kutils.h"
#include "PDControl.h"

void PDControlInit(PDControl* pdc, Motor* motor)
{
	pdc->m_enabled = false;
	pdc->m_readiness = 0;
	pdc->m_lastAngle = 0;
	pdc->m_targetAngle = 0;
	pdc->m_kp = 0.005;
	pdc->m_kd = 0.05;
	pdc->m_lastMSec = 0;
	pdc->m_motor = motor;
}

void PDControlSetKp(PDControl* pdc, float kp)
{
	pdc->m_kp = kp;
}

float PDControlGetKp(PDControl* pdc)
{
	return pdc->m_kp;
}

void PDControlSetKd(PDControl* pdc, float kd)
{
	pdc->m_kd = kd;
}

float PDControlGetKd(PDControl* pdc)
{
	return pdc->m_kd;
}

void PDControlRotate(PDControl* pdc, int16_t angle)
{
	pdc->m_targetAngle = MotorGetCurrentAngle(pdc->m_motor) + angle;
}

int16_t PDControlGetCurrentAngle(PDControl* pdc)
{
	return MotorGetCurrentAngle(pdc->m_motor);
}

#define MAX_ERROR (1)
#define MAX_DELTA_TORQUE (1.0)

void taskPDControl(void* arg)
{
	PDControl* pdc = (PDControl*)arg;
	
	if (!pdc->m_enabled)
		return;
		
	int16_t currentAngle = PDControlGetCurrentAngle(pdc);
	uint32_t now = get_ms();
	
	if (pdc->m_readiness < 1)
		++pdc->m_readiness;
	else
	{
		float lastTorque = MotorGetTorque(pdc->m_motor);
		int16_t errorAngle = pdc->m_targetAngle - currentAngle;
		int16_t deltaAngle = currentAngle - pdc->m_lastAngle;
		float elapsed = now - pdc->m_lastMSec;
		if (elapsed > 0.0)
		{
			if (ABS(errorAngle) > MAX_ERROR)
			{
				float velocity = deltaAngle / elapsed;
				if (pdc->m_readiness < 2)
					++pdc->m_readiness;
				else
				{
					float torque = pdc->m_kp * errorAngle - pdc->m_kd * velocity;
					if (torque < lastTorque - MAX_DELTA_TORQUE)
						torque = lastTorque - MAX_DELTA_TORQUE;
					else
					{
						if (torque > lastTorque + MAX_DELTA_TORQUE)
							torque = lastTorque + MAX_DELTA_TORQUE;
					}
					
					if (-MOTOR_MIN_TORQUE < torque && torque < MOTOR_MIN_TORQUE)
					{
						if (torque < 0.0)
							torque = -MOTOR_MIN_TORQUE;
						else if (torque > 0.0)
							torque = MOTOR_MIN_TORQUE;
					}
					
					s_println("setting torque=%s", s_ftos(torque, 2));
					MotorSetTorque(pdc->m_motor, torque);
				}
			}
		}
	}
	
	pdc->m_lastMSec = now;
	pdc->m_lastAngle = currentAngle;
}

void PDControlSetEnabled(PDControl* pdc, bool enabled)
{
	if (pdc->m_enabled != enabled)
	{
		pdc->m_readiness = 0;
		pdc->m_enabled = enabled;
	}
}

bool PDControlIsEnabled(PDControl* pdc)
{
	return pdc->m_enabled;
}
