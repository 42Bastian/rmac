//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// EXPR.C - Expression Analyzer
// Copyright (C) 199x Landon Dyer, 2011 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source Utilised with the Kind Permission of Landon Dyer
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

#define DEF_KW							// Declare keyword values 
#include "kwtab.h"						// Incl generated keyword tables & defs

static char tokenClass[128];					// Generated table of token classes
static VALUE evstk[EVSTACKSIZE];		// Evaluator value stack
static WORD evattr[EVSTACKSIZE];		// Evaluator attribute stack

// Token-class initialization list
char itokcl[] = {
	0,									// END
	CONST, SYMBOL, 0,					// ID 
	'(', '[', '{', 0,					// OPAR
	')', ']', '}', 0,					// CPAR 
	CR_DEFINED, CR_REFERENCED,			// SUNARY (special unary)
	CR_STREQ, CR_MACDEF,
	CR_DATE, CR_TIME, 0,
	'!', '~', UNMINUS, 0,				// UNARY
	'*', '/', '%', 0,					// MULT 
	'+', '-', 0,						// ADD 
	SHL, SHR, 0,						// SHIFT 
	LE, GE, '<', '>', NE, '=', 0,		// REL 
	'&', 0,								// AND 
	'^', 0,								// XOR 
	'|', 0,								// OR 
	1									// (the end) 
};

const char missym_error[] = "missing symbol";
const char str_error[] = "missing symbol or string";

// Convert expression to postfix
static TOKEN * evalTokenBuffer;			// Deposit tokens here (this is really a
										// pointer to exprbuf from direct.c)
										// (Can also be from others, like riscasm.c)
static symbolNum;						// Pointer to the entry in symbolPtr[]


//
// Obtain a String Value
//
static VALUE str_value(char * p)
{
	VALUE v;

	for(v=0; *p; p++)
		v = (v << 8) | (*p & 0xFF);

	return v;
}


//
// Initialize Expression Analyzer
//
void InitExpression(void)
{
	int i;									// Iterator
	char * p;								// Token pointer

	// Initialize token-class table (all set to END)
	for(i=0; i<128; i++)
		tokenClass[i] = END;

	for(i=0, p=itokcl; *p!=1; p++)
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

	if (*tok == '-' || class == UNARY)
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

			p = string[tok[1]];
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
#if 0
		*evalTokenBuffer++ = (TOKEN)sy;
#else
		*evalTokenBuffer++ = symbolNum;
		symbolPtr[symbolNum] = sy;
		symbolNum++;
#endif
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

		if (orgactive)
			*evalTokenBuffer++ = orgaddr;
		else
			*evalTokenBuffer++ = pcloc;				// Location at start of line

		*evalTokenBuffer++ = ABS | DEFINED;			// Store attribs
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

	// Optimize for single constant or single symbol.
	if ((tok[1] == EOL)
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
			*evalTokenBuffer++ = ENDEXPR;
			return OK;
		}
		else if (*tok == CONST)
		{
			*evalTokenBuffer++ = CONST;
			*evalTokenBuffer++ = *a_value = tok[1];
			*a_attr = ABS | DEFINED;

			if (a_esym != NULL)
				*a_esym = NULL;
		}
		else if (*tok == '*')
		{
			*evalTokenBuffer++ = CONST;

			if (orgactive)
				*evalTokenBuffer++ = *a_value = orgaddr;
			else
				*evalTokenBuffer++ = *a_value = pcloc;

			*a_attr = ABS | DEFINED;

			if (a_esym != NULL)
				*a_esym = NULL;

			tok--;
		}
		else if (*tok == STRING || *tok == SYMBOL)
		{
			p = string[tok[1]];
			j = (*p == '.' ? curenv : 0);
			symbol = lookup(p, LABEL, j);

			if (symbol == NULL)
				symbol = NewSymbol(p, LABEL, j);

			symbol->sattr |= REFERENCED;

			// Check for undefined register equates
			if (symbol->sattre & UNDEF_EQUR)
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

			if ((symbol->sattr & (GLOBAL | DEFINED)) == GLOBAL && a_esym != NULL)
				*a_esym = symbol;
		}
		else
		{
			// Unknown type here... Alert the user!
			error("undefined RISC register in expression");
			tok++;
			return ERROR;
		}

		tok += 2;
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
	WORD * sattr;
	VALUE * sval;
	WORD attr;
	SYM * sy;
	SYM * esym;
	WORD sym_seg;

	sval = evstk;							// (Empty) initial stack
	sattr = evattr;
	esym = NULL;							// No external symbol involved
	sym_seg = 0;

	while (*tk != ENDEXPR)
	{
		switch ((int)*tk++)
		{
		case SYMBOL:
//			sy = (SYM *)*tk++;
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
			sym_seg = (WORD)(sy->sattr & (TEXT | DATA | BSS));
			break;
		case CONST:
			*++sval = *tk++;				// Push value
			*++sattr = ABS | DEFINED;		// Push simple attribs
			break;
		case ACONST:
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
			--sval;							// Pop value
			--sattr;						// Pop attrib 
			*sval += sval[1];				// Compute value

			if (!(*sattr & (TEXT | DATA | BSS)))
				*sattr = sattr[1];
			else if (sattr[1] & (TEXT | DATA | BSS))
				return error(seg_error);

			break;
		case '-':
			--sval;							// Pop value
			--sattr;						// Pop attrib 
			*sval -= sval[1];				// Compute value

			attr = (WORD)(*sattr & (TEXT | DATA | BSS));

			if (!attr)
				*sattr = sattr[1];
			else if (sattr[1] & (TEXT | DATA | BSS))
			{
				if (!(attr & sattr[1]))
					return error(seg_error);
				else
					*sattr &= ~(TEXT | DATA | BSS);
			}

			break;
		// Unary operators only work on ABS items
		case UNMINUS:
			if (*sattr & (TEXT | DATA | BSS))
				error(seg_error);

			*sval = -(int)*sval;
			*sattr = ABS | DEFINED;			// Expr becomes absolute
			break;
		case '!':
			if (*sattr & (TEXT | DATA | BSS))
				error(seg_error);

			*sval = !*sval;
			*sattr = ABS | DEFINED;			// Expr becomes absolute
			break;
		case '~':
			if (*sattr & (TEXT | DATA | BSS))
				error(seg_error);

			*sval = ~*sval;
			*sattr = ABS | DEFINED;			// Expr becomes absolute
			break;
		// Comparison operators must have two values that
		// are in the same segment, but that's the only requirement.
		case LE:
			--sattr;
			--sval;

			if ((*sattr & TDB) != (sattr[1] & TDB))
				error(seg_error);

			*sattr = ABS | DEFINED;
			*sval = *sval <= sval[1];
			break;
		case GE:
			--sattr;
			--sval;

			if ((*sattr & TDB) != (sattr[1] & TDB))
				error(seg_error);

			*sattr = ABS | DEFINED;
			*sval = *sval >= sval[1];
			break;
		case '>':
			--sattr;
			--sval;

			if ((*sattr & TDB) != (sattr[1] & TDB))
				error(seg_error);

			*sattr = ABS | DEFINED;
			*sval = *sval > sval[1];
			break;
		case '<':
			--sattr;
			--sval;

			if ((*sattr & TDB) != (sattr[1] & TDB))
				error(seg_error);

			*sattr = ABS | DEFINED;
			*sval = *sval < sval[1];
			break;
		case NE:
			--sattr;
			--sval;

			if ((*sattr & TDB) != (sattr[1] & TDB))
				error(seg_error);

			*sattr = ABS | DEFINED;
			*sval = *sval != sval[1];
			break;
		case '=':
			--sattr;
			--sval;

			if ((*sattr & TDB) != (sattr[1] & TDB))
				error(seg_error);

			*sattr = ABS | DEFINED;
			*sval = *sval == sval[1];
			break;
		// All other binary operators must have two ABS items
		// to work with.  They all produce an ABS value.
		default:
			// GH - Removed for v1.0.15 as part of the fix for indexed loads.
			//if ((*sattr & (TEXT|DATA|BSS)) || (*--sattr & (TEXT|DATA|BSS)))
			//error(seg_error);
			*sattr = ABS | DEFINED;			// Expr becomes absolute

			switch ((int)tk[-1])
			{
			case '*':
				--sval;
				--sattr;					// Pop attrib 
				*sval *= sval[1];
				break;
			case '/':
				--sval;
				--sattr;					// Pop attrib 

				if (sval[1] == 0)
					return error("divide by zero");

				*sval /= sval[1];
				break;
			case '%':
				--sval;
				--sattr;					// Pop attrib 

				if (sval[1] == 0)
					return error("mod (%) by zero");

				*sval %= sval[1];
				break;
			case SHL:
				--sval;
				--sattr;					// Pop attrib 
				*sval <<= sval[1];
				break;
			case SHR:
				--sval;
				--sattr;					// Pop attrib 
				*sval >>= sval[1];
				break;
			case '&':
				--sval;
				--sattr;					// Pop attrib 
				*sval &= sval[1];
				break;
			case '^':
				--sval;
				--sattr;					// Pop attrib 
				*sval ^= sval[1];
				break;
			case '|':
				--sval;
				--sattr;					// Pop attrib 
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
	//*a_attr = *sattr | sym_seg;                                        // Copy value + attrib

	*a_attr = *sattr;						// Copy value + attrib
	*a_value = *sval;

	return OK;
}
