//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// SYMBOL.C - Symbol Handling
// Copyright (C) 199x Landon Dyer, 2011-2012 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source Utilised with the Kind Permission of Landon Dyer
//

#include "symbol.h"
#include "listing.h"
#include "procln.h"
#include "error.h"


// Macros
#define NBUCKETS 256					// Number of hash buckets (power of 2)

static SYM * symbolTable[NBUCKETS];		// User symbol-table header
int curenv;								// Current enviroment number
static SYM * sorder;					// * -> Symbols, in order of reference
static SYM * sordtail;					// * -> Last symbol in sorder list
static SYM * sdecl;						// * -> Symbols, in order of declaration
static SYM * sdecltail;					// * -> Last symbol in sdecl list
static uint32_t currentUID;				// Symbol UID tracking (done by NewSymbol())

// Tags for marking symbol spaces
// a = absolute
// t = text
// d = data
// ! = "impossible!"
// b = BSS
static char tdb_text[8] = {
   'a', 't', 'd', '!', 'b', SPACE, SPACE, SPACE
};


//
// Initialize symbol table
//
void InitSymbolTable(void)
{
	int i;									// Iterator

	for(i=0; i<NBUCKETS; i++)				// Initialise symbol hash table
		symbolTable[i] = NULL;

	curenv = 1;								// Init local symbol enviroment
	sorder = NULL;							// Init symbol-reference list
	sordtail = NULL;
	sdecl = NULL;							// Init symbol-decl list
	sdecltail = NULL;
	currentUID = 0;
}


//
// Hash the print name and enviroment number
//
int HashSymbol(char * name, int envno)
{
	int sum, k = 0;							// Hash calculation

	for(sum=envno; *name; name++)
	{
		if (k++ == 1)
			sum += *name << 2;
		else
			sum += *name;
	}

	return sum & (NBUCKETS - 1);
}


//
// Make a new symbol of type `type' in enviroment `envno'
//
SYM * NewSymbol(char * name, int type, int envno)
{
	// Allocate the symbol
	SYM * symbol = malloc(sizeof(SYM));

	if (symbol == NULL)
	{
		printf("NewSymbol: MALLOC ERROR (symbol=\"%s\")\n", name);
		return NULL;
	}

	// Fill-in the symbol
	symbol->sname  = strdup(name);
	symbol->stype  = (BYTE)type;
	symbol->senv   = (WORD)envno;
	symbol->sattr  = 0;
	// Don't do this, it could be a forward reference!
//	symbol->sattr  = DEFINED;		// We just defined it...
	// This is a bad assumption. Not every symbol 1st seen in a RISC section is
	// a RISC symbol!
//	symbol->sattre = (rgpu || rdsp ? RISCSYM : 0);
	symbol->sattre = 0;
	symbol->svalue = 0;
	symbol->sorder = NULL;
	symbol->uid    = currentUID++;

	// Install symbol in symbol table
	int hash = HashSymbol(name, envno);
	symbol->snext = symbolTable[hash];
	symbolTable[hash] = symbol;

	// Append symbol to symbol-order list
	if (sorder == NULL)
		sorder = symbol;					// Add first symbol 
	else
		sordtail->sorder = symbol;			// Or append to tail of list

	sordtail = symbol;
	return symbol;
}


//
// Look up the symbol name by its UID and return the pointer to the name.
// If it's not found, return NULL.
//
char * GetSymbolNameByUID(uint32_t uid)
{
	//problem is with string lookup, that's why we're writing this
	//so once this is written, we can put the uid in the token stream

	// A much better approach to the symbol order list would be to make an
	// array--that way you can do away with the UIDs and all the rest, and
	// simply do an array lookup based on position. But meh, let's do this for
	// now until we can rewrite things so they make sense.
	SYM * symbol = sorder;

	for(; symbol; symbol=symbol->sorder)
	{
		if (symbol->uid == uid)
			return symbol->sname;
	}

	return NULL;
}


//
// Lookup the symbol `name', of the specified type, with the specified
// enviroment level
//
SYM * lookup(char * name, int type, int envno)
{
	SYM * symbol = symbolTable[HashSymbol(name, envno)];

	// Do linear-search for symbol in bucket
	while (symbol != NULL)
	{
		if (symbol->stype == type			// Type, envno and name must match
			&& symbol->senv  == envno
			&& *name == *symbol->sname		// Fast check for first character
			&& !strcmp(name, symbol->sname))	// More expensive check
			break;

		symbol = symbol->snext;
	}

	// Return NULL or matching symbol
	return symbol;
}


//
// Put symbol on "order-of-declaration" list of symbols
//
//void sym_decl(SYM * symbol)
void AddToSymbolOrderList(SYM * symbol)
{
	if (symbol->sattr & SDECLLIST)
		return;								// Already on list

	symbol->sattr |= SDECLLIST;				// Mark "already on list"

	if (sdecl == NULL)
		sdecl = symbol;						// First on decl-list
	else 
		sdecltail->sdecl = symbol;			// Add to end of list

	symbol->sdecl = NULL;					// Fix up list's tail
	sdecltail = symbol;
}


//
// Make all referenced, undefined symbols global
//
void ForceUndefinedSymbolsGlobal(void)
{
	SYM * sy;

	DEBUG printf("~ForceUndefinedSymbolsGlobal()\n");

	// Scan through all symbols;
	// If a symbol is REFERENCED but not DEFINED, then make it global.
	for(sy=sorder; sy!=NULL; sy=sy->sorder)
	{
		if (sy->stype == LABEL && sy->senv == 0
			&& ((sy->sattr & (REFERENCED | DEFINED)) == REFERENCED))
			sy->sattr |= GLOBAL;
	}
}


//
// Convert string to uppercase
//
int uc_string(char * s)
{
	for(; *s; s++)
	{
		if (*s >= 'a' && *s <= 'z')
			*s -= 32;
	}

	return 0;
}


//
// Assign numbers to symbols that are to be exported or imported. The symbol
// number is put in `.senv'. Return the number of symbols that will be in the
// symbol table.
//
int sy_assign(char * buf, char *(* constr)())
{
	SYM * sy;
	int scount = 0;

	if (buf == NULL)
	{
		// Append all symbols not appearing on the .sdecl list to the end of
		// the .sdecl list
		for(sy=sorder; sy!=NULL; sy=sy->sorder)
		{
			// Essentially the same as 'sym_decl()' above:
			if (sy->sattr & SDECLLIST)
				continue;				// Already on list 

			sy->sattr |= SDECLLIST;		// Mark "on the list"

			if (sdecl == NULL)
				sdecl = sy;				// First on decl-list 
			else
				sdecltail->sdecl = sy;	// Add to end of list

			sy->sdecl = NULL;			// Fix up list's tail
			sdecltail = sy;
		}
	}

	// Run through all symbols (now on the .sdecl list) and assign numbers to
	// them. We also pick which symbols should be global or not here.
	for(sy=sdecl; sy!=NULL; sy=sy->sdecl)
	{
		if (sy->sattre & UNDEF_EQUR)
			continue;                 // Don't want undefined on our list

		if (sy->sattre & UNDEF_CC)
			continue;                   
		
		// Export or import external references, and export COMMON blocks.
		if ((sy->stype == LABEL)
			&& ((sy->sattr & (GLOBAL | DEFINED)) == (GLOBAL | DEFINED)
			|| (sy->sattr & (GLOBAL | REFERENCED)) == (GLOBAL | REFERENCED))
			|| (sy->sattr & COMMON))
		{
			sy->senv = (WORD)scount++;

			if (buf != NULL)
				buf = (*constr)(buf, sy, 1);
		}
		// Export vanilla labels (but don't make them global). An exception is
		// made for equates, which are not exported unless they are referenced.
		else if (sy->stype == LABEL && lsym_flag
			&& (sy->sattr & (DEFINED | REFERENCED)) != 0
			&& (!as68_flag || *sy->sname != 'L'))
		{
			sy->senv = (WORD)scount++;
			if (buf != NULL) buf = (*constr)(buf, sy, 0);
		}
	}

	return scount;
}


//
// Generate symbol table for listing file
//
int symtable(void)
{
	int i;
	int j;
	SYM * q = NULL;
	SYM * p;
	SYM * r;
	SYM * k;
	SYM ** sy;
	SYM * colptr[4];
	char ln[150];
	char ln1[150];
	char ln2[20];
	char c, c1;
	WORD w;
	int ww;
	int colhei;
	extern int pagelen;

	colhei = pagelen - 5;

	// Allocate storage for list headers and partition all labels.  
	// Throw away macros and macro arguments.
	sy = (SYM **)malloc(128 * sizeof(LONG));

	for(i=0; i<128; ++i)
		sy[i] = NULL;

	for(i=0; i<NBUCKETS; ++i)
	{
		for(p=symbolTable[i]; p!=NULL; p=k)
		{
			k = p->snext;
			j = *p->sname;
			r = NULL;

			// Ignore non-labels
			if ((p->stype != LABEL) || (p->sattre & UNDEF_EQUR))
				continue;

			for(q=sy[j]; q!=NULL; q=q->snext)
			{
				if (strcmp(p->sname, q->sname) < 0)
					break;

				r = q;
			}

			if (r == NULL)
			{
				// Insert at front of list
				p->snext = sy[j];
				sy[j] = p;
			}
			else
			{
				// Insert in middle or append to list
				p->snext = r->snext;
				r->snext = p;
			}
		}
	}

	// Link all symbols onto one list again
	p = NULL;

	for(i=0; i<128; ++i)
	{
		if ((r = sy[i]) != NULL)
		{
			if (p == NULL)
				q = r;
			else
				q->snext = r;

			while (q->snext != NULL)
				q = q->snext;

			if (p == NULL)
				p = r;
		}
	}

	eject();
	strcpy(subttl, "Symbol Table");

	while (p != NULL)
	{
		for(i=0; i<4; ++i)
		{
			colptr[i] = p;

			for(j=0; j<colhei; ++j)
			{
				if (p == NULL)
					break;
				else
					p = p->snext;
			}
		}

		for(i=0; i<colhei; ++i)
		{
			*ln = EOS;

			if (colptr[0] == NULL)
				break;

			for(j=0; j<4; ++j)
			{
				if ((q = colptr[j]) == NULL)
					break;

				colptr[j] = q->snext;
				w = q->sattr;
				ww = q->sattre;
				// Pick a tag:
				// c	common
				// x	external reference
				// g	global (export)
				// space	nothing special
				c1 = SPACE;
				c = SPACE;

				if (w & COMMON)
					c = 'c';
				else if ((w & (DEFINED|GLOBAL)) == GLOBAL)
					c = 'x';
				else if (w & GLOBAL)
					c = 'g';

				c1 = tdb_text[w & TDB];

				if (c == 'x')
					strcpy(ln2, "external");
				else
				{
					sprintf(ln2, "%08X", q->svalue);
					uc_string(ln2);
				}

				sprintf(ln1, "  %16s %s %c%c%c", q->sname, ln2, (ww & EQUATEDREG) ? 'e' : SPACE, c1, c);
				strcat(ln, ln1);
			}

			ship_ln(ln);
		}

		eject();
	}

	return 0;
}

