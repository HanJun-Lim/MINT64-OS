#ifndef __MULTIPROCESSOR_H__
#define __MULTIPROCESSOR_H__

#include "Types.h"


// ---------- Macro ----------


// related to Multi Processor (Variable position in BootLoader.asm)

#define BOOTSTRAPPROCESSOR_FLAGADDRESS		0x7C09

// Max supportable Processor/Core count

#define MAXPROCESSORCOUNT					16



// ---------- Function ----------

BOOL kStartUpApplicationProcessor(void);
BYTE kGetAPICID(void);

static BOOL kWakeUpApplicationProcessor(void);



#endif
