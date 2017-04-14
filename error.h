//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// ERROR.H - Error Handling
// Copyright (C) 199x Landon Dyer, 2017 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source utilised with the kind permission of Landon Dyer
//

#ifndef __ERROR_H__
#define __ERROR_H__

#include "rmac.h"

#define EBUFSIZ         200                                 // Max size of an error message

// Globals, externals etc
extern int errcnt;
extern char * err_fname;

// Prototypes
int error(const char *);
int errors(const char *, char *);
int fatal(const char *);
int warn(const char *);
int warns(const char *, char *);
int warni(const char *, unsigned);
int interror(int);
void cantcreat(const char *);
void err_setup(void);
int at_eol(void);

#endif // __ERROR_H__
