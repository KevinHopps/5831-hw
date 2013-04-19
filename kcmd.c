#include <string.h>
#include "kcmd.h"
#include "kdebug.h"
#include "kserial.h"
#include "kutils.h"

void CIOCommandReset(CommandIO* ciop, void* context)
{
	ciop->m_prompted = 0;
	ciop->m_found = 0;
	ciop->m_argc = 0;
	ciop->m_argv[0] = 0;
	LBReset(&ciop->m_lbuf);
	ciop->m_context = context;
}

void CIOReset(CommandIO* ciop, void* context)
{
	ciop->m_nfuncs = 0;
	CIOCommandReset(ciop, context);
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

bool CIOCheckForCommand(CommandIO* ciop)
{
	bool result = false;

	if (!ciop->m_prompted)
	{
		CIOCommandReset(ciop, ciop->m_context);
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
				CIOCommandReset(ciop, ciop->m_context);
			}
		}
	}

	return result;
}

int CIORunCommand(CommandIO* ciop)
{
	KASSERT(ciop->m_found);
	return ciop->m_found->m_func(ciop->m_argc, ciop->m_argv, ciop->m_context);
}
