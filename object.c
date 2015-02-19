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

	// Write requested object file...
	switch (obj_format)
	{
	case BSD:
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

		break;
	}

	return 0;
}

