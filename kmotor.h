#ifndef _kmotor_h_
#define _kmotor_h_

#include "ktypes.h"

typedef int32_t MotorAngle;

// Connect the wheel encoder input to pins D2 and D3
//

// The motor torque is expressed as being in the range
// of [-1000,1000].

// MOTOR_MIN_TORQUE is the minimum (absolute value) that actually
// produces movement. MOTOR_MAX_TORQUE is the largest magnitude
// allowed.
//
#define MOTOR_MIN_TORQUE 45 // on a scale of 1000
#define MOTOR_MAX_TORQUE 1000

typedef struct Motor_ Motor;
struct Motor_
{
	int16_t m_torque;
};
void MotorInit(Motor* motor);

void MotorSetTorque(Motor* motor, int16_t torque); // [-1000,1000]
int16_t MotorGetTorque(Motor* motor);

MotorAngle MotorGetCurrentAngle(Motor* motor);
void MotorResetCurrentAngle(Motor* motor); // redefine current angle as zero

#endif // #ifndef _kmotor_h_
