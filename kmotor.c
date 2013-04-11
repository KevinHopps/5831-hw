#include <stdio.h>
#include "kdebug.h"
#include "kmotor.h"
#include "kserial.h"
#include "ktimers.h"

#define PWM_TIMER_DDR DDRD
#define PWM_TIMER_BIT (1 << 6)
#define MOTOR_DIRECTION_DDR DDRC
#define MOTOR_DIRECTION_PORT PORTC
#define MOTOR_DIRECTION_BIT (1 << 6)
#define ENCODER_PCMSK PCMSK3
#define ENCODER_DDR DDRD
#define ENCODER_PIN PIND
#define ENCODER_BIT_A (1 << 2) // D2
#define ENCODER_BIT_B (1 << 3) // D3
#define ENCODER_BITS (ENCODER_BIT_A | ENCODER_BIT_B)
#define ENCODER_TICKS_PER_REVOLUTION (128.0) // by experiment, 12749/100
#define ENCODER_DEGREES_PER_TICK (360.0 / ENCODER_TICKS_PER_REVOLUTION) // 2.82
#define ENCODER_MAX_REVOLUTIONS_PER_SECOND (100.0 / 54.63) // by experiment, 1.83
#define ENCODER_MAX_DEGREES_PER_SECOND (ENCODER_MAX_REVOLUTIONS_PER_SECOND * 360.0) // 658.98
#define MIN_TORQUE (0.04) // min torque that makes motor turn

static uint8_t oldBitA;
static uint8_t oldBitB;
static int32_t gEncoderCount;
int16_t gEncoderError;

static void initMotorHW()
{
	PWM_TIMER_DDR |= PWM_TIMER_BIT;
	MOTOR_DIRECTION_DDR |= MOTOR_DIRECTION_BIT;
	
	PCICR = 0xff; // let PCMSKn do the masking
	ENCODER_PCMSK = ENCODER_BITS; // which pins cause interrupt
	ENCODER_DDR &= ~ENCODER_BITS; // configure pins for input
	PCIFR = 0xff; // clear interrupt flags in case they were set for any reason. writing to 1s clears them
	
	oldBitA = 0;
	oldBitB = 0;
	gEncoderCount = 0;
	gEncoderError = 0;
}

ISR(PCINT3_vect)
{
	uint8_t x = ENCODER_PIN;

	uint8_t newBitA = (x & ENCODER_BIT_A) != 0;
	uint8_t newBitB = (x & ENCODER_BIT_B) != 0;

	uint8_t add1 = newBitA ^ oldBitB;
	uint8_t sub1 = newBitB ^ oldBitA;

	if(add1)
		++gEncoderCount;
		
	if(sub1)
		--gEncoderCount;

	if(newBitA != oldBitA && newBitB != oldBitB)
		++gEncoderError;

	oldBitA = newBitA;
	oldBitB = newBitB;
}

static void setMotorDirection(bool forward)
{
	if (forward)
		MOTOR_DIRECTION_PORT |= MOTOR_DIRECTION_BIT;
	else
		MOTOR_DIRECTION_PORT &= ~MOTOR_DIRECTION_BIT;
}

void MotorInit(Motor* motor)
{
	motor->m_targetAngle = 0;
	motor->m_torque = 0.0;
	motor->m_Kp = 0.005;
	motor->m_Kd = 0.05;
	
	initMotorHW();
	
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
	if (torque < -1.0)
		torque = -1.0;
	else if (torque > 1.0)
		torque = 1.0;
		
	if (motor->m_torque != torque)
	{
		bool forward = true;
		if (torque < 0.0)
		{
			forward = false;
			torque = -torque;
		}

		setup_PWM_timer2(0.0);
		delay_ms(1);		
		setMotorDirection(forward);
		setup_PWM_timer2(torque);

		motor->m_torque = torque;
	}
}

float MotorGetTorque(Motor* motor)
{
	return motor->m_torque;
}

void MotorMakeCurrentAngleZero(Motor* motor)
{
	motor->m_targetAngle = 0;
	gEncoderCount = 0;
}

#define ABS(X) ((X) < 0 ? -(X) : (X))
#define SGN(X) ((X) < 0 ? -1 : ((X) > 0 ? 1 : 0))
#define MAX_ERROR (1)

void MotorSetTargetAngle(Motor* motor, int16_t degrees)
{
	const int kDelayMS = 10;
	motor->m_targetAngle = degrees;
	float desiredPos = motor->m_targetAngle;
	float currentPos1 = MotorGetCurrentAngle(motor);
	float lastPrintedPos = currentPos1 + 1; // force them unequal
	float errorPos = 0.0;
	bool done = false;
	int nloops = 0;
	while (!done)
	{
		float currentPos0 = currentPos1;
		delay_ms(kDelayMS);
		currentPos1 = MotorGetCurrentAngle(motor);
		float deltaPos = currentPos1 - currentPos0;
		errorPos = desiredPos - currentPos1;
		float velocity = deltaPos / kDelayMS;
		float t = motor->m_Kp * errorPos - motor->m_Kd * velocity;
		
		if (lastPrintedPos != currentPos1)
		{
			lastPrintedPos = currentPos1;
			char kps[16];
			char prs[16];
			char pms[16];
			char kds[16];
			char vms[16];
			char ts[16];
			char errs[16];
			char kpvs[16];
			char kdvs[16];
			s_ftosb(kps, motor->m_Kp, 6);
			s_ftosb(kds, motor->m_Kd, 6);
			s_ftosb(prs, desiredPos, 1);
			s_ftosb(pms, currentPos1, 1);
			s_ftosb(vms, velocity, 6);
			s_ftosb(errs, errorPos, 1);
			s_ftosb(kpvs, motor->m_Kp * errorPos, 6);
			s_ftosb(kdvs, motor->m_Kd * velocity, 6);
			s_ftosb(ts, t, 3);
			s_println("kp*(pr-pm)-kd*vm=%s*(%s-%s)-%s*%s=%s*%s-%s=%s-%s=%s",
				kps, prs, pms, kds, vms, kps, errs, kdvs, kpvs, kdvs, ts);
		}
		
		MotorSetTorque(motor, t);
		if (t < 0.0)
			t = -t;
		if (t < MIN_TORQUE)
			done = true;
	}
	
	float oldErrorPos = 1000;
	do
	{
		delay_ms(kDelayMS);
		errorPos = desiredPos - MotorGetCurrentAngle(motor);
		float t = MIN_TORQUE * SGN(errorPos);
		if (oldErrorPos != errorPos)
		{
			oldErrorPos = errorPos;
			s_printf("error=%s", s_ftos(errorPos, 5));
			s_println(", t=%s", s_ftos(t, 3));
		}
		MotorSetTorque(motor, t);
	}
	while (ABS(errorPos) > MAX_ERROR);
	
	MotorSetTorque(motor, 0.0);
}

int16_t MotorGetTargetAngle(Motor* motor)
{
	return motor->m_targetAngle;
}

int16_t MotorGetCurrentAngle(Motor* motor)
{
	return 360 * gEncoderCount / ENCODER_TICKS_PER_REVOLUTION;
}

int16_t MotorGetDeltaAngle(Motor* motor)
{
	return MotorGetTargetAngle(motor) - MotorGetCurrentAngle(motor);
}
