//
// RMAC - Reboot's Macro Assembler for all Atari computers
// MACRO.H - Macro Definition and Invocation
// Copyright (C) 199x Landon Dyer, 2011-2020 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source utilised with the kind permission of Landon Dyer
//

#ifndef __MACRO_H__
#define __MACRO_H__

#include "rmac.h"

// Exported variables
extern LONG curuniq;
extern TOKEN * argPtrs[];

// Exported functions
void InitMacro(void);
int ExitMacro(void);
int DefineMacro(void);
int HandleRept(void);
int InvokeMacro(SYM *, WORD);

#endif // __MACRO_H__
