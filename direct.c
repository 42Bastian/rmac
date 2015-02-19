//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// DIRECT.C - Directive Handling
// Copyright (C) 199x Landon Dyer, 2011 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source utilised with the kind permission of Landon Dyer
//

#include "direct.h"
#include "sect.h"
#include "riscasm.h"
#include "error.h"
#include "token.h"
#include "procln.h"
#include "expr.h"
#include "mach.h"
#include "listing.h"
#include "mark.h"
#include "symbol.h"

#define DEF_KW
#include "kwtab.h"


TOKEN exprbuf[128];			// Expression buffer 
SYM * symbolPtr[1000000];	// Symbol pointers table
static long unused;			// For supressing 'write' warnings
char buffer[256];			// Scratch buffer for messages


// Directive handler table
int (*dirtab[])() = {
	d_org,				// 0 org
	d_even,				// 1 even
	d_unimpl,			// 2 .6502
	d_68000,			// 3 .68000 
	d_bss,				// 4 bss
	d_data,				// 5 data 
	d_text,				// 6 text 
	d_abs,				// 7 abs 
	d_comm,				// 8 comm 
	d_init,				// 9 init 
	d_cargs,			// 10 cargs 
	d_goto,				// 11 goto 
	d_dc,				// 12 dc 
	d_ds,				// 13 ds 
	d_undmac,			// 14 undefmac 
	d_gpu,				// 15 .gpu
	d_dsp,				// 16 .dsp
	d_dcb,				// 17 dcb 
	d_unimpl,			// 18* set 
	d_unimpl,			// 19* reg 
	d_unimpl,			// 20 dump 
	d_incbin,			// 21 .incbin //load 
	d_unimpl,			// 22 disable 
	d_unimpl,			// 23 enable 
	d_globl,			// 24 globl 
	d_regbank0,			// 25 .regbank0
	d_regbank1,			// 26 .regbank1
	d_unimpl,			// 27 xdef 
	d_assert,			// 28 assert 
	d_unimpl,			// 29* if 
	d_unimpl,			// 30* endif 
	d_unimpl,			// 31* endc 
	d_unimpl,			// 32* iif 
	d_include,			// 33 include 
	fpop,				// 34 end 
	d_unimpl,			// 35* macro 
	ExitMacro,			// 36* exitm 
	d_unimpl,			// 37* endm 
	d_list,				// 38 list 
	d_nlist,			// 39 nlist 
	d_long,				// 40* rept 
	d_phrase,			// 41* endr 
	d_dphrase,			// 42 struct 
	d_qphrase,			// 43 ends 
	d_title,			// 44 title 
	d_subttl,			// 45 subttl 
	eject,				// 46 eject 
	d_unimpl,			// 47 error 
	d_unimpl,			// 48 warn 
	d_noclear,			// 49 .noclear
	d_equrundef,		// 50 .equrundef/.regundef
	d_ccundef,			// 51 .ccundef
	d_print,			// 52 .print
	d_cstruct,			// 53 .cstruct
	d_jpad,				// 54 .jpad (deprecated)
	d_nojpad,			// 55 .nojpad (deprecated)
	d_gpumain,			// 56 .gpumain (deprecated)
};


//
// .org - Set origin
//
int d_org(void)
{
	VALUE address;

	if (!rgpu && !rdsp) 
		return error(".org permitted only in gpu/dsp section");

	orgaddr = 0;

	if (abs_expr(&address) == ERROR)
	{
		error("cannot determine org'd address");
		return ERROR;
	}

	orgaddr = address;
	orgactive = 1;
//printf("ORG: address = $%08X...\n", orgaddr);

	return 0;
}


//
// Print directive
//
int d_print(void)
{
	char prntstr[LNSIZ];		// String for PRINT directive
	char format[LNSIZ];			// Format for PRINT directive
	int formatting = 0;			// Formatting on/off
	int wordlong = 0;			// WORD = 0, LONG = 1
	int outtype = 0;			// 0:hex, 1:decimal, 2:unsigned

	VALUE eval;					// Expression value
	WORD eattr;					// Expression attributes
	SYM * esym;					// External symbol involved in expr.
	TOKEN r_expr[EXPRSIZE];

	while (*tok != EOL)
	{
		switch(*tok)
		{
		case STRING:
			sprintf(prntstr, "%s", string[tok[1]]);
			printf("%s", prntstr);

			if (list_fd) 
				unused = write(list_fd, prntstr, (LONG)strlen(prntstr));

			tok += 2;
			break;
		case '/':
			formatting = 1;

			if (tok[1] != SYMBOL)
				goto token_err;

//			strcpy(prntstr, (char *)tok[2]);
			strcpy(prntstr, string[tok[2]]);

			switch(prntstr[0])
			{
			case 'l': case 'L': wordlong = 1; break;
			case 'w': case 'W': wordlong = 0; break;
			case 'x': case 'X': outtype  = 0; break;
			case 'd': case 'D': outtype  = 1; break;
			case 'u': case 'U': outtype  = 2; break;
			default:
				error("unknown print format flag");
				return ERROR;
			}

			tok += 3;
			break;
		case ',':
			tok++;
			break;
		default:
			if (expr(r_expr, &eval, &eattr, &esym) != OK)
				goto token_err;
			else
			{
				switch(outtype)
				{
				case 0: strcpy(format, "%X"); break; 
				case 1: strcpy(format, "%d" ); break; 
				case 2: strcpy(format, "%u" ); break; 
				}

				if (wordlong)
					sprintf(prntstr, format, eval);
				else
					sprintf(prntstr, format, eval & 0xFFFF);

				printf("%s", prntstr);

				if (list_fd) 
					unused = write(list_fd, prntstr, (LONG)strlen(prntstr));

				formatting = 0;
				wordlong = 0;
				outtype = 0;
			}

			break;
		}
	}

	printf("\n");

	return 0;

token_err:
	error("illegal print token");
	return ERROR;
}


//
// Undefine an equated condition code
//
int d_ccundef(void)
{
	SYM * ccname;

	// Check that we are in a RISC section
	if (!rgpu && !rdsp)
	{
		error(".ccundef must be defined in .gpu/.dsp section");
		return ERROR;
	}

	if (*tok != SYMBOL)
	{
		error("syntax error; expected symbol");
		return ERROR;
	}

	ccname = lookup(string[tok[1]], LABEL, 0);

	// Make sure symbol is a valid ccdef
	if (!ccname || !(ccname->sattre & EQUATEDCC))
	{
		error("invalid equated condition name specified");
		return ERROR;
	}

	ccname->sattre |= UNDEF_CC;

	return 0;
}


//
// Undefine an equated register
//
int d_equrundef(void)
{
	SYM * regname;

	// Check that we are in a RISC section
	if (!rgpu && !rdsp)
		return error(".equrundef/.regundef must be defined in .gpu/.dsp section");

	while (*tok != EOL)
	{
		// Skip preceeding or seperating commas (if any)
		if (*tok == ',')
			tok++;

		// Check we are dealing with a symbol
		if (*tok != SYMBOL)
			return error("syntax error; expected symbol");

		// Lookup and undef if equated register
		regname = lookup(string[tok[1]], LABEL, 0);

		if (regname && (regname->sattre & EQUATEDREG))
		{
			// Reset the attributes of this symbol...
			regname->sattr = 0;
			regname->sattre &= ~(EQUATEDREG | BANK_0 | BANK_1);
			regname->sattre |= UNDEF_EQUR;
		}

		// Skip over symbol token and address
		tok += 2;
	}

	return 0;
}


//
// Do not allow use of the CLR.L opcode
//
int d_noclear(void)
{
	return 0;
}


// 
// Include binary file
//
int d_incbin(void)
{
	int fd;
	int bytes = 0;
	long pos, size, bytesRead;
	char msg[256];

	// Check to see if we're in BSS, and, if so, throw an error
	if (scattr & SBSS)
	{
		errors("cannot include binary file \"%s\" in BSS section", string[tok[1]]);
		return ERROR;
	}

	if (*tok != STRING)
	{
		error("syntax error; string missing");
		return ERROR;
	}

	if ((fd = open(string[tok[1]],  _OPEN_INC)) < 0)
	{
		errors("cannot open include binary file (%s)", string[tok[1]]);
		return ERROR;
	}

	size = lseek(fd, 0L, SEEK_END);
	pos = lseek(fd, 0L, SEEK_SET);
	chcheck(size);

	DEBUG { printf("INCBIN: File '%s' is %li bytes.\n", string[tok[1]], size); }

	char * fileBuffer = (char *)malloc(size);
	bytesRead = read(fd, fileBuffer, size);

	if (bytesRead != size)
	{
		sprintf(msg, "was only able to read %li bytes from binary file (%s, %li bytes)", bytesRead, string[tok[1]], size);
		error(msg);
		return ERROR;
	}

	memcpy(chptr, fileBuffer, size);
	chptr += size;
	sloc += size;
	ch_size += size;

	if (orgactive)
		orgaddr += size;

	free(fileBuffer);
	close(fd);
	return 0;
}


// 
// Set RISC register banks
//
int d_regbank0(void)
{
	// Set active register bank zero
	regbank = BANK_0;
	return 0;
}


int d_regbank1(void)
{
	// Set active register bank one
	regbank = BANK_1;
	return 0;
}


//
// Helper function, to cut down on mistakes & typing
//
static inline void SkipBytes(unsigned bytesToSkip)
{
	if (!bytesToSkip)
		return;

	if ((scattr & SBSS) == 0)
	{
		chcheck(bytesToSkip);
		D_ZEROFILL(bytesToSkip);
	}
	else
	{
		sloc += bytesToSkip;

		if (orgactive)
			orgaddr += bytesToSkip;
	}
}


//
// Adjust location to an EVEN value
//
int d_even(void)
{
	unsigned skip = (rgpu || rdsp ? orgaddr : sloc) & 0x01;
	
	if (skip)
	{
		if ((scattr & SBSS) == 0)
		{
			chcheck(1);
			D_byte(0);
		}
		else
		{
			sloc++;

			if (orgactive)
				orgaddr++;
		}
	}

	return 0;
}


//
// Adjust location to a LONG value
//
int d_long(void)
{
	unsigned lower2Bits = (rgpu || rdsp ? orgaddr : sloc) & 0x03;
	unsigned bytesToSkip = (0x04 - lower2Bits) & 0x03;
	SkipBytes(bytesToSkip);

	return 0;
}


//
// Adjust location to a PHRASE value
//
// N.B.: We have to handle the GPU/DSP cases separately because you can embed
//       RISC code in the middle of a regular 68K section. Also note that all
//       of the alignment pseudo-ops will have to be fixed this way.
//
// This *must* behave differently when in a RISC section, as following sloc
// (instead of orgaddr) will fuck things up royally. Note that we do it this
// way because you can embed RISC code in a 68K section, and have the origin
// pointing to a different alignment in the RISC section than the 68K section.
//
int d_phrase(void)
{
	unsigned lower3Bits = (rgpu || rdsp ? orgaddr : sloc) & 0x07;
	unsigned bytesToSkip = (0x08 - lower3Bits) & 0x07;
	SkipBytes(bytesToSkip);

	return 0;
}


//
// Adjust location to a DPHRASE value
//
int d_dphrase(void)
{
	unsigned lower4Bits = (rgpu || rdsp ? orgaddr : sloc) & 0x0F;
	unsigned bytesToSkip = (0x10 - lower4Bits) & 0x0F;
	SkipBytes(bytesToSkip);

	return 0;
}


//
// Adjust location to a QPHRASE value
//
int d_qphrase(void)
{
	unsigned lower5Bits = (rgpu || rdsp ? orgaddr : sloc) & 0x1F;
	unsigned bytesToSkip = (0x20 - lower5Bits) & 0x1F;
	SkipBytes(bytesToSkip);

	return 0;
}


//
// Do auto-even.  This must be called ONLY if 'sloc' is odd.
//
// This is made hairy because, if there was a label on the line, we also have
// to adjust its value. This won't work with more than one label on the line,
// which is OK since multiple labels are only allowed in AS68 kludge mode, and
// the C compiler is VERY paranoid and uses ".even" whenever it can
//
// N.B.: This probably needs the same fixes as above...
//
void auto_even(void)
{
	if (scattr & SBSS)
		sloc++;				// Bump BSS section
	else
		D_byte(0);			// Deposit 0.b in non-BSS

	if (lab_sym != NULL)	// Bump label if we have to
		lab_sym->svalue++;
}


//
// Unimplemened directive error
//
int d_unimpl(void)
{
	return error("unimplemented directive");
}


// 
// Return absolute (not TDB) and defined expression or return an error
//
int abs_expr(VALUE * a_eval)
{
	WORD eattr;

	if (expr(exprbuf, a_eval, &eattr, NULL) < 0)
		return ERROR;

	if (!(eattr & DEFINED))
		return error(undef_error);

	if (eattr & TDB)
		return error(rel_error);

	return OK;
}


//
// Hand symbols in a symbol-list to a function (kind of like mapcar...)
//
int symlist(int(* func)())
{
	const char * em = "symbol list syntax";

	for(;;)
	{
		if (*tok != SYMBOL)
			return error(em);

		if ((*func)(string[tok[1]]) != OK)
			break;

		tok += 2;

		if (*tok == EOL)
			break;

		if (*tok != ',')
			return error(em);

		tok++;
	}

	return 0;
}


//
// .include "filename"
//
int d_include(void)
{
	int j;
	int i;
	char * fn;
	char buf[128];
	char buf1[128];

	if (*tok == STRING)			// Leave strings ALONE 
		fn = string[*++tok];
	else if (*tok == SYMBOL)	// Try to append ".s" to symbols
	{
		strcpy(buf, string[*++tok]);
		fext(buf, ".s", 0);
		fn = &buf[0];
	}
	else						// Punt if no STRING or SYMBOL 
		return error("missing filename");

	// Make sure the user didn't try anything like:
	// .include equates.s
	if (*++tok != EOL)
		return error("extra stuff after filename -- enclose it in quotes");

	// Attempt to open the include file in the current directory, then (if that
	// failed) try list of include files passed in the enviroment string or by
	// the "-d" option.
	if ((j = open(fn, 0)) < 0)
	{
		for(i=0; nthpath("RMACPATH", i, buf1)!=0; i++)
		{
			j = strlen(buf1);

			if (j > 0 && buf1[j - 1] != SLASHCHAR)	// Append path char if necessary 
				strcat(buf1, SLASHSTRING);

			strcat(buf1, fn);

			if ((j = open(buf1, 0)) >= 0)
				goto allright;
		}

		return errors("cannot open: \"%s\"", fn);
	}

allright:
	include(j, fn);
	return 0;
}


//
// .assert expression [, expression...]
//
int d_assert(void)
{
	WORD eattr;
	VALUE eval;

	for(; expr(exprbuf, &eval, &eattr, NULL)==OK; ++tok)
	{
		if (!(eattr & DEFINED))
			return error("forward or undefined .assert");

		if (!eval)
			return error("assert failure");

		if (*tok != ',')
			break;
	}

	at_eol();
	return 0;
}


//
// .globl symbol [, symbol] <<<cannot make local symbols global>>>
//
int globl1(char * p)
{
	SYM * sy;

	if (*p == '.')
		return error("cannot .globl local symbol");

	if ((sy = lookup(p, LABEL, 0)) == NULL)
	{
		sy = NewSymbol(p, LABEL, 0);
		sy->svalue = 0;
		sy->sattr = GLOBAL;
//printf("glob1: Making global symbol: attr=%04X, eattr=%08X, %s\n", sy->sattr, sy->sattre, sy->sname);
	}
	else 
		sy->sattr |= GLOBAL;

	return OK;
}


int d_globl(void)
{
	symlist(globl1);
	return 0;
}


//
// .abs [expression]
//
int d_abs(void)
{
	VALUE eval;

	SaveSection();

	if (*tok == EOL)
		eval = 0;
	else if (abs_expr(&eval) != OK)
		return 0;

	SwitchSection(ABS);
	sloc = eval;
	return 0;
}


//
// Switch segments
//
int d_text(void)
{
	if (rgpu || rdsp)
		return error("directive forbidden in gpu/dsp mode");

	if (cursect != TEXT)
	{
		SaveSection();
		SwitchSection(TEXT);
	}

	return 0;
}


int d_data(void)
{
	if (rgpu || rdsp)
		return error("directive forbidden in gpu/dsp mode");

	if (cursect != DATA)
	{
		SaveSection();
		SwitchSection(DATA);
	}

	return 0;
}


int d_bss(void)
{
	if (rgpu || rdsp)
		return error("directive forbidden in gpu/dsp mode");

	if (cursect != BSS)
	{
		SaveSection();
		SwitchSection(BSS);
	}

	return 0;
}


//
// .ds[.size] expression
//
int d_ds(WORD siz)
{
if (verb_flag)
	printf("Directive: .ds.[size] = %u, sloc = $%X\n", siz, sloc);

	VALUE eval;

	// This gets kind of stupid.  This directive is disallowed in normal 68000
	// mode ("for your own good!"), but is permitted for 6502 and Alcyon-
	// compatibility modes. For obvious reasons, no auto-even is done in 8-bit
	// processor modes.
	if (as68_flag == 0 && (scattr & SBSS) == 0)
		return error(".ds permitted only in BSS");

	if (siz != SIZB && (sloc & 1))                            // Automatic .even 
		auto_even();

	if (abs_expr(&eval) != OK)
		return 0;

	// In non-TDB section (BSS, ABS and M6502) just advance the location
	// counter appropriately. In TDB sections, deposit (possibly large) chunks
	//of zeroed memory....
	if (scattr & SBSS)
	{
		listvalue(eval);
		eval *= siz;
		sloc += eval;
		just_bss = 1;                                         // No data deposited (8-bit CPU mode)
	}
	else
	{
		dep_block(eval, siz, (VALUE)0, (WORD)(DEFINED | ABS), NULL);
	}

	at_eol();
	return 0;
}


//
// dc.b, dc.w / dc, dc.l
//
int d_dc(WORD siz)
{
	WORD eattr;
	VALUE eval;
	WORD tdb;
	WORD defined;
	LONG i;
	char * p;
	int movei = 0; // movei flag for dc.i

	if ((scattr & SBSS) != 0)
		return error("illegal initialization of section");

	if ((siz != SIZB) && (sloc & 1))
		auto_even();

	for(;; ++tok)
	{
		// dc.b 'string' [,] ...
		if (siz == SIZB && *tok == STRING && (tok[2] == ',' || tok[2] == EOL))
		{
			i = strlen(string[tok[1]]);

			if ((challoc - ch_size) < i) 
				chcheck(i);

			for(p=string[tok[1]]; *p!=EOS; ++p)
				D_byte(*p);

			tok += 2;
			goto comma;
		}

		if (*tok == 'I')
		{
			movei = 1;
			tok++;
			siz = SIZL;
		}

		// dc.x <expression>
		if (expr(exprbuf, &eval, &eattr, NULL) != OK)
			return 0;

		tdb = (WORD)(eattr & TDB);
		defined = (WORD)(eattr & DEFINED);

		if ((challoc - ch_size) < 4)
			chcheck(4);

		switch (siz)
		{
		case SIZB:
			if (!defined)
			{
				AddFixup(FU_BYTE | FU_SEXT, sloc, exprbuf);
				D_byte(0);
			}
			else
			{
				if (tdb)
					return error("non-absolute byte value");

				if (eval + 0x100 >= 0x200)
				{
					sprintf(buffer, "%s (value = $%X)", range_error, eval);
//					return error(range_error);
					return error(buffer);
				}

				D_byte(eval);
			}

			break;
		case SIZW:
		case SIZN:
			if (!defined)
			{
				AddFixup(FU_WORD | FU_SEXT, sloc, exprbuf);
				D_word(0);
			}
			else
			{
				if (tdb)
					rmark(cursect, sloc, tdb, MWORD, NULL);

				if (eval + 0x10000 >= 0x20000)
					return error(range_error);

				// Deposit 68000 or 6502 (byte-reversed) word 
				D_word(eval);
			}

			break;
		case SIZL:
			if (!defined)
			{
				if (movei)
					AddFixup(FU_LONG | FU_MOVEI, sloc, exprbuf);
				else
					AddFixup(FU_LONG, sloc, exprbuf);

				D_long(0);
			}
			else
			{
				if (tdb)
					rmark(cursect, sloc, tdb, MLONG, NULL);

				if (movei) 
					eval = ((eval >> 16) & 0x0000FFFF) | ((eval << 16) & 0xFFFF0000);

				D_long(eval);
			}
			break;
		}
		
comma:
		if (*tok != ',')
			break;
	}

	at_eol();
	return 0;
}


//
// dcb[.siz] expr1,expr2 - Make 'expr1' copies of 'expr2'
//
int d_dcb(WORD siz)
{
	VALUE evalc, eval;
	WORD eattr;

	if ((scattr & SBSS) != 0)
		return error("illegal initialization of section");

	if (abs_expr(&evalc) != OK)
		return 0;

	if (*tok++ != ',')
		return error("missing comma");

	if (expr(exprbuf, &eval, &eattr, NULL) < 0)
		return 0;

	if ((siz != SIZB) && (sloc & 1))
		auto_even();

	dep_block(evalc, siz, eval, eattr, exprbuf);
	return 0;
}


//
// Generalized initialization directive
// 
// .init[.siz] [#count,] expression [.size] , ...
// 
// The size suffix on the ".init" directive becomes the default size of the
// objects to deposit. If an item is preceeded with a sharp (immediate) sign
// and an expression, it specifies a repeat count. The value to be deposited
// may be followed by a size suffix, which overrides the default size.
//
int d_init(WORD def_siz)
{
	VALUE count;
	VALUE eval;
	WORD eattr;
	WORD siz;

	if ((scattr & SBSS) != 0)
		return error(".init not permitted in BSS or ABS");

	if (rgpu || rdsp)
		return error("directive forbidden in gpu/dsp mode");

	for(;;)
	{
		// Get repeat count (defaults to 1)
		if (*tok == '#')
		{
			++tok;

			if (abs_expr(&count) != OK)
				return 0;

			if (*tok++ != ',')
				return error(comma_error);
		}
		else
			count = 1;

		// Evaluate expression to deposit
		if (expr(exprbuf, &eval, &eattr, NULL) < 0)
			return 0;

		switch ((int)*tok++)
		{                                 // Determine size of object to deposit
		case DOTB: siz = SIZB; break;
		case DOTW: siz = SIZB; break;
		case DOTL: siz = SIZL; break;
		default: 
			siz = def_siz;
			--tok;
			break;
		}

		dep_block(count, siz, eval, eattr, exprbuf);

		switch ((int)*tok)
		{
		case EOL:
			return 0;
		case ',':
			++tok;
			continue;
		default:
			return error(comma_error);
		}
	}
}


//
// Deposit 'count' values of size 'siz' in the current (non-BSS) segment
//
int dep_block(VALUE count, WORD siz, VALUE eval, WORD eattr, TOKEN * exprbuf)
{
	WORD tdb;
	WORD defined;

	tdb = (WORD)(eattr & TDB);
	defined = (WORD)(eattr & DEFINED);

	while (count--)
	{
		if ((challoc - ch_size) < 4)
			chcheck(4L);

		switch(siz)
		{
		case SIZB:
			if (!defined)
			{
				AddFixup(FU_BYTE | FU_SEXT, sloc, exprbuf);
				D_byte(0);
			}
			else
			{
				if (tdb)
					return error("non-absolute byte value");

				if (eval + 0x100 >= 0x200)
					return error(range_error);

				D_byte(eval);
			}

			break;
		case SIZW:
		case SIZN:
			if (!defined)
			{
				AddFixup(FU_WORD | FU_SEXT, sloc, exprbuf);
				D_word(0);
			}
			else
			{
				if (tdb)
					rmark(cursect, sloc, tdb, MWORD, NULL);

				if (eval + 0x10000 >= 0x20000)
					return error(range_error);

				// Deposit 68000 or 6502 (byte-reversed) word
				D_word(eval);
			}

			break;
		case SIZL:
			if (!defined)
			{
				AddFixup(FU_LONG, sloc, exprbuf);
				D_long(0);
			}
			else
			{
				if (tdb)
					rmark(cursect, sloc, tdb, MLONG, NULL);

				D_long(eval);
			}

			break;
		}
	}

	return 0;
}


//
// .comm symbol, size
//
int d_comm(void)
{
	SYM * sym;
	char * p;
	VALUE eval;

	if (*tok != SYMBOL)
		return error("missing symbol");

	p = string[tok[1]];
	tok += 2;

	if (*p == '.')							// Cannot .comm a local symbol
		return error(locgl_error);

	if ((sym = lookup(p, LABEL, 0)) == NULL)
		sym = NewSymbol(p, LABEL, 0);
	else
	{
		if (sym->sattr & DEFINED)
			return error(".comm symbol already defined");
	}

	sym->sattr = GLOBAL | COMMON | BSS;

	if (*tok++ != ',')
		return error(comma_error);

	if (abs_expr(&eval) != OK)				// Parse size of common region
		return 0;

	sym->svalue = eval;						// Install common symbol's size
	at_eol();
	return 0;
}


//
// .list - Turn listing on
//
int d_list(void)
{
	if (list_flag)
		listing++;

	return 0;
}


//
// .nlist - Turn listing off
//
int d_nlist(void)
{
	if (list_flag)
		listing--;

	return 0;
}


//
// .68000 - Back to 68000 TEXT segment
//
int d_68000(void)
{
	rgpu = rdsp = 0;
	// Switching from gpu/dsp sections should reset any ORG'd Address
	orgactive = 0;                               
	orgwarning = 0;
	SaveSection();
	SwitchSection(TEXT);
	return 0;
}


//
// .gpu - Switch to GPU assembler
//
int d_gpu(void)
{
	if ((cursect != TEXT) && (cursect != DATA))
	{
		error(".gpu can only be used in the TEXT or DATA segments");
		return ERROR;
	}

	// If previous section was dsp or 68000 then we need to reset ORG'd Addresses
	if (!rgpu)
	{
//printf("Resetting ORG...\n");
		orgactive = 0;
		orgwarning = 0;
	}
//else printf("NOT resetting ORG!\n");

	rgpu = 1;			// Set GPU assembly
	rdsp = 0;			// Unset DSP assembly
	regbank = BANK_N;	// Set no default register bank
	return 0;
}


//
// .dsp - Switch to DSP assembler
//
int d_dsp(void)
{
	if ((cursect != TEXT) && (cursect != DATA))
	{
		error(".dsp can only be used in the TEXT or DATA segments");
		return ERROR;
	}

	// If previous section was gpu or 68000 then we need to reset ORG'd Addresses
	if (!rdsp)
	{
		orgactive = 0;
		orgwarning = 0;
	}

	rdsp = 1;			// Set DSP assembly
	rgpu = 0;			// Unset GPU assembly
	regbank = BANK_N;	// Set no default register bank
	return 0;
}


//
// .cargs [#offset], symbol[.size], ...
// 
// Lists of registers may also be mentioned; they just take up space. Good for
// "documentation" purposes:
// 
// .cargs a6, .arg1, .arg2, .arg3...
// 
// Symbols thus created are ABS and EQUATED.
//
int d_cargs(void)
{
	VALUE eval = 4;		// Default to 4 if no offset specified (to account for
						// return address)
	WORD rlist;
	SYM * symbol;
	char * p;
	int env;
	int i;

	if (rgpu || rdsp)
		return error("directive forbidden in gpu/dsp mode");

	if (*tok == '#')
	{
		tok++;

		if (abs_expr(&eval) != OK)
			return 0;

		// Eat the comma, if it's there
		if (*tok == ',')
			tok++;
	}

	for(;;)
	{
		if (*tok == SYMBOL)
		{
			p = string[tok[1]];

			// Set env to either local (dot prefixed) or global scope
			env = (*p == '.' ? curenv : 0);
			symbol = lookup(p, LABEL, env);

			if (symbol == NULL)
			{
				symbol = NewSymbol(p, LABEL, env);
				symbol->sattr = 0;
			}
			else if (symbol->sattr & DEFINED)
				return errors("multiply-defined label '%s'", p);

			// Put symbol in "order of definition" list
			AddToSymbolDeclarationList(symbol);

			symbol->sattr |= (ABS | DEFINED | EQUATED);
			symbol->svalue = eval;
			tok += 2;

			// What this does is eat any dot suffixes attached to a symbol. If
			// it's a .L, it adds 4 to eval; if it's .W or .B, it adds 2. If
			// there is no dot suffix, it assumes a size of 2.
			switch ((int)*tok)
			{
			case DOTL:
				eval += 2;
			case DOTB:
			case DOTW:
				tok++;
			}

			eval += 2;
		}
		else if (*tok >= KW_D0 && *tok <= KW_A7)
		{
			if (reglist(&rlist) < 0)
				return 0;

			for(i=0; i<16; i++, rlist>>=1)
			{
				if (rlist & 1)
					eval += 4;
			}
		}
		else
		{
			switch ((int)*tok)
			{
			case KW_USP:
			case KW_SSP:
			case KW_PC:
				eval += 2;
				// FALLTHROUGH
			case KW_SR:
			case KW_CCR:
				eval += 2;
				tok++;
				break;
			case EOL:
				return 0;
			default:
				return error(".cargs syntax");
			}
		}

		// Eat commas in between each argument, if they exist
		if (*tok == ',')
			tok++;
	}
}


//
// .cstruct [#offset], symbol[.size], ...
// 
// Lists of registers may also be mentioned; they just take up space. Good for
// "documentation" purposes:
// 
// .cstruct a6, .arg1, .arg2, .arg3...
// 
// Symbols thus created are ABS and EQUATED. Note that this is for
// compatibility with VBCC and the Remover's library. Thanks to GroovyBee for
// the suggestion.
//
int d_cstruct(void)
{
	VALUE eval = 0;		// Default, if no offset specified, is zero
	WORD rlist;
	SYM * symbol;
	char * symbolName;
	int env;
	int i;

	if (rgpu || rdsp)
		return error("directive forbidden in gpu/dsp mode");

	if (*tok == '#')
	{
		tok++;

		if (abs_expr(&eval) != OK)
			return 0;

		// Eat the comma, if it's there
		if (*tok == ',')
			tok++;
	}

	for(;;)
	{
		if (*tok == SYMBOL)
		{
			symbolName = string[tok[1]];

			// Set env to either local (dot prefixed) or global scope
			env = (symbolName[0] == '.' ? curenv : 0);
			symbol = lookup(symbolName, LABEL, env);

			// If the symbol wasn't found, then define it. Otherwise, throw an
			// error.
			if (symbol == NULL)
			{
				symbol = NewSymbol(symbolName, LABEL, env);
				symbol->sattr = 0;
			}
			else if (symbol->sattr & DEFINED)
				return errors("multiply-defined label '%s'", symbolName);

			// Put symbol in "order of definition" list
			AddToSymbolDeclarationList(symbol);

			tok += 2;

			// Adjust label start address if it's a word or a long, as a byte
			// label might have left us on an odd address.
			switch ((int)*tok)
			{
			case DOTW:
			case DOTL:
				eval += eval & 0x01;
			}

			symbol->sattr |= (ABS | DEFINED | EQUATED);
			symbol->svalue = eval;

			// Check for dot suffixes and adjust space accordingly (longs and
			// words on an odd boundary get bumped to the next word aligned
			// address). If no suffix, then throw an error.
			switch ((int)*tok)
			{
			case DOTL:
				eval += 4;
				break;
			case DOTW:
				eval += 2;
				break;
			case DOTB:
				eval += 1;
				break;
			default:
				return error("Symbol missing dot suffix in .cstruct construct");
			}

			tok++;
		}
		else if (*tok >= KW_D0 && *tok <= KW_A7)
		{
			if (reglist(&rlist) < 0)
				return 0;

			for(i=0; i<16; i++, rlist>>=1)
			{
				if (rlist & 1)
					eval += 4;
			}
		}
		else
		{
			switch ((int)*tok)
			{
			case KW_USP:
			case KW_SSP:
			case KW_PC:
				eval += 2;
				// FALLTHROUGH
			case KW_SR:
			case KW_CCR:
				eval += 2;
				tok++;
				break;
			case EOL:
				return 0;
			default:
				return error(".cstruct syntax");
			}
		}

		// Eat commas in between each argument, if they exist
		if (*tok == ',')
			tok++;
	}
}


//
// Undefine a macro - .undefmac macname [, macname...]
//
int undmac1(char * p)
{
	SYM * symbol = lookup(p, MACRO, 0);

	// If the macro symbol exists, cause it to disappear
//	if ((sy = lookup(p, MACRO, 0)) != NULL)
	if (symbol != NULL)
		symbol->stype = (BYTE)SY_UNDEF;

	return OK;
}


int d_undmac(void)
{
	symlist(undmac1);
	return 0;
}


int d_jpad(void)
{
	warn("JPAD directive is deprecated/non-functional");
	return OK;
}


int d_nojpad(void)
{
	warn("NOJPAD directive is deprecated/non-functional");
	return OK;
}


int d_gpumain(void)
{
	return error("What the hell? Do you think we adhere to the Goof standard?");
}

