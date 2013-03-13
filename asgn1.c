#include <pololu/orangutan.h>
#include "kserial.h"

// By experiment of running the motor 100 rotations at
// speed -100 with a 50 ms delay each loop, the count
// reported was 6394.
//
// When the experiment was re-run without any delay,
// the count reported was exactly 6400. Therefore it
// appears that each revolution gives a count of 64.
//

#define PR1(X) s_println("%s=%d", #X, X)

const char beep_button_top[] PROGMEM = "!c32";

void timingExperiment()
{
	int i;
	set_motors(0, 0);

	// Initialize the encoders and specify the four input pins.
	encoders_init(IO_D2, IO_D3, IO_B4, IO_B5);

	encoders_get_counts_and_reset_m1();
	encoders_get_counts_and_reset_m2();

	for (i = 3; i > 0; --i)
	{
		delay_ms(1000);
		s_println("%d", i);
		play_from_program_space(beep_button_top);
	}
	set_motors(-100, -100);

	while (1)
	{
		s_println("motors: %d %d", encoders_get_counts_m1(), encoders_get_counts_m2());
		if (encoders_check_error_m1())
			s_println("error 1");
		if (encoders_check_error_m2())
			s_println("error 2");
	}
}

static const int kCountPerRevolution = 64;

int main()
{
	set_motors(0, 0);

	encoders_init(IO_D2, IO_D3, IO_B4, IO_B5);
	encoders_get_counts_and_reset_m1();
	encoders_get_counts_and_reset_m2();

	int speed = 100;
	int numRevolutions = 2;
	int totalCount = kCountPerRevolution * numRevolutions;

	while (!button_is_pressed(MIDDLE_BUTTON))
	{
		speed = -speed;
		s_println("speed %d", speed);
		set_motors(speed, speed);
		while (encoders_get_counts_m1() < totalCount)
		{
			if (encoders_check_error_m1())
				s_println("error on motor 1");
		}

		set_motors(0, 0);
		delay_ms(500);

		speed = -speed;
		s_println("speed %d", speed);
		set_motors(speed, speed);
		while (encoders_get_counts_m1() > 0)
		{
			if (encoders_check_error_m1())
				s_println("error on motor 1");
		}

		set_motors(0, 0);
		delay_ms(500);
	}

	while (1)
		set_motors(0, 0);
}
