//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// PROCLN.C - Line Processing
// Copyright (C) 199x Landon Dyer, 2011 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source Utilised with the Kind Permission of Landon Dyer
//

#include "procln.h"
#include "listing.h"
#include "amode.h"
#include "error.h"
#include "sect.h"
#include "expr.h"
#include "mach.h"
#include "direct.h"
#include "macro.h"
#include "symbol.h"
#include "risca.h"

#define DEF_KW					// Declare keyword values 
#include "kwtab.h"				// Incl generated keyword tables & defs

#define DEF_MN					// Incl 68k keyword definitions
#define DECL_MN					// Incl 68k keyword state machine tables
#include "mntab.h"

#define DEF_MR
#define DECL_MR
#include "risckw.h"

IFENT * ifent;					// Current ifent
static IFENT ifent0;			// Root ifent
static IFENT * f_ifent;			// Freelist of ifents
static int disabled;			// Assembly conditionally disabled
int just_bss;					// 1, ds.b in microprocessor mode 
VALUE pcloc;					// Value of "PC" at beginning of line 
IFENT * ifent;					// Current ifent
SYM * lab_sym;					// Label on line (or NULL)

char extra_stuff[] = "extra (unexpected) text found after addressing mode";
char * comma_error = "missing comma";
char * syntax_error = "syntax error";
char * locgl_error = "cannot GLOBL local symbol";
char * lab_ignored = "label ignored";

// Table to convert an addressing-mode number to a bitmask.
LONG amsktab[0112] = {
	M_DREG, M_DREG, M_DREG, M_DREG,
	M_DREG, M_DREG, M_DREG, M_DREG,

	M_AREG, M_AREG, M_AREG, M_AREG,
	M_AREG, M_AREG, M_AREG, M_AREG,

	M_AIND, M_AIND, M_AIND, M_AIND,
	M_AIND, M_AIND, M_AIND, M_AIND,

	M_APOSTINC, M_APOSTINC, M_APOSTINC, M_APOSTINC,
	M_APOSTINC, M_APOSTINC, M_APOSTINC, M_APOSTINC,

	M_APREDEC, M_APREDEC, M_APREDEC, M_APREDEC,
	M_APREDEC, M_APREDEC, M_APREDEC, M_APREDEC,

	M_ADISP, M_ADISP, M_ADISP, M_ADISP,
	M_ADISP, M_ADISP, M_ADISP, M_ADISP,

	M_AINDEXED, M_AINDEXED, M_AINDEXED, M_AINDEXED,
	M_AINDEXED, M_AINDEXED, M_AINDEXED, M_AINDEXED,

	M_ABSW,											// 070
	M_ABSL,											// 071
	M_PCDISP,										// 072
	M_PCINDEXED,									// 073
	M_IMMED,										// 074
	0L,												// 075
	0L,												// 076
	0L,												// 077
	M_ABASE,										// 0100
	M_MEMPOST,										// 0101 
	M_MEMPRE,										// 0102 
	M_PCBASE,										// 0103
	M_PCMPOST,										// 0104
	M_PCMPRE,										// 0105
	M_AM_USP,										// 0106
	M_AM_SR,										// 0107 
	M_AM_CCR,										// 0110
	M_AM_NONE										// 0111 
};													// 0112 length


//
// Initialize Line Processor
//
void init_procln(void)
{
	disabled = 0;
	ifent = &ifent0;
	f_ifent = ifent0.if_prev = NULL;
	ifent0.if_state = 0;
}


//
// Line Processor
//
void assemble(void)
{
	int state;					// Keyword machine state (output)
	int j;						// Random int, must be fast
	char * p;					// Random char ptr, must be fast
	TOKEN * tk;					// First token in line
	char * label;				// Symbol (or NULL)
	char * equate;				// Symbol (or NULL)
	int labtyp = 0;				// Label type (':', DCOLON)
	int equtyp = 0;				// Equ type ('=', DEQUALS)
	VALUE eval;					// Expression value
	WORD eattr;					// Expression attributes
	SYM * esym;					// External symbol involved in expr.
	WORD siz = 0;				// Size suffix to mnem/diretve/macro
	LONG amsk0, amsk1;			// Address-type masks for ea0, ea1
	MNTAB * m;					// Code generation table pointer
	SYM * sy, * sy2;			// Symbol (temp usage)
	char * opname = NULL;		// Name of dirctve/mnemonic/macro
	int listflag;				// 0: Don't call listeol()
	int as68mode = 0;			// 1: Handle multiple labels
	WORD rmask;					// Register list, for REG
	int registerbank;			// RISC register bank
	int riscreg;				// RISC register

	listflag = 0;				// Initialise listing flag

loop:							// Line processing loop label

	// Get another line of tokens
	if (tokln() == TKEOF)
	{
		if (list_flag && listflag)			// Flush last line of source
			listeol();

		if (ifent->if_prev != NULL)			// Check conditional token
			error("hit EOF without finding matching .endif");

		return;
	}

	DEBUG DumpTokenBuffer();

	if (list_flag)
	{
		if (listflag && listing > 0)
			listeol();						// Tell listing generator about EOL

		lstout((char)(disabled ? '-' : lntag));	// Prepare new line for listing
		listflag = 1;						// OK to call `listeol' now
		just_bss = 0;						// Reset just_bss mode
	}

	state = -3;								// No keyword (just EOL)
	label = NULL;							// No label
	lab_sym = NULL;							// No (exported) label
	equate = NULL;							// No equate
	tk = tok;								// Save first token in line
	pcloc = (VALUE)sloc;					// Set beginning-of-line PC

loop1:										// Internal line processing loop

	if (*tok == EOL)						// Restart loop if end-of-line
		goto loop;

	// First token MUST be a symbol
	if (*tok != SYMBOL)
	{
		error(syntax_error);
		goto loop;
	}

	j = (int)tok[2];						// Skip equates (normal statements)

	if (j == '=' || j == DEQUALS || j == SET || j == REG || j == EQUREG || j == CCDEF)
	{
		equate = (char *)tok[1];
		equtyp = j;
		tok += 3;
		goto normal;
	}

	// Skip past label (but record it)
	if (j == ':' || j == DCOLON)
	{
as68label:
		label = (char *)tok[1];				// Get label name
		labtyp = tok[2];					// Get label type
		tok += 3;							// Go to next line token

		// Handle multiple labels; if there's another label, go process it, 
		// and come back at `as68label' above.
		if (as68_flag)
		{
			as68mode = 0;

			if (*tok == SYMBOL && tok[2] == ':')
			{
				as68mode = 1;
				goto do_label;
			}
		}
	}

	if (*tok == EOL)						// EOL is legal here...
		goto normal;

	// Next token MUST be a symbol
	if (*tok++ != SYMBOL)
	{
		error(syntax_error);
		goto loop;
	}

// This is the problem here: On 64-bit platforms, this cuts the native pointer
// in half. We need to figure out how to fix this.
#warning "!!! Bad pointer !!!"
	opname = p = (char *)*tok++;			// Store opcode name here

	// Check to see if the SYMBOL is a keyword (a mnemonic or directive).
	// On output, `state' will have one of the values:
	//    -3          there was no symbol (EOL)
	//    -2..-1      the symbol didn't match any keyword
	//    0..499      vanilla directives (dc, ds, etc.)
	//    500..999    electric directives (macro, rept, etc.)
	//    1000..+     mnemonics (move, lsr, etc.)
	for(state=0; state>=0;)
	{
		j = mnbase[state] + (int)tolowertab[*p];

		// Reject, character doesn't match
		if (mncheck[j] != state)
		{
			state = -1;						// No match
			break;
		}

		// Must accept or reject at EOS
		if (!*++p)
		{
			state = mnaccept[j];			// (-1 on no terminal match)
			break;
		}

		state = mntab[j];
	}

	// Check for ".b" ".w" ".l" after directive, macro or mnemonic.
	siz = SIZN;

	if (*tok == DOTW) 
		siz = SIZW, ++tok;
	else if (*tok == DOTL)
		siz = SIZL, ++tok;
	else if (*tok == DOTB)
		siz = SIZB, ++tok;

	// Do special directives (500..999) (These must be handled in "real time")
	if (state >= 500 && state < 1000)
	{
		switch (state)
		{
		case MN_IF:
			d_if ();
		goto loop;
		case MN_ELSE:
			d_else();
			goto loop;
		case MN_ENDIF:
			d_endif ();
			goto loop;
		case MN_IIF:						// .iif --- immediate if
			if (disabled || expr(exprbuf, &eval, &eattr, &esym) != OK)
				goto loop;

			if (!(eattr & DEFINED))
			{
				error(undef_error);
				goto loop;
			}

			if (*tok++ != ',')
			{
				error(comma_error);
				goto loop;
			}

			if (eval == 0)
				goto loop;

			goto loop1;
		case MN_MACRO:						// .macro --- macro definition
			if (!disabled)
			{
				if (label != NULL)
					warn(lab_ignored);

				defmac();
			}

			goto loop;
		case MN_EXITM:						// .exitm --- exit macro
		case MN_ENDM:						// .endm --- same as .exitm
			if (!disabled)
			{
				if (label != NULL)
					warn(lab_ignored);

				exitmac();
			}

			goto loop;
		case MN_REPT:
			if (!disabled)
			{
				if (label != NULL)
					warn(lab_ignored);

				defrept();
			}

			goto loop;
		case MN_ENDR:
			if (!disabled)
				error("mis-nested .endr");

			goto loop;
		}
	}

normal:
	if (disabled)							// Conditionally disabled code
		goto loop;

	// Do equates
	if (equate != NULL)
	{
		j = 0;								// Pick global or local sym enviroment

		if (*equate == '.')
			j = curenv;

		sy = lookup(equate, LABEL, j);

		if (sy == NULL)
		{
			sy = newsym(equate, LABEL, j);
			sy->sattr = 0;

			if (equtyp == DEQUALS)
			{
				// Can't GLOBAL a local symbol
				if (j)
				{
					error(locgl_error);
					goto loop;
				}

				sy->sattr = GLOBAL;
			}
		}
		else if ((sy->sattr & DEFINED) && equtyp != SET)
		{
			if ((equtyp == EQUREG) && (sy->sattre & UNDEF_EQUR))
			{
				sy->sattre |= ~UNDEF_EQUR; 
				sy->svalue  = 0;
			}
			else if ((equtyp == CCDEF) && (sy->sattre & UNDEF_CC))
			{
				sy->sattre |= ~UNDEF_CC;
				sy->svalue = 0;
			}
			else
			{
				errors("multiple equate to '%s'", sy->sname);
				goto loop;
			}
		}

		// Put symbol in "order of definition" list
		if (!(sy->sattr & SDECLLIST))
			sym_decl(sy);

		// Parse value to equate symbol to;
		// o  .equr
		// o  .reg
		// o  everything else
		if (equtyp == EQUREG)
		{
			// Check that we are in a RISC section
			if (!rgpu && !rdsp)
			{
				error(".equr/.regequ must be defined in .gpu/.dsp section");
				goto loop;
			}

			// Check for register to equate to
			if ((*tok >= KW_R0) && (*tok <= KW_R31))
			{
				sy->sattre  = EQUATEDREG | RISCSYM;	// Mark as equated register
				riscreg = (*tok - KW_R0);
				sy->sattre |= (riscreg << 8);		// Store register number

				if ((tok[1] == ',') && (tok[2] == CONST))
				{
					tok += 3;

					if (*tok == 0)
						registerbank = BANK_0;
					else if (*tok == 1)
						registerbank = BANK_1;
					else
						registerbank = BANK_N;
				}
				else
				{
					registerbank = BANK_N;
				}

				sy->sattre |= regbank;		// Store register bank
				eattr = ABS | DEFINED | GLOBAL;
				eval = 0x80000080 + (riscreg) + (registerbank << 8);
				tok++;
			}
			// Checking for a register symbol
			else if (tok[0] == SYMBOL)
			{
				sy2 = lookup((char *)tok[1], LABEL, j);

				// Make sure symbol is a valid equreg
				if (!sy2 || !(sy2->sattre & EQUATEDREG))
				{
					error("invalid GPU/DSP .equr/.regequ definition");
					goto loop;
				}
				else
				{
					eattr = ABS | DEFINED | GLOBAL;	// Copy symbols attributes
					sy->sattre = sy2->sattre;
					eval = (sy2->svalue & 0xFFFFF0FF);
					tok += 2;
				}
			}
			else
			{
				error("invalid GPU/DSP .equr/.regequ definition");
				goto loop;
			}
		}
		else if (equtyp == REG)
		{
			if (reglist(&rmask) < 0)
				goto loop;

			eval = (VALUE)rmask;
			eattr = ABS | DEFINED;
		}
		else if (equtyp == CCDEF)
		{
			sy->sattre |= EQUATEDCC;
			eattr = ABS | DEFINED | GLOBAL;

			if (tok[0] == SYMBOL)
			{
				sy2 = lookup((char *)tok[1], LABEL, j);

				if (!sy2 || !(sy2->sattre & EQUATEDCC))
				{
					error("invalid gpu/dsp .ccdef definition");
					goto loop;
				}
				else
				{
					eattr = ABS | DEFINED | GLOBAL;
					sy->sattre = sy2->sattre;
					eval = sy2->svalue;
					tok += 2;
				}
			}
			else if (expr(exprbuf, &eval, &eattr, &esym) != OK)
				goto loop;
		}
		//equ a equr
		else if (*tok == SYMBOL)
		{
			sy2 = lookup((char *)tok[1], LABEL, j);

			if (sy2 && (sy2->sattre & EQUATEDREG))
			{
				sy->stype = sy2->stype;
				sy->sattr = sy2->sattr;
				sy->sattre = sy2->sattre;
				sy->svalue = (sy2->svalue & 0xFFFFF0FF);
				goto loop;
			}
			else if (expr(exprbuf, &eval, &eattr, &esym) != OK)
				goto loop;
		}
		else if (expr(exprbuf, &eval, &eattr, &esym) != OK)
			goto loop;

		if (!(eattr & DEFINED))
		{
			error(undef_error);
			goto loop;
		}

		sy->sattr |= eattr | EQUATED;		// Symbol inherits value and attributes
		sy->svalue = eval;

		if (list_flag)						// Put value in listing
			listvalue(eval);

		at_eol();							// Must be at EOL now
		goto loop;
	}

	// Do labels
	if (label != NULL)
	{
do_label:
		j = 0;

		if (*label == '.')
			j = curenv;

		sy = lookup(label, LABEL, j);

		if (sy == NULL)
		{
			sy = newsym(label, LABEL, j);
			sy->sattr = 0;
			sy->sattre = RISCSYM;
		}
		else if (sy->sattr & DEFINED)
		{
			errors("multiply-defined label '%s'", label);
			goto loop;
		}

		// Put symbol in "order of definition" list
		if (!(sy->sattr & SDECLLIST))
			sym_decl(sy);

		if (orgactive)
		{
			sy->svalue = orgaddr;
			sy->sattr |= ABS | DEFINED | EQUATED;
		}
		else
		{
			sy->svalue = sloc;
			sy->sattr |= DEFINED | cursect;
		}

		lab_sym = sy;

		if (!j)
			++curenv;

		// Make label global
		if (labtyp == DCOLON)
		{
			if (j)
			{
				error(locgl_error);
				goto loop;
			}

			sy->sattr |= GLOBAL;
		}

		// If we're in as68 mode, and there's another label, go back and handle it
		if (as68_flag && as68mode)
			goto as68label;
	}

	// Punt on EOL
	if (state == -3)
		goto loop;

	// If we are in GPU or DSP mode and still in need of a mnemonic then search
	// for one
	if ((rgpu || rdsp) && (state < 0 || state >= 1000))
	{
		for(state=0, p=opname; state>=0;)
		{
			j = mrbase[state] + (int)tolowertab[*p];

			// Reject, character doesn't match
			if (mrcheck[j] != state)
			{
				state = -1;					// No match
				break;
			}

			// Must accept or reject at EOS
			if (!*++p)
			{
				state = mraccept[j];		// (-1 on no terminal match)
				break;
			}

			state = mrtab[j];
		}

		// Call RISC code generator if we found a mnemonic
		if (state >= 3000)
		{
			risccg(state);
			goto loop;
		}
	}

	// Invoke macro or complain about bad mnemonic
	if (state < 0)
	{
		if ((sy = lookup(opname, MACRO, 0)) != NULL) 
			invokemac(sy, siz);
		else
			errors("unknown op '%s'", opname);

		goto loop;
	}

	// Call directive handlers
	if (state < 500)
	{
		(*dirtab[state])(siz);
		goto loop;
	}

	// Do mnemonics
	// o  can't deposit instrs in BSS or ABS
	// o  do automatic .EVEN for instrs
	// o  allocate space for largest possible instr
	// o  can't do ".b" operations with an address register
	if (scattr & SBSS)
	{
		error("cannot initialize non-storage (BSS) section");
		goto loop;
	}

	if (sloc & 1)							// Automatic .even
		auto_even();

	if (challoc - ch_size < 18)				// Make sure have space in current chunk
		chcheck(0);

	m = &machtab[state - 1000];

	// Call special-mode handler
	if (m->mnattr & CGSPECIAL)
	{
		(*m->mnfunc)(m->mninst, siz);
		goto loop;
	}

	if (amode(1) < 0)						// Parse 0, 1 or 2 addr modes
		goto loop;

	if (*tok != EOL)
		error(extra_stuff);

	amsk0 = amsktab[am0];
	amsk1 = amsktab[am1];

	// Catch attempts to use ".B" with an address register (yes, this check
	// does work at this level)
	if (siz == SIZB && (am0 == AREG || am1 == AREG))
	{
		error("cannot use '.b' with an address register");
		goto loop;
	}

	for(;;)
	{
		if ((m->mnattr & siz) && (amsk0 & m->mn0) != 0 && (amsk1 & m->mn1) != 0)
		{
			(*m->mnfunc)(m->mninst, siz);
			goto loop;
		}

		m = &machtab[m->mncont];
	}
}


// 
// .if, Start Conditional Assembly
//
int d_if(void)
{
	IFENT * rif;
	WORD eattr;
	VALUE eval;
	SYM * esym;

	// Alloc an IFENTRY
	if ((rif = f_ifent) == NULL)
//		rif = (IFENT *)amem((LONG)sizeof(IFENT));
		rif = (IFENT *)malloc(sizeof(IFENT));
	else
		f_ifent = rif->if_prev;

	rif->if_prev = ifent;
	ifent = rif;

	if (!disabled)
	{
		if (expr(exprbuf, &eval, &eattr, &esym) != OK) return 0;

		if ((eattr & DEFINED) == 0)
			return error(undef_error);

		disabled = !eval;
	}

	rif->if_state = (WORD)disabled;
	return 0;
}


// 
// .else, Do Alternate Case For .if
//
int d_else(void)
{
	IFENT * rif = ifent;

	if (rif->if_prev == NULL)
		return error("mismatched .else");

	if (disabled)
		disabled = rif->if_prev->if_state;
	else
		disabled = 1;

	rif->if_state = (WORD)disabled;
	return 0;
}


//
// .endif, End of conditional assembly block
// This is also called by fpop() to pop levels of IFENTs in case a macro or
// include file exits early with `exitm' or `end'.
//
int d_endif (void)
{
	IFENT * rif = ifent;

	if (rif->if_prev == NULL)
		return error("mismatched .endif");

	ifent = rif->if_prev;
	disabled = rif->if_prev->if_state;
	rif->if_prev = f_ifent;
	f_ifent = rif;
	return 0;
}
