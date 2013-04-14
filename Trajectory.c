#include "Trajectory.h"

void TrajectoryInit(Trajectory* tp, PDControl* pdc)
{
	tp->m_enabled = false;
	tp->m_targetAngle = 0;
	tp->m_pdc = pdc;
}

void TrajectoryRotate(Trajectory* tp, int16_t angle)
{
	PDControlRotate(tp->m_pdc, angle);
}

int16_t TrajectoryGetCurrentAngle(Trajectory* tp)
{
	return PDControlGetCurrentAngle(tp->m_pdc);
}

void taskTrajectory(void* arg)
{
	Trajectory* tp = (Trajectory*)arg;
	
	if (!tp->m_enabled)
		return;
	
	int16_t currentAngle = TrajectoryGetCurrentAngle(tp);
	int16_t errorAngle = tp->m_targetAngle - currentAngle;
	if (errorAngle < -PDCONTROL_MAX_ROTATION)
		PDControlRotate(tp->m_pdc, -PDCONTROL_MAX_ROTATION);
	else if (errorAngle > PDCONTROL_MAX_ROTATION)
		PDControlRotate(tp->m_pdc, PDCONTROL_MAX_ROTATION);
	else
		PDControlRotate(tp->m_pdc, errorAngle);
}

void TrajectorySetEnabled(Trajectory* tp, bool enabled)
{
	if (tp->m_enabled != enabled)
	{
		tp->m_enabled = enabled;
	}
}

bool TrajectoryIsEnabled(Trajectory* tp)
{
	return tp->m_enabled;
}
