/* picoc main program - this varies depending on your operating system and
 * how you're using picoc */
 
/* include only picoc.h here - should be able to use it with only the external interfaces, no internals from interpreter.h */
#include "picoc.h"

/* platform-dependent code for running programs is in this file */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern bool gResetParser;

#define CALL_MAIN_WITH_ARGS_RETURN_DOUBLE "__exit_value = main(__arg);"
#define CALL_MAIN_WITH_2ARGS_RETURN_DOUBLE "__exit_value = main(__arg1, __arg2);"

double PicocEvaluate(Picoc& pc, int paramCount, std::string &errorBuffer)
{
	gResetParser = false;
    
	if (PicocPlatformSetExitPoint(&pc))
	{
		errorBuffer = pc.ErrorBuffer;
		if (errorBuffer.empty())
			errorBuffer = "unknown error";
		return pc.PicocExitValue;
	}
    
	if (paramCount == 1)
		PicocParse(&pc, "startup", CALL_MAIN_WITH_ARGS_RETURN_DOUBLE, strlen(CALL_MAIN_WITH_ARGS_RETURN_DOUBLE), TRUE, TRUE, FALSE);
	else if (paramCount == 2)
		PicocParse(&pc, "startup", CALL_MAIN_WITH_2ARGS_RETURN_DOUBLE, strlen(CALL_MAIN_WITH_2ARGS_RETURN_DOUBLE), TRUE, TRUE, FALSE);
    
    return pc.PicocExitValue;
}

