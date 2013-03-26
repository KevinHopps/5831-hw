#include "kdebug.h"
#include "kio.h"

KIORegs getIORegs(int pin)
{
	KIORegs result;
	get_io_registers(&result, pin);
	KASSERT(result.pinRegister != 0 && result.portRegister != 0 && result.ddrRegister != 0);
	return result;
}

void setDataDir(const KIORegs* ioPin, int dir)
{
	KASSERT(ioPin != 0 && (dir == OUTPUT || dir == INPUT));
	KIORegs regs = *ioPin;
	set_data_direction(&regs, dir);
}

void setIOValue(const KIORegs* ioPin, int value)
{
	KASSERT(ioPin != 0 && (value == HIGH || value == LOW || value == TOGGLE));
	KIORegs regs = *ioPin;
	set_digital_output_value(&regs, value);
}

int getIOValue(const KIORegs* ioPin)
{
	KASSERT(ioPin != 0);
	KIORegs regs = *ioPin;
	int result = get_digital_input_value(&regs) != 0;
	return result;
}
