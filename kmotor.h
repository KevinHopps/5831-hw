#ifndef _kmotor_h_
#define _kmotor_h_

#include "ktypes.h"

// Connect the wheel encoder input to pins D2 and D3
//


// MinTorque is the minimum (absolute value) that actually
// produces movement.
//
#define MOTOR_MIN_TORQUE (0.04)

typedef struct Motor_ Motor;
struct Motor_
{
	float m_torque; // [0..1]
};
void MotorInit(Motor* motor);

void MotorSetTorque(Motor* motor, float torque); // [-1,1]
float MotorGetTorque(Motor* motor);

int16_t MotorGetCurrentAngle(Motor* motor);

int16_t MotorResetCurrentAngle(Motor* motor); // redefine current angle as zero

#endif // #ifndef _kmotor_h_
