#ifndef _kcmd_h_
#define _kcmd_h_

#include "ktypes.h"
#include "klinebuf.h"

#define MAX_ARGC 8
#define MAX_FUNCS 16
#define MAX_NAMELEN 15

typedef int (*CmdFunc)(int argc, char** argv, void* context);

typedef struct Command_ Command;
struct Command_
{
	char m_name[MAX_NAMELEN+1];
	CmdFunc m_func;
};

struct CommandIO_
{
	uint8_t m_prompted;
	uint8_t m_argc;
	uint8_t m_nfuncs;
	Command* m_found;
	char* m_argv[MAX_ARGC];
	Command m_cmd[MAX_FUNCS];
	LineBuf m_lbuf;
	void* m_context; // passed to the CmdFunc
};
typedef struct CommandIO_ CommandIO;

void CIOReset(CommandIO* cmdp, void* context);
void CIORegisterCommand(CommandIO* cmdp, const char* name, CmdFunc func);

// This will scan for serial input and return nonzero when
// a command is ready to run.
//
bool CIOCheckForCommand(CommandIO* cmdp);

// This will run the command found by CIOCheckForCommand
// and return its result.
//
int CIORunCommand(CommandIO* cmdp);

#endif // #ifndef _kcmd_h_
