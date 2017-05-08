//
// RMAC - Reboot's Macro Assembler for all Atari computers
// SYMBOL.H - Symbol Handling
// Copyright (C) 199x Landon Dyer, 2011-2017 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source utilised with the kind permission of Landon Dyer
//

#ifndef __SYMBOL_H__
#define __SYMBOL_H__

#include <inttypes.h>

// Line lists
struct LineList
{
	struct LineList * next;
	uint8_t * line;
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
	uint8_t * sname;				// * -> Symbol's print-name
	struct LineList * lineList;		// * -> Macro's linked list of lines
	struct LineList * last;			// * -> end of macro linked list
	uint32_t uid;					// Symbol's unique ID
};

// Exported variables
extern int curenv;
extern uint32_t firstglobal;// Index of the fist global symbol in an ELF object.

// Exported functions
SYM * lookup(uint8_t *, int, int);
void InitSymbolTable(void);
SYM * NewSymbol(uint8_t *, int, int);
void AddToSymbolDeclarationList(SYM *);
void ForceUndefinedSymbolsGlobal(void);
int symtable(void);
uint32_t sy_assign(uint8_t *, uint8_t *(*)());
uint32_t sy_assign_ELF(uint8_t *, uint8_t *(*)());
uint8_t * GetSymbolNameByUID(uint32_t);

#endif // __SYMBOL_H__

