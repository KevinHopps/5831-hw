#ifndef _kio_h_
#define _kio_h_

#include <pololu/orangutan.h>

typedef struct IOStruct KIORegs;

KIORegs getIORegs(int pin); // e.g. IO_C1
void setDataDir(const KIORegs* ioPin, int dir); // INPUT or OUTPUT
void setIOValue(const KIORegs* ioPin, int value); // LOW, HIGH, or TOGGLE
int getIOValue(const KIORegs* ioPin); // LOW or HIGH

#endif // #ifndef _kio_h_
