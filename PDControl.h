#ifndef _pdCtrl_h_
#define _pdCtrl_h_

#include "kmotor.h"
#include "ktimers.h"
#include "ktypes.h"

#define PDCONTROL_MAX_ROTATION 180

// The TorqueCalc structure is filled in by the PDControl task when it
// calculates a new torque. This is remembered so that the logging task
// in the main loop may retrieve it if desired.
//
typedef struct TorqueCalc_ TorqueCalc;
struct TorqueCalc_
{
	MotorAngle m_Pr;
	MotorAngle m_Pm;
	float m_Kp;
	float m_Kd;
	float m_velocity;
	int32_t m_torqueCalculated;
	int16_t m_torqueUsed;
	bool m_torqueChangeTooHigh;
	bool m_torqueMagnitudeTooHigh;
	bool m_torqueMagnitudeTooLow;
};
void TorqueCalcInit(TorqueCalc* tcp);
bool EqualTorqueCalc(const TorqueCalc* tcp1, const TorqueCalc* tcp2);

// This is all of the data needed by the PDControl task.
//
typedef struct PDControl_ PDControl;
struct PDControl_
{
	bool m_enabled; // task is allowed to run
	bool m_targetAngleSet; // user has set a target angle
	bool m_ready; // set on 2nd iter, so velocity can be calculated
	bool m_idle; // true when the current angle == target
	uint8_t m_maxAccel; // max delta torque is MOTOR_MAX_TORQUE/m_maxAccel
	uint16_t m_period; // period of the PDControl task
	MotorAngle m_lastAngle; // angle during previous iteration
	MotorAngle m_targetAngle; // where we're trying to go
	float m_kp; // used in torque calculation
	float m_kd; // used in torque calculation
	uint32_t m_lastMSec; // time of last iteration
	Motor* m_motor;
	uint32_t m_calcIndex; // incremented each time m_calc is filled in
	TorqueCalc m_calc; // record of last torque calculation
};

// This initializes the PDControl structure and its task.
//
void PDControlInit(PDControl* pdc, Motor* motor);

// Adjust the period of the PDControl task
//
uint16_t PDControlGetPeriod(const PDControl* pdc);
void PDControlSetPeriod(PDControl* pdc, uint16_t periodMSec);

// Adjust the max acceleration factor. So that the max delta
// torque is MOTOR_MAX_TORQUE/maxAccel
//
uint8_t PDControlGetMaxAccel(const PDControl* pdc);
void PDControlSetMaxAccel(PDControl* pdc, uint8_t maxAccel);

// These get/set the Kp parameter, used in the formula
//     t = Kp*errorPosition + Kd*velocity
//
void PDControlSetKp(PDControl* pdc, float kp);
float PDControlGetKp(const PDControl* pdc);

// These get/set the Kd parameter, used in the formula
//     t = Kp*errorPosition + Kd*velocity
//
void PDControlSetKd(PDControl* pdc, float kd);
float PDControlGetKd(const PDControl* pdc);

// This sets the target angle for the PDControl task.
//
void PDControlSetTargetAngle(PDControl* pdc, MotorAngle angle);

// This gets the current motor position.
//
MotorAngle PDControlGetCurrentAngle(const PDControl* pdc);

// This resets the current motor position, treating
// the current location as zero.
//
void PDControlResetCurrentAngle(PDControl* pdc);

// This enables or disables the PDController task.
// The interrupt handler will continue to fire, but this
// determines whether the handler will actually do anything.
//
void PDControlSetEnabled(PDControl* pdc, bool enabled);
bool PDControlGetEnabled(const PDControl* pdc);

// This is the PDController task, which is called
// from the interrupt handler. The arg parameter is a void*
// to make this functions match the required signature of a
// timer callback task. See ktimers.h. But arg actually points
// to a Trajectory struct.
//
void PDControlTask(void* arg);

uint32_t PDControlGetTorqueCalc(const PDControl* pdc, TorqueCalc* tcp); // returns an update index

#endif // #ifndef _pdCtrl_h_
