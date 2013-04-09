#ifndef _kmotor_h_
#define _kmotor_h_

#include "ktypes.h"

// Connect the wheel encoder input to pins D2 and D3
//

typedef struct Motor_ Motor;
struct Motor_
{
	int16_t m_targetAngle;
	float m_torque; // [0..1]
	float m_Kp;
	float m_Kd;
};
void MotorInit(Motor* motor);

void MotorSetKp(Motor* motor, float Kp);
float MotorGetKp(Motor* motor);

void MotorSetKd(Motor* motor, float Kd);
float MotorGetKd(Motor* motor);

void MotorSetTorque(Motor* motor, float torque); // [-1,1]
float MotorGetTorque(Motor* motor);

void MotorMakeCurrentAngleZero(Motor* motor);
void MotorSetTargetAngle(Motor* motor, int16_t degrees); // can be multiple rotations
int16_t MotorGetTargetAngle(Motor* motor);
int16_t MotorGetCurrentAngle(Motor* motor);

#endif // #ifndef _kmotor_h_
