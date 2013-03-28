#include "kserial.h"
#include "kutils.h"

static int iswhite(char c)
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

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

	argv[argc] = 0;
	return argc;
}

int matchIgnoreCase(const char* s1, const char* s2, int maxLen)
{
	int match = 1;

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
			if ('A' <= c1 && c1 <= 'Z')
				c1 += 'a' - 'A';

			if ('A' <= c2 && c2 <= 'Z')
				c2 += 'a' - 'A';

			if (c1 != c2)
				match = 0;
		}
	}

	return match;
}
