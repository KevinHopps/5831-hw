
void calcSpinCount()
{
	int state = 0;
	uint32_t loopCount = 423828;
	uint32_t incr = 2048;
	while (1)
	{
		red_led(++state & 1);
		spin(loopCount);

		int buttons = get_single_debounced_button_press(ANY_BUTTON);
		if (buttons & TOP_BUTTON)
		{
			loopCount -= incr;
			incr /= 2;
			s_println("faster: loopCount=%ld, incr=%ld", loopCount, incr);
		}
		else if (buttons & BOTTOM_BUTTON)
		{
			loopCount += incr;
			incr /= 2;
			s_println("slower: loopCount=%ld, incr=%ld", loopCount, incr);
		}
		else if (buttons & MIDDLE_BUTTON)
		{
			s_println("loopCount=%ld, incr=%ld", loopCount, incr);
		}
		get_single_debounced_button_release(ANY_BUTTON);
	}
}
