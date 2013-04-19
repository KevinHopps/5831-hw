#ifndef _lab2cmds_h_
#define _lab2cmds_h_

#include "kcmd.h"
#include "kmotor.h"
#include "Trajectory.h"

// This Context structure is used by any commands. It is maintained
// in the main loop and passed to any commands executed.
//
typedef struct Context_ Context;
struct Context_
{
	bool m_logging;
	Trajectory* m_tp;
	PDControl* m_pdc;
	Motor* m_motor;
};
void ContextInit(Context* ctx, Trajectory* tp, PDControl* pdc, Motor* mp);

void InitCommands(CommandIO* cio);

#endif // #ifndef _lab2cmds_h_
