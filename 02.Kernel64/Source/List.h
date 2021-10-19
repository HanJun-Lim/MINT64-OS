#ifndef __LIST_H__
#define __LIST_H__

#include "Types.h"


// ---------- Structure ----------

#pragma pack(push, 1)


typedef struct kListLinkStruct		// for data link
{
	void* pvNext;
	QWORD qwID;
} LISTLINK;

/*
 * Example of Definition
 * Front of structure must start with LISTLINK
 *
struct kListItemExampleStruct
{
	// List link
	LISTLINK stLink;
	
	// Data
	int iData1;
	char cData2;
}
*/


typedef struct kListManagerStruct	// for list management
{
	int iItemCount;		// number of node in list
	
	void* pvHeader;		
	void* pvTail;
} LIST;


#pragma pack(pop)



// ---------- Function ----------

void  kInitializeList(LIST* pstList);

int   kGetListCount(LIST* pstList);

void  kAddListToTail(LIST* pstList, void* pvItem);
void  kAddListToHeader(LIST* pstList, void* pvItem);

void* kRemoveList(LIST* pstList, QWORD qwID);
void* kRemoveListFromHeader(LIST* pstList);
void* kRemoveListFromTail(LIST* pstList);

void* kFindList(const LIST* pstList, QWORD qwID);
void* kGetHeaderFromList(const LIST* pstList);
void* kGetTailFromList(const LIST* pstList);
void* kGetNextFromList(const LIST* pstList, void* pstCurrent);


#endif
