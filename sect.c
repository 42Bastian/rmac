//
// RMAC - Reboot's Macro Assembler for all Atari computers
// SECT.C - Code Generation, Fixups and Section Management
// Copyright (C) 199x Landon Dyer, 2011-2018 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source utilised with the kind permission of Landon Dyer
//

#include "sect.h"
#include "6502.h"
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
void MakeSection(int, uint16_t);
void SwitchSection(int);

// Section descriptors
SECT sect[NSECTS];		// All sections...
int cursect;			// Current section number

// These are copied from the section descriptor, the current code chunk
// descriptor and the current fixup chunk descriptor when a switch is made into
// a section. They are copied back to the descriptors when the section is left.
uint16_t scattr;		// Section attributes
uint32_t sloc;			// Current loc in section

CHUNK * scode;			// Current (last) code chunk
uint32_t challoc;		// # bytes alloc'd to code chunk
uint32_t ch_size;		// # bytes used in code chunk
uint8_t * chptr;		// Deposit point in code chunk buffer
uint8_t * chptr_opcode;	// Backup of chptr, updated before entering code generators

// Return a size (SIZB, SIZW, SIZL) or 0, depending on what kind of fixup is
// associated with a location.
static uint8_t fusiztab[] = {
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
static uint8_t fusizoffs[] = {
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
// Initialize sections; setup initial ABS, TEXT, DATA and BSS sections
//
void InitSection(void)
{
	// Initialize all sections
	for(int i=0; i<NSECTS; i++)
		MakeSection(i, 0);

	// Construct default sections, make TEXT the current section
	MakeSection(ABS,   SUSED | SABS | SBSS);	// ABS
	MakeSection(TEXT,  SUSED | TEXT       );	// TEXT
	MakeSection(DATA,  SUSED | DATA       );	// DATA
	MakeSection(BSS,   SUSED | BSS  | SBSS);	// BSS
	MakeSection(M6502, SUSED | TEXT       );	// 6502 code section

	// Switch to TEXT for starters
	SwitchSection(TEXT);
}


//
// Make a new (clean) section
//
void MakeSection(int sno, uint16_t attr)
{
	SECT * p = &sect[sno];
	p->scattr = attr;
	p->sloc = 0;
	p->orgaddr = 0;
	p->scode = p->sfcode = NULL;
	p->sfix = p->sffix = NULL;
}


//
// Switch to another section (copy section & chunk descriptors to global vars
// for fast access)
//
void SwitchSection(int sno)
{
	CHUNK * cp;
	cursect = sno;
	SECT * p = &sect[sno];

	m6502 = (sno == M6502);	// Set 6502-mode flag

	// Copy section vars
	scattr = p->scattr;
	sloc = p->sloc;
	scode = p->scode;
	orgaddr = p->orgaddr;

	// Copy code chunk vars
	if ((cp = scode) != NULL)
	{
		challoc = cp->challoc;
		ch_size = cp->ch_size;
		chptr = cp->chptr + ch_size;

		// For 6502 mode, add the last org'd address
		if (m6502)
			chptr = cp->chptr + orgaddr;
	}
	else
		challoc = ch_size = 0;
}


//
// Save current section
//
void SaveSection(void)
{
	SECT * p = &sect[cursect];

	p->scattr = scattr;				// Bailout section vars
	p->sloc = sloc;
	p->orgaddr = orgaddr;

	if (scode != NULL)				// Bailout code chunk
		scode->ch_size = ch_size;
}


//
// Test to see if a location has a fixup set on it. This is used by the
// listing generator to print 'xx's instead of '00's for forward references
//
int fixtest(int sno, uint32_t loc)
{
	// Force update to sect[] variables
	StopMark();

	// Ugly linear search for a mark on our location. The speed doesn't
	// matter, since this is only done when generating a listing, which is
	// SLOW anyway.
	for(FIXUP * fp=sect[sno].sffix; fp!=NULL; fp=fp->next)
	{
		uint32_t w = fp->attr;
		uint32_t xloc = fp->loc + (int)fusizoffs[w & FUMASK];

		if (xloc == loc)
			return (int)fusiztab[w & FUMASK];
	}

	return 0;
}


//
// Check that there are at least 'amt' bytes left in the current chunk. If
// there are not, allocate another chunk of at least CH_CODE_SIZE bytes or
// 'amt', whichever is larger.
//
// If 'amt' is zero, ensure there are at least CH_THRESHOLD bytes, likewise.
//
void chcheck(uint32_t amt)
{
	DEBUG { printf("chcheck(%u)\n", amt); }

	// If in BSS section, no allocation required
	if (scattr & SBSS)
		return;

	if (amt == 0)
		amt = CH_THRESHOLD;

	DEBUG { printf("    challoc=%i, ch_size=%i, diff=%i\n", challoc, ch_size, challoc - ch_size); }

	if ((int)(challoc - ch_size) >= (int)amt)
		return;

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
		scode->ch_size = ch_size;	// Save old chunk's globals
	}

	// Setup chunk and global vars
	cp->chloc = sloc;
	cp->chnext = NULL;
	challoc = cp->challoc = amt;
	ch_size = cp->ch_size = 0;
	chptr = cp->chptr = ((uint8_t *)cp) + sizeof(CHUNK);
	scode = p->scode = cp;

	return;
}


//
// Arrange for a fixup on a location
//
int AddFixup(uint32_t attr, uint32_t loc, TOKEN * fexpr)
{
	uint16_t exprlen = 0;
	SYM * symbol = NULL;
	uint32_t _orgaddr = 0;

	// First, check to see if the expression is a bare label, otherwise, force
	// the FU_EXPR flag into the attributes and count the tokens.
	if ((fexpr[0] == SYMBOL) && (fexpr[2] == ENDEXPR))
	{
		symbol = symbolPtr[fexpr[1]];

		// Save the org address for JR RISC instruction
		if ((attr & FUMASKRISC) == FU_JR)
			_orgaddr = orgaddr;
	}
	else
	{
		attr |= FU_EXPR;
		exprlen = ExpressionLength(fexpr);
	}

	// Allocate space for the fixup + any expression
	FIXUP * fixup = malloc(sizeof(FIXUP) + (sizeof(TOKEN) * exprlen));

	// Store the relevant fixup information in the FIXUP
	fixup->next = NULL;
	fixup->attr = attr;
	fixup->loc = loc;
	fixup->fileno = cfileno;
	fixup->lineno = curlineno;
	fixup->expr = NULL;
	fixup->symbol = symbol;
	fixup->orgaddr = _orgaddr;

	// Copy the passed in expression to the FIXUP, if any
	if (exprlen > 0)
	{
		fixup->expr = (TOKEN *)((uint8_t *)fixup + sizeof(FIXUP));
		memcpy(fixup->expr, fexpr, sizeof(TOKEN) * exprlen);
	}

	// Finally, put the FIXUP in the current section's linked list
	if (sect[cursect].sffix == NULL)
	{
		sect[cursect].sffix = fixup;
		sect[cursect].sfix = fixup;
	}
	else
	{
		sect[cursect].sfix->next = fixup;
		sect[cursect].sfix = fixup;
	}

	DEBUG { printf("AddFixup: sno=%u, l#=%u, attr=$%X, loc=$%X, expr=%p, sym=%p, org=$%X\n", cursect, fixup->lineno, fixup->attr, fixup->loc, (void *)fixup->expr, (void *)fixup->symbol, fixup->orgaddr);
		if (symbol != NULL)
			printf("          name: %s, value: $lX\n", symbol->sname, symbol->svalue);
	}

	return 0;
}


//
// Resolve fixups in the passed in section
//
int ResolveFixups(int sno)
{
	SECT * sc = &sect[sno];

	// "Cache" first chunk
	CHUNK * cch = sc->sfcode;

	// Can't fixup a section with nothing in it
	if (cch == NULL)
		return 0;

	// Wire the 6502 segment's size to its allocated size (64K)
	if (sno == M6502)
		cch->ch_size = cch->challoc;

	// Get first fixup for the passed in section
	FIXUP * fixup = sect[sno].sffix;

	while (fixup != NULL)
	{
		// We do it this way because we have continues everywhere... :-P
		FIXUP * fup = fixup;
		fixup = fixup->next;

		uint32_t w = fup->attr;		// Fixup long (type + modes + flags)
		uint32_t loc = fup->loc;	// Location to fixup
		cfileno = fup->fileno;
		curlineno = fup->lineno;
		DEBUG { printf("ResolveFixups: sect#=%u, l#=%u, attr=$%X, loc=$%X, expr=%p, sym=%p, org=$%X\n", sno, fup->lineno, fup->attr, fup->loc, (void *)fup->expr, (void *)fup->symbol, fup->orgaddr); }

		// This is based on global vars cfileno, curfname :-P
		// This approach is kinda meh as well. I think we can do better
		// than this.
		SetFilenameForErrorReporting();

		// Search for chunk containing location to fix up; compute a
		// pointer to the location (in the chunk). Often we will find the
		// Fixup is in the "cached" chunk, so the linear-search is seldom
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
				// Fixup (loc) is out of range--this should never happen!
				// Once we call this function, it winds down immediately; it
				// doesn't return.
				interror(7);
			}
		}

		// Location to fix (in current chunk)
		// We use the address of the chunk that loc is actually in, then
		// subtract the chunk's starting location from loc to get the offset
		// into the current chunk.
		uint8_t * locp = cch->chptr + (loc - cch->chloc);

		uint16_t eattr = 0;			// Expression attrib
		SYM * esym = NULL;			// External symbol involved in expr
		uint64_t eval;				// Expression value
		uint16_t flags;				// Mark flags

		// Compute expression/symbol value and attributes

		// Complex expression
		if (w & FU_EXPR)
		{
			if (evexpr(fup->expr, &eval, &eattr, &esym) != OK)
				continue;
		}
		// Simple symbol
		else
		{
			SYM * sy = fup->symbol;
			eattr = sy->sattr;

			if (eattr & DEFINED)
				eval = sy->svalue;
			else
				eval = 0;

			// If the symbol is not defined, but global, set esym to sy
			if ((eattr & (GLOBAL | DEFINED)) == GLOBAL)
				esym = sy;
		}

		uint16_t tdb = eattr & TDB;

		// If the expression/symbol is undefined and no external symbol is
		// involved, then that's an error.
		if (!(eattr & DEFINED) && (esym == NULL))
		{
			error(undef_error);
			continue;
		}

		// Do the fixup
		//
		// If a PC-relative fixup is undefined, its value is *not* subtracted
		// from the location (that will happen in the linker when the external
		// reference is resolved).
		//
		// MWC expects PC-relative things to have the LOC subtracted from the
		// value, if the value is external (that is, undefined at this point).
		//
		// PC-relative fixups must be DEFINED and either in the same section
		// (whereupon the subtraction takes place) or ABS (with no subtract).
		if (w & FU_PCREL)
		{
			if (eattr & DEFINED)
			{
				if (tdb == sno)
					eval -= loc;
				else if (tdb)
				{
					// Allow cross-section PCREL fixups in Alcyon mode
					if (prg_flag)
					{
						switch (tdb)
						{
						case TEXT:
// Shouldn't there be a break here, since otherwise, it will point to the DATA section?
//							break;
						case DATA:
							eval += sect[TEXT].sloc;
							break;
						case BSS:
							eval += sect[TEXT].sloc + sect[DATA].sloc;
							break;
						default:
							error("invalid section");
						break;
						}

						eval -= loc;
					}
					else
					{
						error("PC-relative expr across sections");
						continue;
					}
				}

				if (sbra_flag && (w & FU_LBRA) && (eval + 0x80 < 0x100))
					warn("unoptimized short branch");
			}
			else if (obj_format == MWC)
				eval -= loc;

			tdb = 0;
			eattr &= ~TDB;
		}

		// Handle fixup classes
		switch (w & FUMASK)
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
				goto rangeErr;

			if (eval == 0)
			{
				if (CHECK_OPTS(OPT_NULL_BRA))
				{
					// Just output a NOP
					*locp++ = 0x4E;
					*locp = 0x71;
					continue;
				}
				else
				{
					error("illegal bra.s with zero offset");
					continue;
				}
			}

			*++locp = (uint8_t)eval;
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

			if ((w & FU_PCREL) && ((eval + 0x80) >= 0x100))
				goto rangeErr;

			if (w & FU_SEXT)
			{
				if ((eval + 0x100) >= 0x200)
					goto rangeErr;
			}
			else if (eval >= 0x100)
				goto rangeErr;

			*locp = (uint8_t)eval;
			break;

		// Fixup high/low byte off word for 6502
		case FU_BYTEH:
			if (!(eattr & DEFINED))
			{
				error("external byte reference");
				continue;
			}

			*locp = (uint8_t)(eval >> 8);
			break;

		case FU_BYTEL:
			if (!(eattr & DEFINED))
			{
				error("external byte reference");
				continue;
			}

			*locp = (uint8_t)eval;
			break;

		// Fixup WORD forward references; the word could be unaligned in the
		// section buffer, so we have to be careful.
		case FU_WORD:
			if ((w & FUMASKRISC) == FU_JR)
			{
				int reg;

				if (fup->orgaddr)
					reg = (signed)((eval - (fup->orgaddr + 2)) / 2);
				else
					reg = (signed)((eval - (loc + 2)) / 2);

				if ((reg < -16) || (reg > 15))
				{
					error("relative jump out of range");
					break;
				}

				*locp |= ((uint8_t)reg >> 3) & 0x03;
				locp++;
				*locp |= ((uint8_t)reg & 0x07) << 5;
				break;
			}
			else if ((w & FUMASKRISC) == FU_NUM15)
			{
				if (((int)eval < -16) || ((int)eval > 15))
				{
					error("constant out of range (-16 - +15)");
					break;
				}

				*locp |= ((uint8_t)eval >> 3) & 0x03;
				locp++;
				*locp |= ((uint8_t)eval & 0x07) << 5;
				break;
			}
			else if ((w & FUMASKRISC) == FU_NUM31)
			{
				if (eval > 31)
				{
					error("constant out of range (0-31)");
					break;
				}

				*locp |= ((uint8_t)eval >> 3) & 0x03;
				locp++;
				*locp |= ((uint8_t)eval & 0x07) << 5;
				break;
			}
			else if ((w & FUMASKRISC) == FU_NUM32)
			{
				if ((eval < 1) || (eval > 32))
				{
					error("constant out of range (1-32)");
					break;
				}

				if (w & FU_SUB32)
					eval = (32 - eval);

				eval = (eval == 32) ? 0 : eval;
				*locp |= ((uint8_t)eval >> 3) & 0x03;
				locp++;
				*locp |= ((uint8_t)eval & 0x07) << 5;
				break;
			}
			else if ((w & FUMASKRISC) == FU_REGONE)
			{
				if (eval > 31)
				{
					error("register one value out of range");
					break;
				}

				*locp |= ((uint8_t)eval >> 3) & 0x03;
				locp++;
				*locp |= ((uint8_t)eval & 0x07) << 5;
				break;
			}
			else if ((w & FUMASKRISC) == FU_REGTWO)
			{
				if (eval > 31)
				{
					error("register two value out of range");
					break;
				}

				locp++;
				*locp |= (uint8_t)eval & 0x1F;
				break;
			}

			if (!(eattr & DEFINED))
			{
				flags = MWORD;

				if (w & FU_PCREL)
					flags |= MPCREL;

				MarkRelocatable(sno, loc, 0, flags, esym);
			}
			else
			{
				if (tdb)
					MarkRelocatable(sno, loc, tdb, MWORD, NULL);

				if (w & FU_SEXT)
				{
					if (eval + 0x10000 >= 0x20000)
						goto rangeErr;
				}
				else
				{
					// Range-check BRA and DBRA
					if (w & FU_ISBRA)
					{
						if (eval + 0x8000 >= 0x10000)
							goto rangeErr;
					}
					else if (eval >= 0x10000)
						goto rangeErr;
				}
			}

			// 6502 words are little endian, so handle that here
			if (sno == M6502)
				SETLE16(locp, 0, eval)
			else
				SETBE16(locp, 0, eval)

			break;

		// Fixup LONG forward references;
		// the long could be unaligned in the section buffer, so be careful
		// (again).
		case FU_LONG:
			flags = MLONG;

			if ((w & FUMASKRISC) == FU_MOVEI)
			{
				// Long constant in MOVEI # is word-swapped, so fix it here
				eval = WORDSWAP32(eval);
				flags |= MMOVEI;
			}

			// If the symbol is undefined, make sure to pass the symbol in
			// to the MarkRelocatable() function.
			if (!(eattr & DEFINED))
				MarkRelocatable(sno, loc, 0, flags, esym);
			else if (tdb)
				MarkRelocatable(sno, loc, tdb, flags, NULL);

			SETBE32(locp, 0, eval);
			break;

		// Fixup a 3-bit "QUICK" reference in bits 9..1
		// (range of 1..8) in a word. [Really bits 1..3 in a byte.]
		case FU_QUICK:
			if (!(eattr & DEFINED))
			{
				error("External quick reference");
				continue;
			}

			if ((eval < 1) || (eval > 8))
				goto rangeErr;

			*locp |= (eval & 7) << 1;
			break;

		// Fix up 6502 funny branch
		case FU_6BRA:
			eval -= (loc + 1);

			if (eval + 0x80 >= 0x100)
				goto rangeErr;

			*locp = (uint8_t)eval;
			break;

		// Fixup a 4-byte float
		case FU_FLOATSING:
			warn("FU_FLOATSING missing implementation\n%s", "And you may ask yourself, \"Self, how did I get here?\"");
			break;

		// Fixup a 8-byte float
		case FU_FLOATDOUB:
			warn("FU_FLOATDOUB missing implementation\n%s", "And you may ask yourself, \"Self, how did I get here?\"");
			break;

		// Fixup a 12-byte float
		case FU_FLOATEXT:
			warn("FU_FLOATEXT missing implementation\n%s", "And you may ask yourself, \"Self, how did I get here?\"");
			break;

		default:
			// Bad fixup type--this should *never* happen!
			// Once we call this function, it winds down immediately; it
			// doesn't return.
			interror(4);
		}

		continue;
rangeErr:
		error("expression out of range");
	}

	return 0;
}


//
// Resolve all fixups
//
int ResolveAllFixups(void)
{
	// Make undefined symbols GLOBL
	if (glob_flag)
		ForceUndefinedSymbolsGlobal();

	DEBUG printf("Resolving TEXT sections...\n");
	ResolveFixups(TEXT);
	DEBUG printf("Resolving DATA sections...\n");
	ResolveFixups(DATA);
	DEBUG printf("Resolving 6502 sections...\n");
	ResolveFixups(M6502);		// Fixup 6502 section (if any)

	return 0;
}

