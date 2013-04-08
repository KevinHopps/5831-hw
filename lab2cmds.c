#include <stdlib.h>
#include "kmotor.h"
#include "kserial.h"
#include "lab2cmds.h"

int help_cmd(int argc, char** argv);
// help [command]
// commands:
//		kp [new_value] {set/get current value of Kp}
//		kd [new_value] {set/get current value of Kd}
//		print
//			kp
//			kd
//			torque
//			target-angle
//			current-angle
//		torque [new_value]
//		zero {set current angle to zero}
//		rotate degrees {rotate the motor}
//
int kp_cmd(int argc, char** argv);
int kd_cmd(int argc, char** argv);
int print_cmd(int argc, char** argv);
int zero_cmd(int argc, char** argv);
int torque_cmd(int argc, char** argv);
int rotate_cmd(int argc, char** argv);
int sei_cmd(int argc, char** argv);
int cli_cmd(int argc, char** argv);

static Motor theMotor;

void InitCommands(CommandIO* ciop)
{
	CIORegisterCommand(ciop, "help", help_cmd);
	CIORegisterCommand(ciop, "torque", torque_cmd);
	/*
	CIORegisterCommand(ciop, "kp", kp_cmd);
	CIORegisterCommand(ciop, "kd", kd_cmd);
	CIORegisterCommand(ciop, "print", print_cmd);
	CIORegisterCommand(ciop, "zero", zero_cmd);
	CIORegisterCommand(ciop, "rotate", rotate_cmd);
	CIORegisterCommand(ciop, "sei", sei_cmd);
	CIORegisterCommand(ciop, "cli", cli_cmd);
	*/
	
	MotorInit(&theMotor);
}

int help_cmd(int argc, char** argv)
{
	static const char* help[] =
	{
		"help {produce this output}",
		"kp [new_value] {set/get Kp}",
		"kd [new_value] {set/get Kd}",
		"print {display useful info}",
		"torque [new_value] {directly set motor speed}",
		"zero {set current angle to zero}",
		"rotate degrees {rotate motor by certain amount}",
		"sei {enable interrupts}",
		"cli {disable interrupts}",
		0 // sentinel
	};
	
	const char** cpp;
	for (cpp = help; *cpp; ++cpp)
		s_println("%s", *cpp);
		
	return 0;
}

int torque_cmd(int argc, char** argv)
{
	float torque = atof(argv[1]);
	s_println("Set torque=%s", s_ftos(torque));
	MotorSetTorque(&theMotor, torque);
	
	return 0;
}
