//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// MARK.C - A record of things that are defined relative to any of the sections
// Copyright (C) 199x Landon Dyer, 2011-2012 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source utilised with the kind permission of Landon Dyer
//

#include "mark.h"
#include "error.h"
#include "object.h"
#include "riscasm.h"


MCHUNK * firstmch;		// First mark chunk
MCHUNK * curmch;		// Current mark chunk
PTR markptr;			// Deposit point in current mark chunk
LONG mcalloc;			// #bytes alloc'd to current mark chunk
LONG mcused;			// #bytes used in current mark chunk
uint16_t curfrom;		// Current "from" section

//
//  Imports
//
extern int prg_flag;	// 1, write ".PRG" relocatable executable

//#define DEBUG_IMAGE_MARKING


//
// Initialize marker
//
void InitMark(void)
{
	firstmch = curmch = NULL;
	mcalloc = mcused = 0;
	curfrom = 0;
}


//
// Wrap up marker (called after final mark is made)
//
void StopMark(void)
{
	if (curmch)
	{
		*markptr.wp = MCHEND;		// Mark end of block
		curmch->mcused = mcused;	// Update #used in mark block
	}
}


//
// Mark a word or longword relocatable
//
int rmark(uint16_t from, uint32_t loc, uint16_t to, uint16_t size, SYM * symbol)
{
#ifdef DEBUG_IMAGE_MARKING
printf("rmark: from=%i, loc=$%X, to=$%X, size=$%x, symbol=$%X\n", from, loc, to, size, symbol);
if (symbol)
	printf("      symbol->stype=$%02X, sattr=$%04X, sattre=$%08X, svalue=%i, sname=%s\n", symbol->stype, symbol->sattr, symbol->sattre, symbol->svalue, symbol->sname);
#endif

	if ((mcalloc - mcused) < MIN_MARK_MEM)
		amark();

	uint16_t flags = (size | to);

	if (from != curfrom)
		flags |= MCHFROM;

	if (symbol != NULL)
		flags |= MSYMBOL;

	//
	//  Complain about some things are not allowed in `-p' mode:
	//    o marks that aren't to LONGs;
	//    o external references.
	//
	if (prg_flag)
	{
		if ((flags & MLONG) == 0)
			error("illegal word relocatable (in .PRG mode)");

		if (symbol != NULL)
			errors("illegal external reference (in .PRG mode) to '%s'",
				   symbol->sname);
	}

	mcused += sizeof(WORD) + sizeof(LONG);
	*markptr.wp++ = flags;
	*markptr.lp++ = loc;

	if (flags & MCHFROM)
	{
		curfrom = from;
		*markptr.wp++ = from;
		mcused += sizeof(WORD);
	}

	if (flags & MSYMBOL)
	{
		*markptr.sy++ = symbol;
		mcused += sizeof(SYM *);
	}

	*markptr.wp = 0x0000;

	return 0;
}


//
// Allocate another chunk of mark space
//
int amark(void)
{
//	MCHUNK * p;

	// Alloc mark block header (and data) and set it up.
//	p = (MCHUNK *)amem((long)(sizeof(MCHUNK)) + MARK_ALLOC_INCR);
	MCHUNK * p = (MCHUNK *)malloc(sizeof(MCHUNK) + MARK_ALLOC_INCR);
	p->mcnext = NULL;
	p->mcalloc = MARK_ALLOC_INCR;
	p->mcptr.cp = (char *)(((char *)p) + sizeof(MCHUNK));

	if (curmch)
	{
		// Link onto previous chunk 
		*markptr.wp++ = MCHEND;		// Mark end of block 
		curmch->mcused = mcused;
		curmch->mcnext = p;
	}

	if (!firstmch)
		firstmch = p;

	curmch = p;						// Setup global vars 
	markptr = p->mcptr;
	mcalloc = MARK_ALLOC_INCR;
	mcused = 0;

	return 0;
}


/*
 *  Table to convert from TDB to fixup triad
 *
 */
static char mark_tr[] = {
	0,				/* (n/a) */
	2,				/* TEXT relocatable */
	1, 0,				/* DATA relocatable */
	3				/* BSS relocatable */
};


/*
 *  Make mark image for Alcyon .o file
 *  okflag	--  1, ok to deposit reloc information
 */
LONG markimg(register char * mp, LONG siz, LONG tsize, int okflag)
{
	MCHUNK * mch;		/* -> mark chunk */
	register PTR p;		/* source point from within mark chunk */
	WORD from;			/* section fixups are currently FROM */
	register WORD w;	/* a word (temp) */
	LONG loc;			/* location (temp) */
	LONG lastloc;		/* last location fixed up (RELMOD) */
	SYM * symbol;		/* -> symbols (temp) */
	char * wp;			/* pointer into raw relocation information */
	register char * dp;	/* deposit point for RELMOD information */
	int firstp;			/* 1, first relocation (RELMOD) */
	LONG diff;			/* difference to relocate (RELMOD) */

	if (okflag)
		//clear(mp, siz);		/* zero relocation buffer */
		memset(mp, 0, siz);		/* zero relocation buffer */

	from = 0;

	for(mch=firstmch; mch!=NULL; mch=mch->mcnext)
	{
		for(p=mch->mcptr;;)
		{
			w = *p.wp++;		/* w = next mark entry */

			if (w & MCHEND)		/* (end of mark chunk) */
				break;

			/*
			 *  Get mark record
			 */
			symbol = NULL;
			loc = *p.lp++;		/* mark location */

			if (w & MCHFROM)	/* maybe change "from" section */
				from = *p.wp++;

			if (w & MSYMBOL)	/* maybe includes a symbol */
				symbol = *p.sy++;

			/*
			 *  Compute mark position in relocation information;
			 *  in RELMOD mode, get address of data to fix up.
			 */
			if (from == DATA)
				loc += tsize;

			wp = (char *)(mp + loc);

			if (okflag && (w & MLONG)) /* indicate first word of long */
			{
				wp[1] = 5;
				wp += 2;
			}

			if (symbol)
			{
				/*
				 *  Deposit external reference
				 */
				if (okflag)
				{
					if (w & MPCREL)
						w = 6;		/* pc-relative fixup */
					else
						w = 4;		/* absolute fixup */

					w |= symbol->senv << 3;
					*wp++ = w >> 8;
					*wp = w;
				}
			}
			else
			{
				/*
				 *  Deposit section-relative mark;
				 *  in RELMOD mode, fix it up in the chunk,
				 *  kind of like a sleazoid linker.
				 *
				 *  In RELMOD mode, marks to words (MWORDs) "cannot happen,"
				 *  checks are made when mark() is called, so we don't have
				 *  to check again here.
				 */
				w &= TDB;

				if (okflag)
					wp[1] = mark_tr[w];
				else if (prg_flag && (w & (DATA | BSS)))
				{
					dp = wp;
					diff = ((LONG)(*dp++ & 0xff)) << 24;
					diff |= ((LONG)(*dp++ & 0xff)) << 16;
					diff |= ((LONG)(*dp++ & 0xff)) << 8;
					diff |= (LONG)(*dp & 0xff);

#ifdef DO_DEBUG
					DEBUG printf("diff=%lx ==> ", diff);
#endif
					diff += sect[TEXT].sloc;

					if (w == BSS)
						diff += sect[DATA].sloc;

					dp = wp;
					*dp++ = (char)(diff >> 24);
					*dp++ = (char)(diff >> 16);
					*dp++ = (char)(diff >> 8);
					*dp = (char)diff;
#ifdef DO_DEBUG
					DEBUG printf("%lx\n", diff);
#endif
				}
			}
		}
	}

	/*
	 *  Generate ".PRG" relocation information in place in
	 *  the relocation words (the ``RELMOD'' operation).
	 */
	if (okflag && prg_flag)
	{
		firstp = 1;
		wp = mp;
		dp = mp;

		for(loc=0; loc<siz;)
		{
			if ((wp[1] & 7) == 5)
			{
				if (firstp)
				{
					*dp++ = (char)(loc >> 24);
					*dp++ = (char)(loc >> 16);
					*dp++ = (char)(loc >> 8);
					*dp++ = (char)loc;
					firstp = 0;
				}
				else
				{
					for(diff=loc-lastloc; diff>254; diff-= 254)
						*dp++ = 1;

					*dp++ = (char)diff;
				}

				wp += 4;
				lastloc = loc;
				loc += 4;
			}
			else 
			{
				loc += 2;
				wp += 2;
			}
		}

		/*
		 *  Terminate relocation list with 0L (if there was no
		 *  relocation) or 0.B (if relocation information was
		 *  written).
		 */
		if (!firstp)
			*dp++ = 0;
		else for (firstp = 0; firstp < 4; ++firstp)
			*dp++ = 0;

		/*
		 *  Return size of relocation information
		 */
		loc = dp - mp;
		return loc;
	}

	return siz;
}


//
// Make mark image for BSD .o file
//
uint32_t bsdmarkimg(char * mp, LONG siz, LONG tsize, int reqseg)
{
	MCHUNK * mch;				// Mark chunk
	PTR p;						// Source point from within mark chunk
	uint16_t from;				// Section fixups are currently FROM
	uint16_t w;					// A word (temp)
	uint32_t loc;				// Location (temp) 
	SYM * symbol;				// Symbols (temp)
	uint8_t * wp;				// Pointer into raw relocation info
	uint8_t * dp;				// Deposit point for RELMOD info
	uint32_t diff;				// Difference to relocate (RELMOD)
	uint32_t raddr, rflag = 0;	// BSD relocation address and flags
	uint32_t rsize;				// Relocation size
	int validsegment = 0;		// Valid segment being processed

#ifdef DEBUG_IMAGE_MARKING
printf("bsdmarkimg():\n");
#endif
	// Initialise relocation size
	rsize = 0;
	chptr = mp;
	from = 0;

	for(mch=firstmch; mch!=NULL; mch=mch->mcnext)
	{
		for(p=mch->mcptr;;)
		{
			w = *p.wp++;			// Next mark entry

			if (w & MCHEND)
				break;				// End of mark chunk

			// Get mark record
			symbol = NULL;
			loc = *p.lp++;			// Mark location

			if (w & MCHFROM)
			{
				// Maybe change "from" section
				from = *p.wp++;

				if (obj_format == BSD)
				{
					if (reqseg == TEXT)
					{
						// Requested segment is TEXT
						if (from == TEXT)
							validsegment = 1; 
						else
							validsegment = 0;
					}
					else
					{
						// Requested segment is DATA
						if (from == DATA)
							validsegment = 1; 
						else
							validsegment = 0;
					}
				}
			}

			if (w & MSYMBOL)			// Maybe includes a symbol
				symbol = *p.sy++;

			if (obj_format == BSD)
			{
				raddr = loc;			// Set relocation address

				if (validsegment)
#ifdef DEBUG_IMAGE_MARKING
{
printf(" validsegment: raddr = $%08X\n", raddr);
#endif
					D_long(raddr);		// Write relocation address
#ifdef DEBUG_IMAGE_MARKING
}
#endif

				if (w & MPCREL)
					rflag = 0x000000A0;	// PC-relative fixup
				else
					rflag = 0x00000040;	// Absolute fixup

// This flag tells the linker to WORD swap the LONG when doing the fixup.
				if (w & MMOVEI)
//{
//printf("bsdmarkimg: ORing $01 to rflag (MMOVEI) [symbol=%s]...\n", symbol->sname);
					rflag |= 0x00000001;
//}
			}

			// Compute mark position in relocation information;
			// in RELMOD mode, get address of data to fix up.
			if (from == DATA)
				loc += tsize;

			wp = (uint8_t *)(mp + loc);

			if (symbol)
			{
				// Deposit external reference
				if (obj_format == BSD)
				{
					rflag |= 0x00000010;			// Set external reloc flag bit
					rflag |= (symbol->senv << 8);	// Put symbol index in flags

// Looks like this is completely unnecessary (considering it does the wrong thing!)
#if 0
					if (symbol->sattre & RISCSYM)
{
printf("bsdmarkimg: ORing $01 to rflag (RISCSYM) [symbol=%s]...\n", symbol->sname);
						rflag |= 0x00000001;
}
#endif

					if (validsegment)
					{
#ifdef DEBUG_IMAGE_MARKING
printf("  validsegment(2): rflag = $%08X\n", rflag);
#endif
						D_long(rflag);				// Write relocation flags
						rsize += 8;					// Increment relocation size
					}
				}
			}
			else
			{
				if (obj_format == BSD)
				{
#ifdef DEBUG_IMAGE_MARKING
printf("  w = $%04X\n", w);
#endif
					w &= TDB;						// Set reloc flags to segment

					switch (w)
					{
					case TEXT: rflag |= 0x00000400; break;
					case DATA: rflag |= 0x00000600; break;
					case BSS:  rflag |= 0x00000800; break;
					}

					if (validsegment)
					{
#ifdef DEBUG_IMAGE_MARKING
printf("  validsegment(3): rflag = $%08X\n", rflag);
#endif
						D_long(rflag);				// Write relocation flags
						rsize += 8;					// Increment relocation size
					}

					w &= TDB;

					if (validsegment)
					{
						if (w & (DATA|BSS))
						{
							dp = objImage + BSDHDRSIZE + loc;
							diff = ((LONG)(*dp++ & 0xFF)) << 24;
							diff |= ((LONG)(*dp++ & 0xFF)) << 16;
							diff |= ((LONG)(*dp++ & 0xFF)) << 8;
							diff |= (LONG)(*dp & 0xFF);
							DEBUG printf("diff=%ux ==> ", diff);
#ifdef DEBUG_IMAGE_MARKING
printf("  validsegment(4): diff = $%08X --> ", diff);
#endif
	
							if (rflag & 0x01)
								diff = ((diff >> 16) & 0x0000FFFF) | ((diff << 16) & 0xFFFF0000);

							diff += sect[TEXT].sloc;

							if (w == BSS)
								diff += sect[DATA].sloc;

							if (rflag & 0x01)
								diff = ((diff >> 16) & 0x0000FFFF) | ((diff << 16) & 0xFFFF0000);

							dp = objImage + BSDHDRSIZE + loc;
							*dp++ = (char)(diff >> 24);
							*dp++ = (char)(diff >> 16);
							*dp++ = (char)(diff >> 8);
							*dp = (char)diff;
							DEBUG printf("%ux\n", diff);
#ifdef DEBUG_IMAGE_MARKING
printf("$%08X\n", diff);
#endif
						}
					}
				}
			}
		}
	}

	// Return relocation size
	if (obj_format == BSD)
#ifdef DEBUG_IMAGE_MARKING
{
printf("  rsize = $%X\n", rsize);
#endif
		return rsize;                                        
#ifdef DEBUG_IMAGE_MARKING
}
#endif

	return siz;
}

