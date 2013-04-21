#ifndef _Trajectory_h_
#define _Trajectory_h_

#include "ktypes.h"
#include "PDControl.h"

typedef struct Trajectory_ Trajectory;
struct Trajectory_
{
	bool m_enabled;
	MotorAngle m_targetAngle;
	PDControl* m_pdc;
};

// This initializes the Trajectory structure and its task. The
// msecPeriod specifies the period of the task, in milliseconds.
//
void TrajectoryInit(Trajectory* tp, PDControl* pdc, uint16_t msecPeriod);

// This sets/gets the target angle for the motor. It may be called at
// any time and the running task will adjust its behavior as
// necessary.
//
void TrajectorySetTargetAngle(Trajectory* tp, MotorAngle angle);
MotorAngle TrajectoryGetTargetAngle(const Trajectory* tp);

// This returns the current angle of the motor.
//
MotorAngle TrajectoryGetCurrentAngle(const Trajectory* tp);

// This enables or disables the Trajectory Interpolator task.
// The interrupt handler will continue to fire, but this
// determines whether the handler will actually do anything.
//
void TrajectorySetEnabled(Trajectory* tp, bool enabled);
bool TrajectoryGetEnabled(const Trajectory* tp);

// This is the Trajectory Interpolator task, which is called
// from the interrupt handler. The arg parameter is a void*
// to make this functions match the required signature of a
// timer callback task. See ktimers.h. But arg actually points
// to a Trajectory struct.
//
void TrajectoryTask(void* arg);

#endif // #ifndef _Trajectory_h_
