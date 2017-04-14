//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// SECT.H - Code Generation, Fixups and Section Management
// Copyright (C) 199x Landon Dyer, 2017 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source utilised with the kind permission of Landon Dyer
//

#ifndef __SECT_H__
#define __SECT_H__

#include "rmac.h"

// Macros to deposit code in the current section
// D_rword deposits a "6502" format (low, high) word (01).
// D_rlong deposits a MWC "canonical byte order" longword (2301).
#define D_byte(b)	{*chptr++=(uint8_t)(b); sloc++; ch_size++; \
						if(orgactive) orgaddr++;}
#define D_word(w)	{chcheck(2);*chptr++=(uint8_t)((w)>>8); \
						*chptr++=(uint8_t)(w); \
						sloc += 2; ch_size += 2; if(orgactive) orgaddr += 2;}
#define D_long(lw)	{*chptr++=(uint8_t)((lw)>>24); \
						*chptr++=(uint8_t)((lw)>>16);\
						*chptr++=(uint8_t)((lw)>>8); \
						*chptr++=(uint8_t)(lw); \
						sloc += 4; ch_size += 4; if(orgactive) orgaddr += 4;}
//#define D_rword(w)	{*chptr++=(uint8_t)w; *chptr++=(uint8_t)(w>>8); \
//						sloc+=2; ch_size+=2;if(orgactive) orgaddr += 2;}
//#define D_rlong(lw)	{*chptr++=(uint8_t)(lw>>16);*chptr++=(uint8_t)(lw>>24);\
//						*chptr++=(uint8_t)lw;*chptr++=(uint8_t)(lw>>8); \
//						sloc+=4; ch_size += 4;if(orgactive) orgaddr += 4;}
// Fill n bytes with zeroes
#define D_ZEROFILL(n)	{memset(chptr, 0, n); chptr+=n; sloc+=n; ch_size+=n;\
						if (orgactive) orgaddr+=n;}

#define NSECTS       16			// Max. number of sections

// Tunable (storage) definitions
#define CH_THRESHOLD    64		// Minimum amount of space in code chunk
#define CH_CODE_SIZE    2048	// Code chunk normal allocation
#define CH_FIXUP_SIZE   1024	// Fixup chunk normal allocation

// Section attributes (.scattr)
#define SUSED        0x8000		// Section is used (really, valid)
#define SBSS         0x4000		// Section can contain no data
#define SABS         0x2000		// Section is absolute
#define SPIC         0x1000		// Section is position-independent code

// Fixup record a WORD of these bits, followed by a loc and then a pointer
// to a symbol or an ENDEXPR-terminated postfix expression.
//
// SYMBOL		EXPRESSION
// ------		----------
// ~FU_EXPR    FU_EXPR     fixup type
// loc.L       loc.L       location in section
// fileno.W    fileno.W    file number fixup occurred in
// lineno.W    lineno.W    line number fixup occurred in
// symbol.L    size.W      &symbol  /  size of expression
// token.L     expression list
// (etc)
// ENDEXPR.L	(end of expression)
#define FUMASK       007		// Mask for fixup cases:
#define FU_QUICK     000		// Fixup 3-bit quick instr field
#define FU_BYTE      001		// Fixup byte
#define FU_WORD      002		// Fixup word
#define FU_WBYTE     003		// Fixup byte (at loc+1)
#define FU_LONG      004		// Fixup long
#define FU_BBRA      005		// Fixup byte branch
#define FU_6BRA      007		// Fixup 6502-format branch offset
#define FU_SEXT      010		// Ok to sign extend
#define FU_PCREL     020		// Subtract PC first
#define FU_EXPR      040		// Expression (not symbol) follows

#define FU_GLOBAL    0x0080		// Mark global symbol

#define FUMASKRISC   0x0F00		// Mask for RISC fixup cases
#define FU_MOVEI     0x0100
#define FU_JR        0x0200
#define FU_REGONE    0x0400
#define FU_NUM15     0x0500
#define FU_NUM31     0x0600
#define FU_NUM32     0x0700
#define FU_REGTWO    0x0800

#define FU_SUB32     0x1000
#define FU_ISBRA     0x2000		// Word forward fixup is a BRA or DBRA
#define FU_LBRA      0x4000		// Long branch, for short branch detect
#define FU_DONE      0x8000		// Fixup has been done

// Chunks are used to hold generated code and fixup records
#define CHUNK  struct _chunk
CHUNK {
	CHUNK * chnext;				// Next, previous chunks in section
	CHUNK * chprev;
	uint32_t chloc;				// Base addr of this chunk
	uint32_t challoc;			// # bytes allocated for chunk
	uint32_t ch_size;			// # bytes chunk actually uses
	uint8_t * chptr;			// Data for this chunk
};

// Section descriptor
#define SECT   struct _sect
SECT {
	uint16_t scattr;			// Section attributes
	uint32_t sloc;				// Current loc-in / size-of section
	uint32_t relocs;			// # of relocations for this section
	CHUNK * sfcode;				// First chunk in section
	CHUNK * scode;				// Last chunk in section
	CHUNK * sffix;				// First fixup chunk
	CHUNK * sfix;				// Last fixup chunk
};

// Globals, external etc
extern uint32_t sloc;
extern uint16_t scattr;
extern uint8_t * chptr;
extern uint8_t * chptr_opcode;
extern uint32_t ch_size;
extern int cursect;
extern SECT sect[];
extern uint32_t challoc;
extern CHUNK * scode;

// Prototypes
void InitSection(void);
void SwitchSection(int);
void SaveSection(void);
int fixtest(int, uint32_t);
int chcheck(uint32_t);
int AddFixup(uint16_t, uint32_t, TOKEN *);
int ResolveAllFixups(void);

#endif // __SECT_H__

