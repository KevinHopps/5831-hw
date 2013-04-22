#include "kdebug.h"
#include "kserial.h"
#include "kutils.h"
#include "Trajectory.h"

// Initialize the structure and setup timer 0. The TrajectoryTask
// function will be called by the interrupt handler for timer 0,
// and the Trajectory* will be passed to it.
//
void TrajectoryInit(Trajectory* tp, PDControl* pdc, uint16_t msecPeriod)
{
	tp->m_enabled = false;
	tp->m_targetAngle = 0;
	tp->m_pdc = pdc;

	setup_CTC_timer(0, msecPeriod*1000L, TrajectoryTask, tp);
}

// This adjusts the current target angle. The TrajectoryTask will
// notice the new target and adapt.
//
void TrajectorySetTargetAngle(Trajectory* tp, MotorAngle angle)
{
	tp->m_targetAngle = angle;
}

MotorAngle TrajectoryGetTargetAngle(const Trajectory* tp)
{
	return tp->m_targetAngle;
}

// This returns the current motor position.
//
MotorAngle TrajectoryGetCurrentAngle(const Trajectory* tp)
{
	return PDControlGetCurrentAngle(tp->m_pdc);
}

// This tells the TrajectoryTask whether it should do anything
// or merely return.
//
void TrajectorySetEnabled(Trajectory* tp, bool enabled)
{
	tp->m_enabled = enabled;
}

// This returns true if the Trajectory Interpolator task is enabled.
//
bool TrajectoryGetEnabled(const Trajectory* tp)
{
	return tp->m_enabled;
}

// This is called from the timer 0 interrupt vector. This *is* the
// Trajectory Interpolator task.
//
void TrajectoryTask(void* arg)
{
	Trajectory* tp = (Trajectory*)arg;
	
	if (!tp->m_enabled)
		return;
		
	MotorAngle currentAngle = PDControlGetCurrentAngle(tp->m_pdc);
	MotorAngle errorAngle = tp->m_targetAngle - currentAngle;
	
	if (errorAngle < -PDCONTROL_MAX_ROTATION)
		errorAngle = -PDCONTROL_MAX_ROTATION;
	else if (errorAngle > PDCONTROL_MAX_ROTATION)
		errorAngle = PDCONTROL_MAX_ROTATION;
		
	PDControlSetTargetAngle(tp->m_pdc, currentAngle + errorAngle);
}
