//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// DEBUG.H - Debugging Messages
// Copyright (C) 199x Landon Dyer, 2011 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source utilised with the kind permission of Landon Dyer
//

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "rmac.h"

// Prototypes
int mudump(void);
int mdump(char *, LONG, int, LONG);
int dumptok(TOKEN *);
int dump_everything(void);

#endif // __DEBUG_H__
