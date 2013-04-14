#include <stdlib.h>
#include "kmotor.h"
#include "kserial.h"
#include "lab2cmds.h"

void ContextInit(Context* ctx, Trajectory* tp, PDControl* pdc, Motor* mp)
{
	ctx->m_logging = false;
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
int torque_cmd(int argc, char** argv, void* context);
int rotate_cmd(int argc, char** argv, void* context);

void InitCommands(CommandIO* ciop)
{
	CIORegisterCommand(ciop, "help", help_cmd);
	CIORegisterCommand(ciop, "?", help_cmd);
	CIORegisterCommand(ciop, "L", L_cmd);
	CIORegisterCommand(ciop, "l", L_cmd);
	CIORegisterCommand(ciop, "P", P_cmd);
	CIORegisterCommand(ciop, "p", P_cmd);
	CIORegisterCommand(ciop, "D", D_cmd);
	CIORegisterCommand(ciop, "d", D_cmd);
	CIORegisterCommand(ciop, "kp", kp_cmd);
	CIORegisterCommand(ciop, "kd", kd_cmd);
	CIORegisterCommand(ciop, "info", info_cmd);
	CIORegisterCommand(ciop, "torque", torque_cmd);
	CIORegisterCommand(ciop, "rotate", rotate_cmd);
}

static void showHint(bool* shown)
{
	if (!*shown)
	{
		*shown = true;
		s_println("type '?' for help");
	}
}

static void showHelp()
{
	static const char* help[] =
	{
		"help or '?' {displays the following}",
		"commands:",
		"    L {start/stop logging Pr, Pm, and T}",
		"    l {same as L}",
		"    P {increase Kp by an amount of your choice (0.001)}",
		"    p {decrease Kp by an amount of your choice (0.001)}",
		"    D {increase Kd by an amount of your choice (0.01)}",
		"    d {decrease Kd by an amount of your choice (0.01)}",
		"    kp [new_value] {set/get current value of Kp}",
		"    kd [new_value] {set/get current value of Kd}",
		"    info {displays the following info}",
		"        kp",
		"        kd",
		"        torque",
		"        target-angle",
		"        current-angle",
		"    torque [new_value]",
		"    rotate degrees {rotate the motor}",
		"    go",
		"    stop",
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

int L_cmd(int argc, char** argv, void* context)
{
	static bool hintShown = false;
	showHint(&hintShown);
	
	Context* ctx = (Context*)context;
	ctx->m_logging = !ctx->m_logging;

	return 0;
}

static float incrKp = 0.001;

int P_cmd(int argc, char** argv, void* context)
{
	static bool hintShown = false;
	showHint(&hintShown);
	
	Context* ctx = (Context*)context;
	float delta = incrKp;
	if (**argv == 'p')
		delta = -delta;
	PDControlSetKp(ctx->m_pdc, PDControlGetKp(ctx->m_pdc) + delta);
	
	return 0;
}

static float incrKd = 0.01;

int D_cmd(int argc, char** argv, void* context)
{
	static bool hintShown = false;
	showHint(&hintShown);
	
	Context* ctx = (Context*)context;
	float delta = incrKp;
	if (**argv == 'd')
		delta = -delta;
	PDControlSetKd(ctx->m_pdc, PDControlGetKd(ctx->m_pdc) + delta);
	
	return 0;
}

int kp_cmd(int argc, char** argv, void* context)
{
	if (argc != 2)
		showHelp();
		
	Context* ctx = (Context*)context;
	PDControlSetKp(ctx->m_pdc, atof(argv[1]));
	
	return 0;
}

int kd_cmd(int argc, char** argv, void* context)
{
	if (argc != 2)
		showHelp();
		
	Context* ctx = (Context*)context;
	PDControlSetKd(ctx->m_pdc, atof(argv[1]));
	
	return 0;
}

int info_cmd(int argc, char** argv, void* context)
{
	Context* ctx = (Context*)context;
	s_println("torque=%s", s_ftos(MotorGetTorque(ctx->m_motor), 3));
	s_println("angle=%d", MotorGetCurrentAngle(ctx->m_motor));
	s_println("Kp=%s", s_ftos(PDControlGetKp(ctx->m_pdc), 5));
	s_println("Kd=%s", s_ftos(PDControlGetKd(ctx->m_pdc), 5));
	
	return 0;
}

int torque_cmd(int argc, char** argv, void* context)
{
	if (argc != 2)
		showHelp();
		
	float torque = atof(argv[1]);
	Context* ctx = (Context*)context;
	MotorSetTorque(ctx->m_motor, torque);
	
	return 0;
}

int rotate_cmd(int argc, char** argv, void* context)
{
	if (argc != 2)
		showHelp();
		
	int angle = atoi(argv[1]);
	Context* ctx = (Context*)context;
	TrajectoryRotate(ctx->m_tp, angle);
	
	return 0;
}

int go_cmd(int argc, char** argv, void* context)
{
	Context* ctx = (Context*)context;

	PDControlSetEnabled(ctx->m_pdc, true);
	TrajectorySetEnabled(ctx->m_tp, true);

	return 0;
}

int stop_cmd(int argc, char** argv, void* context)
{
	Context* ctx = (Context*)context;

	PDControlSetEnabled(ctx->m_pdc, false);
	TrajectorySetEnabled(ctx->m_tp, false);

	return 0;
}
