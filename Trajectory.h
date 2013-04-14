#ifndef _Trajectory_h_
#define _Trajectory_h_

#include "ktypes.h"
#include "PDControl.h"

typedef struct Trajectory_ Trajectory;
struct Trajectory_
{
	bool m_enabled;
	int16_t m_targetAngle;
	PDControl* m_pdc;
};
void TrajectoryInit(Trajectory* tp, PDControl* pdc);
void TrajectoryRotate(Trajectory* tp, int16_t angle);
int16_t TrajectoryGetCurrentAngle(Trajectory* tp);
void TrajectorySetEnabled(Trajectory* tp, bool enabled);
bool TrajectoryIsEnabled(Trajectory* tp);

void taskTrajectory(void* arg);

#endif // #ifndef _Trajectory_h_
