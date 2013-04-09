#include <pololu/orangutan.h>

#include "kserial.h"

int main()
{
	while (1)
		set_motors(0, 0);
}
