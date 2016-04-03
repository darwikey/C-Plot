/* picoc main program - this varies depending on your operating system and
 * how you're using picoc */
 
/* include only picoc.h here - should be able to use it with only the external interfaces, no internals from interpreter.h */
#include "picoc.h"

/* platform-dependent code for running programs is in this file */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PICOC_STACK_SIZE (128*1024)              /* space for the the stack */

double parse(const char* fCode, double arg, int* isCrash, char errorBuffer[ERROR_BUFFER_SIZE])
{
	*isCrash = 0;
    int ParamCount = 1;
    int DontRunMain = FALSE;
    int StackSize = getenv("STACKSIZE") ? atoi(getenv("STACKSIZE")) : PICOC_STACK_SIZE;
    Picoc pc;
    
    /*if (argc < 2)
    {
        printf("Format: picoc <csource1.c>... [- <arg1>...]    : run a program (calls main() to start it)\n"
               "        picoc -s <csource1.c>... [- <arg1>...] : script mode - runs the program without calling main()\n"
               "        picoc -i                               : interactive mode\n");
        exit(1);
    }*/
    
    PicocInitialise(&pc, StackSize);
    
    if (0)//strcmp(argv[ParamCount], "-s") == 0 || strcmp(argv[ParamCount], "-m") == 0)
    {
        DontRunMain = TRUE;
        PicocIncludeAllSystemHeaders(&pc);
        ParamCount++;
    }
        
    if (0)//argc > ParamCount && strcmp(argv[ParamCount], "-i") == 0)
    {
        PicocIncludeAllSystemHeaders(&pc);
        PicocParseInteractive(&pc);
    }
    else
    {
        if (PicocPlatformSetExitPoint(&pc))
        {
			*isCrash = 1;
			strcpy_s(errorBuffer, ERROR_BUFFER_SIZE, pc.ErrorBuffer);
            PicocCleanup(&pc);
            return pc.PicocExitValue;
        }
        
        //for (; ParamCount < argc && strcmp(argv[ParamCount], "-") != 0; ParamCount++)
        PicocPlatformScanFile(&pc, fCode);
        
        if (!DontRunMain)
			PicocCallMain(&pc, arg);// argc - ParamCount, &argv[ParamCount]);
    }
    
    PicocCleanup(&pc);
    return pc.PicocExitValue;
}

