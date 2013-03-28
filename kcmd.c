#include <string.h>
#include "kcmd.h"
#include "kdebug.h"
#include "kserial.h"

void CIOCommandReset(CommandIO* ciop)
{
	ciop->m_prompted = 0;
	ciop->m_found = 0;
	ciop->m_argc = 0;
	ciop->m_argv[0] = 0;
	LBReset(&ciop->m_lbuf);
}

void CIOReset(CommandIO* ciop)
{
	ciop->m_nfuncs = 0;
	CIOCommandReset(ciop);
}

void CIORegisterCommand(CommandIO* ciop, const char* name, CmdFunc func)
{
	KASSERT(ciop->m_nfuncs < MAX_FUNCS);
	KASSERT(strlen(name) <= MAX_NAMELEN);
	Command* cmdp = &ciop->m_cmd[ciop->m_nfuncs++];
	strcpy(cmdp->m_name, name);
	cmdp->m_func = func;
}

int CIOIndex(CommandIO* ciop, const char* name)
{
	int i = ciop->m_nfuncs;
	while (--i >= 0 && !matchIgnoreCase(name, ciop->m_cmd[i].m_name, strlen(name)))
		continue;
	return i;
}

int CIOLookup(CommandIO* ciop, const char* name)
{
	int result = 0;

	int i = CIOIndex(ciop, name);
	if (i >= 0)
	{
		ciop->m_found = &ciop->m_cmd[i];
		result = 1;
	}

	return result;
}

int CIOCheckForCommand(CommandIO* ciop)
{
	static long numCalls;

	int result = 0;

	if (!ciop->m_prompted)
	{
		CIOCommandReset(ciop);
		s_printf("> ");
		ciop->m_prompted = 1;
	}

	char* gotLine = LBGetLine(&ciop->m_lbuf);
	if (gotLine)
	{
		ciop->m_prompted = 0;

		ciop->m_argc = make_argv(ciop->m_argv, gotLine);
		if (ciop->m_argc > 0)
		{
			result = CIOLookup(ciop, ciop->m_argv[0]);
			if (!result)
			{
				s_println("%s: not found", ciop->m_argv[0]);
				CIOCommandReset(ciop);
			}
		}
	}

	return result;
}

int CIORunCommand(CommandIO* ciop)
{
	KASSERT(ciop->m_found);
	return ciop->m_found->m_func(ciop->m_argc, ciop->m_argv);
}
