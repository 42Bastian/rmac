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
#include "risca.h"
#include "sect.h"
#include "symbol.h"
#include "token.h"

#define DEF_KW							// Declare keyword values 
#include "kwtab.h"						// Incl generated keyword tables & defs

static char tokcl[128];					// Generated table of token classes
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
static TOKEN * tk;						// Deposit tokens here (this is really a
										// pointer to exprbuf from direct.c)
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
void init_expr(void)
{
	int i;									// Iterator
	char * p;								// Token pointer

	// Initialize token-class table
	for(i=0; i<128; i++)					// Mark all entries END
		tokcl[i] = END;

	for(i=0, p=itokcl; *p!=1; p++)
	{
		if (*p == 0)
			i++;
		else 
			tokcl[(int)(*p)] = (char)i;
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
	
	while (tokcl[*tok] >= MULT)
	{
		t = *tok++;

		if (expr1() != OK)
			return ERROR;

		*tk++ = t;
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

	class = tokcl[*tok];

	if (*tok == '-' || class == UNARY)
	{
		t = *tok++;

		if (expr2() != OK)
			return ERROR;

		if (t == '-')
			t = UNMINUS;

		*tk++ = t;
	}
	else if (class == SUNARY)
	{
		switch ((int)*tok++)
		{
		case CR_TIME:
			*tk++ = CONST;
			*tk++ = dos_time();
			break;
		case CR_DATE:
			*tk++ = CONST;
			*tk++ = dos_date();
			break;
		case CR_MACDEF:                                    // ^^macdef <macro-name>
			if (*tok++ != SYMBOL)
				return error(missym_error);

#if 0
			p = (char *)*tok++;
#else
			p = string[*tok++];
#endif
			w = (lookup(p, MACRO, 0) == NULL ? 0 : 1);
			*tk++ = CONST;
			*tk++ = (TOKEN)w;
			break;
		case CR_DEFINED:
			w = DEFINED;
			goto getsym;
		case CR_REFERENCED:
			w = REFERENCED;
getsym:
			if (*tok++ != SYMBOL)
				return error(missym_error);

#if 0
			p = (char *)*tok++;
#else
			p = string[*tok++];
#endif
			j = 0;

			if (*p == '.')
				j = curenv;

			w = ((sy = lookup(p, LABEL, j)) != NULL && (sy->sattr & w) ? 1 : 0);
			*tk++ = CONST;
			*tk++ = (TOKEN)w;
			break;
		case CR_STREQ:
			if (*tok != SYMBOL && *tok != STRING)
				return error(str_error);

#if 0
			p = (char *)tok[1];
#else
			p = string[tok[1]];
#endif
			tok +=2;

			if (*tok++ != ',')
				return error(comma_error);

			if (*tok != SYMBOL && *tok != STRING)
				return error(str_error);

#if 0
			p2 = (char *)tok[1];
#else
			p = string[tok[1]];
#endif
			tok += 2;

			w = (WORD)(!strcmp(p, p2));
			*tk++ = CONST;
			*tk++ = (TOKEN)w;
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
		*tk++ = CONST;
		*tk++ = *tok++;
		break;
	case SYMBOL:
#if 0
		p = (char *)*tok++;
#else
		p = string[*tok++];
#endif
		j = 0;

		if (*p == '.')
			j = curenv;

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

		*tk++ = SYMBOL;
#if 0
		*tk++ = (TOKEN)sy;
#else
		*tk++ = symbolNum;
		symbolPtr[symbolNum] = sy;
		symbolNum++;
#endif
		break;
	case STRING:
		*tk++ = CONST;
#if 0
		*tk++ = str_value((char *)*tok++);
#else
		*tk++ = str_value(string[*tok++]);
#endif
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
		*tk++ = ACONST;                                    // Attributed const
		*tk++ = sloc;                                      // Current location
		*tk++ = cursect | DEFINED;                         // Store attribs
		break;
	case '*':
		*tk++ = ACONST;                                    // Attributed const

		if (orgactive)
			*tk++ = orgaddr;
		else
			*tk++ = pcloc;                                  // Location at start of line

		*tk++ = ABS | DEFINED;                             // Store attribs
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
	// expression analyzer gets its input from "tok", and not from anything
	// passed in by the user.
	SYM * sy;
	char * p;
	int j;

	tk = otk;			// Set token pointer to 'exprbuf' (direct.c)
						// Also set in various other places too (risca.c, e.g.)
//	symbolNum = 0;		// Set symbol number in symbolPtr[] to 0

	// Optimize for single constant or single symbol.
	if ((tok[1] == EOL)
		|| (((*tok == CONST || *tok == SYMBOL) || (*tok >= KW_R0 && *tok <= KW_R31))
		&& (tokcl[tok[2]] < UNARY)))
	{
		if (*tok >= KW_R0 && *tok <= KW_R31)
		{
			*tk++ = CONST;
			*tk++ = *a_value = (*tok - KW_R0);
			*a_attr = ABS | DEFINED;

			if (a_esym != NULL)
				*a_esym = NULL;

			tok++;
			*tk++ = ENDEXPR;
			return OK;
		}
		else if (*tok == CONST)
		{
			*tk++ = CONST;
			*tk++ = *a_value = tok[1];
			*a_attr = ABS | DEFINED;

			if (a_esym != NULL)
				*a_esym = NULL;
		}
		else if (*tok == '*')
		{
			*tk++ = CONST;

			if (orgactive)
				*tk++ = *a_value = orgaddr;
			else
				*tk++ = *a_value = pcloc;

			*a_attr = ABS | DEFINED;
			//*tk++ = 

			if (a_esym != NULL)
				*a_esym = NULL;

			tok--;
		}
		else
		{
#if 0
			p = (char *)tok[1];
#else
			p = string[tok[1]];
#endif
			j = 0;

			if (*p == '.')
				j = curenv;

			sy = lookup(p, LABEL, j);

			if (sy == NULL)
				sy = NewSymbol(p, LABEL, j);

			sy->sattr |= REFERENCED;

			// Check register bank usage
			if (sy->sattre & EQUATEDREG)
			{
				if ((regbank == BANK_0) && (sy->sattre & BANK_1) && !altbankok)   
					warns("equated symbol \'%s\' cannot be used in register bank 0", sy->sname);

				if ((regbank == BANK_1) && (sy->sattre & BANK_0) && !altbankok)
					warns("equated symbol \'%s\' cannot be used in register bank 1", sy->sname);
			}

			*tk++ = SYMBOL;
#if 0
			*tk++ = (TOKEN)sy;
#else
			*tk++ = symbolNum;
			symbolPtr[symbolNum] = sy;
			symbolNum++;
#endif

			if (sy->sattr & DEFINED)
				*a_value = sy->svalue;
			else
				*a_value = 0;

			if (sy->sattre & EQUATEDREG) 
				*a_value &= 0x1F;

			*a_attr = (WORD)(sy->sattr & ~GLOBAL);

			if ((sy->sattr & (GLOBAL | DEFINED)) == GLOBAL && a_esym != NULL)
				*a_esym = sy;
		}

		tok += 2;
		*tk++ = ENDEXPR;
		return OK;
	}

	if (expr0() != OK)
		return ERROR;

	*tk++ = ENDEXPR;
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
