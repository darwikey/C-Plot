/* picoc main program - this varies depending on your operating system and
 * how you're using picoc */
 
/* include only picoc.h here - should be able to use it with only the external interfaces, no internals from interpreter.h */
#include "picoc.h"

/* platform-dependent code for running programs is in this file */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PICOC_STACK_SIZE (128*1024)              /* space for the the stack */

extern bool gResetParser;

double parse(const char* fCode, double* arg, int paramCount, std::vector<Tweakable>& tweakables, bool& isCrash, std::string &errorBuffer)
{
	isCrash = false;
	gResetParser = false;

    int StackSize = getenv("STACKSIZE") ? atoi(getenv("STACKSIZE")) : PICOC_STACK_SIZE;
    Picoc pc;
	memset(&pc, '\0', sizeof(pc));
    
	if (PicocPlatformSetExitPoint(&pc))
	{
		isCrash = true;
		errorBuffer = pc.ErrorBuffer;
		PicocCleanup(&pc);
		return pc.PicocExitValue;
	}

    PicocInitialise(&pc, StackSize, tweakables);
    
        
    //if (0)//argc > ParamCount && strcmp(argv[ParamCount], "-i") == 0)
    //{
    //    PicocIncludeAllSystemHeaders(&pc);
    //    PicocParseInteractive(&pc);
    //}
    //else
    {
            
        //for (; ParamCount < argc && strcmp(argv[ParamCount], "-") != 0; ParamCount++)
        PicocPlatformScanFile(&pc, fCode);
        
		if (paramCount == 1)
			PicocCallMain(&pc, arg[0]);// argc - ParamCount, &argv[ParamCount]);
		else if (paramCount == 2)
			PicocCallMain(&pc, arg[0], arg[1]);

    }
    
    PicocCleanup(&pc);
    return pc.PicocExitValue;
}

