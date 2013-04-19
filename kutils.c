#include "kserial.h"
#include "kutils.h"

static bool iswhite(char c)
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// Parse the given str and make argv[] point into it at
// the beginning of the non-white substrings. Return the
// number of items in argv[].
//
int make_argv(char** argv, char* str)
{
	int argc = 0;

	while (*str)
	{
		while (iswhite(*str))
			*str++ = 0;

		if (*str)
		{
			argv[argc++] = str;
			while (*++str && !iswhite(*str))
				continue;
		}
	}

	argv[argc] = NULL;
	return argc;
}

bool matchIgnoreCase(const char* s1, const char* s2, int maxLen)
{
	bool match = true;

	while (match && maxLen != 0)
	{
		if (maxLen > 0)
			--maxLen;

		char c1 = *s1++;
		char c2 = *s2++;
		if (c1 == c2)
		{
			if (c1 == 0)
				maxLen = 0; // stop looping
		}
		else
		{
			static const int to_lower = (int)'a' - (int)'A';
			static const int to_upper = -to_lower;
			int diff = (int)c1 - (int)c2;
			if (diff != to_lower && diff != to_upper)
				match = false;
		}
	}

	return match;
}

// Return true iff interrupts are currently enabled.
//
bool getInterruptsEnabled()
{
	uint8_t sreg = SREG; // get status register contents
	uint8_t bit = 1 << SREG_I; // interrupts-enabled bit
	uint8_t result = sreg & bit;
	return result != 0;
}

// Enable or disable interrupts. Return the previous
// interrupts-enabled state.
//
bool setInterruptsEnabled(bool enable)
{
	bool result = getInterruptsEnabled();
	
	if (enable)
		sei();
	else
		cli();
		
	return result;
}
