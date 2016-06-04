/* picoc include system - can emulate system includes from built-in libraries
 * or it can include and parse files if the system has files */
 
#include "picoc.h"
#include "interpreter.h"

#ifndef NO_HASH_INCLUDE


/* initialise the built-in include libraries */
void IncludeInit(Picoc *pc)
{
#ifndef BUILTIN_MINI_STDLIB
    IncludeRegister(pc, "ctype.h", NULL, &StdCtypeFunctions[0], NULL, NULL);
    IncludeRegister(pc, "errno.h", &StdErrnoSetupFunc, NULL, NULL, NULL);
# ifndef NO_FP
    IncludeRegister(pc, "math.h", &MathSetupFunc, &MathFunctions[0], &MathConstants[0], NULL);
# endif
    IncludeRegister(pc, "stdbool.h", &StdboolSetupFunc, NULL, &StdboolConstants[0], StdboolDefs);
    IncludeRegister(pc, "stdio.h", &StdioSetupFunc, &StdioFunctions[0], NULL, StdioDefs);
    IncludeRegister(pc, "stdlib.h", &StdlibSetupFunc, &StdlibFunctions[0], NULL, NULL);
    IncludeRegister(pc, "string.h", &StringSetupFunc, &StringFunctions[0], NULL, NULL);
    IncludeRegister(pc, "time.h", &StdTimeSetupFunc, &StdTimeFunctions[0], &StdTimeConstants[0], StdTimeDefs);
# if 0//ndef WIN32
    IncludeRegister(pc, "unistd.h", &UnistdSetupFunc, &UnistdFunctions[0], UnistdDefs);
# endif
#endif
}

/* clean up space used by the include system */
void IncludeCleanup(Picoc *pc)
{
    struct IncludeLibrary *ThisInclude = pc->IncludeLibList;
    struct IncludeLibrary *NextInclude;
    
    while (ThisInclude != NULL)
    {
        NextInclude = ThisInclude->NextLib;
        HeapFreeMem(pc, ThisInclude);
        ThisInclude = NextInclude;
    }

    pc->IncludeLibList = NULL;
}

/* register a new build-in include file */
void IncludeRegister(Picoc *pc, const char *IncludeName, void (*SetupFunction)(Picoc *pc), struct LibraryFunction *FuncList, struct LibraryConstant *CstList, const char *SetupCSource)
{
    /*struct IncludeLibrary *NewLib = HeapAllocMem(pc, sizeof(struct IncludeLibrary));
    NewLib->IncludeName = TableStrRegister(pc, IncludeName);
    NewLib->SetupFunction = SetupFunction;
    NewLib->FuncList = FuncList;
    NewLib->SetupCSource = SetupCSource;
    NewLib->NextLib = pc->IncludeLibList;
    pc->IncludeLibList = NewLib;*/
	char* name = TableStrRegister(pc, IncludeName);

	VariableDefine(pc, NULL, name, NULL, &pc->VoidType, FALSE);

	// run an extra startup function if there is one 
	if (SetupFunction != NULL)
		SetupFunction(pc);

	if (CstList != NULL)
		LibraryAddConstants(pc, CstList);

	// parse the setup C source code - may define types etc. 
	if (SetupCSource != NULL)
		PicocParse(pc, name, SetupCSource, strlen(SetupCSource), TRUE, TRUE, FALSE, FALSE);

	// set up the library functions 
	if (FuncList != NULL)
		LibraryAdd(pc, &pc->GlobalTable, name, FuncList);
}

/* include all of the system headers */
void PicocIncludeAllSystemHeaders(Picoc *pc)
{
    struct IncludeLibrary *ThisInclude = pc->IncludeLibList;
    
    for (; ThisInclude != NULL; ThisInclude = ThisInclude->NextLib)
        IncludeFile(pc, ThisInclude->IncludeName);
}

/* include one of a number of predefined libraries, or perhaps an actual file */
void IncludeFile(Picoc *pc, char *FileName)
{
    /*struct IncludeLibrary *LInclude;
    
    // scan for the include file name to see if it's in our list of predefined includes 
    for (LInclude = pc->IncludeLibList; LInclude != NULL; LInclude = LInclude->NextLib)
    {
        if (strcmp(LInclude->IncludeName, FileName) == 0)
        {
            // found it - protect against multiple inclusion
            if (!VariableDefined(pc, FileName))
            {
                VariableDefine(pc, NULL, FileName, NULL, &pc->VoidType, FALSE);
                
                // run an extra startup function if there is one 
                if (LInclude->SetupFunction != NULL)
                    (*LInclude->SetupFunction)(pc);
                
                // parse the setup C source code - may define types etc. 
                if (LInclude->SetupCSource != NULL)
                    PicocParse(pc, FileName, LInclude->SetupCSource, strlen(LInclude->SetupCSource), TRUE, TRUE, FALSE, FALSE);
                
                // set up the library functions 
                if (LInclude->FuncList != NULL)
                    LibraryAdd(pc, &pc->GlobalTable, FileName, LInclude->FuncList);
            }
            
            return;
        }
    }*/
    
    /* not a predefined file, read a real file */
    //PicocPlatformScanFile(pc, FileName);
}

void GetBuiltInFunction(std::string& list, LibraryFunction* lib)
{
	for (int Count = 0; lib[Count].Prototype != NULL; Count++)
	{
		list.append(lib[Count].Prototype);
		list.push_back('\n');
	}
}

void GetBuiltInConstants(std::string& list, LibraryConstant* lib)
{
	for (int Count = 0; lib[Count].CstValue != NULL; Count++)
	{
		list.append(lib[Count].Name);
		list.append(" = ");
		if (lib[Count].Type == TypeInt)
			list.append(std::to_string(lib[Count].CstValue->Integer));
		else if (lib[Count].Type == TypeFP)
			list.append(std::to_string(lib[Count].CstValue->FP));
		list.push_back('\n');
	}
}

void GetBuiltInFunctionConstants(std::string& list)
{
	GetBuiltInFunction(list, MathFunctions);
	GetBuiltInFunction(list, StdCtypeFunctions);
	GetBuiltInFunction(list, StdioFunctions);
	GetBuiltInFunction(list, StdlibFunctions);
	GetBuiltInFunction(list, StringFunctions);
	GetBuiltInFunction(list, StdTimeFunctions);
	list.append("\n\n");
	GetBuiltInConstants(list, MathConstants);
	GetBuiltInConstants(list, StdboolConstants);
	GetBuiltInConstants(list, StdTimeConstants);
}

#endif /* NO_HASH_INCLUDE */
