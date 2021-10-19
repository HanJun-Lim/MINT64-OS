#ifndef __TYPES_H__
#define __TYPES_H__

#define BYTE	unsigned char		
#define WORD	unsigned short
#define DWORD	unsigned int
#define QWORD	unsigned long
#define BOOL	unsigned char

#define TRUE	1
#define FALSE	0
#define NULL	0

#define offsetof(TYPE, MEMBER) __builtin_offsetof (TYPE, MEMBER)		// included in stddef.h



#pragma pack(push, 1)	// size of structure aligned by 1 byte

// data structure constructing text mode in Video Mode

typedef struct kCharactorStruct
{
	BYTE bCharactor;
	BYTE bAttribute;
} CHARACTER;

#pragma pack(pop)


#endif /*__TYPES_H__*/
