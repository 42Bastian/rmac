//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// PROCLN.H - Line Processing
// Copyright (C) 199x Landon Dyer, 2017 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source utilised with the kind permission of Landon Dyer
//

#ifndef __PROCLN_H__
#define __PROCLN_H__

#include "rmac.h"
#include "token.h"

// Globals, externals etc
extern IFENT * ifent;
extern const char comma_error[];
extern const char locgl_error[];
extern const char syntax_error[];
extern const char extra_stuff[];
extern int just_bss;
extern VALUE pcloc;
extern SYM * lab_sym;
extern LONG amsktab[];

// Prototypes
void InitLineProcessor(void);
void Assemble(void);
int d_if(void);
int d_else(void);
int d_endif(void);

#endif // __PROCLN_H__
