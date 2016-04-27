//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// OBJECT.C - Writing Object Files
// Copyright (C) 199x Landon Dyer, 2011 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source utilised with the kind permission of Landon Dyer
//

#include "object.h"
#include "sect.h"
#include "symbol.h"
#include "mark.h"
#include "error.h"
#include "riscasm.h"
#include "mark.h"

/*
 *  Imports
 */
extern int obj_format;		// object file format to write
extern int prg_flag;		// generate Atari ST direct executable

LONG symsize = 0;			// Size of BSD symbol table
LONG strindx = 0x00000004;	// BSD string table index
char * strtable;			// Pointer to the symbol string table
char * objImage;			// Global object image pointer


//
// Add an entry to the BSD symbol table
//
char * constr_bsdsymtab(char * buf, SYM * sym, int globflag)
{
	chptr = buf;						// Point to buffer for deposit longs
	D_long(strindx);					// Deposit the symbol string index

	WORD w1 = sym->sattr;				// Obtain symbol attribute
	int w2 = sym->sattre;
	LONG z = 0;							// Initialise resulting symbol flags

	if (w1 & EQUATED)
	{                                       
		z = 0x02000000;					// Set equated flag
	}
	else
	{
		switch (w1 & TDB)
		{
		case TEXT: z = 0x04000000; break;	// Set TEXT segment flag
		case DATA: z = 0x06000000; break;	// Set DATA segment flag
		case BSS : z = 0x08000000; break;	// Set BSS segment flag
		}
	}

	if (globflag)
		z |= 0x01000000;				// Set global flag if requested

	D_long(z);							// Deposit symbol attribute

	z = sym->svalue;					// Obtain symbol value
	w1 &= DATA | BSS;					// Determine DATA or BSS flag

	if (w1)                                                   
		z += sect[TEXT].sloc;			// If DATA or BSS add TEXT segment size

	if (w1 & BSS) 
		z += sect[DATA].sloc;			// If BSS add DATA segment size

	D_long(z);							// Deposit symbol value

	strcpy(strtable + strindx, sym->sname);

	strindx += strlen(sym->sname) + 1;	// Incr string index incl null terminate
	buf += 12;							// Increment buffer to next record
	symsize += 12;						// Increment symbol table size

	return buf;
}


/*
 *  Alcyon symbol flags
 */
#define	AL_DEFINED	0x8000
#define	AL_EQUATED	0x4000
#define	AL_GLOBAL	0x2000
#define	AL_EQUREG	0x1000
#define	AL_EXTERN	0x0800
#define	AL_DATA		0x0400
#define	AL_TEXT		0x0200
#define	AL_BSS		0x0100
#define	AL_FILE		0x0080

LONG PRGFLAGS;		/* PRGFLAGS as defined in Atari Compendium Chapter 2 */
					/* Definition	Bit(s) 	Meaning */
					/* PF_FASTLOAD	0 		If set, clear only the BSS area on program load,              */
					/* 						otherwise clear the entire heap.                              */
					/* PF_TTRAMLOAD	1 		If set, the program may be loaded into alternative RAM,       */
					/* 						otherwise it must be loaded into standard RAM.                */
					/* PF_TTRAMMEM	2 		If set, the program's Malloc() requests may be satisfied      */
					/* 						from alternative RAM, otherwise they must be satisfied        */
					/* 						from standard RAM.                                            */
					/* -			3 		Currently unused.                                             */
					/* See left.	4 & 5 	If these bits are set to 0 (PF_PRIVATE), the processes'       */
					/* 						entire memory space will be considered private                */
					/* 						(when memory protection is enabled).If these bits are         */
					/* 						set to 1 (PF_GLOBAL), the processes' entire memory space      */
					/* 						will be readable and writable by any process (i.e. global).   */
					/* 						If these bits are set to 2 (PF_SUPERVISOR), the processes'    */
					/* 						entire memory space will only be readable and writable by     */
					/* 						itself and any other process in supervisor mode.If these      */
					/* 						bits are set to 3 (PF_READABLE), the processes' entire memory */
					/* 						space will be readable by any application but only            */
					/* 						writable by itself.                                           */
					/* -			6-15 	Currently unused.                                             */

static WORD tdb_tab[] = {
	0,				/* absolute */
	AL_TEXT,		/* TEXT segment based */
	AL_DATA, 0,		/* DATA segment based */
	AL_BSS			/* BSS segment based */
};


#define HDRSIZE 0x1C		/* size of Alcyon header */


/*
 *  Add entry to symbol table;
 *  if `globflag' is 1, make the symbol global;
 *  if in .PRG mode, adjust symbol values for fake link.
 *
 */
char * constr_symtab(register char * buf, SYM * sym, int globflag)
{
	register int i;
	register char * s;
	register WORD w;
	register LONG z;
	register WORD w1;

	/*
	 *  Copy symbol name
	 */
	s = sym->sname;

	for(i=0; i<8 && *s; i++)
		*buf++ = *s++;

	while (i++ < 8)
		*buf++ = '\0';

	/*
	 *  Construct and deposit flag word
	 *
	 *	o  all symbols are AL_DEFINED
	 *	o  install T/D/B/A base
	 *    o  install 'equated'
	 *    o  commons (COMMON) are AL_EXTERN, but not BSS
	 *	o  exports (DEFINED) are AL_GLOBAL
	 *	o  imports (~DEFINED) are AL_EXTERN
	 *
	 */
	w1 = sym->sattr;
	w = AL_DEFINED | tdb_tab[w1 & TDB];

	if (w1 & EQUATED)		/* equated */
		w |= AL_EQUATED;

	if (w1 & COMMON)
	{
		w |= AL_EXTERN | AL_GLOBAL;	/* common symbol */
		w &= ~AL_BSS;		/* they're not BSS in Alcyon object files */
	}
	else if (w1 & DEFINED)
	{
		if (globflag)		/* export the symbol */
			w |= AL_GLOBAL;
	}
	else w |= AL_EXTERN;		/* imported symbol */

	*buf++ = w >> 8;
	*buf++ = (char)w;

	z = sym->svalue;

	if (prg_flag)			/* relocate value in .PRG segment */
	{
		w1 &= DATA | BSS;

		if (w1)
			z += sect[TEXT].sloc;

		if (w1 & BSS)
			z += sect[DATA].sloc;
	}

	*buf++ = z >> 24;		/* deposit symbol value */
	*buf++ = z >> 16;
	*buf++ = z >> 8;
	*buf++ = z;

	return buf;
}


//
// Write an object file to the passed in file descriptor
// N.B.: Return value is ignored...
//
int WriteObject(int fd)
{
	LONG t;					// Scratch long
	LONG tds;				// TEXT & DATA segment size
	int i;					// Temporary int
	CHUNK * cp;				// Chunk (for gather)
	char * buf;				// Scratch area
	char * p;				// Temporary ptr
	LONG ssize;				// Size of symbols
	LONG trsize, drsize;	// Size of relocations
	long unused;			// For supressing 'write' warnings

	if (verb_flag)
	{
		printf("TEXT segment: %d bytes\n", sect[TEXT].sloc);
		printf("DATA segment: %d bytes\n", sect[DATA].sloc);
		printf("BSS  segment: %d bytes\n", sect[BSS].sloc);
	}

	// Write requested object file...
	if (obj_format==BSD || (obj_format==ALCYON && prg_flag==0))
    {
		if (verb_flag)
		{
			printf("Total       : %d bytes\n", sect[TEXT].sloc + sect[DATA].sloc + sect[BSS].sloc);
		}
		ssize = ((LONG)sy_assign(NULL, NULL));		// Assign index numbers to the symbols
		tds = sect[TEXT].sloc + sect[DATA].sloc;	// Get size of TEXT and DATA segment
		buf = malloc(0x600000);						// Allocate 6mb object file image memory

		if (buf == NULL)
		{
			error("cannot allocate object file memory (in BSD mode)");
			return ERROR;
		}

		memset(buf, 0, 0x600000);		// Reset allocated memory
		objImage = buf;					// Set global object image pointer
		strtable = malloc(0x200000);	// Allocate 2mb scratch buffer 

		if (strtable == NULL)
		{
			error("cannot allocate string table memory (in BSD mode)");
			return ERROR;
		}

		memset(strtable, 0, 0x200000);	// Reset allocated memory

		// Build object file header
		chptr = buf;					// Base of header
		D_long(0x00000107);				// Magic number
		D_long(sect[TEXT].sloc);		// TEXT size 
		D_long(sect[DATA].sloc);		// DATA size 
		D_long(sect[BSS].sloc);			// BSS size 
		D_long(0x00000000);				// Symbol size
		D_long(0x00000000);				// First entry (0L)
		D_long(0x00000000);				// TEXT relocation size
		D_long(0x00000000);				// BSD relocation size

		// Construct TEXT and DATA segments (without relocation changes)
		p = buf + BSDHDRSIZE;

		for(i=TEXT; i<=DATA; ++i)
		{
			for(cp=sect[i].sfcode; cp!=NULL; cp=cp->chnext)
			{
				memcpy(p, cp->chptr, cp->ch_size);
				p += cp->ch_size;
			}
		}

		// Do relocation tables (and make changes to segment data)
		p = buf + (BSDHDRSIZE + tds);	// Move obj image ptr to reloc info
		trsize = bsdmarkimg(p, tds, sect[TEXT].sloc, TEXT);// Do TEXT relocation table
		chptr = buf + 24;				// Point to relocation hdr entry
		D_long(trsize);					// Write the relocation table size
		p = buf + (BSDHDRSIZE + tds + trsize);	// Move obj image ptr to reloc info
		drsize = bsdmarkimg(p, tds, sect[TEXT].sloc, DATA);// Do DATA relocation table
		chptr = buf + 28;				// Point to relocation hdr entry
		D_long(drsize);					// Write the relocation table size

		p = buf + (BSDHDRSIZE + tds + trsize + drsize);// Point to start of symbol table
		sy_assign(p, constr_bsdsymtab);	// Build symbol and string tables
		chptr = buf + 16;				// Point to sym table size hdr entry
		D_long(symsize);				// Write the symbol table size

		// Point to string table
		p = buf + (BSDHDRSIZE + tds + trsize + drsize + symsize);    

		memcpy(p, strtable, strindx);	// Copy string table to object image

		if (buf)
			free(strtable);				// Free allocated memory

		chptr = p;						// Point to string table size long
		D_long(strindx);				// Write string table size

		// Write the BSD object file from the object image buffer
		unused = write(fd, buf, BSDHDRSIZE + tds + trsize + drsize + symsize + strindx + 4);

		if (buf)
			free(buf);					// Free allocated memory

    }
    else if (obj_format==ALCYON)
    {
		if (verb_flag)
		{
			if (prg_flag)
			{
				printf("TOS header  : 28 bytes\n");
				printf("Total       : %d bytes\n", 28 + sect[TEXT].sloc + sect[DATA].sloc + sect[BSS].sloc);
			}
			else
			{
				printf("Total       : %d bytes\n", sect[TEXT].sloc + sect[DATA].sloc + sect[BSS].sloc);
			}
		}
		/*
		 *  Compute size of symbol table;
		 *   assign numbers to the symbols...
		 */
		ssize = 0;

		if (prg_flag != 1)
			ssize = ((LONG)sy_assign(NULL, NULL)) * 14;

		/*
		 *  Alloc memory for header+text+data, symbol and
		 *   relocation information construction.
		 */
		t = tds = sect[TEXT].sloc + sect[DATA].sloc;

		if (t < ssize)
			t = ssize;

		buf = (char *)((int)malloc(t + HDRSIZE) + HDRSIZE);

		/*
		 *  Build object file header
		 *   just before the text+data image
		 */
		chptr = buf - HDRSIZE;	/* -> base of header */
		D_word(0x601a);			/* 00 - magic number */
		t = sect[TEXT].sloc;	/* 02 - TEXT size */
		D_long(t);
		t = sect[DATA].sloc;	/* 06 - DATA size */
		D_long(t);
		t = sect[BSS].sloc;		/* 0a - BSS size */
		D_long(t);
		D_long(ssize);			/* 0e - symbol table size */
		D_long(0);				/* 12 - stack size (unused) */
		D_long(PRGFLAGS);		/* 16 - PRGFLAGS */
		D_word(0);				/* 1a - relocation information exists */

		/*
		 *  Construct text and data segments;
		 *  fixup relocatable longs in .PRG mode;
		 *  finally write the header+text+data
		 */
		p = buf;

		for(i=TEXT; i<=DATA; i++)
		{
			for (cp=sect[i].sfcode; cp!=NULL; cp=cp->chnext)
			{
				memcpy(p, cp->chptr, cp->ch_size);
				p += cp->ch_size;
			}
		}

		if (prg_flag)
			markimg(buf, tds, sect[TEXT].sloc, 0);

		write(fd, buf - HDRSIZE, tds + HDRSIZE);

		/*
		 *  Construct and write symbol table
		 */
		if (prg_flag != 1)
		{
			sy_assign(buf, constr_symtab);
			write(fd, buf, ssize);
		}

		/*
		 *  Construct and write relocation information;
		 *   the size of it changes if we're writing a RELMODed executable.
		 */
		tds = markimg(buf, tds, sect[TEXT].sloc, 1);
		write(fd, buf, tds);
	}

	return 0;
}

