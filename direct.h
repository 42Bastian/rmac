//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// DIRECT.H - Directive Handling
// Copyright (C) 199x Landon Dyer, 2011 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source utilised with the kind permission of Landon Dyer
//

#ifndef __DIRECT_H__
#define __DIRECT_H__

#include "rmac.h"

// Globals, Externals etc
extern TOKEN exprbuf[];
extern SYM * symbolPtr[];
extern int (* dirtab[])();

// Prototypes
int d_even(void);
void auto_even(void);
int dep_block(VALUE, WORD, VALUE, WORD, TOKEN *);
int d_unimpl(void);
int d_even(void);
int d_6502(void);
int d_68000(void);
int d_bss(void);
int d_data(void);
int d_text(void);
int d_abs(void);
int d_comm(void);
int d_dc(WORD);
int d_ds(WORD);
int d_dcb(WORD);
int d_globl(void);
int d_gpu(void);
int d_dsp(void);
int d_assert(void);
int d_if(void);
int d_endif(void);
int d_include(void);
int ExitMacro(void);
int d_list(void);
int d_nlist(void);
int d_title(void);
int d_subttl(void);  
int eject(void);
int d_error(char *);
int d_warn(char *);
int d_org(void);
int d_init(WORD);
int d_cargs(void);
int d_undmac(void);
int d_regbank0(void);
int d_regbank1(void);
int d_long(void);
int d_phrase(void);
int d_dphrase(void);
int d_qphrase(void);
int d_incbin(void);
int d_noclear(void);
int d_equrundef(void);
int d_ccundef(void);
int d_print(void);
int d_gpumain(void);
int d_jpad(void);
int d_nojpad(void);
int d_fail(void);
int symlist(int(*)());
int abs_expr(VALUE *);
int d_cstruct(void);
int d_jpad(void);
int d_nojpad(void);
int d_gpumain(void);
int d_prgflags(void);

#endif // __DIRECT_H__
