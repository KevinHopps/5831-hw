#ifndef _kcmd_h_
#define _kcmd_h_

#include "ktypes.h"
#include "klinebuf.h"

// The CommandIO structure and related functions poll
// the serial input and accumulate the characters into
// a command line until the user hits return/enter. At
// that point, the command line is parsed into an
// argc/argv pair, and the registered command names are
// searched for a matching command. If one is found,
// the registered function is invoked.
//

#define MAX_ARGC 8 // max argc for any command
#define MAX_FUNCS 24 // max commands that can be registered
#define MAX_NAMELEN 15 // max name of any command, not including null terminator

// This is the signature of a command that gets invoked.
// The context is an arbitrary data structure that may be
// registered along with the set of commands. The context
// is not used by the CommandIO functions but will be dutifully
// passed along to the command functions. In this way, you
// can avoid global variables.
//
typedef int (*CmdFunc)(int argc, char** argv, void* context);

// This is the struct that holds a command name and its
// associated function. The CommandIO struct holds an
// array of these. They are searched for a matching command
// when the user hits return/enter.
//
typedef struct Command_ Command;
struct Command_
{
	char m_name[MAX_NAMELEN+1];
	CmdFunc m_func;
};

typedef struct CommandIO_ CommandIO;
struct CommandIO_
{
	bool m_prompted;
	uint8_t m_argc;
	uint8_t m_nfuncs;
	Command* m_found;
	char* m_argv[MAX_ARGC];
	Command m_cmd[MAX_FUNCS];
	LineBuf m_lbuf;
	void* m_context; // passed to the CmdFunc
};

// This resets the CommandIO structure.
// All commands are unregistered.
//
void CIOReset(CommandIO* cmdp, void* context);

// Use this to register a given command, providing its
// name and function.
//
void CIORegisterCommand(CommandIO* cmdp, const char* name, CmdFunc func);

// This will scan for serial input and return true when
// a command is ready to run. Then you may call CIORunCommand()
// to execute it.
//
bool CIOCheckForCommand(CommandIO* cmdp);

// This will run the command found by CIOCheckForCommand
// and return its result.
//
int CIORunCommand(CommandIO* cmdp);

#endif // #ifndef _kcmd_h_
