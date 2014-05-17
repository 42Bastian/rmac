//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// SECT.C - Code Generation, Fixups and Section Management
// Copyright (C) 199x Landon Dyer, 2011 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source Utilised with the Kind Permission of Landon Dyer
//

#include "sect.h"
#include "direct.h"
#include "error.h"
#include "expr.h"
#include "listing.h"
#include "mach.h"
#include "mark.h"
#include "riscasm.h"
#include "symbol.h"
#include "token.h"


// Function prototypes
void MakeSection(int, WORD);
void SwitchSection(int);

// Section descriptors
SECT sect[NSECTS];				// All sections... 
int cursect;					// Current section number

// These are copied from the section descriptor, the current code chunk
// descriptor and the current fixup chunk descriptor when a switch is made into
// a section.  They are copied back to the descriptors when the section is left.
WORD scattr;					// Section attributes 
LONG sloc;						// Current loc in section 

CHUNK * scode;					// Current (last) code chunk 
LONG challoc;					// # bytes alloc'd to code chunk 
LONG ch_size;					// # bytes used in code chunk 
char * chptr;					// Deposit point in code chunk buffer 

CHUNK * sfix;					// Current (last) fixup chunk
LONG fchalloc;					// # bytes alloc'd to fixup chunk
LONG fchsize;					// # bytes used in fixup chunk
PTR fchptr;						// Deposit point in fixup chunk buffer

// Return a size (SIZB, SIZW, SIZL) or 0, depending on what kind of fixup is
// associated with a location.
static char fusiztab[] = {
   0,	// FU_QUICK
   1,	// FU_BYTE
   2,	// FU_WORD
   2,	// FU_WBYTE
   4,	// FU_LONG
   1,	// FU_BBRA
   0,	// (unused)
   1,	// FU_6BRA
};

// Offset to REAL fixup location
static char fusizoffs[] = {
   0,	// FU_QUICK
   0,	// FU_BYTE
   0,	// FU_WORD
   1,	// FU_WBYTE
   0,	// FU_LONG
   1,	// FU_BBRA
   0,	// (unused)
   0,	// FU_6BRA
};


//
// Initialize Sections; Setup initial ABS, TEXT, DATA and BSS sections
//
void InitSection(void)
{
	int i;

	// Cleanup all sections
	for(i=0; i<NSECTS; i++)
		MakeSection(i, 0);

	// Construct default sections, make TEXT the current section
	MakeSection(ABS,   SUSED | SABS | SBSS);		// ABS
	MakeSection(TEXT,  SUSED | TEXT       );		// TEXT
	MakeSection(DATA,  SUSED | DATA       );		// DATA
	MakeSection(BSS,   SUSED | BSS  | SBSS);		// BSS
//	MakeSection(M6502, SUSED | TEXT       );		// 6502 code section

	SwitchSection(TEXT);						// Switch to TEXT for starters
}


//
// Make a New (Clean) Section
//
void MakeSection(int sno, WORD attr)
{
	SECT * p = &sect[sno];
	p->scattr = attr;
	p->sloc = 0;
	p->scode = p->sfcode = NULL;
	p->sfix = p->sffix = NULL;
}


//
// Switch to another section (copy section & chunk descriptors to global vars
// for fast access)
//
void SwitchSection(int sno)
{
	CHUNK * cp;								// Chunk pointer
	cursect = sno;
	SECT * p = &sect[sno];

	scattr = p->scattr;						// Copy section vars
	sloc = p->sloc;
	scode = p->scode;
	sfix = p->sfix;

	// Copy code chunk vars
	if ((cp = scode) != NULL)
	{
		challoc = cp->challoc;
		ch_size = cp->ch_size;
		chptr = cp->chptr + ch_size;
	}
	else
		challoc = ch_size = 0;

	// Copy fixup chunk vars 
	if ((cp = sfix) != NULL)
	{
		fchalloc = cp->challoc;
		fchsize = cp->ch_size;
		fchptr.cp = cp->chptr + fchsize;
	}
	else
		fchalloc = fchsize = 0;
}


//
// Save current section
//
void SaveSection(void)
{
	SECT * p = &sect[cursect];

	p->scattr = scattr;						// Bailout section vars
	p->sloc = sloc;

	if (scode != NULL)						// Bailout code chunk
		scode->ch_size = ch_size;

	if (sfix != NULL)						// Bailout fixup chunk
		sfix->ch_size = fchsize;
}


//
// Test to see if a location has a fixup sic'd on it.  This is used by the
// listing generator to print 'xx's instead of '00's for forward references
//
int fixtest(int sno, LONG loc)
{
	CHUNK * ch;
	PTR fup;
	char * fuend;
	WORD w;
	LONG xloc;

	stopmark();								// Force update to sect[] variables

	// Hairy, ugly linear search for a mark on our location;
	// the speed doesn't matter, since this is only done when generating a
	// listing, which is SLOW.
	for(ch=sect[sno].sffix; ch!=NULL; ch=ch->chnext)
	{
		fup.cp = (char *)ch->chptr;
		fuend = fup.cp + ch->ch_size;

		while (fup.cp < fuend)
		{
			w = *fup.wp++;
			xloc = *fup.lp++ + (int)fusizoffs[w & FUMASK];
			fup.wp += 2;

			if (xloc == loc)
				return (int)fusiztab[w & FUMASK];

			if (w & FU_EXPR)
			{
				w = *fup.wp++;
				fup.lp += w;
			}
			else
				++fup.lp;
		}
	}

	return 0;
}


// 
// Check that there are at least `amt' bytes left in the current chunk. If
// there are not, allocate another chunk of at least `amt' bytes (and probably
// more).
// 
// If `amt' is zero, ensure there are at least CH_THRESHOLD bytes, likewise.
//
int chcheck(LONG amt)
{
	DEBUG { printf("chcheck(%u)\n", amt); }
	// If in BSS section, no allocation required
	if (scattr & SBSS)
		return 0;

	if (!amt)
		amt = CH_THRESHOLD;

	DEBUG { printf("    challoc=%i, ch_size=%i, diff=%i\n", challoc, ch_size, challoc-ch_size); }
	if ((int)(challoc - ch_size) >= (int)amt) 
		return 0;

	if (amt < CH_CODE_SIZE)
		amt = CH_CODE_SIZE;

	DEBUG { printf("    amt (adjusted)=%u\n", amt); }
	SECT * p = &sect[cursect];
	CHUNK * cp = malloc(sizeof(CHUNK) + amt);

	// First chunk in section
	if (scode == NULL)
	{
		cp->chprev = NULL;
		p->sfcode = cp;
	}
	// Add chunk to other chunks
	else
	{
		cp->chprev = scode;
		scode->chnext = cp;
		scode->ch_size = ch_size;			// Save old chunk's globals 
	}

	// Setup chunk and global vars
	cp->chloc = sloc;
	cp->chnext = NULL;
	challoc = cp->challoc = amt;
	ch_size = cp->ch_size = 0;
	chptr = cp->chptr = ((char *)cp) + sizeof(CHUNK);
	scode = p->scode = cp;

	return 0;
}


// This is really wrong. We need to make some proper structures here so we don't
// have to count sizes of objects, that's what the compiler's for! :-P
#define FIXUP_BASE_SIZE (sizeof(WORD) + sizeof(LONG) + sizeof(WORD) + sizeof(WORD))
//
// Arrange for a fixup on a location
//
int AddFixup(WORD attr, LONG loc, TOKEN * fexpr)
{
	LONG i;
	LONG len = 0;
	CHUNK * cp;
	SECT * p;
	// Shamus: Expression lengths are voodoo ATM (variable "i"). Need to fix this.
#warning "!!! AddFixup() is filled with VOODOO !!!"
	DEBUG printf("FIXUP@$%X: $%X\n", loc, attr);

	// Compute length of expression (could be faster); determine if it's the
	// single-symbol case; no expression if it's just a mark. This code assumes
	// 16 bit WORDs and 32 bit LONGs
	if (*fexpr == SYMBOL && fexpr[2] == ENDEXPR)
	{
		// Just a single symbol
		// SCPCD : correct bit mask for attr (else other FU_xxx will match) NYAN !
		if ((attr & FUMASKRISC) == FU_JR)
		{
//			i = 18;
//			i = FIXUP_BASE_SIZE + (sizeof(LONG) * 2);
			i = FIXUP_BASE_SIZE + sizeof(SYM *) + sizeof(LONG);
		}
		else
		{
//			i = 14;
			i = FIXUP_BASE_SIZE + sizeof(SYM *);
		}
	}
	else
	{
		attr |= FU_EXPR;

		for(len=0; fexpr[len]!=ENDEXPR; len++)
		{
			if (fexpr[len] == CONST || fexpr[len] == SYMBOL)
				len++;
		}

		len++;								// Add 1 for ENDEXPR 
//		i = (len << 2) + 12;
		i = FIXUP_BASE_SIZE + sizeof(WORD) + (len * sizeof(TOKEN));
	}

	// Maybe alloc another fixup chunk for this one to fit in
	if ((fchalloc - fchsize) < i)
	{
		p = &sect[cursect];
		cp = (CHUNK *)malloc(sizeof(CHUNK) + CH_FIXUP_SIZE);

		// First fixup chunk in section
		if (sfix == NULL)
		{
			cp->chprev = NULL;
			p->sffix = cp;
		}
		// Add to other chunks
		else
		{
			cp->chprev = sfix;
			sfix->chnext = cp;
			sfix->ch_size = fchsize;
		}

		// Setup fixup chunk and its global vars
		cp->chnext = NULL;
		fchalloc = cp->challoc = CH_FIXUP_SIZE;
		fchsize = cp->ch_size = 0;
		fchptr.cp = cp->chptr = ((char *)cp) + sizeof(CHUNK);
		sfix = p->sfix = cp;
	}

	// Record fixup type, fixup location, and the file number and line number
	// the fixup is located at.
	*fchptr.wp++ = attr;
	*fchptr.lp++ = loc;
	*fchptr.wp++ = cfileno;
	*fchptr.wp++ = (WORD)curlineno;

	// Store postfix expression or pointer to a single symbol, or nothing for a
	// mark.
	if (attr & FU_EXPR)
	{
		*fchptr.wp++ = (WORD)len;

		while (len--)
			*fchptr.lp++ = (LONG)*fexpr++;
	}
	else
	{
//		*fchptr.lp++ = (LONG)fexpr[1];
		*fchptr.sy++ = symbolPtr[fexpr[1]];
	}

	// SCPCD : correct bit mask for attr (else other FU_xxx will match) NYAN !
	if ((attr & FUMASKRISC) == FU_JR)
	{
		if (orgactive)
			*fchptr.lp++ = orgaddr;
		else
			*fchptr.lp++ = 0x00000000;
	}

	fchsize += i;
	return 0;
}


//
// Resolve all Fixups
//
int ResolveAllFixups(void)
{
	unsigned i;
	char buf[EBUFSIZ];

	// Make undefined symbols GLOBL
	if (glob_flag)
		syg_fix();

	DEBUG printf("Resolving TEXT sections...\n");
	ResolveFixups(TEXT);
	DEBUG printf("Resolving DATA sections...\n");
	ResolveFixups(DATA);

	return 0;
}


//
// Resolve Fixups in a Section
//
int ResolveFixups(int sno)
{
	PTR fup;					// Current fixup
	WORD * fuend;				// End of last fixup (in this chunk)
	WORD w;						// Fixup word (type+modes+flags)
	char * locp;				// Location to fix (in cached chunk) 
	LONG loc;					// Location to fixup
	VALUE eval;					// Expression value 
	WORD eattr;					// Expression attrib
	SYM * esym;					// External symbol involved in expr
	SYM * sy;					// (Temp) pointer to a symbol
	WORD i;						// (Temp) word
	WORD tdb;					// eattr & TDB
	LONG oaddr;
	int reg2;
	WORD flags;
	unsigned page_jump = 0;
	unsigned address = 0;
	unsigned j;
	char buf[EBUFSIZ];
	
	SECT * sc = &sect[sno];
	CHUNK * ch = sc->sffix;

	if (ch == NULL)
		return 0;

	// "Cache" first chunk
	CHUNK * cch = sc->sfcode;

	// Can't fixup a sect with nothing in it
	if (cch == NULL)
		return 0;

	do
	{
		fup.cp = ch->chptr;					// fup -> start of chunk
		fuend = (WORD *)(fup.cp + ch->ch_size);	// fuend -> end of chunk

		while (fup.wp < fuend)
		{
			w = *fup.wp++;
			loc = *fup.lp++;
			cfileno = *fup.wp++;
			curlineno = (int)*fup.wp++;
DEBUG { printf("ResolveFixups: cfileno=%u\n", cfileno); }
			// This is based on global vars cfileno, curfname :-P
			// This approach is kinda meh as well. I think we can do better than this.
			SetFilenameForErrorReporting();

			esym = NULL;

			// Search for chunk containing location to fix up; compute a
			// pointer to the location (in the chunk). Often we will find the
			// fixup is in the "cached" chunk, so the linear-search is seldom
			// executed.
			if (loc < cch->chloc || loc >= (cch->chloc + cch->ch_size))
			{
				for(cch=sc->sfcode; cch!=NULL; cch=cch->chnext)
				{
					if (loc >= cch->chloc && loc < (cch->chloc + cch->ch_size))
						break;
				}

				if (cch == NULL)
				{
					// Fixup (loc) out of range 
					interror(7);
					// NOTREACHED
				}
			}

			locp = cch->chptr + (loc - cch->chloc);
			eattr = 0;

			// Compute expression/symbol value and attribs
			// Complex expression
			if (w & FU_EXPR)
			{
				i = *fup.wp++;

				if (evexpr(fup.tk, &eval, &eattr, &esym) != OK)
				{
					fup.lp += i;
					continue;
				}

				fup.lp += i;
			}
			// Simple symbol
			else
			{
				sy = *fup.sy++;
				eattr = sy->sattr;

				if (eattr & DEFINED)
					eval = sy->svalue;
				else
					eval = 0;

				if ((eattr & (GLOBAL | DEFINED)) == GLOBAL)
					esym = sy;
			}

			tdb = (WORD)(eattr & TDB);

			// If the expression is undefined and no external symbol is
			// involved, then it's an error.
			if (!(eattr & DEFINED) && (esym == NULL))
			{
				error(undef_error);
				continue;
			}

// It seems that this is completely unnecessary!
#if 0
			if (((w & FUMASKRISC) == FU_MOVEI) && esym)
//{
//printf("DoFixups: Setting symbol attre to RISCSYM...\n");
				esym->sattre |= RISCSYM;
//}
#endif

			// Do the fixup
			// 
			// If a PC-relative fixup is undefined, its value is *not*
			// subtracted from the location (that will happen in the linker
			// when the external reference is resolved).
			// 
			// MWC expects PC-relative things to have the LOC subtracted from
			// the value, if the value is external (that is, undefined at this
			// point).
			// 
			// PC-relative fixups must be DEFINED and either in the same
			// section (whereupon the subtraction takes place) or ABS (with no
			// subtract).
			if (w & FU_PCREL)
			{
				if (eattr & DEFINED)
				{
					if (tdb == sno)
						eval -= (VALUE)loc;
					else if (tdb)
					{
						error("PC-relative expr across sections");
						continue;
					}

					if (sbra_flag && (w & FU_LBRA) && (eval + 0x80 < 0x100))
						warn("unoptimized short branch");
				}
				else if (obj_format == MWC)
					eval -= (VALUE)loc;

				tdb = 0;
				eattr &= ~TDB;
			}

			// Do fixup classes
			switch ((int)(w & FUMASK))
			{
			// FU_BBRA fixes up a one-byte branch offset.
			case FU_BBRA:
				if (!(eattr & DEFINED))
				{
					error("external short branch");
					continue;
				}

				eval -= 2;

				if (eval + 0x80 >= 0x100)
					goto range;

				if (eval == 0)
				{
					error("illegal bra.s with zero offset");
					continue;
				}

				*++locp = (char)eval;
				break;
			// Fixup one-byte value at locp + 1.
			case FU_WBYTE:
				locp++;
				// FALLTHROUGH
			// Fixup one-byte forward references
			case FU_BYTE:
				if (!(eattr & DEFINED))
				{
					error("external byte reference");
					continue;
				}

				if (tdb)
				{
					error("non-absolute byte reference");
					continue;
				}

				if ((w & FU_PCREL) && eval + 0x80 >= 0x100)
					goto range;

				if (w & FU_SEXT)
				{
					if (eval + 0x100 >= 0x200)
						goto range;
				}
				else if (eval >= 0x100)
					goto range;

				*locp = (char)eval;
				break;
			// Fixup WORD forward references; 
			// the word could be unaligned in the section buffer, so we have to
			// be careful.
			case FU_WORD:
				if ((w & FUMASKRISC) == FU_JR)// || ((w & 0x0F00) == FU_MJR))
				{
					oaddr = *fup.lp++;

					if (oaddr)
						reg2 = (signed)((eval - (oaddr + 2)) / 2);// & 0x1F;
					else
						reg2 = (signed)((eval - (loc + 2)) / 2);// & 0x1F;

#if 0
					if ((w & 0x0F00) == FU_MJR)
					{
						// Main code destination alignment checking here for
						// forward declared labels
						address = (oaddr) ? oaddr : loc;

						if (((address >= 0xF03000) && (address < 0xF04000)
							&& (eval < 0xF03000)) || ((eval >= 0xF03000)
							&& (eval < 0xF04000) && (address < 0xF03000)))
						{
							warni("* \'jr\' at $%08X - cannot jump relative between "
								"main memory and local gpu ram", address);
						}
						else
						{
							page_jump = (address & 0xFFFFFF00) - (eval & 0xFFFFFF00);

							if (page_jump)
							{
								// This jump is to a page outside of the
								// current 256 byte page
								if (eval % 4)
								{
									warni("* \'jr\' at $%08X - destination address not aligned for long page jump, insert a \'nop\' before the destination address", address);
								}
							}
							else
							{
								// This jump is in the current 256 byte page
								if ((eval - 2) % 4)
								{
									warni("* \'jr\' at $%08X - destination address not aligned for short page jump, insert a \'nop\' before the destination address", address);
								}
							}
						}
					}
#endif

					if ((reg2 < -16) || (reg2 > 15))
					{
						error("relative jump out of range");
						break;
					}

					*locp = (char)(*locp | ((reg2 >> 3) & 0x03));
					locp++;
					*locp = (char)(*locp | ((reg2 & 0x07) << 5));
					break;
				}

				if ((w & FUMASKRISC) == FU_NUM15)
				{
					if (eval < -16 || eval > 15)
					{
						error("constant out of range");
						break;
					}

					*locp = (char)(*locp | ((eval >> 3) & 0x03));
					locp++;
					*locp = (char)(*locp | ((eval & 0x07) << 5));
					break;
				}

				if ((w & FUMASKRISC) == FU_NUM31)
				{
					if (eval < 0 || eval > 31)
					{
						error("constant out of range");
						break;
					}

					*locp = (char)(*locp | ((eval >> 3) & 0x03));
					locp++;
					*locp = (char)(*locp | ((eval & 0x07) << 5));
					break;
				}

				if ((w & FUMASKRISC) == FU_NUM32)
				{
					if (eval < 1 || eval > 32)
					{
						error("constant out of range");
						break;
					}

					if (w & FU_SUB32)
						eval = (32 - eval);

					eval = (eval == 32) ? 0 : eval;
					*locp = (char)(*locp | ((eval >> 3) & 0x03));
					locp++;
					*locp = (char)(*locp | ((eval & 0x07) << 5));
					break;
				}

				if ((w & FUMASKRISC) == FU_REGONE)
				{
					if (eval < 0 || eval > 31)
					{
						error("register value out of range");
						break;
					}

					*locp = (char)(*locp | ((eval >> 3) & 0x03));
					locp++;
					*locp = (char)(*locp | ((eval & 0x07) << 5));
					break;
				}

				if ((w & FUMASKRISC) == FU_REGTWO)
				{
					if (eval < 0 || eval > 31)
					{
						error("register value out of range");
						break;
					}

					locp++;
					*locp = (char)(*locp | (eval & 0x1F));
					break;
				}

				if (!(eattr & DEFINED))
				{
					if (w & FU_PCREL)
						w = MPCREL | MWORD;
					else
						w = MWORD;

					rmark(sno, loc, 0, w, esym);
				}
				else
				{
					if (tdb)
						rmark(sno, loc, tdb, MWORD, NULL);

					if (w & FU_SEXT)
					{
						if (eval + 0x10000 >= 0x20000)
							goto range;
					}
					else
					{
						// Range-check BRA and DBRA
						if (w & FU_ISBRA)
						{
							if (eval + 0x8000 >= 0x10000)
							goto range;
						}
						else if (eval >= 0x10000)
							goto range;
					}
				}

				*locp++ = (char)(eval >> 8);
				*locp = (char)eval;
				break;
			// Fixup LONG forward references;
			// the long could be unaligned in the section buffer, so be careful
			// (again).
			case FU_LONG:
				if ((w & FUMASKRISC) == FU_MOVEI)
				{
#if 0
					address = loc + 4;

					if (eattr & DEFINED)
					{
						for(j=0; j<fwindex; j++)
						{
							if (fwdjump[j] == address)
							{
								page_jump = (address & 0xFFFFFF00) - (eval & 0xFFFFFF00);

								if (page_jump)
								{
									if (eval % 4)
									{
										err_setup();
										sprintf(buf, "* \'jump\' at $%08X - destination address not aligned for long page jump, insert a \'nop\' before the destination address", address);

										if (listing > 0)
											ship_ln(buf);

										if (err_flag)
											write(err_fd, buf, (LONG)strlen(buf));
										else
											printf("%s\n", buf);
									}          
								}
								else
								{
									if (!(eval & 0x0000000F) || ((eval - 2) % 4))
									{
										err_setup();
										sprintf(buf, "* \'jump\' at $%08X - destination address not aligned for short page jump, insert a \'nop\' before the destination address", address);

										if (listing > 0)
											ship_ln(buf);

										if (err_flag)
											write(err_fd, buf, (LONG)strlen(buf));
										else
											printf("%s\n", buf);
									}          
								}

								// Clear this jump as it has been checked
								fwdjump[j] = 0;
								j = fwindex;
							}
						}
					}
#endif

					// Long constant in MOVEI # is word-swapped, so fix it here
					eval = ((eval >> 16) & 0x0000FFFF) | ((eval << 16) & 0xFFFF0000);
					flags = (MLONG | MMOVEI);
				}
				else
					flags = MLONG;

				if (!(eattr & DEFINED))
				{
//printf("Fixup (long): Symbol undefined. loc = $%X, long = $%X, flags = $%x\n", loc, eval, flags);
					rmark(sno, loc, 0, flags, esym);
				}
				else if (tdb)
				{
//printf("Fixup (long): TDB = $%X. loc =$%X, long = $%X, flags = $%x\n", tdb, loc, eval, flags);
					rmark(sno, loc, tdb, flags, NULL);
				}
//else
//printf("Fixup (long): TDB = $%X. loc =$%X, long = $%X, flags = $%x\n", tdb, loc, eval, flags);

				*locp++ = (char)(eval >> 24);
				*locp++ = (char)(eval >> 16);
				*locp++ = (char)(eval >> 8);
				*locp = (char)eval;
				break;

			// Fixup a 3-bit "QUICK" reference in bits 9..1
			// (range of 1..8) in a word.  Really bits 1..3 in a byte.
			case FU_QUICK:
				if (!(eattr & DEFINED))
				{
					error("External quick reference");
					continue;
				}

				if (eval < 1 || eval > 8)
					goto range;

				*locp |= (eval & 7) << 1;
				break;

			// Fix up 6502 funny branch
			case FU_6BRA:
				eval -= (loc + 1);

				if (eval + 0x80 >= 0x100)
					goto range;

				*locp = (char)eval;
				break;

			default:
				interror(4);                                 // Bad fixup type
				// NOTREACHED
			}
			continue;
range:
			error("expression out of range");
		}

		ch = ch->chnext;
	}
	while (ch != NULL);

	return 0;
}

