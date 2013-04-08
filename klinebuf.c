#include "kdebug.h"
#include "klinebuf.h"
#include "kserial.h"

void LBReset(LineBuf* lbuf)
{
	lbuf->m_len = 0;
	lbuf->m_frozen = 0;
}

int LBLength(const LineBuf* lbuf)
{
	return lbuf->m_len;
}

int LBCharAt(const LineBuf* lbuf, int index)
{
	KASSERT(0 <= index && index < lbuf->m_len);
	return lbuf->m_buf[index] & 0xff;
}

char* LBFreeze(LineBuf* lbuf)
{
	lbuf->m_frozen = 1;
	lbuf->m_buf[lbuf->m_len] = 0;
	return lbuf->m_buf;
}

void LBErase(LineBuf* lbuf)
{
	if (lbuf->m_len > 0)
		--lbuf->m_len;
}

void LBPut(LineBuf* lbuf, char c)
{
	if (lbuf->m_frozen)
		LBReset(lbuf);

	KASSERT(lbuf->m_len < sizeof(lbuf->m_buf));

	lbuf->m_buf[lbuf->m_len++] = c;
}

#define ERASE 0x7f

char* LBGetLine(LineBuf* lbuf)
{
	char* result = 0;

	char c;
	while (result == 0 && s_read(&c, 1, 0) > 0)
	{
		if (c == '\r')
		{
			s_println("");
			result = LBFreeze(lbuf);
		}
		else if (c == ERASE)
		{
			static const char erase[] = { 8, 32, 8 };
			s_write(erase, sizeof(erase));
			LBErase(lbuf);
		}
		else
		{
			s_write(&c, 1);
			LBPut(lbuf, c);
		}
	}

	return result;
}
