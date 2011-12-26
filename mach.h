//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// MACH.H - Code Generation
// Copyright (C) 199x Landon Dyer, 2011 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source Utilised with the Kind Permission of Landon Dyer
//

#ifndef __MACH_H__
#define __MACH_H__

#include "rmac.h"
#include "amode.h"

// Globals, Externals etc
extern char *seg_error;
extern char *undef_error;
extern char *rel_error;
extern char *range_error;
extern char *abs_error;
extern MNTAB machtab[];

// Prototypes 
int m_unimp(), m_badmode(), m_bad6mode(), m_bad6inst();
int m_self(WORD);
int m_abcd(WORD, WORD);
int m_reg(WORD, WORD);
int m_imm(WORD, WORD);
int m_imm8(WORD, WORD);
int m_shi(WORD, WORD);
int m_shr(WORD, WORD);
int m_bitop(WORD, WORD);
int m_exg(WORD, WORD);
int m_ea(WORD, WORD);
int m_br(WORD, WORD);
int m_dbra(WORD, WORD);
int m_link(WORD, WORD);
int m_adda(WORD, WORD);
int m_addq(WORD, WORD);
int m_move(WORD, int);
int m_moveq(WORD, WORD);
int m_usp(WORD, WORD);
int m_movep(WORD, WORD);
int m_trap(WORD, WORD);
int m_movem(WORD, WORD);
int m_clra(WORD, WORD);

#endif // __MACH_H__
