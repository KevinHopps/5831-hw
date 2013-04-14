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
// At half speed (setting torque=0.5), the revolutions per second was
// 70/66.02 and 70/64.69, or an average of 1.07 RPS.
// With torque=0.25, we get 30/57.94, 30/57.66, or 0.52 RPS
// With torque=1.0, we get 50/26.78, 50/26.61, 50/26.80 or 1.87 RPS
// Torque RPS    Ratio to Full Speed
// ------ -----  -------------------
//  1.00   1.87   1.00
//  0.50   1.07   0.57
//  0.25   0.52   0.28
// -1.00  -1.78  -0.95
// -0.50  -1.09  -0.58
// -0.25  -0.54  -0.29

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
	motor->m_torque = 0.0;
	
	initMotorHW();
	
	MotorSetTorque(motor, 0.0);
}

void MotorSetTorque(Motor* motor, float torque)
{
	s_println("MotorSetTorque %s", s_ftos(torque, 2));
	
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

int16_t MotorGetCurrentAngle(Motor* motor)
{
	cli();
	int32_t count = gEncoderCount; // This takes two instructions
	sei();
	
	int16_t result = 360 * count / ENCODER_TICKS_PER_REVOLUTION;
	
	return result;
}
