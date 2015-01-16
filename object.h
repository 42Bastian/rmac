//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// OBJECT.H - Writing Object Files
// Copyright (C) 199x Landon Dyer, 2011 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source Utilised with the Kind Permission of Landon Dyer
//

#ifndef __OBJECT_H__
#define __OBJECT_H__

// Size of BSD header
#define BSDHDRSIZE   0x20

// Globals, externals, etc.
extern char * objImage;

// Exported functions
int WriteObject(int);

#endif // __OBJECT_H__

