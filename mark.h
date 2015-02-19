//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// MARK.H - A record of things that are defined relative to any of the sections
// Copyright (C) 199x Landon Dyer, 2011 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source utilised with the kind permission of Landon Dyer
//

#ifndef __MARK_H__
#define __MARK_H__

#include "rmac.h"
#include "sect.h"

#define MARK_ALLOC_INCR 1024		// # bytes to alloc for more mark space 
#define MIN_MARK_MEM    (3 * sizeof(WORD) + 2 * sizeof(LONG))

// Globals, externals, etc.
extern MCHUNK * firstmch;

// Exported functions
void InitMark(void);
void StopMark(void);
int rmark(uint16_t, uint32_t, uint16_t, uint16_t, SYM *);
int amark(void);
LONG bsdmarkimg(char *, LONG, LONG, int);

#endif // __MARK_H__

