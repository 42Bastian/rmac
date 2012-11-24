//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// SYMBOL.H - Symbol Handling
// Copyright (C) 199x Landon Dyer, 2011 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source Utilised with the Kind Permission of Landon Dyer
//

#ifndef __SYMBOL_H__
#define __SYMBOL_H__

#include <inttypes.h>

// Line lists
struct LineList
{
	struct LineList * next;
	char * line;
};

// Symbols
#define SYM struct _sym
SYM
{
	SYM * snext;					// * -> Next symbol on hash-chain
	SYM * sorder;					// * -> Next sym in order of reference
	SYM * sdecl;					// * -> Next sym in order of declaration
	uint8_t stype;					// Symbol type 
	uint16_t sattr;					// Attribute bits
	uint32_t sattre;				// Extended attribute bits
	uint16_t senv;					// Enviroment number
	uint32_t svalue;				// Symbol value
	char * sname;					// * -> Symbol's print-name
	struct LineList * lineList;		// * -> Macro's linked list of lines
	struct LineList * last;			// * -> end of macro linked list
};

// Globals, externals etc
extern int curenv;
extern char subttl[];

// Prototypes
SYM * lookup(char *, int, int);
void InitSymbolTable(void);
SYM * NewSymbol(char *, int, int);
void sym_decl(SYM *);
int syg_fix(void);
int symtable(void);
int sy_assign(char *, char *(*)());

#endif // __SYMBOL_H__
