#include <stdlib.h>
#include "kmotor.h"
#include "kserial.h"
#include "lab2cmds.h"

void ContextInit(Context* ctx, Trajectory* tp, PDControl* pdc, Motor* mp)
{
	ctx->m_logging = false;
	ctx->m_runningProgram = false;
	ctx->m_counter = 0;
	ctx->m_tp = tp;
	ctx->m_pdc = pdc;
	ctx->m_motor = mp;
}

// help
//     -or-
// ? {displays the following}
// commands:
//      L: start/stop logging Pr, Pm, and T
//      l: same as L
//      P: increase Kp by an amount of your choice (0.001)
//      p: decrease Kp by an amount of your choice (0.001)
//      D: increase Kd by an amount of your choice (0.01)
//      d: decrease Kd by an amount of your choice (0.01)
//		kp [new_value] {set/get current value of Kp}
//		kd [new_value] {set/get current value of Kd}
//		info {displays the following info}
//			kp
//			kd
//			torque
//			target-angle
//			current-angle
//		torque [new_value]
//		rotate degrees {rotate the motor}
//
int help_cmd(int argc, char** argv, void* context);
int L_cmd(int argc, char** argv, void* context);
int P_cmd(int argc, char** argv, void* context);
int D_cmd(int argc, char** argv, void* context);
int kp_cmd(int argc, char** argv, void* context);
int kd_cmd(int argc, char** argv, void* context);
int info_cmd(int argc, char** argv, void* context);
int acceleration_cmd(int argc, char** argv, void* context);
int torque_cmd(int argc, char** argv, void* context);
int rotate_cmd(int argc, char** argv, void* context);
int period_cmd(int argc, char** argv, void* context);
int zero_cmd(int argc, char** argv, void* context);
int go_cmd(int argc, char** argv, void* context);
int stop_cmd(int argc, char** argv, void* context);
int xprog_cmd(int argc, char** argv, void* context);

void InitCommands(CommandIO* ciop)
{
	CIORegisterCommand(ciop, "help", help_cmd);
	CIORegisterCommand(ciop, "?", help_cmd);
	CIORegisterCommand(ciop, "L", L_cmd);
	CIORegisterCommand(ciop, "l", L_cmd);
	CIORegisterCommand(ciop, "kp", kp_cmd);
	CIORegisterCommand(ciop, "kd", kd_cmd);
	CIORegisterCommand(ciop, "info", info_cmd);
	CIORegisterCommand(ciop, "acceleration", acceleration_cmd);
	CIORegisterCommand(ciop, "torque", torque_cmd);
	CIORegisterCommand(ciop, "rotate", rotate_cmd);
	CIORegisterCommand(ciop, "period", period_cmd);
	CIORegisterCommand(ciop, "zero", zero_cmd);
	CIORegisterCommand(ciop, "go", go_cmd);
	CIORegisterCommand(ciop, "stop", stop_cmd);
	CIORegisterCommand(ciop, "xprog", xprog_cmd);
}

static void showHint(bool* shown)
{
	if (!*shown)
	{
		*shown = true;
		s_println("type '?' for help");
	}
}

void showHelp()
{
	static const char* help[] =
	{
		"help or '?' {displays the following}",
		"commands:",
		"    L {start/stop logging Pr, Pm, and T}",
		"    l {same as L}",
		"    kp new_value {set/get current value of Kp}",
		"    kd new_value {set/get current value of Kd}",
		"    info {displays the following info}",
		"        kp",
		"        kd",
		"        torque",
		"        target-angle",
		"        current-angle",
		"        max-acceleration",
		"        PDControl-period",
		"    acceleration factor {max acceleration will be MAX_TORQUE/factor}",
		"    torque new_value",
		"    rotate degrees {rotate the motor}",
		"    period {set msec period of PDControl task}",
		"    zero {stop motor, handlers, set angle to 0}",
		"    go",
		"    stop",
		"    xprog {run the canned program}",
		0 // sentinel
	};
	
	const char** cpp;
	for (cpp = help; *cpp; ++cpp)
		s_println("%s", *cpp);
}

int help_cmd(int argc, char** argv, void* context)
{
	showHelp();
	return 0;
}

void showInfo(Context* ctx)
{
	s_println("torque %d", MotorGetTorque(ctx->m_motor));
	s_println("angle %ld", (long)MotorGetCurrentAngle(ctx->m_motor));
	s_println("kp %s", s_ftos(PDControlGetKp(ctx->m_pdc), 5));
	s_println("kd %s", s_ftos(PDControlGetKd(ctx->m_pdc), 5));
	s_println("acceleration %d", PDControlGetMaxAccel(ctx->m_pdc));
	s_println("period %d", PDControlGetPeriod(ctx->m_pdc));
}

bool ContextGetLogging(const Context* ctx)
{
	return ctx->m_logging;
}

bool ContextSetLogging(Context* ctx, bool enabled)
{
	bool result = ctx->m_logging;
	ctx->m_logging = enabled;
	return result;
}

int L_cmd(int argc, char** argv, void* context)
{
	static bool hintShown = false;
	showHint(&hintShown);
	
	Context* ctx = (Context*)context;
	ContextSetLogging(ctx, !ContextGetLogging(ctx));
	s_println("Logging is %s", ctx->m_logging ? "on" : "off");

	return 0;
}

float ContextGetKp(const Context* ctx)
{
	return PDControlGetKp(ctx->m_pdc);
}

float ContextSetKp(Context* ctx, float kp)
{
	float result = PDControlGetKp(ctx->m_pdc);
	PDControlSetKp(ctx->m_pdc, kp);
	return result;
}

int kp_cmd(int argc, char** argv, void* context)
{
	if (argc != 2)
		showHelp();
		
	Context* ctx = (Context*)context;
	ContextSetKp(ctx, atof(argv[1]));
	
	showInfo(ctx);
	
	return 0;
}

float ContextGetKd(const Context* ctx)
{
	return PDControlGetKd(ctx->m_pdc);
}

float ContextSetKd(Context* ctx, float kd)
{
	float result = PDControlGetKd(ctx->m_pdc);
	PDControlSetKd(ctx->m_pdc, kd);
	return result;
}

int kd_cmd(int argc, char** argv, void* context)
{
	if (argc != 2)
		showHelp();
		
	Context* ctx = (Context*)context;
	ContextSetKd(ctx, atof(argv[1]));
	
	showInfo(ctx);
	
	return 0;
}

int info_cmd(int argc, char** argv, void* context)
{
	Context* ctx = (Context*)context;

	showInfo(ctx);
	
	return 0;
}

uint8_t ContextGetMaxAccel(const Context* ctx)
{
	return PDControlGetMaxAccel(ctx->m_pdc);
}

uint8_t ContextSetMaxAccel(Context* ctx, uint8_t maxAccel)
{
	uint8_t result = PDControlGetMaxAccel(ctx->m_pdc);
	PDControlSetMaxAccel(ctx->m_pdc, maxAccel);
	return result;
}

int acceleration_cmd(int argc, char** argv, void* context)
{
	if (argc != 2)
		showHelp();
		
	int16_t factor = atoi(argv[1]);
	Context* ctx = (Context*)context;
	
	ContextSetMaxAccel(ctx, factor);
	
	return 0;
}

MotorTorque ContextGetTorque(const Context* ctx)
{
	return MotorGetTorque(ctx->m_motor);
}

MotorTorque ContextSetTorque(Context* ctx, MotorTorque torque)
{
	MotorTorque result = MotorGetTorque(ctx->m_motor);
	MotorSetTorque(ctx->m_motor, torque);
	return result;
}

int torque_cmd(int argc, char** argv, void* context)
{
	if (argc != 2)
		showHelp();
		
	MotorTorque torque = atoi(argv[1]);
	Context* ctx = (Context*)context;
	
	ContextSetTorque(ctx, torque);
	
	showInfo(ctx);
	
	return 0;
}

MotorAngle ContextGetTargetAngle(const Context* ctx)
{
	return TrajectoryGetTargetAngle(ctx->m_tp);
}

MotorAngle ContextSetTargetAngle(Context* ctx, MotorAngle target)
{
	MotorAngle result = TrajectoryGetTargetAngle(ctx->m_tp);
	TrajectorySetTargetAngle(ctx->m_tp, target);
	ctx->m_counter = 0;
	return result;
}

MotorAngle ContextGetCurrentAngle(const Context* ctx)
{
	return MotorGetCurrentAngle(ctx->m_motor);
}

int rotate_cmd(int argc, char** argv, void* context)
{
	if (argc != 2)
		showHelp();
		
	MotorAngle rotation = atoi(argv[1]);
	Context* ctx = (Context*)context;
	
	MotorAngle target = TrajectoryGetCurrentAngle(ctx->m_tp) + rotation;
	ContextSetTargetAngle(ctx, target);
	
	return 0;
}

uint16_t ContextGetPeriod(const Context* ctx)
{
	return PDControlGetPeriod(ctx->m_pdc);
}

uint16_t ContextSetPeriod(Context* ctx, uint16_t periodMSec)
{
	uint16_t result = PDControlGetPeriod(ctx->m_pdc);
	PDControlSetPeriod(ctx->m_pdc, periodMSec);
	return result;
}

int period_cmd(int argc, char** argv, void* context)
{
	if (argc != 2)
		showHelp();
		
	int period = atoi(argv[1]);
	Context* ctx = (Context*)context;
	
	int result = 0;
	if (period < 1 || period > 1000)
	{
		s_println("must be in range [1,1000]");
		result = 1;
	}
	else
		ContextSetPeriod(ctx, period);
	
	return result;
}

void ContextReset(Context* ctx)
{
	TrajectorySetTargetAngle(ctx->m_tp, 0);
	PDControlSetTargetAngle(ctx->m_pdc, 0);
	MotorSetTorque(ctx->m_motor, 0);
	MotorResetCurrentAngle(ctx->m_motor);
}

int zero_cmd(int argc, char** argv, void* context)
{
	if (argc != 1)
		showHelp();
		
	Context* ctx = (Context*)context;
	
	ContextReset(ctx);
	
	showInfo(ctx);
	
	return 0;
}

bool ContextGetTasksRunning(const Context* ctx)
{
	bool result = TrajectoryGetEnabled(ctx->m_tp);
	PDControlSetEnabled(ctx->m_pdc, result);
	return result;
}

bool ContextSetTasksRunning(Context* ctx, bool shouldRun)
{
	bool result = ContextGetTasksRunning(ctx);
	PDControlSetEnabled(ctx->m_pdc, shouldRun);
	TrajectorySetEnabled(ctx->m_tp, shouldRun);
	return result;
}

int go_cmd(int argc, char** argv, void* context)
{
	Context* ctx = (Context*)context;

	ContextSetTasksRunning(ctx, true);
	
	return 0;
}

int stop_cmd(int argc, char** argv, void* context)
{
	Context* ctx = (Context*)context;

	ContextSetTasksRunning(ctx, false);

	return 0;
}

int xprog_cmd(int argc, char** argv, void* context)
{
	Context* ctx = (Context*)context;

	ctx->m_runningProgram = true;

	return 0;
}
