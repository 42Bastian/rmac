//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// DIRECT.C - Directive Handling
// Copyright (C) 199x Landon Dyer, 2011 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source Utilised with the Kind Permission of Landon Dyer
//

#include "direct.h"
#include "sect.h"
#include "risca.h"
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

TOKEN exprbuf[128];		// Expression buffer 

// Directive handler table
int (*dirtab[])() = {
   d_org,				// 0 org
   d_even,				// 1 even
   d_unimpl,			// 2 .6502
   d_68000,				// 3 .68000 
   d_bss,				// 4 bss
   d_data,				// 5 data 
   d_text,				// 6 text 
   d_abs,				// 7 abs 
   d_comm,				// 8 comm 
   d_init,				// 9 init 
   d_cargs,				// 10 cargs 
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
   d_globl,				// 24 globl 
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
   exitmac,				// 36* exitm 
   d_unimpl,			// 37* endm 
   d_list,				// 38 list 
   d_nlist,				// 39 nlist 
   d_long,				// 40* rept 
   d_phrase,			// 41* endr 
   d_dphrase,			// 42 struct 
   d_qphrase,			// 43 ends 
   d_title,				// 44 title 
   d_subttl,			// 45 subttl 
   eject,				// 46 eject 
   d_unimpl,			// 47 error 
   d_unimpl,			// 48 warn 
   d_noclear,			// 49 .noclear
   d_equrundef,			// 50 .equrundef/.regundef
   d_ccundef,			// 51 .ccundef
   d_print,				// 52 .print
   d_gpumain,			// 53 .gpumain
   d_jpad,				// 54 .jpad
   d_nojpad,			// 55 .nojpad
   d_fail,				// 56 .fail
};


//
// .fail - User abort
//
int d_fail(void)
{
	fatal("user abort");
	return 0;
}


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

	return 0;
}


//
// NOP Padding Directive
//
int d_jpad(void)
{
	jpad = 1;
	return 0;
}


int d_nojpad(void)
{
	jpad = 0;
	return 0;
}


//
// Print Directive
//
int d_print(void)
{
	char prntstr[LNSIZ];                                     // String for PRINT directive
	char format[LNSIZ];                                      // Format for PRINT directive
	int formatting = 0;                                      // Formatting on/off
	int wordlong = 0;                                        // WORD = 0, LONG = 1
	int outtype = 0;                                         // 0:hex, 1:decimal, 2:unsigned

	VALUE eval;                                              // Expression value
	WORD eattr;                                              // Expression attributes
	SYM * esym;                                              // External symbol involved in expr.
	TOKEN r_expr[EXPRSIZE];

	while (*tok != EOL)
	{
		switch(*tok)
		{
		case STRING:
			sprintf(prntstr, "%s", (char *)tok[1]);
			printf("%s", prntstr);

			if (list_fd) 
				write(list_fd, prntstr, (LONG)strlen(prntstr));

			tok += 2;
			break;
		case '/':
			formatting = 1;

			if (tok[1] != SYMBOL)
				goto token_err;

			strcpy(prntstr, (char *)tok[2]);

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
					write(list_fd, prntstr, (LONG)strlen(prntstr));

				formatting = 0;
				wordlong = 0;
				outtype = 0;
			}

			break;
		}
	}

	printf("\n");
	println("\n");

	return 0;

token_err:
	error("illegal print token");
	return ERROR;
}


//
// Undefine an Equated Condition Code
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
		error(syntax_error);
		return ERROR;
	}

	ccname = lookup((char *)tok[1], LABEL, 0);

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
// Undefine an Equated Register
//
int d_equrundef(void)
{
	SYM * regname;

	// Check that we are in a RISC section
	if (!rgpu && !rdsp)
	{
		error(".equrundef/.regundef must be defined in .gpu/.dsp section");
		return ERROR;
	}

	while (*tok != EOL)
	{
		// Skip preceeding or seperating commas
		if (*tok == ',')
			tok++;

		// Check we are dealing with a symbol
		if (*tok != SYMBOL)
		{
			error(syntax_error);
			return ERROR;
		}

		// Lookup and undef if equated register
		regname = lookup((char *)tok[1], LABEL, 0);

		if (regname && (regname->sattre & EQUATEDREG))
			regname->sattre |= UNDEF_EQUR;

		// Skip over symbol token and address
		tok += 2;
	}

	return 0;
}


//
// Do Not Allow the Use of the CLR.L Opcode
//
int d_noclear(void)
{
	return 0;
}


// 
// Include Binary File
//
int d_incbin(void)
{
	int i, j;
	int bytes = 0;
	long pos, size;
	char buf;

	if (*tok != STRING)
	{
		error(syntax_error);
		return ERROR;
	}

	if ((j = open((char *)tok[1],  _OPEN_INC)) >= 0)
	{
		size = lseek(j, 0L, SEEK_END);
		chcheck(size);
		pos = lseek(j, 0L, SEEK_SET);
		
		for(i=0; i<size; i++)
		{
			buf = '\0';
			bytes = read(j, &buf, 1);
			D_byte(buf);
		}
	}
	else
	{
		errors("cannot open include binary file (%s)", (char *)tok[1]);
		return ERROR;
	}

	close(j);
	return 0;
}


// 
// Set RISC Register Banks
//
int d_regbank0(void)
{
	regbank = BANK_0;                                        // Set active register bank zero
	return 0;
}


int d_regbank1(void)
{
	regbank = BANK_1;                                        // Set active register bank one
	return 0;
}


//
// Adjust Location to an EVEN Value
//
int d_even(void)
{
	if (sloc & 1)
	{
		if ((scattr & SBSS) == 0)
		{
			chcheck(1);
			D_byte(0);
		}
		else
		{
			++sloc;
		}
	}

	return 0;
}


//
// Adjust Location to an LONG Value
//
int d_long(void)
{
	unsigned i;
	unsigned val = 4;
			
	i = sloc & ~(val - 1);

	if (i != sloc)
		val = val - (sloc - i); 
	else
		val = 0;

	if (val)
	{
		if ((scattr & SBSS) == 0)
		{
			chcheck(val);

			for(i=0; i<val; i++) 
				D_byte(0);
		}
		else
		{
			sloc += val;
		}
	}

	return 0;
}


//
// Adjust Location to an PHRASE Value
//
int d_phrase(void)
{
	unsigned i;
	unsigned val = 8;
			
	i = sloc & ~(val - 1);

	if (i != sloc)
		val = val - (sloc - i); 
	else
		val = 0;

	if (val)
	{
		if ((scattr & SBSS) == 0)
		{
			chcheck(val);

			for(i=0; i<val; i++) 
				D_byte(0);
		}
		else
		{
			sloc += val;
		}
	}

	return 0;
}


//
// Adjust Location to an DPHRASE Value
//
int d_dphrase(void)
{
	unsigned i;
	unsigned val = 16;
			
	i = sloc & ~(val - 1);

	if (i != sloc)
		val = val - (sloc - i); 
	else
		val = 0;

	if (val)
	{
		if ((scattr & SBSS) == 0)
		{
			chcheck(val);

			for(i=0; i<val; i++) 
				D_byte(0);
		}
		else
		{
			sloc += val;
		}
	}

	return 0;
}


//
// Adjust Location to an QPHRASE Value
//
int d_qphrase(void)
{
	unsigned i;
	unsigned val = 32;
			
	i = sloc & ~(val - 1);

	if (i != sloc)
		val = val - (sloc - i); 
	else
		val = 0;

	if (val)
	{
		if ((scattr & SBSS) == 0)
		{
			savsect();
			chcheck(val);

			for(i=0; i<val; i++) 
				D_byte(0);
		}
		else
		{
			sloc += val;
		}
	}

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
void auto_even(void)
{
	if (scattr & SBSS)
		++sloc;                                               // Bump BSS section
	else
		D_byte(0)                                             // Deposit 0.b in non-BSS

	if (lab_sym != NULL)                                      // Bump label if we have to
		++lab_sym->svalue;
}


//
// Unimplemened Directive Error
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
	char * em = "symbol list syntax";

	for(;;)
	{
		if (*tok != SYMBOL)
			return error(em);

		if ((*func)(tok[1]) != OK)
			break;

		tok += 2;

		if (*tok == EOL)
			break;

		if (*tok != ',')
			return error(em);

		++tok;
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

	if (*tok == STRING)                                       // Leave strings ALONE 
		fn = (char *)*++tok;
	else if (*tok == SYMBOL)					// Try to append ".s" to symbols
	{
		strcpy(buf, (char *)*++tok);
		fext(buf, ".s", 0);
		fn = &buf[0];
	}
	else                                                   // Punt if no STRING or SYMBOL 
		return error("missing filename");

	// Make sure the user didn't try anything like:
	// .include equates.s
	if (*++tok != EOL)
		return error("extra stuff after filename -- enclose it in quotes");

	// Attempt to open the include file in the current directory, then (if that failed) try list
	// of include files passed in the enviroment string or by the "-d" option.
	if ((j = open(fn, 0)) < 0)
	{
		for(i=0; nthpath("RMACPATH", i, buf1)!=0; ++i)
		{
			j = strlen(buf1);

			if (j > 0 && buf1[j-1] != SLASHCHAR)                // Append path char if necessary 
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
	SYM *sy;

	if (*p == '.')
		return error("cannot .globl local symbol");

	if ((sy = lookup(p, LABEL, 0)) == NULL)
	{
		sy = newsym(p, LABEL, 0);
		sy->svalue = 0;
		sy->sattr = GLOBAL;
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

	savsect();

	if (*tok == EOL)
		eval = 0;
	else if (abs_expr(&eval) != OK)
		return 0;

	switchsect(ABS);
	sloc = eval;
	return 0;
}


//
// Switch Segments
//

int d_text(void)
{
	if (rgpu || rdsp)
		return error("directive forbidden in gpu/dsp mode");

	if (cursect != TEXT)
	{
		savsect();
		switchsect(TEXT);
	}

	return 0;
}


int d_data(void)
{
	if (rgpu || rdsp)
		return error("directive forbidden in gpu/dsp mode");

	if (cursect != DATA)
	{
		savsect();
		switchsect(DATA);
	}

	return 0;
}


int d_bss(void)
{
	if (rgpu || rdsp)
		return error("directive forbidden in gpu/dsp mode");

	if (cursect != BSS)
	{
		savsect();
		switchsect(BSS);
	}

	return 0;
}


//
// .ds[.size] expression
//
int d_ds(WORD siz)
{
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
	if ((scattr & SBSS))
	{
		listvalue(eval);
		eval *= siz;
		sloc += eval;
		just_bss = 1;                                         // No data deposited (8-bit CPU mode)
	}
	else
	{
		dep_block(eval, siz, (VALUE)0, (WORD)(DEFINED|ABS), NULL);
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
			i = strlen((const char*)tok[1]);

			if ((challoc - ch_size) < i) 
				chcheck(i);

			for(p=(char *)tok[1]; *p!=EOS; ++p)
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
				fixup(FU_BYTE|FU_SEXT, sloc, exprbuf);
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
				fixup(FU_WORD|FU_SEXT, sloc, exprbuf);
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
					fixup(FU_LONG|FU_MOVEI, sloc, exprbuf);
				else
					fixup(FU_LONG, sloc, exprbuf);

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
				fixup(FU_BYTE|FU_SEXT, sloc, exprbuf);
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
				fixup(FU_WORD|FU_SEXT, sloc, exprbuf);
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
				fixup(FU_LONG, sloc, exprbuf);
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

	p = (char *)tok[1];
	tok += 2;

	if (*p == '.')                                            // Cannot .comm a local symbol
		return error(locgl_error);

	if ((sym = lookup(p, LABEL, 0)) == NULL)
		sym = newsym(p, LABEL, 0);
	else
	{
		if (sym->sattr & DEFINED)
			return error(".comm symbol already defined");
	}

	sym->sattr = GLOBAL|COMMON|BSS;

	if (*tok++ != ',')
		return error(comma_error);

	if (abs_expr(&eval) != OK)                                // Parse size of common region
		return 0;

	sym->svalue = eval;                                      // Install common symbol's size
	at_eol();
	return 0;
}


//
// .list - Turn listing on
//
int d_list(void)
{
	if (list_flag)
		++listing;

	return 0;
}


//
// .nlist - Turn listing off
//
int d_nlist(void)
{
	if (list_flag)
		--listing;

	return 0;
}


//
// .68000 - Back to 68000 TEXT segment
//
int d_68000(void)
{
	rgpu = rdsp = 0;
	in_main = 0;
	// Switching from gpu/dsp sections should reset any ORG'd Address
	orgactive = 0;                               
	orgwarning = 0;
	savsect();
	switchsect(TEXT);
	return 0;
}


//
// .gpu - Switch to GPU Assembler
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
		orgactive = 0;
		orgwarning = 0;
	}

	rgpu = 1;                                                // Set GPU assembly
	rdsp = 0;                                                // Unset DSP assembly
	regbank = BANK_N;                                        // Set no default register bank
	in_main = 0;
	jpad = 0;
	return 0;
}


//
// GPU Main Code Directive
//

int d_gpumain(void)
{
	if ((cursect != TEXT) && (cursect != DATA))
	{
		error(".gpumain can only be used in the TEXT or DATA segments");
		return ERROR;
	}

	// If previous section was dsp or 68000 then we need to reset ORG'd Addresses
	if (!rgpu)
	{
		orgactive = 0;
		orgwarning = 0;
	}

	rgpu = 1;                                                // Set GPU assembly
	rdsp = 0;                                                // Unset DSP assembly
	regbank = BANK_N;                                        // Set no default register bank
	in_main = 1;                                             // Enable main code execution rules
	jpad = 0;
	return 0;
}


//
// .dsp - Switch to DSP Assembler
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

	rdsp = 1;                                                // Set DSP assembly
	rgpu = 0;                                                // Unset GPU assembly
	regbank = BANK_N;                                        // Set no default register bank
	in_main = 0;
	jpad = 0;
	return 0;
}


//
// .cargs [#offset], symbol[.size], ...
// 
// Lists of registers may also be mentioned; they just take up space. Good for
// "documentation" purposes.
// 
// .cargs a6,.arg1, .arg2, .arg3...
// 
// The symbols are ABS and EQUATED.
//
int d_cargs(void)
{
	VALUE eval;
	WORD rlist;
	SYM * sy;
	char * p;
	int env;
	int i;

	if (rgpu || rdsp)
		return error("directive forbidden in gpu/dsp mode");

	if (*tok == '#')
	{
		++tok;

		if (abs_expr(&eval) != OK)
			return 0;

		if (*tok == ',')                                       // Eat comma if it's there
			++tok;
	}
	else 
		eval = 4;

	for(;;)
	{
		if (*tok == SYMBOL)
		{
			p = (char *)tok[1];

			if (*p == '.')
				env = curenv;
			else
				env = 0;

			sy = lookup(p, LABEL, env);

			if (sy == NULL)
			{
				sy = newsym(p, LABEL, env);
				sy->sattr = 0;
			}
			else if (sy->sattr & DEFINED)
				return errors("multiply-defined label '%s'", p);

			// Put symbol in "order of definition" list
			if (!(sy->sattr & SDECLLIST))
				sym_decl(sy);

			sy->sattr |= ABS|DEFINED|EQUATED;
			sy->svalue = eval;
			tok += 2;

			switch((int)*tok)
			{
			case DOTL:
				eval += 2;
			case DOTB:
			case DOTW:
				++tok;
			}

			eval += 2;
		}
		else 
		{
			if (*tok >= KW_D0 && *tok <= KW_A7)
			{
				if (reglist(&rlist) < 0)
					return 0;

				for(i=0; i++<16; rlist>>=1)
					if (rlist & 1)
						eval += 4;
			}
			else
			{
				switch((int)*tok)
				{
				case KW_USP:
				case KW_SSP:
				case KW_PC:
					eval += 2;
					// FALLTHROUGH
				case KW_SR:
				case KW_CCR:
					eval += 2;
					++tok;
					break;
				case EOL:
					return 0;
				default:
					return error(".cargs syntax");
				}
			}

			if (*tok == ',')
				++tok;
		}
	}
}


//
// Undefine a macro - .undefmac macname [, macname...]
//
int undmac1(char * p)
{
	SYM * sy;

	// If the macro symbol exists, cause it to dissappear
	if ((sy = lookup(p, MACRO, 0)) != NULL)
		sy->stype = (BYTE)SY_UNDEF;

	return OK;
}


int d_undmac(void)
{
	symlist(undmac1);
	return 0;
}
