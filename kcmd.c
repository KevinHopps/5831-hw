#include <string.h>
#include "kcmd.h"
#include "kdebug.h"
#include "kserial.h"
#include "kutils.h"

// This resets the serial buffer and command-found logic.
//
void CIOCommandReset(CommandIO* ciop, void* context)
{
	ciop->m_prompted = false;
	ciop->m_found = NULL;
	ciop->m_argc = 0;
	ciop->m_argv[0] = 0;
	LBReset(&ciop->m_lbuf);
	ciop->m_context = context;
}

// This initializes everything.
//
void CIOReset(CommandIO* ciop, void* context)
{
	ciop->m_nfuncs = 0;
	CIOCommandReset(ciop, context);
}

// This registers a command and its function.
//
void CIORegisterCommand(CommandIO* ciop, const char* name, CmdFunc func)
{
	KASSERT(ciop->m_nfuncs < MAX_FUNCS);
	KASSERT(strlen(name) <= MAX_NAMELEN);
	Command* cmdp = &ciop->m_cmd[ciop->m_nfuncs++];
	strcpy(cmdp->m_name, name);
	cmdp->m_func = func;
}

// This returns the index of a command that matches name.
// If none match, or if multiple match, this returns negative.
//
int CIOIndex(CommandIO* ciop, const char* name)
{
	int numFound = 0;
	int found = -1;
	int nameLen = strlen(name);
	
	int i = ciop->m_nfuncs;
	while (--i >= 0)
	{
		if (strncmp(name, ciop->m_cmd[i].m_name, nameLen) == 0)
		{
			found = i;
			++numFound;
		}
	}
	
	if (numFound != 1)
		found = -1;
		
	return found;
}

// This tries to lookup a command that matches the given name.
// If a unique one is found, true is returned, and the found
// command is recorded for use later.
//
bool CIOLookup(CommandIO* ciop, const char* name)
{
	bool result = false;

	int i = CIOIndex(ciop, name);
	if (i >= 0)
	{
		ciop->m_found = &ciop->m_cmd[i];
		result = true;
	}

	return result;
}

// This polls for serial input and accumulates it. It returns
// true when return/enter is hit and a single matching command
// is found. If return/enter is hit, and no matching command
// is found (or multiple matching commands are found), then
// a message is displayed, the command/input structure is reset,
// and false is returned.
//
bool CIOCheckForCommand(CommandIO* ciop)
{
	bool result = false;

	if (!ciop->m_prompted)
	{
		CIOCommandReset(ciop, ciop->m_context);
		s_printf("> ");
		ciop->m_prompted = true;
	}

	char* gotLine = LBGetLine(&ciop->m_lbuf);
	if (gotLine)
	{
		ciop->m_prompted = false;

		ciop->m_argc = make_argv(ciop->m_argv, gotLine);
		if (ciop->m_argc > 0)
		{
			result = CIOLookup(ciop, ciop->m_argv[0]);
			if (!result)
			{
				s_println("%s: not found", ciop->m_argv[0]);
				CIOCommandReset(ciop, ciop->m_context);
			}
		}
	}

	return result;
}

// This runs the command that was found during CIOCheckForCommand().
//
int CIORunCommand(CommandIO* ciop)
{
	KASSERT(ciop->m_found);
	return ciop->m_found->m_func(ciop->m_argc, ciop->m_argv, ciop->m_context);
}
