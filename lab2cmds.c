#include <stdlib.h>
#include "kmotor.h"
#include "kserial.h"
#include "lab2cmds.h"

void ContextInit(Context* ctx)
{
	ctx->m_logging = false;
	MotorInit(&ctx->m_motor);
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
	MotorSetKp(&ctx->m_motor, MotorGetKp(&ctx->m_motor) + delta);
	
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
	MotorSetKd(&ctx->m_motor, MotorGetKd(&ctx->m_motor) + delta);
	
	return 0;
}

int kp_cmd(int argc, char** argv, void* context)
{
	if (argc != 2)
		showHelp();
		
	Context* ctx = (Context*)context;
	MotorSetKp(&ctx->m_motor, atof(argv[1]));
	
	return 0;
}

int kd_cmd(int argc, char** argv, void* context)
{
	if (argc != 2)
		showHelp();
		
	Context* ctx = (Context*)context;
	MotorSetKd(&ctx->m_motor, atof(argv[1]));
	
	return 0;
}

int info_cmd(int argc, char** argv, void* context)
{
	Context* ctx = (Context*)context;
	s_println("torque=%s", s_ftos(MotorGetTorque(&ctx->m_motor), 3));
	s_println("angle=%d", MotorGetCurrentAngle(&ctx->m_motor));
	s_println("Kp=%s", s_ftos(MotorGetKp(&ctx->m_motor), 5));
	s_println("Kd=%s", s_ftos(MotorGetKd(&ctx->m_motor), 5));
	
	return 0;
}

int torque_cmd(int argc, char** argv, void* context)
{
	if (argc != 2)
		showHelp();
		
	float torque = atof(argv[1]);
	Context* ctx = (Context*)context;
	MotorSetTorque(&ctx->m_motor, torque);
	
	return 0;
}

int rotate_cmd(int argc, char** argv, void* context)
{
	if (argc != 2)
		showHelp();
		
	int angle = atoi(argv[1]);
	Context* ctx = (Context*)context;
	MotorSetTargetAngle(&ctx->m_motor, angle);
	
	return 0;
}
