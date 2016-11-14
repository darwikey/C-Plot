/* picoc's interface to the underlying platform. most platform-specific code
 * is in platform/platform_XX.c and platform/library_XX.c */
 
#include "picoc.h"
#include "interpreter.h"

#define PICOC_STACK_SIZE (128*1024)              /* space for the the stack */


/* initialise everything */
void PicocInitialise(Picoc *pc, double* arg, int paramCount, const std::string &SourceCode, std::vector<Tweakable>& tweakables, std::string &errorBuffer)
{
	if (PicocPlatformSetExitPoint(pc))
	{
		errorBuffer = pc->ErrorBuffer;
		if (errorBuffer.empty())
			errorBuffer = "unknown error";
		return;
	}

    PlatformInit(pc);
    BasicIOInit(pc);
    HeapInit(pc, PICOC_STACK_SIZE);
    TableInit(pc);
    VariableInit(pc);
    LexInit(pc);
    TypeInit(pc);
#ifndef NO_HASH_INCLUDE
    IncludeInit(pc);
#endif
    LibraryInit(pc);
	for (Tweakable& it : tweakables)
	{
		VariableDefinePlatformVar(pc, NULL, it.name.c_str(), &pc->FPType, (union AnyValue *)&it.value, FALSE);
	}
#ifdef BUILTIN_MINI_STDLIB
    LibraryAdd(pc, &GlobalTable, "c library", &CLibrary[0]);
    CLibraryInit(pc);
#endif
    DebugInit(pc);

	PicocParse(pc, "main.c", SourceCode.c_str(), SourceCode.size(), TRUE, FALSE, FALSE);

	/* check if the program wants arguments */
	if (!VariableDefined(pc, TableStrRegister(pc, "main")))
		ProgramFailNoParser(pc, "main() is not defined");
	
	struct Value *FuncValue = NULL;
	VariableGet(pc, NULL, TableStrRegister(pc, "main"), &FuncValue);
	if (FuncValue->Typ->Base != TypeFunction)
		ProgramFailNoParser(pc, "main is not a function - can't call it");

	if (FuncValue->Val->FuncDef.ReturnType != &pc->FPType)
	{
		ProgramFailNoParser(pc, "main function must return a double");
	}

	VariableDefinePlatformVar(pc, NULL, "__exit_value", &pc->FPType, (union AnyValue *)&pc->PicocExitValue, TRUE);

	if (paramCount == 1)
	{
		
		if (FuncValue->Val->FuncDef.NumParams != 0)
		{
			/* define the arguments */
			VariableDefinePlatformVar(pc, NULL, "__arg", &pc->FPType, (union AnyValue *)arg, FALSE);
		}

		if (FuncValue->Val->FuncDef.NumParams != 1)// || FuncValue->Val->FuncDef.ParamType != &pc->FPType)
			ProgramFailNoParser(pc, "main function must take a double as a param");
	}
	else
	{

		if (FuncValue->Val->FuncDef.NumParams != 0)
		{
			/* define the arguments */
			VariableDefinePlatformVar(pc, NULL, "__arg1", &pc->FPType, (union AnyValue *)arg, FALSE);
			VariableDefinePlatformVar(pc, NULL, "__arg2", &pc->FPType, (union AnyValue *)(&arg[1]), FALSE);
		}

		if (FuncValue->Val->FuncDef.NumParams != 2)// || FuncValue->Val->FuncDef.ParamType != &pc->FPType)
			ProgramFailNoParser(pc, "main function must take two double as a param");
	}
}

/* free memory */
void PicocCleanup(Picoc *pc)
{
    DebugCleanup(pc);
#ifndef NO_HASH_INCLUDE
    IncludeCleanup(pc);
#endif
    ParseCleanup(pc);
    LexCleanup(pc);
    VariableCleanup(pc);
    TypeCleanup(pc);
    TableStrFree(pc);
    HeapCleanup(pc);
    PlatformCleanup(pc);
}

void PrintSourceTextErrorLine(Picoc *pc, const char *FileName, const char *SourceText, int Line, int CharacterPos)
{
    int LineCount;
    const char *LinePos;
    const char *CPos;
    int CCount;
    
    if (SourceText != NULL)
    {
        /* find the source line */
        for (LinePos = SourceText, LineCount = 1; *LinePos != '\0' && LineCount < Line; LinePos++)
        {
            if (*LinePos == '\n')
                LineCount++;
        }
        
        /* display the line */
		for (CPos = LinePos; *CPos != '\n' && *CPos != '\0'; CPos++)
			PlatformPrintError(pc, *CPos);
		PlatformPrintError(pc, '\n');
        
        /* display the error position */
        for (CPos = LinePos, CCount = 0; *CPos != '\n' && *CPos != '\0' && (CCount < CharacterPos || *CPos == ' '); CPos++, CCount++)
        {
            if (*CPos == '\t')
				PlatformPrintError(pc, '\t');
            else
				PlatformPrintError(pc, ' ');
        }
    }
    else
    {
        /* assume we're in interactive mode - try to make the arrow match up with the input text */
        for (CCount = 0; CCount < CharacterPos + (int)strlen(INTERACTIVE_PROMPT_STATEMENT); CCount++)
			PlatformPrintError(pc, ' ');
    }
    PlatformPrintError(pc, "line");
	PlatformPrintError(pc, std::to_string(Line));
	PlatformPrintError(pc, ": ");
}

/* exit with a message */
void ProgramFail(struct ParseState *Parser, const std::string &Message)
{
    PrintSourceTextErrorLine(Parser->pc, Parser->FileName, Parser->SourceText, Parser->Line, Parser->CharacterPos);
	PlatformPrintError(Parser->pc, Message);
    PlatformExit(Parser->pc, 1);
}

/* exit with a message, when we're not parsing a program */
void ProgramFailNoParser(Picoc *pc, const std::string &Message)
{
    PlatformPrintError(pc, Message);
    PlatformExit(pc, 1);
}

/* like ProgramFail() but gives descriptive error messages for assignment */
void AssignFail(struct ParseState *Parser, const std::string &message, struct ValueType *Type1, struct ValueType *Type2, const char *FuncName, int ParamNo)
{
    PrintSourceTextErrorLine(Parser->pc, Parser->FileName, Parser->SourceText, Parser->Line, Parser->CharacterPos);
    PlatformPrintError(Parser->pc, "can't ");   
	if (FuncName == NULL)
		PlatformPrintError(Parser->pc, "assign ");
	else
		PlatformPrintError(Parser->pc, "set ");

	if (Type1 != NULL)
		PrintType(Type1, Parser->pc);
	
	PlatformPrintError(Parser->pc, message);
	
	if (Type2 != NULL)
		PrintType(Type2, Parser->pc);

    if (FuncName != NULL)
		PlatformPrintError(Parser->pc, " in argument " + std::to_string(ParamNo) + " of call to " + std::string(FuncName) + "()");
    
	PlatformPrintError(Parser->pc, "\n");
    PlatformExit(Parser->pc, 1);
}

/* exit lexing with a message */
void LexFail(Picoc *pc, struct LexState *Lexer, const std::string &Message)
{
    PrintSourceTextErrorLine(pc, Lexer->FileName, Lexer->SourceText, Lexer->Line, Lexer->CharacterPos);
    PlatformPrintError(pc, Message);
    PlatformExit(pc, 1);
}

/* printf for compiler error reporting */
void PlatformPrintError(Picoc *pc, const std::string &error)
{
    pc->ErrorBuffer.append(error);
}

void PlatformPrintError(Picoc *pc, char c)
{
	pc->ErrorBuffer.push_back(c);
}

/*void PlatformVPrintf(IOFILE *Stream, const char *Format, va_list Args)
{
    const char *FPos;
    
    for (FPos = Format; *FPos != '\0'; FPos++)
    {
        if (*FPos == '%')
        {
            FPos++;
            switch (*FPos)
            {
            case 's': PrintStr(va_arg(Args, char *), Stream); break;
            case 'd': PrintSimpleInt(va_arg(Args, int), Stream); break;
            case 'c': PrintCh(va_arg(Args, int), Stream); break;
            case 't': PrintType(va_arg(Args, struct ValueType *), Stream); break;
#ifndef NO_FP
            case 'f': PrintFP(va_arg(Args, double), Stream); break;
#endif
            case '%': PrintCh('%', Stream); break;
            case '\0': FPos--; break;
            }
        }
        else
            PrintCh(*FPos, Stream);
    }
}*/

/* make a new temporary name. takes a static buffer of char [7] as a parameter. should be initialised to "XX0000"
 * where XX can be any characters */
char *PlatformMakeTempName(Picoc *pc, char *TempNameBuffer)
{
    int CPos = 5;
    
    while (CPos > 1)
    {
        if (TempNameBuffer[CPos] < '9')
        {
            TempNameBuffer[CPos]++;
            return TableStrRegister(pc, TempNameBuffer);
        }
        else
        {
            TempNameBuffer[CPos] = '0';
            CPos--;
        }
    }

    return TableStrRegister(pc, TempNameBuffer);
}
