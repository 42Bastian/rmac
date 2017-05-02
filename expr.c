//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// EXPR.C - Expression Analyzer
// Copyright (C) 199x Landon Dyer, 2017 Reboot and Friends
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
static VALUE evstk[EVSTACKSIZE];	// Evaluator value stack
static WORD evattr[EVSTACKSIZE];	// Evaluator attribute stack

// Token-class initialization list
char itokcl[] = {
	0,								// END
	CONST, SYMBOL, 0,				// ID
	'(', '[', '{', 0,				// OPAR
	')', ']', '}', 0,				// CPAR
	CR_DEFINED, CR_REFERENCED,		// SUNARY (special unary)
	CR_STREQ, CR_MACDEF,
	CR_DATE, CR_TIME,
	CR_ABSCOUNT, 0,
	'!', '~', UNMINUS, 0,			// UNARY
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

// Convert expression to postfix
static TOKEN * evalTokenBuffer;		// Deposit tokens here (this is really a
									// pointer to exprbuf from direct.c)
									// (Can also be from others, like
									// riscasm.c)
static int symbolNum;				// Pointer to the entry in symbolPtr[]


//
// Obtain a string value
//
static VALUE str_value(char * p)
{
	VALUE v;

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
	TOKEN t;

	if (expr1() != OK)
		return ERROR;

	while (tokenClass[*tok] >= MULT)
	{
		t = *tok++;

		if (expr1() != OK)
			return ERROR;

		*evalTokenBuffer++ = t;
	}

	return OK;
}


//
// Unary operators (detect unary '-')
// ggn: If expression starts with a plus then also eat it up.
//      For some reason the parser gets confused when this happens and
//      emits a "bad expression".
//
int expr1(void)
{
	int class;
	TOKEN t;
	SYM * sy;
	char * p, * p2;
	WORD w;
	int j;

	class = tokenClass[*tok];

	if (*tok == '-' || *tok == '+' || class == UNARY)
	{
		t = *tok++;

		if (expr2() != OK)
			return ERROR;

		if (t == '-')
			t = UNMINUS;

		*evalTokenBuffer++ = t;
	}
	else if (class == SUNARY)
	{
		switch ((int)*tok++)
		{
		case CR_ABSCOUNT:
			*evalTokenBuffer++ = CONST;
			*evalTokenBuffer++ = (LONG)sect[ABS].sloc;
			break;
		case CR_TIME:
			*evalTokenBuffer++ = CONST;
			*evalTokenBuffer++ = dos_time();
			break;
		case CR_DATE:
			*evalTokenBuffer++ = CONST;
			*evalTokenBuffer++ = dos_date();
			break;
		case CR_MACDEF:                                    // ^^macdef <macro-name>
			if (*tok++ != SYMBOL)
				return error(missym_error);

			p = string[*tok++];
			w = (lookup(p, MACRO, 0) == NULL ? 0 : 1);
			*evalTokenBuffer++ = CONST;
			*evalTokenBuffer++ = (TOKEN)w;
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
			j = (*p == '.' ? curenv : 0);
			w = ((sy = lookup(p, LABEL, j)) != NULL && (sy->sattr & w) ? 1 : 0);
			*evalTokenBuffer++ = CONST;
			*evalTokenBuffer++ = (TOKEN)w;
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

			p2 = string[tok[1]];
			tok += 2;

			w = (WORD)(!strcmp(p, p2));
			*evalTokenBuffer++ = CONST;
			*evalTokenBuffer++ = (TOKEN)w;
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
	char * p;
	SYM * sy;
	int j;

	switch ((int)*tok++)
	{
	case CONST:
		*evalTokenBuffer++ = CONST;
		*evalTokenBuffer++ = *tok++;
		break;
	case SYMBOL:
		p = string[*tok++];
		j = (*p == '.' ? curenv : 0);
		sy = lookup(p, LABEL, j);

		if (sy == NULL)
			sy = NewSymbol(p, LABEL, j);

		// Check register bank usage
		if (sy->sattre & EQUATEDREG)
		{
			if ((regbank == BANK_0) && (sy->sattre & BANK_1) && !altbankok)
				warns("equated symbol \'%s\' cannot be used in register bank 0", sy->sname);

			if ((regbank == BANK_1) && (sy->sattre & BANK_0) && !altbankok)
				warns("equated symbol \'%s\' cannot be used in register bank 1", sy->sname);
		}

		*evalTokenBuffer++ = SYMBOL;
		*evalTokenBuffer++ = symbolNum;
		symbolPtr[symbolNum] = sy;
		symbolNum++;
		break;
	case STRING:
		*evalTokenBuffer++ = CONST;
		*evalTokenBuffer++ = str_value(string[*tok++]);
		break;
	case '(':
		if (expr0() != OK)
			return ERROR;

		if (*tok++ != ')')
			return error("missing close parenthesis ')'");

		break;
	case '[':
		if (expr0() != OK)
			return ERROR;

		if (*tok++ != ']')
			return error("missing close parenthesis ']'");

		break;
	case '$':
		*evalTokenBuffer++ = ACONST;				// Attributed const
		*evalTokenBuffer++ = sloc;					// Current location
		*evalTokenBuffer++ = cursect | DEFINED;		// Store attribs
		break;
	case '*':
		*evalTokenBuffer++ = ACONST;				// Attributed const

		// pcloc == location at start of line
		*evalTokenBuffer++ = (orgactive ? orgaddr : pcloc);
		// '*' takes attributes of current section, not ABS!
		*evalTokenBuffer++ = cursect | DEFINED;
		break;
	default:
		return error("bad expression");
	}

	return OK;
}


//
// Recursive-descent expression analyzer (with some simple speed hacks)
//
int expr(TOKEN * otk, VALUE * a_value, WORD * a_attr, SYM ** a_esym)
{
	// Passed in values (once derefenced, that is) can all be zero. They are
	// there so that the expression analyzer can fill them in as needed. The
	// expression analyzer gets its input from the global token pointer "tok",
	// and not from anything passed in by the user.
	SYM * symbol;
	char * p;
	int j;

	evalTokenBuffer = otk;	// Set token pointer to 'exprbuf' (direct.c)
							// Also set in various other places too (riscasm.c, e.g.)

//printf("expr(): tokens 0-2: %i %i %i (%c %c %c); tc[2] = %i\n", tok[0], tok[1], tok[2], tok[0], tok[1], tok[2], tokenClass[tok[2]]);
	// Optimize for single constant or single symbol.
	// Shamus: Subtle bug here. EOL token is 101; if you have a constant token
	//         followed by the value 101, it will trigger a bad evaluation here.
	//         This is probably a really bad assumption to be making here...!
	//         (assuming tok[1] == EOL is a single token that is)
	//         Seems that even other tokens (SUNARY type) can fuck this up too.
//	if ((tok[1] == EOL)
	if ((tok[1] == EOL && (tok[0] != CONST && tokenClass[tok[0]] != SUNARY))
		|| (((*tok == CONST || *tok == SYMBOL) || (*tok >= KW_R0 && *tok <= KW_R31))
		&& (tokenClass[tok[2]] < UNARY)))
	{
		if (*tok >= KW_R0 && *tok <= KW_R31)
		{
			*evalTokenBuffer++ = CONST;
			*evalTokenBuffer++ = *a_value = (*tok - KW_R0);
			*a_attr = ABS | DEFINED;

			if (a_esym != NULL)
				*a_esym = NULL;

			tok++;
		}
		else if (*tok == CONST)
		{
			*evalTokenBuffer++ = CONST;
			*evalTokenBuffer++ = *a_value = tok[1];
			*a_attr = ABS | DEFINED;

			if (a_esym != NULL)
				*a_esym = NULL;

			tok += 2;
//printf("Quick eval in expr(): CONST = %i, tokenClass[tok[2]] = %i\n", *a_value, tokenClass[*tok]);
		}
		else if (*tok == '*')
		{
			*evalTokenBuffer++ = CONST;

			if (orgactive)
				*evalTokenBuffer++ = *a_value = orgaddr;
			else
				*evalTokenBuffer++ = *a_value = pcloc;

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
				errors("undefined register equate '%s'", symbol->sname);
//if we return right away, it returns some spurious errors...
//				return ERROR;
			}

			// Check register bank usage
			if (symbol->sattre & EQUATEDREG)
			{
				if ((regbank == BANK_0) && (symbol->sattre & BANK_1) && !altbankok)
					warns("equated symbol '%s' cannot be used in register bank 0", symbol->sname);

				if ((regbank == BANK_1) && (symbol->sattre & BANK_0) && !altbankok)
					warns("equated symbol '%s' cannot be used in register bank 1", symbol->sname);
			}

			*evalTokenBuffer++ = SYMBOL;
#if 0
			*evalTokenBuffer++ = (TOKEN)symbol;
#else
/*
While this approach works, it's wasteful. It would be better to use something
that's already available, like the symbol "order defined" table (which needs to
be converted from a linked list into an array).
*/
			*evalTokenBuffer++ = symbolNum;
			symbolPtr[symbolNum] = symbol;
			symbolNum++;
#endif

			if (symbol->sattr & DEFINED)
				*a_value = symbol->svalue;
			else
				*a_value = 0;

/*
All that extra crap that was put into the svalue when doing the equr stuff is
thrown away right here. What the hell is it for?
*/
			if (symbol->sattre & EQUATEDREG)
				*a_value &= 0x1F;

			*a_attr = (WORD)(symbol->sattr & ~GLOBAL);

			if ((symbol->sattr & (GLOBAL | DEFINED)) == GLOBAL
				&& a_esym != NULL)
				*a_esym = symbol;

			tok += 2;
		}
		else
		{
			// Unknown type here... Alert the user!,
			error("undefined RISC register in expression");
			// Prevent spurious error reporting...
			tok++;
			return ERROR;
		}

		*evalTokenBuffer++ = ENDEXPR;
		return OK;
	}

	if (expr0() != OK)
		return ERROR;

	*evalTokenBuffer++ = ENDEXPR;
	return evexpr(otk, a_value, a_attr, a_esym);
}


//
// Evaluate expression.
// If the expression involves only ONE external symbol, the expression is
// UNDEFINED, but it's value includes everything but the symbol value, and
// `a_esym' is set to the external symbol.
//
int evexpr(TOKEN * tk, VALUE * a_value, WORD * a_attr, SYM ** a_esym)
{
	WORD attr;
	SYM * sy;
	VALUE * sval = evstk;					// (Empty) initial stack
	WORD * sattr = evattr;
	SYM * esym = NULL;						// No external symbol involved
	WORD sym_seg = 0;

	while (*tk != ENDEXPR)
	{
		switch ((int)*tk++)
		{
		case SYMBOL:
//printf("evexpr(): SYMBOL\n");
			sy = symbolPtr[*tk++];
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
			{
				*++sval = sy->svalue;		// Push symbol's value
			}
			else
			{
				*++sval = 0;				// 0 for undefined symbols
			}

			*++sattr = (WORD)(sy->sattr & ~GLOBAL);	// Push attribs
			sym_seg = (WORD)(sy->sattr & TDB);
			break;
		case CONST:
//printf("evexpr(): CONST = %i\n", *tk);
			*++sval = *tk++;				// Push value
			*++sattr = ABS | DEFINED;		// Push simple attribs
			break;
		case ACONST:
//printf("evexpr(): ACONST = %i\n", *tk);
			*++sval = *tk++;				// Push value
			*++sattr = (WORD)*tk++;			// Push attribs
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
			*sval += sval[1];				// Compute value
//printf("%i\n", *sval);

			if (!(*sattr & TDB))
				*sattr = sattr[1];
			else if (sattr[1] & TDB)
				return error(seg_error);

			break;
		case '-':
//printf("evexpr(): -\n");
			--sval;							// Pop value
			--sattr;						// Pop attrib
//printf("--> N-N: %i - %i = ", *sval, sval[1]);
			*sval -= sval[1];				// Compute value
//printf("%i\n", *sval);

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
				error(seg_error);

			*sval = -(int)*sval;
			*sattr = ABS | DEFINED;			// Expr becomes absolute
			break;
		case '!':
//printf("evexpr(): !\n");
			if (*sattr & TDB)
				error(seg_error);

			*sval = !*sval;
			*sattr = ABS | DEFINED;			// Expr becomes absolute
			break;
		case '~':
//printf("evexpr(): ~\n");
			if (*sattr & TDB)
				error(seg_error);

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
				error(seg_error);

			*sattr = ABS | DEFINED;
			*sval = *sval <= sval[1];
			break;
		case GE:
//printf("evexpr(): GE\n");
			sattr--;
			sval--;

			if ((*sattr & TDB) != (sattr[1] & TDB))
				error(seg_error);

			*sattr = ABS | DEFINED;
			*sval = *sval >= sval[1];
			break;
		case '>':
//printf("evexpr(): >\n");
			sattr--;
			sval--;

			if ((*sattr & TDB) != (sattr[1] & TDB))
				error(seg_error);

			*sattr = ABS | DEFINED;
			*sval = *sval > sval[1];
			break;
		case '<':
//printf("evexpr(): <\n");
			sattr--;
			sval--;

			if ((*sattr & TDB) != (sattr[1] & TDB))
				error(seg_error);

			*sattr = ABS | DEFINED;
			*sval = *sval < sval[1];
			break;
		case NE:
//printf("evexpr(): NE\n");
			sattr--;
			sval--;

			if ((*sattr & TDB) != (sattr[1] & TDB))
				error(seg_error);

			*sattr = ABS | DEFINED;
			*sval = *sval != sval[1];
			break;
		case '=':
//printf("evexpr(): =\n");
			sattr--;
			sval--;

			if ((*sattr & TDB) != (sattr[1] & TDB))
				error(seg_error);

			*sattr = ABS | DEFINED;
			*sval = *sval == sval[1];
			break;
		// All other binary operators must have two ABS items
		// to work with.  They all produce an ABS value.
		default:
//printf("evexpr(): default\n");
			// GH - Removed for v1.0.15 as part of the fix for indexed loads.
			//if ((*sattr & (TEXT|DATA|BSS)) || (*--sattr & (TEXT|DATA|BSS)))
			//error(seg_error);
			*sattr = ABS | DEFINED;			// Expr becomes absolute

			switch ((int)tk[-1])
			{
			case '*':
				sval--;
				sattr--;					// Pop attrib
//printf("--> NxN: %i x %i = ", *sval, sval[1]);
				*sval *= sval[1];
//printf("%i\n", *sval);
				break;
			case '/':
				sval--;
				sattr--;					// Pop attrib

				if (sval[1] == 0)
					return error("divide by zero");

//printf("--> N/N: %i / %i = ", sval[0], sval[1]);
				// Compiler is picky here: Without casting these, it discards
				// the sign if dividing a negative # by a positive one,
				// creating a bad result. :-/
				// Probably a side effect of using VALUE intead of ints.
				*sval = (int)sval[0] / (int)sval[1];
//printf("%i\n", *sval);
				break;
			case '%':
				sval--;
				sattr--;					// Pop attrib

				if (sval[1] == 0)
					return error("mod (%) by zero");

				*sval %= sval[1];
				break;
			case SHL:
				sval--;
				sattr--;					// Pop attrib
				*sval <<= sval[1];
				break;
			case SHR:
				sval--;
				sattr--;					// Pop attrib
				*sval >>= sval[1];
				break;
			case '&':
				sval--;
				sattr--;					// Pop attrib
				*sval &= sval[1];
				break;
			case '^':
				sval--;
				sattr--;					// Pop attrib
				*sval ^= sval[1];
				break;
			case '|':
				sval--;
				sattr--;					// Pop attrib
				*sval |= sval[1];
				break;
			default:
				interror(5);				// Bad operator in expression stream
			}
		}
	}

	if (esym != NULL)
		*sattr &= ~DEFINED;

	if (a_esym != NULL)
		*a_esym = esym;

	// sym_seg added in 1.0.16 to solve a problem with forward symbols in
	// expressions where absolute values also existed. The absolutes were
	// overiding the symbol segments and not being included :(
	//*a_attr = *sattr | sym_seg;           // Copy value + attrib

	*a_attr = *sattr;						// Copy value + attrib
	*a_value = *sval;

	return OK;
}

