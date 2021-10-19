#ifndef __SYSTEMCALL_H__
#define __SYSTEMCALL_H__


// ---------- Macro ----------

// Maximum Parameter count
#define SYSTEMCALL_MAXPARAMETERCOUNT		20



// ---------- Structure  ----------

#pragma pack(push, 1)

typedef struct kSystemcallParameterTableStruct
{
	QWORD vqwValue[SYSTEMCALL_MAXPARAMETERCOUNT];
} PARAMETERTABLE;

#pragma pack(pop)



#define PARAM(x)	( pstParameter->vqwValue[(x)] )



// ---------- Function ----------

void kSystemCallEntryPoint(QWORD qwServiceNumber, PARAMETERTABLE* pstParameter);
QWORD kProcessSystemCall(QWORD qwServiceNumber, PARAMETERTABLE* pstParameter);

void kSystemCallTestTask(void);



#endif
