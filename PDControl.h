#ifndef _pdCtrl_h_
#define _pdCtrl_h_

#include "kmotor.h"
#include "ktimers.h"
#include "ktypes.h"

#define PDCONTROL_MAX_ROTATION 180

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

typedef struct PDControl_ PDControl;
struct PDControl_
{
	bool m_enabled;
	bool m_targetAngleSet;
	bool m_ready;
	MotorAngle m_lastAngle;
	MotorAngle m_targetAngle;
	float m_kp;
	float m_kd;
	uint32_t m_lastMSec;
	Motor* m_motor;
	uint32_t m_calcIndex;
	TorqueCalc m_calc;
};

// This initializes the PDControl structure and its task. The
// periodMSec specifies the period of this task.
//
void PDControlInit(PDControl* pdc, Motor* motor, uint16_t periodMSec);

// These get/set the Kp parameter, used in the formula
//     t = Kp*errorPosition + Kd*velocity
//
void PDControlSetKp(PDControl* pdc, float kp);
float PDControlGetKp(PDControl* pdc);

// These get/set the Kd parameter, used in the formula
//     t = Kp*errorPosition + Kd*velocity
//
void PDControlSetKd(PDControl* pdc, float kd);
float PDControlGetKd(PDControl* pdc);

// This sets the target angle for the PDControl task.
//
void PDControlSetTargetAngle(PDControl* pdc, MotorAngle angle);

// This gets the current motor position.
//
MotorAngle PDControlGetCurrentAngle(PDControl* pdc);

// This resets the current motor position, treating
// the current location as zero.
//
void PDControlResetCurrentAngle(PDControl* pdc);

// This enables or disables the PDController task.
// The interrupt handler will continue to fire, but this
// determines whether the handler will actually do anything.
//
void PDControlSetEnabled(PDControl* pdc, bool enabled);
bool PDControlIsEnabled(PDControl* pdc);

bool PDControlSetLogging(PDControl* pdc, bool enabled);
bool PDControlGetLogging(PDControl* pdc);
const char* PDControlGetLog(PDControl* pdc);

// This is the PDController task, which is called
// from the interrupt handler. The arg parameter is a void*
// to make this functions match the required signature of a
// timer callback task. See ktimers.h. But arg actually points
// to a Trajectory struct.
//
void PDControlTask(void* arg);

uint32_t PDControlGetTorqueCalc(PDControl* pdc, TorqueCalc* tcp); // returns an update index

#endif // #ifndef _pdCtrl_h_
