#include <pololu/orangutan.h>

#include "kserial.h"

volatile float f = 1.0/3.0;

int main()
{
	f = -10.0 / 3.0;
	while (f < 5.0)
	{
		s_println("f=%s, (int)f=%d", s_ftos(f), (int)f);
		f += 1.0;
	}
	while (1)
		set_motors(0, 0);
}
