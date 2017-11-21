//
// RMAC - Reboot's Macro Assembler for all Atari computers
// SECT.H - Code Generation, Fixups and Section Management
// Copyright (C) 199x Landon Dyer, 2011-2017 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source utilised with the kind permission of Landon Dyer
//

#ifndef __SECT_H__
#define __SECT_H__

#include "rmac.h"

// Macros to deposit code in the current section
// D_rword deposits a "6502" format (low, high) word (01).
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
#define D_quad(qw)	{*chptr++=(uint8_t)((qw)>>56); \
						*chptr++=(uint8_t)((qw)>>48);\
						*chptr++=(uint8_t)((qw)>>40);\
						*chptr++=(uint8_t)((qw)>>32);\
						*chptr++=(uint8_t)((qw)>>24);\
						*chptr++=(uint8_t)((qw)>>16);\
						*chptr++=(uint8_t)((qw)>>8); \
						*chptr++=(uint8_t)(qw); \
						sloc += 8; ch_size += 8; if(orgactive) orgaddr += 8;}
#define D_rword(w)	{*chptr++=(uint8_t)(w); *chptr++=(uint8_t)((w)>>8); \
						sloc+=2; ch_size+=2;if(orgactive) orgaddr += 2;}
#define D_single(w) {chcheck(4);*chptr++ = ((char *)&w)[3]; \
						*chptr++ = ((char *)&w)[2]; \
						*chptr++ = ((char *)&w)[1]; \
						*chptr++=((char *)&w)[0]; \
						sloc+=4; ch_size += 4; if(orgactive) orgaddr += 4;}
#define D_double(w) {chcheck(8);*chptr++=(char)((*(unsigned long long *)&w)); \
						*chptr++=(char)((*(unsigned long long *)&w)>>8); \
						*chptr++=(char)((*(unsigned long long *)&w)>>16); \
						*chptr++=(char)((*(unsigned long long *)&w)>>24); \
						*chptr++=(char)((*(unsigned long long *)&w)>>32); \
						*chptr++=(char)((*(unsigned long long *)&w)>>40); \
						*chptr++=(char)((*(unsigned long long *)&w)>>48); \
						*chptr++=(char)((*(unsigned long long *)&w)>>56); \
						sloc+=8; ch_size += 8; if(orgactive) orgaddr += 8;}
#ifdef _MSC_VER
#define D_extend(w) {chcheck(12); *chptr++ = (char)((*(unsigned long long *)&w) >> 56); \
*chptr++ = (char)(((*(unsigned long long *)&w) >> (52)) & 0xf); \
*chptr++ = (char)(0); \
*chptr++ = (char)(0); \
*chptr++ = (char)(((*(unsigned long long *)&w) >> (48 - 3))|0x80 /* assume that the number is normalised */); \
*chptr++ = (char)((*(unsigned long long *)&w) >> (40 - 3)); \
*chptr++ = (char)((*(unsigned long long *)&w) >> (32 - 3)); \
*chptr++ = (char)((*(unsigned long long *)&w) >> (24 - 3)); \
*chptr++ = (char)((*(unsigned long long *)&w) >> (16 - 3)); \
*chptr++ = (char)((*(unsigned long long *)&w) >> (8 - 3)); \
*chptr++ = (char)((*(unsigned long long *)&w << 3)); \
*chptr++=(char)(0); \
	sloc+=12; ch_size += 12; if(orgactive) orgaddr += 12;}
#elif defined(LITTLE_ENDIAN)
#define D_extend(w) {chcheck(12);*chptr++=((char *)&w)[0]; \
						*chptr++=((char *)&w)[1]; \
						*chptr++=((char *)&w)[2]; \
						*chptr++=((char *)&w)[3]; \
						*chptr++=((char *)&w)[4]; \
						*chptr++=((char *)&w)[5]; \
						*chptr++=((char *)&w)[6]; \
						*chptr++=((char *)&w)[7]; \
						*chptr++=((char *)&w)[8]; \
						*chptr++=((char *)&w)[9]; \
						*chptr++=((char *)&w)[10]; \
						*chptr++=((char *)&w)[11]; \
						sloc+=12; ch_size += 12; if(orgactive) orgaddr += 12;}

#else

#error Please implement a non-byte swapped D_extend!

#endif
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
#define FUMASK       0x000F		// Mask for fixup cases:
#define FU_QUICK     0x0000		// Fixup 3-bit quick instruction field
#define FU_BYTE      0x0001		// Fixup byte
#define FU_WORD      0x0002		// Fixup word
#define FU_WBYTE     0x0003		// Fixup byte (at loc+1)
#define FU_LONG      0x0004		// Fixup long
#define FU_BBRA      0x0005		// Fixup byte branch
#define FU_6BRA      0x0007		// Fixup 6502-format branch offset
#define FU_BYTEH     0x0008		// Fixup 6502 high byte of immediate word
#define FU_BYTEL     0x0009		// Fixup 6502 low byte of immediate word

#define FU_SEXT      0x0010		// Ok to sign extend
#define FU_PCREL     0x0020		// Subtract PC first
#define FU_EXPR      0x0040		// Expression (not symbol) follows

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

// FPU fixups
// TODO: these are obviously bogus for now!
#define FU_FLOATSING 0x0D0B		// Fixup 32-bit float
#define FU_FLOATDOUB 0x0E0B		// Fixup 64-bit float
#define FU_FLOATEXT  0x0F0B		// Fixup 96-bit float

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

// 680x0 defines
#define CPU_68000 1
#define CPU_68020 2
#define CPU_68030 4
#define CPU_68040 8
#define CPU_68060 16
#define FPU_NONE  0
#define FPU_68881 1
#define FPU_68882 2
#define FPU_68040 4

// Helper macros to test for active CPU
#define CHECK00 if (activecpu == CPU_68000) return error(unsupport)
#define CHECK20 if (activecpu == CPU_68020) return error(unsupport)
#define CHECK30 if (activecpu == CPU_68030) return error(unsupport)
#define CHECK40 if (activecpu == CPU_68040) return error(unsupport)
#define CHECK60 if (activecpu == CPU_68060) return error(unsupport)
#define CHECKNO00 if (activecpu != CPU_68000) return error(unsupport)
#define CHECKNO20 if (activecpu != CPU_68020) return error(unsupport)
#define CHECKNO30 if (activecpu != CPU_68030) return error(unsupport)
#define CHECKNO40 if (activecpu != CPU_68040) return error(unsupport)
#define CHECKNO60 if (activecpu != CPU_68060) return error(unsupport)


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
int AddFixup(uint16_t, uint32_t, TOKENPTR);
int ResolveAllFixups(void);

#endif // __SECT_H__

