/* picoc external interface. This should be the only header you need to use if
 * you're using picoc as a library. Internal details are in interpreter.h */
#ifndef PICOC_H
#define PICOC_H

/* picoc version number */
#ifdef VER
#define PICOC_VERSION "v2.2 beta r" VER         /* VER is the subversion version number, obtained via the Makefile */
#else
#define PICOC_VERSION "v2.2"
#endif

/* handy definitions */
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#include "interpreter.h"

double PicocEvaluate(Picoc& pc, int paramCount, std::string &errorBuffer);

#include <setjmp.h>

/* this has to be a macro, otherwise errors will occur due to the stack being corrupt */
#define PicocPlatformSetExitPoint(pc) setjmp((pc)->PicocExitBuf)

/* parse.c */
void PicocParse(Picoc *pc, const char *FileName, const char *Source, int SourceLen, int RunIt, int CleanupNow, int EnableDebugger);
void PicocParseInteractive(Picoc *pc);

/* platform.c */
void PicocInitialise(Picoc *pc, double* arg, int paramCount, const std::string &SourceCode, std::vector<Tweakable>& tweakables, std::string &errorBuffer);
void PicocCleanup(Picoc *pc);
void PicocPlatformScanFile(Picoc *pc, const char *SourceStr);

/* include.c */
void PicocIncludeAllSystemHeaders(Picoc *pc);

#endif /* PICOC_H */
