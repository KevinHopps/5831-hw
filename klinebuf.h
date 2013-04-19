#ifndef _klinebuf_h_
#define _klinebuf_h_

#include "ktypes.h"

typedef struct LineBuf_ LineBuf;
struct LineBuf_
{
	uint8_t m_len;
	bool m_frozen;
	char m_buf[64];
};

// Reset the structure to be empty.
//
void LBReset(LineBuf* lbuf);

// Return the current length of the line of input.
//
int LBLength(const LineBuf* lbuf);

// Return the character at the specified index.
//
int LBCharAt(const LineBuf* lbuf, int index);

// This will possibly read characters into lbuf->m_buf each
// time it is called. When it receives a '\r' it will trim
// it and return lbuf->m_buf. Otherwise it will return 0.
//
char* LBGetLine(LineBuf* lbuf);

#endif // #ifndef _klinebuf_h_
