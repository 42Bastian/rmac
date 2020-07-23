//
// RMAC - Reboot's Macro Assembler for all Atari computers
// EXPR.C - Expression Analyzer
// Copyright (C) 199x Landon Dyer, 2011-2020 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source utilised with the kind permission of Landon Dyer
//

#include "expr.h"
#include "direct.h"
#include "error.h"
#include "listing.h"
#include "mach.h"
#include "procln.h"
#include "riscasm.h"
#include "sect.h"
#include "symbol.h"
#include "token.h"

#define DEF_KW						// Declare keyword values
#include "kwtab.h"					// Incl generated keyword tables & defs

// N.B.: The size of tokenClass should be identical to the largest value of
//       a token; we're assuming 256 but not 100% sure!
static char tokenClass[256];		// Generated table of token classes
static uint64_t evstk[EVSTACKSIZE];	// Evaluator value stack
static WORD evattr[EVSTACKSIZE];	// Evaluator attribute stack

// Token-class initialization list
char itokcl[] = {
	0,								// END
	CONST, FCONST, SYMBOL, 0,		// ID
	'(', '[', '{', 0,				// OPAR
	')', ']', '}', 0,				// CPAR
	CR_DEFINED, CR_REFERENCED,		// SUNARY (special unary)
	CR_STREQ, CR_MACDEF,
	CR_DATE, CR_TIME,
	CR_ABSCOUNT, CR_FILESIZE, 0,
	'!', '~', UNMINUS, UNLT, UNGT, 0,	// UNARY
	'*', '/', '%', 0,				// MULT
	'+', '-', 0,					// ADD
	SHL, SHR, 0,					// SHIFT
	LE, GE, '<', '>', NE, '=', 0,	// REL
	'&', 0,							// AND
	'^', 0,							// XOR
	'|', 0,							// OR
	1								// (the end)
};

const char missym_error[] = "missing symbol";
const char str_error[] = "missing symbol or string";
const char noflt_error[] = "operator not usable with float";

// Convert expression to postfix
static PTR evalTokenBuffer;		// Deposit tokens here (this is really a
								// pointer to exprbuf from direct.c)
								// (Can also be from others, like
								// riscasm.c)
static int symbolNum;			// Pointer to the entry in symbolPtr[]


//
// Obtain a string value
//
static uint32_t str_value(char * p)
{
	uint32_t v;

	for(v=0; *p; p++)
		v = (v << 8) | (*p & 0xFF);

	return v;
}


//
// Initialize expression analyzer
//
void InitExpression(void)
{
	// Initialize token-class table (all set to END)
	for(int i=0; i<256; i++)
		tokenClass[i] = END;

	int i = 0;

	for(char * p=itokcl; *p!=1; p++)
	{
		if (*p == 0)
			i++;
		else
			tokenClass[(int)(*p)] = (char)i;
	}

	symbolNum = 0;
}


//
// Binary operators (all the same precedence)
//
int expr0(void)
{
	if (expr1() != OK)
		return ERROR;

	while (tokenClass[*tok] >= MULT)
	{
		TOKEN t = *tok++;

		if (expr1() != OK)
			return ERROR;

		*evalTokenBuffer.u32++ = t;
	}

	return OK;
}


//
// Unary operators (detect unary '-')
// ggn: If expression starts with a plus then also eat it up. For some reason
//      the parser gets confused when this happens and emits a "bad
//      expression".
//
int expr1(void)
{
	char * p;
	WORD w;

	int class = tokenClass[*tok];

	if (*tok == '-' || *tok == '+' || *tok == '<' || *tok == '>' || class == UNARY)
	{
		TOKEN t = *tok++;

		if (expr2() != OK)
			return ERROR;

		if (t == '-')
			t = UNMINUS;
		else if (t == '<')
			t = UNLT;
		else if (t == '>')
			t = UNGT;

		// With leading + we don't have to deposit anything to the buffer
		// because there's no unary '+' nor we have to do anything about it
		if (t != '+')
			*evalTokenBuffer.u32++ = t;
	}
	else if (class == SUNARY)
	{
		switch (*tok++)
		{
		case CR_ABSCOUNT:
			if (cursect != ABS)
			{
				*evalTokenBuffer.u32++ = CONST;
				*evalTokenBuffer.u64++ = sect[ABS].sloc;
			}
			else
			{
				*evalTokenBuffer.u32++ = CONST;
				*evalTokenBuffer.u64++ = sloc;
			}
			break;
		case CR_FILESIZE:
			if (*tok++ != STRING)
				return error("^^FILESIZE expects filename inside string");
			*evalTokenBuffer.u32++ = CONST;
			// @@copypasted from d_incbin, maybe factor this out somehow?
			// Attempt to open the include file in the current directory, then (if that
			// failed) try list of include files passed in the enviroment string or by
			// the "-d" option.
			int fd, i;
			char buf1[256];

			if ((fd = open(string[*tok], _OPEN_INC)) < 0)
			{
				for(i=0; nthpath("RMACPATH", i, buf1)!=0; i++)
				{
					fd = strlen(buf1);

					// Append path char if necessary
					if ((fd > 0) && (buf1[fd - 1] != SLASHCHAR))
						strcat(buf1, SLASHSTRING);

					strcat(buf1, string[*tok]);

					if ((fd = open(buf1, _OPEN_INC)) >= 0)
						goto allright;
				}

				return error("cannot open: \"%s\"", string[tok[1]]);
			}

allright:
			*evalTokenBuffer.u64++ = (uint64_t)lseek(fd, 0L, SEEK_END);
			close(fd);

			// Advance tok because of consumed string token
			tok++;
			break;
		case CR_TIME:
			*evalTokenBuffer.u32++ = CONST;
			*evalTokenBuffer.u64++ = dos_time();
			break;
		case CR_DATE:
			*evalTokenBuffer.u32++ = CONST;
			*evalTokenBuffer.u64++ = dos_date();
			break;
		case CR_MACDEF: // ^^macdef <macro-name>
			if (*tok++ != SYMBOL)
				return error(missym_error);

			p = string[*tok++];
			w = (lookup(p, MACRO, 0) == NULL ? 0 : 1);
			*evalTokenBuffer.u32++ = CONST;
			*evalTokenBuffer.u64++ = (uint64_t)w;
			break;
		case CR_DEFINED:
			w = DEFINED;
			goto getsym;
		case CR_REFERENCED:
			w = REFERENCED;
getsym:
			if (*tok++ != SYMBOL)
				return error(missym_error);

			p = string[*tok++];
			int j = (*p == '.' ? curenv : 0);
			SYM * sy = lookup(p, LABEL, j);
			w = ((sy != NULL) && (sy->sattr & w ? 1 : 0));
			*evalTokenBuffer.u32++ = CONST;
			*evalTokenBuffer.u64++ = (uint64_t)w;
			break;
		case CR_STREQ:
			if (*tok != SYMBOL && *tok != STRING)
				return error(str_error);

			p = string[tok[1]];
			tok +=2;

			if (*tok++ != ',')
				return error(comma_error);

			if (*tok != SYMBOL && *tok != STRING)
				return error(str_error);

			char * p2 = string[tok[1]];
			tok += 2;

			w = (WORD)(!strcmp(p, p2));
			*evalTokenBuffer.u32++ = CONST;
			*evalTokenBuffer.u64++ = (uint64_t)w;
			break;
		}
	}
	else
		return expr2();

	return OK;
}


//
// Terminals (CONSTs) and parenthesis grouping
//
int expr2(void)
{
	PTR ptk;

	switch (*tok++)
	{
	case CONST:
		ptk.u32 = tok;
		*evalTokenBuffer.u32++ = CONST;
		*evalTokenBuffer.u64++ = *ptk.u64++;
		tok = ptk.u32;
		break;
	case FCONST:
		ptk.u32 = tok;
		*evalTokenBuffer.u32++ = FCONST;
		*evalTokenBuffer.u64++ = *ptk.u64++;
		tok = ptk.u32;
		break;
	case SYMBOL:
	{
		char * p = string[*tok++];
		int j = (*p == '.' ? curenv : 0);
		SYM * sy = lookup(p, LABEL, j);

		if (sy == NULL)
			sy = NewSymbol(p, LABEL, j);

		// Check register bank usage
		if (sy->sattre & EQUATEDREG)
		{
			if ((regbank == BANK_0) && (sy->sattre & BANK_1) && !altbankok)
				warn("equated symbol \'%s\' cannot be used in register bank 0", sy->sname);

			if ((regbank == BANK_1) && (sy->sattre & BANK_0) && !altbankok)
				warn("equated symbol \'%s\' cannot be used in register bank 1", sy->sname);
		}

		*evalTokenBuffer.u32++ = SYMBOL;
		*evalTokenBuffer.u32++ = symbolNum;
		symbolPtr[symbolNum] = sy;
		symbolNum++;
		break;
 	}
	case STRING:
		*evalTokenBuffer.u32++ = CONST;
		*evalTokenBuffer.u64++ = str_value(string[*tok++]);
		break;
	case '(':
		if (expr0() != OK)
			return ERROR;

		if (*tok++ != ')')
			return error("missing closing parenthesis ')'");

		break;
	case '[':
		if (expr0() != OK)
			return ERROR;

		if (*tok++ != ']')
			return error("missing closing bracket ']'");

		break;
	case '{':
		if (expr0() != OK)	// Eat up first parameter (register or immediate)
			return ERROR;

		if (*tok++ != ':')	// Demand a ':' there
			return error("missing colon ':'");

		if (expr0() != OK)	// Eat up second parameter (register or immediate)
			return ERROR;

		if (*tok++ != '}')
			return error("missing closing brace '}'");

		break;
	case '$':
		*evalTokenBuffer.u32++ = ACONST;			// Attributed const
		*evalTokenBuffer.u32++ = sloc;				// Current location
		*evalTokenBuffer.u32++ = cursect | DEFINED;	// Store attribs
		break;
	case '*':
		*evalTokenBuffer.u32++ = ACONST;			// Attributed const

		// pcloc == location at start of line
		*evalTokenBuffer.u32++ = (orgactive ? orgaddr : pcloc);
		// '*' takes attributes of current section, not ABS!
		*evalTokenBuffer.u32++ = cursect | DEFINED;
		break;
	default:
		return error("bad expression");
	}

	return OK;
}


//
// Recursive-descent expression analyzer (with some simple speed hacks)
//
int expr(TOKEN * otk, uint64_t * a_value, WORD * a_attr, SYM ** a_esym)
{
	// Passed in values (once derefenced, that is) can all be zero. They are
	// there so that the expression analyzer can fill them in as needed. The
	// expression analyzer gets its input from the global token pointer "tok",
	// and not from anything passed in by the user.
	SYM * symbol;
	char * p;
	int j;
	PTR ptk;

	evalTokenBuffer.u32 = otk;	// Set token pointer to 'exprbuf' (direct.c)
							// Also set in various other places too (riscasm.c,
							// e.g.)

//printf("expr(): tokens 0-2: %i %i %i (%c %c %c); tc[2] = %i\n", tok[0], tok[1], tok[2], tok[0], tok[1], tok[2], tokenClass[tok[2]]);
	// Optimize for single constant or single symbol.
	// Shamus: Subtle bug here. EOL token is 101; if you have a constant token
	//         followed by the value 101, it will trigger a bad evaluation here.
	//         This is probably a really bad assumption to be making here...!
	//         (assuming tok[1] == EOL is a single token that is)
	//         Seems that even other tokens (SUNARY type) can fuck this up too.
#if 0
//	if ((tok[1] == EOL)
	if ((tok[1] == EOL && ((tok[0] != CONST || tok[0] != FCONST) && tokenClass[tok[0]] != SUNARY))
//		|| (((*tok == CONST || *tok == FCONST || *tok == SYMBOL) || (*tok >= KW_R0 && *tok <= KW_R31))
//		&& (tokenClass[tok[2]] < UNARY)))
		|| (((tok[0] == SYMBOL) || (tok[0] >= KW_R0 && tok[0] <= KW_R31))
			&& (tokenClass[tok[2]] < UNARY))
		|| ((tok[0] == CONST || tok[0] == FCONST) && (tokenClass[tok[3]] < UNARY))
		)
#else
// Shamus: Seems to me that this could be greatly simplified by 1st checking if the first token is a multibyte token, *then* checking if there's an EOL after it depending on the actual length of the token (multiple vs. single). Otherwise, we have the horror show that is the following:
	if ((tok[1] == EOL
			&& (tok[0] != CONST && tokenClass[tok[0]] != SUNARY))
		|| (((tok[0] == SYMBOL)
				|| (tok[0] >= KW_R0 && tok[0] <= KW_R31))
			&& (tokenClass[tok[2]] < UNARY))
		|| ((tok[0] == CONST) && (tokenClass[tok[3]] < UNARY))
		)
// Shamus: Yes, you can parse that out and make some kind of sense of it, but damn, it takes a while to get it and understand the subtle bugs that result from not being careful about what you're checking; especially vis-a-vis niavely checking tok[1] for an EOL. O_o
#endif
	{
		if (*tok >= KW_R0 && *tok <= KW_R31)
		{
			*evalTokenBuffer.u32++ = CONST;
			*evalTokenBuffer.u64++ = *a_value = (*tok - KW_R0);
			*a_attr = ABS | DEFINED | RISCREG;

			if (a_esym != NULL)
				*a_esym = NULL;

			tok++;
		}
		else if (*tok == CONST)
		{
			ptk.u32 = tok;
			*evalTokenBuffer.u32++ = *ptk.u32++;
			*evalTokenBuffer.u64++ = *a_value = *ptk.u64++;
			*a_attr = ABS | DEFINED;
			tok = ptk.u32;

			if (a_esym != NULL)
				*a_esym = NULL;

//printf("Quick eval in expr(): CONST = %i, tokenClass[tok[2]] = %i\n", *a_value, tokenClass[*tok]);
		}
// Not sure that removing float constant here is going to break anything and/or
// make things significantly slower, but having this here seems to cause the
// complexity of the check to get to this part of the parse to go through the
// roof, and dammit, I just don't feel like fighting that fight ATM. :-P
#if 0
		else if (*tok == FCONST)
		{
			*evalTokenBuffer.u32++ = *tok++;
			*evalTokenBuffer.u64++ = *a_value = *tok.u64++;
			*a_attr = ABS | DEFINED | FLOAT;

			if (a_esym != NULL)
				*a_esym = NULL;

//printf("Quick eval in expr(): CONST = %i, tokenClass[tok[2]] = %i\n", *a_value, tokenClass[*tok]);
		}
#endif
		else if (*tok == '*')
		{
			*evalTokenBuffer.u32++ = CONST;

			if (orgactive)
				*evalTokenBuffer.u64++ = *a_value = orgaddr;
			else
				*evalTokenBuffer.u64++ = *a_value = pcloc;

			// '*' takes attributes of current section, not ABS!
			*a_attr = cursect | DEFINED;

			if (a_esym != NULL)
				*a_esym = NULL;

			tok++;
		}
		else if (*tok == STRING || *tok == SYMBOL)
		{
			p = string[tok[1]];
			j = (*p == '.' ? curenv : 0);
			symbol = lookup(p, LABEL, j);
#if 0
printf("eval: Looking up symbol (%s) [=%08X]\n", p, symbol);
if (symbol)
	printf("      attr=%04X, attre=%08X, val=%i, name=%s\n", symbol->sattr, symbol->sattre, symbol->svalue, symbol->sname);
#endif

			if (symbol == NULL)
				symbol = NewSymbol(p, LABEL, j);

			symbol->sattr |= REFERENCED;

			// Check for undefined register equates, but only if it's not part
			// of a #<SYMBOL> construct, as it could be that the label that's
			// been undefined may later be used as an address label--which
			// means it will be fixed up later, and thus, not an error.
			if ((symbol->sattre & UNDEF_EQUR) && !riscImmTokenSeen)
			{
				error("undefined register equate '%s'", symbol->sname);
//if we return right away, it returns some spurious errors...
//				return ERROR;
			}

			// Check register bank usage
			if (symbol->sattre & EQUATEDREG)
			{
				if ((regbank == BANK_0) && (symbol->sattre & BANK_1) && !altbankok)
					warn("equated symbol '%s' cannot be used in register bank 0", symbol->sname);

				if ((regbank == BANK_1) && (symbol->sattre & BANK_0) && !altbankok)
					warn("equated symbol '%s' cannot be used in register bank 1", symbol->sname);
			}

			*evalTokenBuffer.u32++ = SYMBOL;
#if 0
			*evalTokenBuffer++ = (TOKEN)symbol;
#else
/*
While this approach works, it's wasteful. It would be better to use something
that's already available, like the symbol "order defined" table (which needs to
be converted from a linked list into an array).
*/
			*evalTokenBuffer.u32++ = symbolNum;
			symbolPtr[symbolNum] = symbol;
			symbolNum++;
#endif

			*a_value = (symbol->sattr & DEFINED ? symbol->svalue : 0);
			*a_attr = (WORD)(symbol->sattr & ~GLOBAL);

/*
All that extra crap that was put into the svalue when doing the equr stuff is
thrown away right here. What the hell is it for?
*/
			if (symbol->sattre & EQUATEDREG)
			{
				*a_value &= 0x1F;
				*a_attr |= RISCREG; // Mark it as a register, 'cause it is
				*a_esym = symbol;
			}

			if ((symbol->sattr & (GLOBAL | DEFINED)) == GLOBAL
				&& a_esym != NULL)
				*a_esym = symbol;

			tok += 2;
		}
		// Holy hell... This is likely due to the fact that LSR is mistakenly set as a SUNARY type... Need to fix this... !!! FIX !!!
		else if (m6502)
		{
			*evalTokenBuffer.u32++ = *tok++;
		}
		else
		{
			// Unknown type here... Alert the user!,
			error("undefined RISC register in expression [token=$%X]", *tok);
			// Prevent spurious error reporting...
			tok++;
			return ERROR;
		}

		*evalTokenBuffer.u32++ = ENDEXPR;
		return OK;
	}

	if (expr0() != OK)
		return ERROR;

	*evalTokenBuffer.u32++ = ENDEXPR;
	return evexpr(otk, a_value, a_attr, a_esym);
}


//
// Evaluate expression.
// If the expression involves only ONE external symbol, the expression is
// UNDEFINED, but it's value includes everything but the symbol value, and
// 'a_esym' is set to the external symbol.
//
int evexpr(TOKEN * _tk, uint64_t * a_value, WORD * a_attr, SYM ** a_esym)
{
	WORD attr;
	SYM * sy;
	uint64_t * sval = evstk;				// (Empty) initial stack
	WORD * sattr = evattr;
	SYM * esym = NULL;						// No external symbol involved
	WORD sym_seg = 0;
	PTR tk;
	tk.u32 = _tk;

	while (*tk.u32 != ENDEXPR)
	{
		switch ((int)*tk.u32++)
		{
		case SYMBOL:
//printf("evexpr(): SYMBOL\n");
			sy = symbolPtr[*tk.u32++];
			sy->sattr |= REFERENCED;		// Set "referenced" bit

			if (!(sy->sattr & DEFINED))
			{
				// Reference to undefined symbol
				if (!(sy->sattr & GLOBAL))
				{
					*a_attr = 0;
					*a_value = 0;
					return OK;
				}

				if (esym != NULL)			// Check for multiple externals
					return error(seg_error);

				esym = sy;
			}

			if (sy->sattr & DEFINED)
				*++sval = sy->svalue;		// Push symbol's value
			else
				*++sval = 0;				// 0 for undefined symbols

			*++sattr = (WORD)(sy->sattr & ~GLOBAL);	// Push attribs
			sym_seg = (WORD)(sy->sattr & TDB);
			break;

		case CONST:
			*++sval = *tk.u64++;
//printf("evexpr(): CONST = %lX\n", *sval);
			*++sattr = ABS | DEFINED;		// Push simple attribs
			break;

		case FCONST:
//printf("evexpr(): FCONST = %lf\n", *tk.dp);
			// Even though it's a double, we can treat it like a uint64_t since
			// we're just moving the bits around.
			*++sval = *tk.u64++;
			*++sattr = ABS | DEFINED | FLOAT; // Push simple attribs
			break;

		case ACONST:
//printf("evexpr(): ACONST = %i\n", *tk.u32);
			*++sval = *tk.u32++;				// Push value
			*++sattr = (WORD)*tk.u32++;			// Push attribs
			break;

			// Binary "+" and "-" matrix:
			//
			// 	          ABS	 Sect	  Other
			//     ----------------------------
			//   ABS     |	ABS   |  Sect  |  Other |
			//   Sect    |	Sect  |  [1]   |  Error |
			//   Other   |	Other |  Error |  [1]   |
			//      ----------------------------
			//
			//   [1] + : Error
			//       - : ABS

		case '+':
//printf("evexpr(): +\n");
			--sval;							// Pop value
			--sattr;						// Pop attrib
//printf("--> N+N: %i + %i = ", *sval, sval[1]);
			// Get FLOAT attribute, if any
			attr = (sattr[0] | sattr[1]) & FLOAT;

			// Since adding an int to a fp value promotes it to a fp value, we
			// don't care whether it's first or second; we cast to to a double
			// regardless.
			if (attr == FLOAT)
			{
				PTR p;
				p.u64 = sval;
				double fpval1 = (sattr[0] & FLOAT ? *p.dp : (double)*p.i64);
				p.u64++;
				double fpval2 = (sattr[1] & FLOAT ? *p.dp : (double)*p.i64);
				*(double *)sval = fpval1 + fpval2;
			}
			else
			{
				*sval += sval[1];				// Compute value
			}
//printf("%i\n", *sval);

			if (!(*sattr & TDB))
				*sattr = sattr[1] | attr;
			else if (sattr[1] & TDB)
				return error(seg_error);

			break;

		case '-':
//printf("evexpr(): -\n");
			--sval;							// Pop value
			--sattr;						// Pop attrib
//printf("--> N-N: %i - %i = ", *sval, sval[1]);
			// Get FLOAT attribute, if any
			attr = (sattr[0] | sattr[1]) & FLOAT;

			// Since subtracting an int to a fp value promotes it to a fp
			// value, we don't care whether it's first or second; we cast to to
			// a double regardless.
			if (attr == FLOAT)
			{
				PTR p;
				p.u64 = sval;
				double fpval1 = (sattr[0] & FLOAT ? *p.dp : (double)*p.i64);
				p.u64++;
				double fpval2 = (sattr[1] & FLOAT ? *p.dp : (double)*p.i64);
				*(double *)sval = fpval1 - fpval2;
			}
			else
			{
				*sval -= sval[1];
			}
//printf("%i\n", *sval);

			*sattr |= attr;					// Inherit FLOAT attribute
			attr = (WORD)(*sattr & TDB);
#if 0
printf("EVEXPR (-): sym1 = %X, sym2 = %X\n", attr, sattr[1]);
#endif
			// If symbol1 is ABS, take attributes from symbol2
			if (!attr)
				*sattr = sattr[1];
			// Otherwise, they're both TDB and so attributes cancel out
			else if (sattr[1] & TDB)
				*sattr &= ~TDB;

			break;

		// Unary operators only work on ABS items
		case UNMINUS:
//printf("evexpr(): UNMINUS\n");
			if (*sattr & TDB)
				return error(seg_error);

			if (*sattr & FLOAT)
			{
				double * dst = (double *)sval;
				*dst = -*dst;
				*sattr = ABS | DEFINED | FLOAT; // Expr becomes absolute
			}
			else
			{
				*sval = -(int64_t)*sval;
				*sattr = ABS | DEFINED;			// Expr becomes absolute
			}

			break;

		case UNLT: // Unary < (get the low byte of a word)
//printf("evexpr(): UNLT\n");
			if (*sattr & TDB)
				return error(seg_error);

			if (*sattr & FLOAT)
				return error(noflt_error);

			*sval = (int64_t)((*sval) & 0x00FF);
			*sattr = ABS | DEFINED;			// Expr becomes absolute
			break;

		case UNGT: // Unary > (get the high byte of a word)
//printf("evexpr(): UNGT\n");
			if (*sattr & TDB)
				return error(seg_error);

			if (*sattr & FLOAT)
				return error(noflt_error);

			*sval = (int64_t)(((*sval) >> 8) & 0x00FF);
			*sattr = ABS | DEFINED;			// Expr becomes absolute
			break;

		case '!':
//printf("evexpr(): !\n");
			if (*sattr & TDB)
				return error(seg_error);

			if (*sattr & FLOAT)
				return error("floating point numbers not allowed with operator '!'.");

			*sval = !*sval;
			*sattr = ABS | DEFINED;			// Expr becomes absolute
			break;

		case '~':
//printf("evexpr(): ~\n");
			if (*sattr & TDB)
				return error(seg_error);

			if (*sattr & FLOAT)
				return error("floating point numbers not allowed with operator '~'.");

			*sval = ~*sval;
			*sattr = ABS | DEFINED;			// Expr becomes absolute
			break;

		// Comparison operators must have two values that
		// are in the same segment, but that's the only requirement.
		case LE:
//printf("evexpr(): LE\n");
			sattr--;
			sval--;

			if ((*sattr & TDB) != (sattr[1] & TDB))
				return error(seg_error);

			// Get FLOAT attribute, if any
			attr = (sattr[0] | sattr[1]) & FLOAT;

			// Cast any ints in the comparison to double, if there's at least
			// one double in the comparison.
			if (attr == FLOAT)
			{
				PTR p;
				p.u64 = sval;
				double fpval1 = (sattr[0] & FLOAT ? *p.dp : (double)*p.i64);
				p.u64++;
				double fpval2 = (sattr[1] & FLOAT ? *p.dp : (double)*p.i64);
				*sval = (fpval1 <= fpval2);
			}
			else
			{
				*sval = (*sval <= sval[1]);
			}

			*sattr = ABS | DEFINED;
			break;

		case GE:
//printf("evexpr(): GE\n");
			sattr--;
			sval--;

			if ((*sattr & TDB) != (sattr[1] & TDB))
				return error(seg_error);

			// Get FLOAT attribute, if any
			attr = (sattr[0] | sattr[1]) & FLOAT;

			// Cast any ints in the comparison to double, if there's at least
			// one double in the comparison.
			if (attr == FLOAT)
			{
				PTR p;
				p.u64 = sval;
				double fpval1 = (sattr[0] & FLOAT ? *p.dp : (double)*p.i64);
				p.u64++;
				double fpval2 = (sattr[1] & FLOAT ? *p.dp : (double)*p.i64);
				*sval = (fpval1 >= fpval2);
			}
			else
			{
				*sval = (*sval >= sval[1]);
			}

			*sattr = ABS | DEFINED;
			break;

		case '>':
//printf("evexpr(): >\n");
			sattr--;
			sval--;

			if ((*sattr & TDB) != (sattr[1] & TDB))
				return error(seg_error);

			// Get FLOAT attribute, if any
			attr = (sattr[0] | sattr[1]) & FLOAT;

			// Cast any ints in the comparison to double, if there's at least
			// one double in the comparison.
			if (attr == FLOAT)
			{
				PTR p;
				p.u64 = sval;
				double fpval1 = (sattr[0] & FLOAT ? *p.dp : (double)*p.i64);
				p.u64++;
				double fpval2 = (sattr[1] & FLOAT ? *p.dp : (double)*p.i64);
				*sval = (fpval1 > fpval2);
			}
			else
			{
				*sval = (*sval > sval[1]);
			}

			*sattr = ABS | DEFINED;
			break;

		case '<':
//printf("evexpr(): <\n");
			sattr--;
			sval--;

			if ((*sattr & TDB) != (sattr[1] & TDB))
				return error(seg_error);

			// Get FLOAT attribute, if any
			attr = (sattr[0] | sattr[1]) & FLOAT;

			// Cast any ints in the comparison to double, if there's at least
			// one double in the comparison.
			if (attr == FLOAT)
			{
				PTR p;
				p.u64 = sval;
				double fpval1 = (sattr[0] & FLOAT ? *p.dp : (double)*p.i64);
				p.u64++;
				double fpval2 = (sattr[1] & FLOAT ? *p.dp : (double)*p.i64);
				*sval = (fpval1 < fpval2);
			}
			else
			{
				*sval = (*sval < sval[1]);
			}

			*sattr = ABS | DEFINED;		// Expr forced to ABS
			break;

		case NE:
//printf("evexpr(): NE\n");
			sattr--;
			sval--;

			if ((*sattr & TDB) != (sattr[1] & TDB))
				return error(seg_error);

			// Get FLOAT attribute, if any
			attr = (sattr[0] | sattr[1]) & FLOAT;

			// Cast any ints in the comparison to double, if there's at least
			// one double in the comparison.
			if (attr == FLOAT)
			{
				PTR p;
				p.u64 = sval;
				double fpval1 = (sattr[0] & FLOAT ? *p.dp : (double)*p.i64);
				p.u64++;
				double fpval2 = (sattr[1] & FLOAT ? *p.dp : (double)*p.i64);
				*sval = (fpval1 != fpval2);
			}
			else
			{
				*sval = (*sval != sval[1]);
			}

			*sattr = ABS | DEFINED;		// Expr forced to ABS
			break;

		case '=':
//printf("evexpr(): =\n");
			sattr--;
			sval--;

			if ((*sattr & TDB) != (sattr[1] & TDB))
				return error(seg_error);

			// Get FLOAT attribute, if any
			attr = (sattr[0] | sattr[1]) & FLOAT;

			// Cast any ints in the comparison to double, if there's at least
			// one double in the comparison.
			if (attr == FLOAT)
			{
				PTR p;
				p.u64 = sval;
				double fpval1 = (sattr[0] & FLOAT ? *p.dp : (double)*p.i64);
				p.u64++;
				double fpval2 = (sattr[1] & FLOAT ? *p.dp : (double)*p.i64);
				*sval = (fpval1 == fpval2);
			}
			else
			{
				*sval = (*sval == sval[1]);
			}

			*sattr = ABS | DEFINED;		// Expr forced to ABS

			break;

		// All other binary operators must have two ABS items to work with.
		// They all produce an ABS value.
		// Shamus: Is this true? There's at least one counterexample of legit
		//         code where this assumption fails to produce correct code.
		default:
//printf("evexpr(): default\n");

			switch ((int)tk.u32[-1])
			{
			case '*':
				sval--;
				sattr--;
//printf("--> NxN: %i x %i = ", *sval, sval[1]);
				// Get FLOAT attribute, if any
				attr = (sattr[0] | sattr[1]) & FLOAT;

				// Since multiplying an int to a fp value promotes it to a fp
				// value, we don't care whether it's first or second; it will
				// be cast to a double regardless.
/*
An open question here is do we promote ints to floats as signed or unsigned? It makes a difference if, say, the int is put in as -1 but is promoted to a double as $FFFFFFFFFFFFFFFF--you get very different results that way! For now, we promote as signed until proven detrimental otherwise.
*/
				if (attr == FLOAT)
				{
					PTR p;
					p.u64 = sval;
					double fpval1 = (sattr[0] & FLOAT ? *p.dp : (double)*p.i64);
					p.u64++;
					double fpval2 = (sattr[1] & FLOAT ? *p.dp : (double)*p.i64);
					*(double *)sval = fpval1 * fpval2;
				}
				else
				{
					*sval *= sval[1];
				}
//printf("%i\n", *sval);

//no				*sattr = ABS | DEFINED | attr;		// Expr becomes absolute
				break;

			case '/':
				sval--;
				sattr--;
//printf("--> N/N: %i / %i = ", sval[0], sval[1]);
				// Get FLOAT attribute, if any
				attr = (sattr[0] | sattr[1]) & FLOAT;

				if (attr == FLOAT)
				{
					PTR p;
					p.u64 = sval;
					double fpval1 = (sattr[0] & FLOAT ? *p.dp : (double)*p.i64);
					p.u64++;
					double fpval2 = (sattr[1] & FLOAT ? *p.dp : (double)*p.i64);

					if (fpval2 == 0)
						return error("divide by zero");

					*(double *)sval = fpval1 / fpval2;
				}
				else
				{
					if (sval[1] == 0)
						return error("divide by zero");
//printf("--> N/N: %i / %i = ", sval[0], sval[1]);

					// Compiler is picky here: Without casting these, it
					// discards the sign if dividing a negative # by a
					// positive one, creating a bad result. :-/
					// Definitely a side effect of using uint32_ts intead of
					// ints.
					*sval = (int32_t)sval[0] / (int32_t)sval[1];
				}
//printf("%i\n", *sval);

//no				*sattr = ABS | DEFINED | attr;		// Expr becomes absolute
				break;

			case '%':
				sval--;
				sattr--;

				if ((*sattr | sattr[1]) & FLOAT)
					return error("floating point numbers not allowed with operator '%'.");

				if (sval[1] == 0)
					return error("mod (%) by zero");

				*sval %= sval[1];
//no				*sattr = ABS | DEFINED;			// Expr becomes absolute
				break;

			case SHL:
				sval--;
				sattr--;					// Pop attrib

				if ((*sattr | sattr[1]) & FLOAT)
					return error("floating point numbers not allowed with operator '<<'.");

				*sval <<= sval[1];
//no				*sattr = ABS | DEFINED;			// Expr becomes absolute
				break;

			case SHR:
				sval--;
				sattr--;					// Pop attrib

				if ((*sattr | sattr[1]) & FLOAT)
					return error("floating point numbers not allowed with operator '>>'.");

				*sval >>= sval[1];
//no				*sattr = ABS | DEFINED;			// Expr becomes absolute
				break;

			case '&':
				sval--;
				sattr--;					// Pop attrib

				if ((*sattr | sattr[1]) & FLOAT)
					return error("floating point numbers not allowed with operator '&'.");

				*sval &= sval[1];
//no				*sattr = ABS | DEFINED;			// Expr becomes absolute
				break;

			case '^':
				sval--;
				sattr--;					// Pop attrib

				if ((*sattr | sattr[1]) & FLOAT)
					return error("floating point numbers not allowed with operator '^'.");

				*sval ^= sval[1];
//no				*sattr = ABS | DEFINED;			// Expr becomes absolute
				break;

			case '|':
				sval--;
				sattr--;

				if ((*sattr | sattr[1]) & FLOAT)
					return error("floating point numbers not allowed with operator '|'.");

				*sval |= sval[1];
//no				*sattr = ABS | DEFINED;			// Expr becomes absolute
				break;

			default:
				// Bad operator in expression stream (this should never happen!)
				interror(5);
			}
		}
	}

	if (esym != NULL)
		*sattr &= ~DEFINED;

	if (a_esym != NULL)
		*a_esym = esym;

	// Copy value + attrib into return variables
	*a_value = *sval;
	*a_attr = *sattr;

	return OK;
}


//
// Count the # of tokens in the passed in expression
// N.B.: 64-bit constants count as two tokens each
//
uint16_t ExpressionLength(TOKEN * tk)
{
	uint16_t length;

	for(length=0; tk[length]!=ENDEXPR; length++)
	{
		// Add one to length for 2X tokens, two for 3X tokens
		if (tk[length] == SYMBOL)
			length++;
		else if ((tk[length] == CONST) || (tk[length] == FCONST))
			length += 2;
	}

	// Add 1 for ENDEXPR
	length++;

	return length;
}

