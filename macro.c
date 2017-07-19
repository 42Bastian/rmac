//
// RMAC - Reboot's Macro Assembler for all Atari computers
// MACRO.C - Macro Definition and Invocation
// Copyright (C) 199x Landon Dyer, 2011-2017 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source utilised with the kind permission of Landon Dyer
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


LONG curuniq;				// Current macro's unique number
int macnum;					// Unique number for macro definition
TOKEN * argPtrs[128];		// 128 arguments ought to be enough for anyone
static int argp;

static LONG macuniq;		// Unique-per-macro number
static SYM * curmac;		// Macro currently being defined
static VALUE argno;			// Formal argument count

static LLIST * firstrpt;	// First .rept line
static LLIST * nextrpt;		// Last .rept line
static int rptlevel;		// .rept nesting level

// Function prototypes
static int KWMatch(char *, char *);
static int LNCatch(int (*)(), char *);


//
// Initialize macro processor
//
void InitMacro(void)
{
	macuniq = 0;
	macnum = 1;
	argp = 0;
}


//
// Exit from a macro;
// -- pop any intervening include files and repeat blocks;
// -- restore argument stack;
// -- pop the macro.
//
int ExitMacro(void)
{
WARNING(!!! Bad macro exiting !!!)
/*
This is a problem. Currently, the argument logic just keeps the current
arguments and doesn't save anything if a new macro is called in the middle
of another (nested macros). Need to fix that somehow.
*/
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

//	/*TOKEN ** p = */argp--;
//	argp = (TOKEN **)*argp;
	DEBUG { printf("ExitMacro: argp: %d -> ", argp); }
	argp -= imacro->im_nargs;
	DEBUG { printf("%d (nargs = %d)\n", argp, imacro->im_nargs); }

	return fpop();
}


//
// Add a formal argument to a macro definition
//
int defmac2(char * argname)
{
	if (lookup(argname, MACARG, (int)curmac->sattr) != NULL)
		return error("multiple formal argument definition");

	SYM * arg = NewSymbol(argname, MACARG, (int)curmac->sattr);
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
If this is the 2nd time through, derefence the ** to point to the memory you
just allocated. Then, set the ** to the location of the memory you allocated
for the next pass through.

This is a really low level way to do a linked list, and by bypassing all the
safety features of the language. Seems like we can do better here.
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
			curmac->lineList = malloc(sizeof(LLIST));
			curmac->lineList->next = NULL;
			curmac->lineList->line = strdup(ln);
			curmac->last = curmac->lineList;
		}
		else
		{
			curmac->last->next = malloc(sizeof(LLIST));
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
//  `defmac2' processes the formal arguments (and sticks them into the symbol
//   table)
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
	LNCatch(defmac1, "endm ");

	return 0;
}


//
// Add lines to a .rept definition
//
int defr1(char * line, int kwno)
{
	if (list_flag)
	{
		listeol();			// Flush previous source line
		lstout('#');		// Mark this a 'rept' block
	}

	if (kwno == 0)			// .endr
	{
		if (--rptlevel == 0)
			return 0;
	}
	else if (kwno == 1)		// .rept
		rptlevel++;

//DEBUG { printf("  defr1: line=\"%s\", kwno=%d, rptlevel=%d\n", line, kwno, rptlevel); }

#if 0
//MORE stupidity here...
WARNING("!!! Casting (char *) as LONG !!!")
	// Allocate length of line + 1('\0') + LONG
	LONG * p = (LONG *)malloc(strlen(line) + 1 + sizeof(LONG));
	*p = 0;
	strcpy((char *)(p + 1), line);

	if (nextrpt == NULL)
		firstrpt = p;		// First line of rept statement
	else
		*nextrpt = (LONG)p;

	nextrpt = p;
#else
	if (firstrpt == NULL)
	{
		firstrpt = malloc(sizeof(LLIST));
		firstrpt->next = NULL;
		firstrpt->line = strdup(line);
		nextrpt = firstrpt;
	}
	else
	{
		nextrpt->next = malloc(sizeof(LLIST));
		nextrpt->next->next = NULL;
		nextrpt->next->line = strdup(line);
		nextrpt = nextrpt->next;
	}
#endif

	return rptlevel;
}


//
// Handle a .rept block; this gets hairy because they can be nested
//
int HandleRept(void)
{
	VALUE eval;

	// Evaluate repeat expression
	if (abs_expr(&eval) != OK)
		return ERROR;

	// Suck in lines for .rept block
	firstrpt = NULL;
	nextrpt = NULL;
	rptlevel = 1;
	LNCatch(defr1, "endr rept ");

//DEBUG { printf("HandleRept: firstrpt=$%X\n", firstrpt); }
	// Alloc and init input object
	if (firstrpt)
	{
		INOBJ * inobj = a_inobj(SRC_IREPT);	// Create a new REPT input object
		IREPT * irept = inobj->inobj.irept;
		irept->ir_firstln = firstrpt;
		irept->ir_nextln = NULL;
		irept->ir_count = eval;
	}

	return 0;
}


//
// Hand off lines of text to the function 'lnfunc' until a line containing one
// of the directives in 'dirlist' is encountered.
//
// 'dirlist' contains space-separated terminated keywords. A final space
// terminates the list. Directives are case-insensitively compared to the
// keywords.
//
// If 'lnfunc' is NULL, then lines are simply skipped.
// If 'lnfunc' returns an error, processing is stopped.
//
// 'lnfunc' is called with an argument of -1 for every line but the last one,
// when it is called with an argument of the keyword number that caused the
// match.
//
static int LNCatch(int (* lnfunc)(), char * dirlist)
{
	if (lnfunc != NULL)
		lnsave++;			// Tell tokenizer to keep lines

	while (1)
	{
		if (TokenizeLine() == TKEOF)
		{
			error("encountered end-of-file looking for '%s'", dirlist);
			fatal("cannot continue");
		}

		// Test for end condition.  Two cases to handle:
		//            <directive>
		//    symbol: <directive>
		char * p = NULL;
		int k = -1;

		if (*tok == SYMBOL)
		{
			// A string followed by a colon or double colon is a symbol and
			// *not* a directive, see if we can find the directive after it
			if ((tok[2] == ':' || tok[2] == DCOLON))
			{
				if (tok[3] == SYMBOL)
					p = string[tok[4]];
			}
			else
			{
				// Otherwise, just grab the directive
				p = string[tok[1]];
			}
		}

		if (p != NULL)
		{
			if (*p == '.')		// Ignore leading periods
				p++;

			k = KWMatch(p, dirlist);
		}

		// Hand-off line to function
		// if it returns 0, and we found a keyword, stop looking.
		// if it returns 1, hand off the line and keep looking.
		if (lnfunc != NULL)
			k = (*lnfunc)(lnbuf, k);

		if (k == 0)
			break;
	}

	if (lnfunc != NULL)
		lnsave--;				// Tell tokenizer to stop keeping lines

	return 0;
}


//
// See if the string `kw' matches one of the keywords in `kwlist'. If so,
// return the number of the keyword matched. Return -1 if there was no match.
// Strings are compared without regard for case.
//
static int KWMatch(char * kw, char * kwlist)
{
	for(int k=0; *kwlist; k++)
	{
		for(char * p=kw;;)
		{
			char c1 = *kwlist++;
			char c2 = *p++;

			if (c2 >= 'A' && c2 <= 'Z')
				c2 += 32;

			if (c1 == ' ' && c2 == EOS)
				return k;

			if (c1 != c2)
				break;
		}

		// Skip to beginning of next keyword in `kwlist'
		while (*kwlist && (*kwlist != ' '))
			++kwlist;

		if (*kwlist== ' ')
			kwlist++;
	}

	return -1;
}


//
// Invoke a macro
//  o  parse, count and copy arguments
//  o  push macro's string-stream
//
int InvokeMacro(SYM * mac, WORD siz)
{
	TOKEN * p = NULL;
	int dry_run;
	WORD arg_siz = 0;
//	TOKEN ** argptr = NULL;
//Doesn't need to be global! (or does it???--it does)
//	argp = 0;
	DEBUG printf("InvokeMacro: argp: %d -> ", argp);

	INOBJ * inobj = a_inobj(SRC_IMACRO);		// Alloc and init IMACRO
	IMACRO * imacro = inobj->inobj.imacro;
	imacro->im_siz = siz;
	WORD nargs = 0;
	TOKEN * beg_tok = tok;						// 'tok' comes from token.c
	TOKEN * startOfArg;
	TOKEN * dest;
	int stringNum = 0;
	int argumentNum = 0;

	for(dry_run=1; ; dry_run--)
	{
		for(tok=beg_tok; *tok!=EOL;)
		{
			if (dry_run)
				nargs++;
			else
			{
#if 0
				*argptr++ = p;
#else
				argPtrs[argp++] = p;
				startOfArg = p;
#endif
			}

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
//Shamus: Possible bug. ACONST has 2 tokens after it, not just 1
				case ACONST:
					if (dry_run)
					{
						arg_siz += sizeof(TOKEN);
						tok++;
					}
					else
					{
						*p++ = *tok++;
					}
				// FALLTHROUGH (picks up the arg after a CONST, SYMBOL or ACONST)
				default:
					if (dry_run)
					{
						arg_siz += sizeof(TOKEN);
						tok++;
					}
					else
					{
						*p++ = *tok++;
					}

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

			// Do our QnD token grabbing (this will be redone once we get all
			// the data structures fixed as this is a really dirty hack)
			if (!dry_run)
			{
				dest = imacro->argument[argumentNum].token;
				stringNum = 0;

				do
				{
					// Remap strings to point the IMACRO internal token storage
					if (*startOfArg == SYMBOL || *startOfArg == STRING)
					{
						*dest++ = *startOfArg++;
						imacro->argument[argumentNum].string[stringNum] = strdup(string[*startOfArg++]);
						*dest++ = stringNum++;
					}
					else
						*dest++ = *startOfArg++;
				}
				while (*startOfArg != EOL);

				*dest = *startOfArg;		// Copy EOL...
				argumentNum++;
			}
		}

		// Allocate space for argument ptrs and so on and then go back and
		// construct the arg frame
		if (dry_run)
		{
			if (nargs != 0)
				p = (TOKEN *)malloc(arg_siz);
//				p = (TOKEN *)malloc(arg_siz + sizeof(TOKEN));

/*
Shamus:
This construct is meant to deal with nested macros, so the simple minded way
we deal with them now won't work. :-/ Have to think about how to fix.
What we could do is simply move the argp with each call, and move it back by
the number of arguments in the macro that's ending. That would solve the
problem nicely.
[Which we do now. But that uncovered another problem: the token strings are all
stale by the time a nested macro gets to the end. But they're supposed to be
symbols, which means if we put symbol references into the argument token
streams, we can alleviate this problem.]
*/
#if 0
			argptr = (TOKEN **)malloc((nargs + 1) * sizeof(LONG));
			*argptr++ = (TOKEN *)argp;
			argp = argptr;
#else
			// We don't need to do anything here since we already advance argp
			// when parsing the arguments.
//			argp += nargs;
#endif
		}
		else
			break;
	}

	DEBUG { printf("%d\n", argp); }

	// Setup imacro:
	//  o  # arguments;
	//  o  -> macro symbol;
	//  o  -> macro definition string list;
	//  o  save 'curuniq', to be restored when the macro pops;
	//  o  bump `macuniq' counter and set 'curuniq' to it;
	imacro->im_nargs = nargs;
	imacro->im_macro = mac;
	imacro->im_nextln = mac->lineList;
	imacro->im_olduniq = curuniq;
	curuniq = macuniq++;
	imacro->argBase = argp - nargs;	// Shamus: keep track of argument base

	DEBUG
	{
		printf("nargs=%d\n", nargs);

		for(nargs=0; nargs<imacro->im_nargs; nargs++)
		{
			printf("arg%d=", nargs);
			dumptok(argPtrs[(argp - imacro->im_nargs) + nargs]);
		}
	}

	return OK;
}

