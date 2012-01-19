//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// SECT.H - Code Generation, Fixups and Section Management
// Copyright (C) 199x Landon Dyer, 2011 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source Utilised with the Kind Permission of Landon Dyer
//

#ifndef __SECT_H__
#define __SECT_H__

#include "rmac.h"

// Macros to deposit code in the current section
// D_rword deposits a "6502" format (low, high) word (01).
// D_rlong deposits a MWC "canonical byte order" longword (2301).
#define D_byte(b)    {*chptr++=(char)b; ++sloc; ++ch_size; if(orgactive) ++orgaddr;}
#define D_word(w)	   {chcheck(2);*chptr++=(char)(w>>8); *chptr++=(char)w; \
                      sloc+=2; ch_size+=2; if(orgactive) orgaddr += 2;}
#define D_long(lw)   {*chptr++=(char)(lw>>24); *chptr++=(char)(lw>>16);\
	                   *chptr++=(char)(lw>>8); *chptr++=(char)lw; \
                      sloc+=4; ch_size += 4; if(orgactive) orgaddr += 4;}
#define D_rword(w)   {*chptr++=(char)w; *chptr++=(char)(w>>8); \
                      sloc+=2; ch_size+=2;if(orgactive) orgaddr += 2;}
#define D_rlong(lw)  {*chptr++=(char)(lw>>16);*chptr++=(char)(lw>>24);\
                      *chptr++=(char)lw;*chptr++=(char)(lw>>8); \
                      sloc+=4; ch_size += 4;if(orgactive) orgaddr += 4;}

#define NSECTS       16                                     // Max. number of sections

// Tunable (storage) definitions
#define CH_THRESHOLD    64                                  // Minimum amount of space in code chunk
#define CH_CODE_SIZE    2048                                // Code chunk normal allocation
#define CH_FIXUP_SIZE   1024                                // Fixup chunk normal allocation

// Section attributes (.scattr)
#define SUSED        0x8000                                 // Section is used (really, valid)
#define SBSS         0x4000                                 // Section can contain no data
#define SABS         0x2000                                 // Section is absolute
#define SPIC         0x1000                                 // Section is position-independent code

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
#define FUMASK       007                                    // Mask for fixup cases:
#define FU_QUICK     000                                    // Fixup 3-bit quick instr field
#define FU_BYTE      001                                    // Fixup byte
#define FU_WORD      002                                    // Fixup word
#define FU_WBYTE     003                                    // Fixup byte (at loc+1)
#define FU_LONG      004                                    // Fixup long
#define FU_BBRA      005                                    // Fixup byte branch
#define FU_6BRA      007                                    // Fixup 6502-format branch offset
#define FU_SEXT      010                                    // Ok to sign extend
#define FU_PCREL     020                                    // Subtract PC first
#define FU_EXPR      040                                    // Expression (not symbol) follows

#define FU_MOVEI     0x0100
#define FU_JR        0x0200
#define FU_MJR       0x0300
#define FU_REGONE    0x0400
#define FU_NUM15     0x0500
#define FU_NUM31     0x0600
#define FU_NUM32     0x0700
#define FU_REGTWO    0x0800
#define FU_SUB32     0x1000
#define FU_ISBRA     0x2000                                 // Word forward fixup is a BRA or DBRA
#define FU_LBRA      0x4000                                 // Long branch, for short branch detect
#define FU_DONE      0x8000                                 // Fixup has been done

// Chunks are used to hold generated code and fixup records
#define CHUNK  struct _chunk
CHUNK {
   CHUNK * chnext;                                          // Next, previous chunks in section
   CHUNK * chprev;
   LONG chloc;                                              // Base addr of this chunk
   LONG challoc;                                            // #bytes allocated for chunk
   LONG ch_size;                                            // #bytes chunk actually uses
   char * chptr;                                            // Data for this chunk
};

// Section descriptor
#define SECT   struct _sect
SECT {
   WORD scattr;                                             // Section attributes
   LONG sloc;                                               // Current loc-in / size-of section 
   CHUNK * sfcode;                                          // First chunk in section
   CHUNK * scode;                                           // Last chunk in section
   CHUNK * sffix;                                           // First fixup chunk
   CHUNK * sfix;                                            // Last fixup chunk
};

// A mark is of the form:
// .W    <to+flags>	section mark is relative to, and flags in upper byte
// .L    <loc>		location of mark in "from" section
// .W    [from]		new from section
// .L    [symbol]	symbol involved in external reference
#define MCHUNK struct _mchunk
MCHUNK {
   MCHUNK * mcnext;                                         // Next mark chunk
   PTR mcptr;                                               // Vector of marks
   LONG mcalloc;                                            // #marks allocted to mark block
   LONG mcused;                                             // #marks used in block
};

#define MWORD        0x0000                                 // Marked word
#define MLONG        0x0100                                 // Marked long 
#define MMOVEI       0x0200
#define MCHFROM      0x8000                                 // Mark includes change-to-from
#define MSYMBOL      0x4000                                 // Mark includes symbol number
#define MCHEND       0x2000                                 // Indicates end of mark chunk
#define MPCREL       0x1000                                 // Mark is PC-relative

#define MAXFWDJUMPS  1024                                   // Maximum forward jumps to check
extern unsigned fwdjump[MAXFWDJUMPS];
extern unsigned fwindex;

// Globals, external etc
extern LONG sloc;
extern WORD scattr;
extern char * chptr;
extern LONG ch_size;
extern int cursect;
extern SECT sect[];
extern LONG challoc;
extern CHUNK * scode;

// Prototypes
void init_sect(void);
void switchsect(int);
void savsect(void);
int fixtest(int, LONG);
int chcheck(LONG);
int fixup(WORD, LONG, TOKEN *);
int fixups(void);
int resfix(int);

#endif // __SECT_H__
