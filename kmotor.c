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
#define ENCODER_TICKS_PER_REVOLUTION (127.49) // by experiment, 12749/100

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
	motor->m_Kp = 0.0;
	motor->m_Kd = 0.0;
	
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
	motor->m_targetAngle = 0;
	gEncoderCount = 0;
}

void MotorSetTargetAngle(Motor* motor, int16_t degrees)
{
	motor->m_targetAngle = degrees;
	int16_t delta;
	while ((delta = motor->m_targetAngle - MotorGetCurrentAngle(motor)))
	{
		// T = Kp(Pr - Pm) - Kd*Vm
		if (delta < 0)
			MotorSetTorque(motor, -0.1);
		else
			MotorSetTorque(motor, 0.1);
	}
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
