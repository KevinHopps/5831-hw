#ifndef _lab2cmds_h_
#define _lab2cmds_h_

#include "kcmd.h"
#include "kmotor.h"

typedef struct Context_ Context;
struct Context_
{
	bool m_logging;
	Motor m_motor;
};
void ContextInit(Context* ctx);

void InitCommands(CommandIO* cio);

#endif // #ifndef _lab2cmds_h_
