//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// MACH.H - Code Generation
// Copyright (C) 199x Landon Dyer, 2017 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source utilised with the kind permission of Landon Dyer
//

#ifndef __MACH_H__
#define __MACH_H__

#include "rmac.h"
#include "amode.h"

// Exported variables
extern char seg_error[];
extern char undef_error[];
extern char rel_error[];
extern char range_error[];
extern char abs_error[];
extern MNTAB machtab[];

#endif // __MACH_H__

