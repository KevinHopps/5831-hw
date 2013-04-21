#ifndef _lab2cmds_h_
#define _lab2cmds_h_

#include "kcmd.h"
#include "kmotor.h"
#include "Trajectory.h"

// This Context structure is used by any commands. It is maintained
// in the main loop and passed to any commands executed.
//
typedef struct Context_ Context;
struct Context_
{
	bool m_logging;
	uint16_t m_counter;
	Trajectory* m_tp;
	PDControl* m_pdc;
	Motor* m_motor;
};
void ContextInit(Context* ctx, Trajectory* tp, PDControl* pdc, Motor* mp);
bool ContextGetTasksRunning(const Context* ctx);
bool ContextSetTasksRunning(Context* ctx, bool shouldRun);
void ContextReset(Context* ctx);
uint16_t ContextGetPeriod(const Context* ctx);
uint16_t ContextSetPeriod(Context* ctx, uint16_t periodMSec);
MotorAngle ContextSetTargetAngle(Context* ctx, MotorAngle target);
MotorAngle ContextGetTargetAngle(const Context* ctx);
MotorTorque ContextSetTorque(Context* ctx, MotorTorque torque);
MotorTorque ContextGetTorque(const Context* ctx);
uint8_t ContextSetMaxAccel(Context* ctx, uint8_t maxAccel);
uint8_t ContextGetMaxAccel(const Context* ctx);
float ContextSetKd(Context* ctx, float kd);
float ContextGetKd(const Context* ctx);
float ContextSetKp(Context* ctx, float kp);
float ContextGetKp(const Context* ctx);
bool ContextSetLogging(Context* ctx, bool enabled);
bool ContextGetLogging(const Context* ctx);
void showInfo(Context* ctx);
void showHelp();

void InitCommands(CommandIO* cio);

#endif // #ifndef _lab2cmds_h_
