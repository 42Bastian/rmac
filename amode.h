//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// AMODE.H - Addressing Modes
// Copyright (C) 199x Landon Dyer, 2017 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source utilised with the kind permission of Landon Dyer
//

#ifndef __AMODE_H__
#define __AMODE_H__

#include "rmac.h"

// 68000 and 68020 addressing modes
#define DREG         000				// Dn
#define AREG         010				// An
#define AIND         020				// (An)
#define APOSTINC     030				// (An)+
#define APREDEC      040				// -(An)
#define ADISP        050				// (d16,An) d16(An)
#define AINDEXED     060				// (d8,An,Xn) d8(An,Xn)
#define ABSW         070				// xxx.W
#define ABSL         071				// xxx or xxx.L
#define PCDISP       072				// (d16,PC) d16(PC)
#define PCINDEXED    073				// (d16,PC,Xn) d16(PC,Xn)
#define IMMED        074				// #data
#define ABASE        0100				// (bd,An,Xn)
#define MEMPOST      0101				// ([bd,An],Xn,od)
#define MEMPRE       0102				// ([bc,An,Xn],od)
#define PCBASE       0103				// (bd,PC,Xn)
#define PCMPOST      0104				// ([bd,PC],Xn,od)
#define PCMPRE       0105				// ([bc,PC,Xn],od)
#define AM_USP       0106
#define AM_SR        0107
#define AM_CCR       0110
#define AM_NONE      0111				// Nothing

// Addressing-mode masks
#define M_DREG       0x00000001L		// Dn
#define M_AREG       0x00000002L		// An
#define M_AIND       0x00000004L		// (An)
#define M_APOSTINC   0x00000008L		// (An)+
#define M_APREDEC    0x00000010L		// -(An)
#define M_ADISP      0x00000020L		// (d16,An) d16(An)
#define M_AINDEXED   0x00000040L		// (d8,An,Xn) d8(An,Xn)
#define M_ABSW       0x00000080L		// xxx.W
#define M_ABSL       0x00000100L		// xxx or xxx.L
#define M_PCDISP     0x00000200L		// (d16,PC) d16(PC)
#define M_PCINDEXED  0x00000400L		// (d16,PC,Xn) d16(PC,Xn)
#define M_IMMED      0x00000800L		// #data
#define M_ABASE      0x00001000L		// (bd,An,Xn)
#define M_MEMPOST    0x00002000L		// ([bd,An],Xn,od)
#define M_MEMPRE     0x00004000L		// ([bc,An,Xn],od)
#define M_PCBASE     0x00008000L		// (bd,PC,Xn)
#define M_PCMPOST    0x00010000L		// ([bd,PC],Xn,od)
#define M_PCMPRE     0x00020000L		// ([bc,PC,Xn],od)
#define M_AM_USP     0x00040000L		// USP
#define M_AM_SR      0x00080000L		// SR
#define M_AM_CCR     0x00100000L		// CCR
#define M_AM_NONE    0x00200000L		// (nothing)

// Addr mode categories
#define C_ALL        0x00000FFFL
#define C_DATA       0x00000FFDL
#define C_MEM        0x00000FFCL
#define C_CTRL       0x000007E4L
#define C_ALT        0x000001FFL

#define C_ALTDATA    (C_DATA&C_ALT)
#define C_ALTMEM     (C_MEM&C_ALT)
#define C_ALTCTRL    (C_CTRL&C_ALT)
#define C_LABEL      (M_ABSW|M_ABSL)
#define C_NONE       M_AM_NONE

// Scales
#define TIMES1       00000				// (empty or *1)
#define TIMES2       01000				// *2
#define TIMES4       02000				// *4
#define TIMES8       03000				// *8

#define EXPRSIZE     128				// Maximum #tokens in an expression

// Addressing mode variables, output of amode()
extern int nmodes;
extern int am0, am1;
extern int a0reg, a1reg;
extern TOKEN a0expr[], a1expr[];
extern VALUE a0exval, a1exval;
extern WORD a0exattr, a1exattr;
extern int a0ixreg, a1ixreg;
extern int a0ixsiz, a1ixsiz;
extern TOKEN a0oexpr[], a1oexpr[];
extern VALUE a0oexval, a1oexval;
extern WORD a0oexattr, a1oexattr;
extern SYM * a0esym, * a1esym;

// Mnemonic table structure
#define MNTAB  struct _mntab
MNTAB {
   WORD mnattr;							// Attributes (CGSPECIAL, SIZN, ...)
   LONG mn0, mn1;						// Addressing modes
   WORD mninst;							// Instruction mask
   WORD mncont;							// Continuation (or -1)
   int (*mnfunc)(WORD, WORD);			// Mnemonic builder
};

// mnattr:
#define CGSPECIAL    0x8000				// Special (don't parse addr modes)

// Prototypes
int amode(int);
int reglist(WORD *);

#endif // __AMODE_H__

