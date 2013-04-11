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

Motor theMotor;

void InitCommands(CommandIO* ciop)
{
	CIORegisterCommand(ciop, "help", help_cmd);
	CIORegisterCommand(ciop, "torque", torque_cmd);
	CIORegisterCommand(ciop, "zero", zero_cmd);
	CIORegisterCommand(ciop, "print", print_cmd);
	CIORegisterCommand(ciop, "rotate", rotate_cmd);
	CIORegisterCommand(ciop, "kp", kp_cmd);
	CIORegisterCommand(ciop, "kd", kd_cmd);
	CIORegisterCommand(ciop, "sei", sei_cmd);
	CIORegisterCommand(ciop, "cli", cli_cmd);
	
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
	s_println("Set torque=%s", s_ftos(torque, 3));
	MotorSetTorque(&theMotor, torque);
	
	return 0;
}

int zero_cmd(int argc, char** argv)
{
	s_println("zero");
	MotorMakeCurrentAngleZero(&theMotor);
	
	return 0;
}

int print_cmd(int argc, char** argv)
{
	s_println("torque=%s", s_ftos(MotorGetTorque(&theMotor), 3));
	s_println("angle=%d", MotorGetCurrentAngle(&theMotor));
	s_println("Kp=%s", s_ftos(MotorGetKp(&theMotor), 5));
	s_println("Kd=%s", s_ftos(MotorGetKd(&theMotor), 5));
	
	return 0;
}

int rotate_cmd(int argc, char** argv)
{
	int angle = atoi(argv[1]);
	MotorSetTargetAngle(&theMotor, angle);
	
	return 0;
}

int kp_cmd(int argc, char** argv)
{
	MotorSetKp(&theMotor, atof(argv[1]));
	return 0;
}

int kd_cmd(int argc, char** argv)
{
	MotorSetKd(&theMotor, atof(argv[1]));
	return 0;
}

int sei_cmd(int argc, char** argv)
{
	sei();
	return 0;
}

int cli_cmd(int argc, char** argv)
{
	cli();
	return 0;
}
