//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// MACRO.C - Macro Definition and Invocation
// Copyright (C) 199x Landon Dyer, 2011 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source Utilised with the Kind Permission of Landon Dyer
//

#include "macro.h"
#include "debug.h"
#include "direct.h"
#include "error.h"
#include "expr.h"
#include "listing.h"
#include "procln.h"
#include "symbol.h"
#include "token.h"

LONG curuniq;								// Current macro's unique number
//TOKEN ** argp;								// Free spot in argptrs[]
int macnum;									// Unique number for macro definition
TOKEN * argPtrs[128];						// 128 arguments ought to be enough for anyone
static int argp;

static LONG macuniq;						// Unique-per-macro number
static SYM * curmac;						// Macro currently being defined
//static char ** curmln;						// Previous macro line (or NULL)
static VALUE argno;							// Formal argument count 

static LONG * firstrpt;						// First .rept line 
static LONG * nextrpt;						// Last .rept line 
static int rptlevel;						// .rept nesting level 


//
// Initialize Macro Processor
//
void init_macro(void)
{
	macuniq = 0;
	macnum = 1;
//	argp = NULL;
	argp = 0;
	ib_macro();
}


//
// Exit from a Macro;
// -- pop any intervening include files and repeat blocks;
// -- restore argument stack;
// -- pop the macro.
//
int exitmac(void)
{
	// Pop intervening include files and .rept blocks
	while (cur_inobj != NULL && cur_inobj->in_type != SRC_IMACRO)
		fpop();

	if (cur_inobj == NULL)
		fatal("too many ENDMs");

	// Restore
	// o  old arg context
	// o  old unique number
	// ...and then pop the macro.

	IMACRO * imacro = cur_inobj->inobj.imacro;
	curuniq = imacro->im_olduniq;

	/*TOKEN ** p = */argp--;
//	argp = (TOKEN **)*argp;

	fpop();
	mjump_align = 0;
	return 0;
}


//
// Add a Formal Argument to a Macro Definition
//
int defmac2(char * argname)
{
	SYM * arg;

	if (lookup(argname, MACARG, (int)curmac->sattr) != NULL)
		return error("multiple formal argument definition");

	arg = NewSymbol(argname, MACARG, (int)curmac->sattr);
	arg->svalue = argno++;

	return OK;
}


//
// Add a line to a macro definition; also print lines to listing file (if
// enabled). The last line of the macro (containing .endm) is not included in
// the macro. A label on that line will be lost.
// notEndFlag is -1 for all lines but the last one (.endm), when it is 0.
//
int defmac1(char * ln, int notEndFlag)
{
	PTR p;
	LONG len;

	if (list_flag)
	{
		listeol();							// Flush previous source line
		lstout('.');						// Mark macro definition with period
	}

	// This is just wrong, wrong, wrong. It makes up a weird kind of string with
	// a pointer on front, and then uses a ** to manage them: This is a recipe
	// for disaster.
	// How to manage it then?
	// Could use a linked list, like Landon uses everywhere else.
/*
How it works:
Allocate a space big enough for the string + NULL + pointer.
Set the pointer to NULL.
Copy the string to the space after the pointer.
If this is the 1st time through, set the SYM * "svalue" to the pointer.
If this is the 2nd time through, derefence the ** to point to the memory you just allocated.
Then, set the ** to the location of the memory you allocated for the next pass through.

This is a really low level way to do a linked list, and by bypassing all the safety
features of the language. Seems like we can do better here.
*/
	if (notEndFlag)
	{
#if 0
		len = strlen(ln) + 1 + sizeof(LONG);
		p.cp = malloc(len);
		*p.lp = 0;
		strcpy(p.cp + sizeof(LONG), ln);

		// Link line of text onto end of list
		if (curmln == NULL)
			curmac->svalue = p.cp;
		else
			*curmln = p.cp;

		curmln = (char **)p.cp;
		return 1;							// Keep looking 
#else
		if (curmac->lineList == NULL)
		{
			curmac->lineList = malloc(sizeof(struct LineList));
			curmac->lineList->next = NULL;
			curmac->lineList->line = strdup(ln);
			curmac->last = curmac->lineList;
		}
		else
		{
			curmac->last->next = malloc(sizeof(struct LineList));
			curmac->last->next->next = NULL;
			curmac->last->next->line = strdup(ln);
			curmac->last = curmac->last->next;
		}

		return 1;							// Keep looking
#endif
	}

	return 0;								// Stop looking at the end
}


//
// Define macro
//
// macro foo arg1,arg2,...
//    :
//     :
//    endm
//
//  Helper functions:
//  -----------------
//  `defmac1' adds lines of text to the macro definition
//  `defmac2' processes the formal arguments (and sticks them into the symbol table)
//
int DefineMacro(void)
{
	// Setup entry in symbol table, make sure the macro isn't a duplicate
	// entry, and that it doesn't override any processor mnemonic or assembler
	// directive.
	if (*tok++ != SYMBOL)
		return error("missing symbol");

	char * name = string[*tok++];

	if (lookup(name, MACRO, 0) != NULL)
		return error("duplicate macro definition");

	curmac = NewSymbol(name, MACRO, 0);
	curmac->svalue = 0;
	curmac->sattr = (WORD)(macnum++);

	// Parse and define formal arguments in symbol table
	if (*tok != EOL)
	{
		argno = 0;
		symlist(defmac2);
		at_eol();
	}

	// Suck in the macro definition; we're looking for an ENDM symbol on a line
	// by itself to terminate the definition.
//	curmln = NULL;
	curmac->lineList = NULL;
	lncatch(defmac1, "endm ");

	return 0;
}


//
// Add lines to a .rept definition
//
int defr1(char * ln, int kwno)
{
	LONG len;
	LONG * p;

	if (list_flag)
	{
		listeol();								// Flush previous source line
		lstout('#');							// Mark this a 'rept' block
	}

	switch (kwno)
	{
	case 0:										// .endr 
		if (--rptlevel == 0)
		return(0);
		goto addln;
	case 1:										// .rept 
		rptlevel++;
	default:
//MORE stupidity here...
#warning "!!! Casting (char *) as LONG !!!"
	addln:
		// Allocate length of line + 1('\0') + LONG
		len = strlen(ln) + 1 + sizeof(LONG);
//		p = (LONG *)amem(len);
		p = (LONG *)malloc(len);
		*p = 0;

		strcpy((char *)(p + 1), ln);
		
		if (nextrpt == NULL)
		{
			firstrpt = p;           // First line of rept statement
		}
		else
		{
			*nextrpt = (LONG)p;
		}

		nextrpt = p;

		return rptlevel;
	}
}


//
// Define a .rept block, this gets hairy because they can be nested
//
int defrept(void)
{
	INOBJ * inobj;
	IREPT * irept;
	VALUE eval;

	// Evaluate repeat expression
	if (abs_expr(&eval) != OK)
		return ERROR;

	// Suck in lines for .rept block
	firstrpt = NULL;
	nextrpt = NULL;
	rptlevel = 1;
	lncatch(defr1, "endr rept ");

	// Alloc and init input object
	if (firstrpt)
	{
		inobj = a_inobj(SRC_IREPT);				// Create a new REPT input object
		irept = inobj->inobj.irept;
		irept->ir_firstln = firstrpt;
		irept->ir_nextln = NULL;
		irept->ir_count = eval;
	}

	return 0;
}


//
// Hand off lines of text to the function `lnfunc' until a line containing one
// of the directives in `dirlist' is encountered. Return the number of the
// keyword encountered (0..n)
// 
// `dirlist' contains null-seperated terminated keywords.  A final null
// terminates the list. Directives are compared to the keywords without regard
// to case.
// 
// If `lnfunc' is NULL, then lines are simply skipped.
// If `lnfunc' returns an error, processing is stopped.
// 
// `lnfunc' is called with an argument of -1 for every line but the last one,
// when it is called with an argument of the keyword number that caused the
// match.
//
int lncatch(int (* lnfunc)(), char * dirlist)
{
	char * p;
	int k;

	if (lnfunc != NULL)
		lnsave++;								// Tell tokenizer to keep lines 

	for(;;)
	{
		if (tokln() == TKEOF)
		{
			errors("encountered end-of-file looking for '%s'", dirlist);
			fatal("cannot continue");
		}

		// Test for end condition.  Two cases to handle:
		//            <directive>
		//    symbol: <directive>
		p = NULL;
		k = -1;

		if (*tok == SYMBOL)
		{
			if ((tok[2] == ':' || tok[2] == DCOLON))
			{
				if (tok[3] == SYMBOL)			// label: symbol
#if 0
					p = (char *)tok[4];
#else
					p = string[tok[4]];
#endif
			}
			else
			{
#if 0
				p = (char *)tok[1];				// symbol 
#else
				p = string[tok[1]];				// Symbol
#endif
			}
		}

		if (p != NULL)
		{
			if (*p == '.')						// ignore leading '.'s 
				p++;

			k = kwmatch(p, dirlist);
		}

		// Hand-off line to function
		// if it returns 0, and we found a keyword, stop looking.
		// if it returns 1, hand off the line and keep looking.
		if (lnfunc != NULL)
			k = (*lnfunc)(lnbuf, k);

		if (!k)
			break;
	}

	if (lnfunc != NULL)
		lnsave--;								// Tell tokenizer to stop keeping lines

	return 0;
}


//
// See if the string `kw' matches one of the keywords in `kwlist'.  If so,
// return the number of the keyword matched.  Return -1 if there was no match.
// Strings are compared without regard for case.
//
int kwmatch(char * kw, char * kwlist)
{
	char * p;
	char c1;
	char c2;
	int k;

	for(k=0; *kwlist; ++k)
	{
		for(p=kw;;)
		{
			c1 = *kwlist++;
			c2 = *p++;

			if (c2 >= 'A' && c2 <= 'Z')
				c2 += 32;

			if (c1 == ' ' && c2 == EOS)
				return k;

			if (c1 != c2)
				break;
		}

		// Skip to beginning of next keyword in `kwlist'
		while (*kwlist && *kwlist != ' ')
			++kwlist;

		if (*kwlist== ' ')
			kwlist++;
	}

	return -1;
}


//
// Invoke a macro
// o  parse, count and copy arguments
// o  push macro's string-stream
//
int InvokeMacro(SYM * mac, WORD siz)
{
	TOKEN * p = NULL;
	int dry_run;
	WORD arg_siz = 0;
//	TOKEN ** argptr = NULL;
//Doesn't need to be global! (or does it???)
	argp = 0;

	if ((!strcmp(mac->sname, "mjump") || !strcmp(mac->sname, "mpad")) && !in_main)
	{
		error("macro cannot be used outside of .gpumain");
		return ERROR;
	}

	INOBJ * inobj = a_inobj(SRC_IMACRO);		// Alloc and init IMACRO 
	IMACRO * imacro = inobj->inobj.imacro;
	imacro->im_siz = siz;
	WORD nargs = 0;
	TOKEN * beg_tok = tok;						// 'tok' comes from token.c

	for(dry_run=1; ; dry_run--)
	{
		for(tok=beg_tok; *tok!=EOL;)
		{
			if (dry_run)
				nargs++;
			else
#if 0				
				*argptr++ = p;
#else
				argPtrs[argp++] = p;
#endif

			// Keep going while tok isn't pointing at a comma or EOL
			while (*tok != ',' && *tok != EOL)
			{
				// Skip over backslash character, unless it's followed by an EOL
				if (*tok == '\\' && tok[1] != EOL)
					tok++;

				switch (*tok)
				{
				case CONST:
				case SYMBOL:
				case ACONST:
					if (dry_run)
					{
						arg_siz += sizeof(TOKEN);
						tok++;
					}
					else
						*p++ = *tok++;
				// FALLTHROUGH (picks up the arg after a CONST, SYMBOL or ACONST)
				default:
					if (dry_run)
					{
						arg_siz += sizeof(TOKEN);
						tok++;
					}
					else
						*p++ = *tok++;

					break;
				}
			}

			// We hit the comma or EOL, so count/stuff it
			if (dry_run)
				arg_siz += sizeof(TOKEN);
			else
				*p++ = EOL;

			// If we hit the comma instead of an EOL, skip over it
			if (*tok == ',')
				tok++;
		}

		// Allocate space for argument ptrs and so on and then go back and
		// construct the arg frame
		if (dry_run)
		{
			if (nargs != 0)
//Barfing here with memory corruption in glibc. TOKEN is defined as LONG, which is uint32_t
				p = (TOKEN *)malloc(arg_siz);
//				p = (TOKEN *)malloc(arg_siz + sizeof(TOKEN));

// This construct is meant to deal with nested macros, so the simple minded way
// we deal with them now won't work. :-/ Have to think about how to fix.
#if 0
			argptr = (TOKEN **)malloc((nargs + 1) * sizeof(LONG));
			*argptr++ = (TOKEN *)argp;
			argp = argptr;
#else
#endif
		}
		else 
			break;
	}

	// Setup imacro:
	// o  #arguments;
	// o  -> macro symbol;
	// o  -> macro definition string list;
	// o  save 'curuniq', to be restored when the macro pops;
	// o  bump `macuniq' counter and set 'curuniq' to it;
	imacro->im_nargs = nargs;
	imacro->im_macro = mac;
//	imacro->im_nextln = (TOKEN *)mac->svalue;
	imacro->im_nextln = mac->lineList;
	imacro->im_olduniq = curuniq;
	curuniq = macuniq++;

	DEBUG
	{
		printf("nargs=%d\n", nargs);

		for(nargs=0; nargs<imacro->im_nargs; ++nargs)
		{
			printf("arg%d=", nargs);
//			dumptok(argp[imacro->im_nargs - nargs - 1]);
			dumptok(argPtrs[imacro->im_nargs - nargs - 1]);
		}
	}

	return OK;
}


//
// Setup inbuilt macros
//
void ib_macro(void)
{
	curmac = NewSymbol("mjump", MACRO, 0);
	curmac->svalue = 0;
	curmac->sattr = (WORD)(macnum++);
	argno = 0;
	defmac2("cc");
	defmac2("addr");
	defmac2("jreg");
//	curmln = NULL;
	curmac->lineList = NULL;
	defmac1("  nop", -1);
	defmac1("  movei #\\addr,\\jreg", -1);
	defmac1("  jump  \\cc,(\\jreg)", -1);
	defmac1("  nop", -1);
	defmac1("  nop", -1);

	curmac = NewSymbol("mjr", MACRO, 0);
	curmac->svalue = 0;
	curmac->sattr = (WORD)(macnum++);
	argno = 0;
	defmac2("cc");
	defmac2("addr");
//	curmln = NULL;
	curmac->lineList = NULL;
	defmac1("  jr    \\cc,\\addr", -1);
	defmac1("  nop", -1);
	defmac1("  nop", -1);

	curmac = NewSymbol("mpad", MACRO, 0);
	curmac->svalue = 0;
	curmac->sattr = (WORD)(macnum++);
	argno = 0;
	defmac2("size");
//	curmln = NULL;
	curmac->lineList = NULL;
	defmac1("  .rept (\\size/2)", -1);
	defmac1("     nop", -1);
	defmac1("  .endr", -1);
}
