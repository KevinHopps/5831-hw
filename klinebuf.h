#ifndef _klinebuf_h_
#define _klinebuf_h_

struct LineBuf_
{
	uint8_t m_len;
	uint8_t m_frozen;
	char m_buf[30];
};
typedef struct LineBuf_ LineBuf;

void LBReset(LineBuf* lbuf);
int LBLength(const LineBuf* lbuf);
int LBCharAt(const LineBuf* lbuf, int index);

// This will possibly read characters into lbuf->m_buf each
// time it is called. When it receives a '\r' it will trim
// it and return lbuf->m_buf. Otherwise it will return 0.
//
char* LBGetLine(LineBuf* lbuf);

#endif // #ifndef _klinebuf_h_
