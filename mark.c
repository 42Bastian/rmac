//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// MARK.C - A record of things that are defined relative to any of the sections
// Copyright (C) 199x Landon Dyer, 2011-2012 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source Utilised with the Kind Permission of Landon Dyer
//

#include "mark.h"
#include "error.h"
#include "object.h"
#include "risca.h"


MCHUNK * firstmch;		// First mark chunk
MCHUNK * curmch;		// Current mark chunk
PTR markptr;			// Deposit point in current mark chunk
LONG mcalloc;			// #bytes alloc'd to current mark chunk
LONG mcused;			// #bytes used in current mark chunk
WORD curfrom;			// Current "from" section


//
// Initialize Marker
//
void init_mark(void)
{
	firstmch = curmch = NULL;
	mcalloc = mcused = 0;
	curfrom = 0;
}


//
// Wrap up marker (called after final mark is made)
//
void stopmark(void)
{
	if (curmch)
	{
		*markptr.wp = MCHEND;                                 // Mark end of block
		curmch->mcused = mcused;                              // Update #used in mark block
	}
}


//
// Mark a word or longword relocatable
//
int rmark(int from, LONG loc, int to, int size, SYM * symbol)
{
	WORD w;

	if ((mcalloc - mcused) < MIN_MARK_MEM)
		amark();

	w = (WORD)(size | to);

	if (from != curfrom)
		w |= MCHFROM;

	if (symbol != NULL)
		w |= MSYMBOL;

	mcused += sizeof(WORD) + sizeof(LONG);
	*markptr.wp++ = w;
	*markptr.lp++ = loc;

	if (w & MCHFROM)
	{
		*markptr.wp++ = (WORD)from;
		curfrom = (WORD)from;
		mcused += sizeof(WORD);
	}

	if (w & MSYMBOL)
	{
		*markptr.sy++ = symbol;
		mcused += sizeof(LONG);
	}

	*markptr.wp = 0x0000;

	return 0;
}


//
// Allocate another chunk of mark space
//
int amark(void)
{
	MCHUNK * p;

	// Alloc mark block header (and data) and set it up.
//	p = (MCHUNK *)amem((long)(sizeof(MCHUNK)) + MARK_ALLOC_INCR);
	p = (MCHUNK *)malloc(sizeof(MCHUNK) + MARK_ALLOC_INCR);
	p->mcnext = NULL;
	p->mcalloc = MARK_ALLOC_INCR;
	p->mcptr.cp = (char *)(((char *)p) + sizeof(MCHUNK));

	if (curmch)
	{                                             // Link onto previous chunk 
		*markptr.wp++ = MCHEND;                               // Mark end of block 
		curmch->mcused = mcused;
		curmch->mcnext = p;
	}

	if (!firstmch)
		firstmch = p;

	curmch = p;                                              // Setup global vars 
	markptr = p->mcptr;
	mcalloc = MARK_ALLOC_INCR;
	mcused = 0;

	return 0;
}


//
// Make mark image for BSD .o file
//
LONG bsdmarkimg(char * mp, LONG siz, LONG tsize, int reqseg)
{
	MCHUNK * mch;                                             // Mark chunk
	PTR p;                                                   // Source point from within mark chunk
	WORD from;                                               // Section fixups are currently FROM
	WORD w;                                                  // A word (temp)
	LONG loc;                                                // Location (temp) 
	SYM * symbol;                                             // Symbols (temp)
	char * wp;                                                // Pointer into raw relocation info
	char * dp;                                                // Deposit point for RELMOD info
	LONG diff;                                               // Difference to relocate (RELMOD)
	LONG raddr, rflag = 0;                                   // BSD relocation address and flags
	LONG rsize;                                              // Relocation size
	int validsegment = 0;                                    // Valid segment being processed

	rsize = 0;                                               // Initialise relocation size
	chptr = mp;

	from = 0;
	for(mch = firstmch; mch != NULL; mch = mch->mcnext)
	{
		for(p=mch->mcptr;;)
		{
			w = *p.wp++;                                       // Next mark entry

			if (w & MCHEND)
				break;                              // End of mark chunk

			// Get mark record
			symbol = NULL;
			loc = *p.lp++;                                     // Mark location

			if (w & MCHFROM)
			{                                  // Maybe change "from" section
				from = *p.wp++;

				if (obj_format == BSD)
				{
					if (reqseg == TEXT)
					{                         // Requested segment is TEXT
						if (from == TEXT)
							validsegment = 1; 
						else
							validsegment = 0;
					}
					else
					{                                     // Requested segment is DATA
						if (from == DATA)
							validsegment = 1; 
						else
							validsegment = 0;
					}
				}
			}

			if (w & MSYMBOL)                                    // Maybe includes a symbol
				symbol = *p.sy++;

			if (obj_format == BSD)
			{
				raddr = loc;                                    // Set relocation address

				if (validsegment)
					D_long(raddr);                               // Write relocation address

				if (w & MPCREL)
					rflag = 0x000000A0;                          // PC-relative fixup
				else
					rflag = 0x00000040;                          // Absolute fixup

				if (w & MMOVEI)
					rflag |= 0x00000001;
			}

			// Compute mark position in relocation information;
			// in RELMOD mode, get address of data to fix up.
			if (from == DATA)
				loc += tsize;

			wp = (char *)(mp + loc);

			if (symbol)
			{
				// Deposit external reference
				if (obj_format == BSD)
				{
					rflag |= 0x00000010;                         // Set external reloc flag bit
					rflag |= (symbol->senv << 8);                // Put symbol index in flags

					if (symbol->sattre & RISCSYM)
						rflag |= 0x00000001;

					if (validsegment)
					{
						D_long(rflag);                            // Write relocation flags
						rsize += 8;                               // Increment relocation size
					}
				}
			}
			else
			{
				if (obj_format == BSD)
				{
					w &= TDB;                                    // Set reloc flags to segment

					switch (w)
					{
					case TEXT: rflag |= 0x00000400; break;
					case DATA: rflag |= 0x00000600; break;
					case BSS:  rflag |= 0x00000800; break;
					}

					if (validsegment)
					{
						D_long(rflag);                            // Write relocation flags
						rsize += 8;                               // Increment relocation size
					}

					w &= TDB;

					if (validsegment)
					{
						if (w & (DATA|BSS))
						{
							dp = objimage + BSDHDRSIZE + loc;
							diff = ((LONG)(*dp++ & 0xFF)) << 24;
							diff |= ((LONG)(*dp++ & 0xFF)) << 16;
							diff |= ((LONG)(*dp++ & 0xFF)) << 8;
							diff |= (LONG)(*dp & 0xFF);
							DEBUG printf("diff=%ux ==> ", diff);
	
							if (rflag & 0x01)
								diff = ((diff >> 16) & 0x0000FFFF) | ((diff << 16) & 0xFFFF0000);

							diff += sect[TEXT].sloc;

							if (w == BSS)
								diff += sect[DATA].sloc;

							if (rflag & 0x01)
								diff = ((diff >> 16) & 0x0000FFFF) | ((diff << 16) & 0xFFFF0000);

							dp = objimage + BSDHDRSIZE + loc;
							*dp++ = (char)(diff >> 24);
							*dp++ = (char)(diff >> 16);
							*dp++ = (char)(diff >> 8);
							*dp = (char)diff;
							DEBUG printf("%ux\n", diff);
						}
					}
				}
			}
		}
	}

	// Return relocation size
	if (obj_format == BSD)
		return rsize;                                        

	return siz;
}
