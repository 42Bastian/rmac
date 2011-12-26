//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// ERROR.H - Error Handling
// Copyright (C) 199x Landon Dyer, 2011 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source Utilised with the Kind Permission of Landon Dyer
//

#ifndef __ERROR_H__
#define __ERROR_H__

#include "rmac.h"

#define EBUFSIZ         200                                 // Max size of an error message

// Globals, externals etc
extern int errcnt;
extern char *err_fname;

// Prototypes
int error(char *);
int errors(char *, char *);
int fatal(char *);
int warn(char *);
int warns(char *, char *);
int warni(char *, unsigned);
int interror(int);
void cantcreat(char *);
void err_setup(void);

#endif // __ERROR_H__
