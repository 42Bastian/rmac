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

static SYM * sytab[NBUCKETS];                               // User symbol-table header
int curenv;                                                 // Current enviroment number
SYM * sorder;                                               // * -> Symbols, in order of reference
SYM * sordtail;                                             // * -> Last symbol in sorder list
SYM * sdecl;                                                // * -> Symbols, in order of declaration
SYM * sdecltail;                                            // * -> Last symbol in sdecl list

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
// Initialize Symbol Table
//
void init_sym(void)
{
	int i;                                                   // Iterator

	for(i=0; i<NBUCKETS; ++i)                            // Initialise symbol hash table
		sytab[i] = NULL;

	curenv = 1;                                              // Init local symbol enviroment
	sorder = NULL;                                           // Init symbol-reference list
	sordtail = NULL;
	sdecl = NULL;                                            // Init symbol-decl list
	sdecltail = NULL;
}


//
// Allocate and Return Pointer to a Copy of a String
//
char * nstring(char * str)
{
	long i;
	char * s, * d;

	for(i=0; str[i]; ++i)
		;

	s = d = amem(i + 1);

	while (*str)
		*d++ = *str++;

	*d++ = '\0';
	
	return s;
}


//
// Hash the Print Name and Enviroment Number
//
int syhash(char * name, int envno)
{
	int sum, k;                                              // Hash calculation
	k = 0;

	for(sum=envno; *name; ++name)
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
SYM * newsym(char * name, int type, int envno)
{
	int hash;                                                // Symbol hash value
	SYM * sy;                                                // Pointer to symbol

	// Allocate the symbol
	sy = (SYM *)amem((long)(sizeof(SYM)));

	if (sy == NULL)
	{
		printf("SYMALLOC ERROR (%s)\n", name);
		return NULL;
	}

	sy->sname = nstring(name);

	// Fill-in the symbol
	sy->stype  = (BYTE)type;
	sy->senv   = (WORD)envno;
	sy->sattr  = 0;

	if (rgpu || rdsp)
		sy->sattre = RISCSYM;
	else
		sy->sattre = 0;

	sy->svalue = 0;

	// Install symbol in symbol table
	hash = syhash(name, envno);
	sy->snext = sytab[hash];
	sytab[hash] = sy;

	// Append symbol to symbol-order list
	if (sorder == NULL)
		sorder = sy;                                          // Add first symbol 
	else
		sordtail->sorder = sy;                                // Or append to tail of list

	sy->sorder = NULL;
	sordtail = sy;

	return sy;                                              // Return pointer to symbol
}


//
// Lookup the symbol `name', of the specified type, with the specified
// enviroment level
//
SYM * lookup(char * name, int type, int envno)
{
	SYM * sy;                                                // Symbol record pointer
	int k, sum;                                              // Hash bucket calculation
	char * s;                                                // String pointer

	// Pick a hash-bucket (SAME algorithm as syhash())
	k = 0;
	s = name;

	for(sum=envno; *s;)
	{
		if (k++ == 1)
			sum += *s++ << 2;
		else
			sum += *s++;
	}

	sy = sytab[sum & (NBUCKETS-1)];

	// Do linear-search for symbol in bucket
	while (sy != NULL)
	{
		if (sy->stype == type &&                         // Type, envno and name must match
			sy->senv  == envno &&
			*name == *sy->sname &&                         // Fast check for first character
			!strcmp(name, sy->sname))
			break;
		else
			sy = sy->snext;
	}

	return sy;                                              // Return NULL or matching symbol
}


//
// Put symbol on "order-of-declaration" list of symbols
//
void sym_decl(SYM * sym)
{
	if (sym->sattr & SDECLLIST)
		return;								// Already on list

	sym->sattr |= SDECLLIST;				// Mark "already on list"

	if (sdecl == NULL)
		sdecl = sym;						// First on decl-list
	else 
		sdecltail->sdecl = sym;				// Add to end of list

	sym->sdecl = NULL;						// Fix up list's tail
	sdecltail = sym;
}


//
// Make all referenced, undefined symbols global
//
int syg_fix(void)
{
	SYM * sy;

	DEBUG printf("~syg_fix()\n");

	// Scan through all symbols;
	// If a symbol is REFERENCED but not DEFINED, then make it global.
	for(sy = sorder; sy != NULL; sy = sy->sorder)
	{
		if (sy->stype == LABEL && sy->senv == 0
			&& ((sy->sattr & (REFERENCED|DEFINED)) == REFERENCED))
			sy->sattr |= GLOBAL;
	}

	return 0;
}


//
// Convert string to uppercase
//
int uc_string(char * s)
{
	for(; *s; ++s)
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
int sy_assign(char * buf, char *(*constr)())
{
	SYM * sy;
	int scount;
	//int i;

	scount = 0;

	if (buf == NULL)
	{
		// Append all symbols not appearing on the .sdecl list to the end of
		// the .sdecl list
		for(sy=sorder; sy!=NULL; sy=sy->sorder)
		{
			// Essentially the same as 'sym_decl()' above:
			if (sy->sattr & SDECLLIST)
				continue;                // Already on list 

			sy->sattr |= SDECLLIST;                            // Mark "on the list"

			if (sdecl == NULL)
				sdecl = sy;                      // First on decl-list 
			else
				sdecltail->sdecl = sy;                        // Add to end of list

			sy->sdecl = NULL;                                  // Fix up list's tail
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
			&& ((sy->sattr & (GLOBAL|DEFINED)) == (GLOBAL|DEFINED)
			|| (sy->sattr & (GLOBAL|REFERENCED)) == (GLOBAL|REFERENCED))
			|| (sy->sattr & COMMON))
		{
			sy->senv = (WORD)scount++;

			if (buf != NULL)
				buf = (*constr)(buf, sy, 1);
		}
		// Export vanilla labels (but don't make them global). An exception is
		// made for equates, which are not exported unless they are referenced.
		else if (sy->stype == LABEL && lsym_flag
			&& (sy->sattr & (DEFINED|REFERENCED)) != 0
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
	sy = (SYM **)amem((LONG)(128 * sizeof(LONG)));

	for(i=0; i<128; ++i)
		sy[i] = NULL;

	for(i=0; i<NBUCKETS; ++i)
	{
		for(p=sytab[i]; p!=NULL; p=k)
		{
			k = p->snext;
			j = *p->sname;
			r = NULL;

			if (p->stype != LABEL)
				continue;                    // Ignore non-labels

			if (p->sattre & UNDEF_EQUR)
				continue;

			for(q=sy[j]; q!=NULL; q=q->snext)
			{
				if (strcmp(p->sname, q->sname) < 0)
					break;
				else
					r = q;
			}

			if (r == NULL)
			{                                    // Insert at front of list
				p->snext = sy[j];
				sy[j] = p;
			}
			else
			{                                           // Insert in middle or append to list
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
					sprintf(ln2, "%08ux", q->svalue);
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
