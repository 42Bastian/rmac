//
// RMAC - Reboot's Macro Assembler for all Atari computers
// DIRECT.H - Directive Handling
// Copyright (C) 199x Landon Dyer, 2011-2017 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source utilised with the kind permission of Landon Dyer
//

#ifndef __DIRECT_H__
#define __DIRECT_H__

#include "rmac.h"

// Exported variables
extern TOKEN exprbuf[];
extern SYM * symbolPtr[];
extern int (* dirtab[])();

// Exported functions
void auto_even(void);
int dep_block(VALUE, WORD, VALUE, WORD, TOKEN *);
int eject(void);
int abs_expr(VALUE *);
int symlist(int(*)());

int d_even(void);
int d_long(void);
int d_phrase(void);
int d_dphrase(void);
int d_qphrase(void);

#endif // __DIRECT_H__

