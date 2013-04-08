#include "kdebug.h"
#include "kmotor.h"
#include "ktimers.h"

#define PWM_DATA_DIR DDRD
#define PWM_PIN 6
#define MOTOR_DATA_DIR DDRC
#define MOTOR_DIR PORTC
#define MOTOR_PIN 6

static void initMotorPort()
{
	PWM_DATA_DIR |= 1 << PWM_PIN;
	MOTOR_DATA_DIR |= 1 << MOTOR_PIN;
}

static void setMotorDirection(bool forward)
{
	uint8_t bit = 1 << MOTOR_PIN;
	if (forward)
		MOTOR_DIR |= bit;
	else
		MOTOR_DIR &= ~bit;
}

void MotorInit(Motor* motor)
{
	motor->m_currentAngle = 0;
	motor->m_targetAngle = 0;
	motor->m_torque = 0.0;
	motor->m_Kp = 0.0;
	motor->m_Kd = 0.0;
	
	initMotorPort();
	
	MotorSetTorque(motor, 0.0);
}

void MotorSetKp(Motor* motor, float Kp)
{
	motor->m_Kp = Kp;
}

float MotorGetKp(Motor* motor)
{
	return motor->m_Kp;
}

void MotorSetKd(Motor* motor, float Kd)
{
	motor->m_Kd = Kd;
}

float MotorGetKd(Motor* motor)
{
	return motor->m_Kd;
}

void MotorSetTorque(Motor* motor, float torque)
{
	bool forward = true;
	if (torque < 0.0f)
	{
		forward = false;
		torque = -torque;
	}

	s_println("MotorSetTorque %s", s_ftos(torque));
	
	setMotorDirection(forward);
	setup_PWM_timer2(torque);

	motor->m_torque = torque;
}

float MotorGetTorque(Motor* motor)
{
	return motor->m_torque;
}

void MotorMakeCurrentAngleZero(Motor* motor)
{
	motor->m_targetAngle -= motor->m_currentAngle;
	motor->m_currentAngle = 0;
}

void MotorSetTargetAngle(Motor* motor, int16_t degrees)
{
	motor->m_targetAngle = degrees;
}

int16_t MotorGetTargetAngle(Motor* motor)
{
	return motor->m_targetAngle;
}

int16_t MotorGetCurrentAngle(Motor* motor)
{
	return motor->m_currentAngle;
}

int16_t MotorGetDeltaAngle(Motor* motor)
{
	return MotorGetTargetAngle(motor) - MotorGetCurrentAngle(motor);
}
