//
// 6502 Assembler
//
//    Init6502	initialization
//    d_6502    handle ".6502" directive
//    m6502cg	generate code for a 6502 mnemonic
//    d_org	handle 6502 section's ".org" directive
//    m6502obj	generate 6502 object file
//
#include "direct.h"
#include "expr.h"
#include "error.h"
#include "mach.h"
#include "procln.h"
#include "riscasm.h"
#include "rmac.h"
#include "sect.h"
#include "token.h"

#define	UPSEG_SIZE	0x10010L // size of 6502 code buffer, 64K+16bytes

// Internal vars
static uint16_t orgmap[1024][2];		// Mark all 6502 org changes

// Exported vars
const char in_6502mode[] = "directive illegal in .6502 section";
uint16_t * currentorg = &orgmap[0][0];	// Current org range
char strtoa8[128];	// ASCII to Atari 800 internal conversion table

//
// 6502 addressing modes;
// DO NOT CHANGE THESE VALUES.
//
#define	A65_ABS		0
#define	A65_ABSX	1
#define	A65_ABSY	2
#define	A65_IMPL	3
#define	A65_IMMED	4
#define	A65_INDX	5
#define	A65_INDY	6
#define	A65_IND		7
#define	A65_REL		8
#define	A65_ZP		9
#define	A65_ZPX		10
#define	A65_ZPY		11
#define A65_IMMEDH  12
#define A65_IMMEDL  13

#define	NMACHOPS 56		// Number of machine ops
#define	NMODES	14		// Number of addressing modes
#define	NOP	0xEA		// 6502 NOP instruction
#define	ILLEGAL	0xFF	// 'Illegal instr' marker
#define END65	0xFF	// End-of-an-instr-list

static char imodes[] =
{
	A65_IMMED, 0x69, A65_ABS, 0x6d, A65_ZP, 0x65, A65_INDX, 0x61, A65_INDY, 0x71,
	A65_ZPX, 0x75, A65_ABSX, 0x7d, A65_ABSY, 0x79, END65,
	A65_IMMED, 0x29, A65_ABS, 0x2d, A65_ZP, 0x25, A65_INDX, 0x21, A65_INDY, 0x31,
	A65_ZPX, 0x35, A65_ABSX, 0x3d, A65_ABSY, 0x39, END65,
	A65_ABS, 0x0e, A65_ZP, 0x06, A65_IMPL, 0x0a, A65_ZPX, 0x16, A65_ABSX,
	0x1e, END65,
	A65_REL, 0x90, END65,
	A65_REL, 0xb0, END65,
	A65_REL, 0xf0, END65,
	A65_REL, 0xd0, END65,
	A65_REL, 0x30, END65,
	A65_REL, 0x10, END65,
	A65_REL, 0x50, END65,
	A65_REL, 0x70, END65,
	A65_ABS, 0x2c, A65_ZP, 0x24, END65,
	A65_IMPL, 0x00, END65,
	A65_IMPL, 0x18, END65,
	A65_IMPL, 0xd8, END65,
	A65_IMPL, 0x58, END65,
	A65_IMPL, 0xb8, END65,
	A65_IMMED, 0xc9, A65_ABS, 0xcd, A65_ZP, 0xc5, A65_INDX, 0xc1, A65_INDY, 0xd1,
	A65_ZPX, 0xd5, A65_ABSX, 0xdd, A65_ABSY, 0xd9, END65,
	A65_IMMED, 0xe0, A65_ABS, 0xec, A65_ZP, 0xe4, END65,
	A65_IMMED, 0xc0, A65_ABS, 0xcc, A65_ZP, 0xc4, END65,
	A65_ABS, 0xce, A65_ZP, 0xc6, A65_ZPX, 0xd6, A65_ABSX, 0xde, END65,
	A65_IMPL, 0xca, END65,
	A65_IMPL, 0x88, END65,
	A65_IMMED, 0x49, A65_ABS, 0x4d, A65_ZP, 0x45, A65_INDX, 0x41, A65_INDY, 0x51,
	A65_ZPX, 0x55, A65_ABSX, 0x5d, A65_ABSY, 0x59, END65,
	A65_ABS, 0xee, A65_ZP, 0xe6, A65_ZPX, 0xf6, A65_ABSX, 0xfe, END65,
	A65_IMPL, 0xe8, END65,
	A65_IMPL, 0xc8, END65,
	A65_ABS, 0x4c, A65_IND, 0x6c, END65,
	A65_ABS, 0x20, END65,
	A65_IMMED, 0xa9, A65_ABS, 0xad, A65_ZP, 0xa5, A65_INDX, 0xa1, A65_INDY, 0xb1,
	A65_ZPX, 0xb5, A65_ABSX, 0xbd, A65_ABSY, 0xb9, A65_IMMEDH, 0xa9, A65_IMMEDL, 0xa9, END65,
	A65_IMMED, 0xa2, A65_ABS, 0xae, A65_ZP, 0xa6, A65_ABSY, 0xbe,
	A65_ZPY, 0xb6, A65_IMMEDH, 0xa2, A65_IMMEDL, 0xa2, END65,
	A65_IMMED, 0xa0, A65_ABS, 0xac, A65_ZP, 0xa4, A65_ZPX, 0xb4,
	A65_ABSX, 0xbc, A65_IMMEDH, 0xa0, A65_IMMEDL, 0xa0, END65,
	A65_ABS, 0x4e, A65_ZP, 0x46, A65_IMPL, 0x4a, A65_ZPX, 0x56,
	A65_ABSX, 0x5e, END65,
	A65_IMPL, 0xea, END65,
	A65_IMMED, 0x09, A65_ABS, 0x0d, A65_ZP, 0x05, A65_INDX, 0x01, A65_INDY, 0x11,
	A65_ZPX, 0x15, A65_ABSX, 0x1d, A65_ABSY, 0x19, END65,
	A65_IMPL, 0x48, END65,
	A65_IMPL, 0x08, END65,
	A65_IMPL, 0x68, END65,
	A65_IMPL, 0x28, END65,
	A65_ABS, 0x2e, A65_ZP, 0x26, A65_IMPL, 0x2a, A65_ZPX, 0x36,
	A65_ABSX, 0x3e, END65,
	A65_ABS, 0x6e, A65_ZP, 0x66, A65_IMPL, 0x6a, A65_ZPX, 0x76,
	A65_ABSX, 0x7e, END65,
	A65_IMPL, 0x40, END65,
	A65_IMPL, 0x60, END65,
	A65_IMMED, 0xe9, A65_ABS, 0xed, A65_ZP, 0xe5, A65_INDX, 0xe1, A65_INDY, 0xf1,
	A65_ZPX, 0xf5, A65_ABSX, 0xfd, A65_ABSY, 0xf9, END65,
	A65_IMPL, 0x38, END65,
	A65_IMPL, 0xf8, END65,
	A65_IMPL, 0x78, END65,
	A65_ABS, 0x8d, A65_ZP, 0x85, A65_INDX, 0x81, A65_INDY, 0x91, A65_ZPX, 0x95,
	A65_ABSX, 0x9d, A65_ABSY, 0x99, END65,
	A65_ABS, 0x8e, A65_ZP, 0x86, A65_ZPY, 0x96, END65,
	A65_ABS, 0x8c, A65_ZP, 0x84, A65_ZPX, 0x94, END65,
	A65_IMPL, 0xaa, END65,
	A65_IMPL, 0xa8, END65,
	A65_IMPL, 0xba, END65,
	A65_IMPL, 0x8a, END65,
	A65_IMPL, 0x9a, END65,
	A65_IMPL, 0x98, END65
};

static char ops[NMACHOPS][NMODES];			// Opcodes
static unsigned char inf[NMACHOPS][NMODES];	// Construction info

// Absolute-to-zeropage translation table
static int abs2zp[] =
{
	A65_ZP,		// ABS
	A65_ZPX,	// ABSX
	A65_ZPY,	// ABSY
	-1,			// IMPL
	-1,			// IMMED
	-1,			// INDX
	-1,			// INDY
	-1,			// IND
	-1,			// REL
	-1,			// ZP
	-1,			// ZPX
	-1			// ZPY
};

static char a8internal[] =
{
    ' ', 0,   '!', 1,   '"', 2,   '#', 3,   '$',  4,   '%', 5,   '&', 6,   '\'', 7,
    '(', 8,   ')', 9,   '*', 10,  '+', 11,  ',',  12,  '-', 13,  '.', 14,  '/',  15,
    '0', 16,  '1', 17,  '2', 18,  '3', 19,  '4',  20,  '5', 21,  '6', 22,  '7',  23,
    '8', 24,  '9', 25,  ':', 26,  ';', 27,  '<',  28,  '=', 29,  '>', 30,  '?',  31,
    '@', 32,  'A', 33,  'B', 34,  'C', 35,  'D',  36,  'E', 37,  'F', 38,  'G',  39,
    'H', 40,  'I', 41,  'J', 42,  'K', 43,  'L',  44,  'M', 45,  'N', 46,  'O',  47,
    'P', 48,  'Q', 49,  'R', 50,  'S', 51,  'T',  52,  'U', 53,  'V', 54,  'W',  55,
    'X', 56,  'Y', 57,  'Z', 58,  '[', 59,  '\\', 60,  ']', 61,  '^', 62,  '_',  63,
    'a', 97,  'b', 98,  'c', 99,  'd', 100, 'e',  101, 'f', 102, 'g', 103, 'h',  104,
    'i', 105, 'j', 106, 'k', 107, 'l', 108, 'm',  109, 'n', 110, 'o', 111, 'p',  112,
    'q', 113, 'r', 114, 's', 115, 't', 116, 'u',  117, 'v', 118, 'w', 119, 'x',  120,
    'y', 121, 'z', 122
};


//
// Initialize 6502 assembler
//
void Init6502()
{
	register int i;
	register int j;

	register char * s = imodes;

	// Set all instruction slots to illegal
	for(i=0; i<NMACHOPS; i++)
		for(j=0; j<NMODES; j++)
			inf[i][j] = ILLEGAL;

	// Uncompress legal instructions into their slots
	for(i=0; i<NMACHOPS; i++)
	{
		do
		{
			j = *s & 0xFF;
			inf[i][j] = *s;
			ops[i][j] = s[1];

			/* hack A65_REL mode */
			if (*s == A65_REL)
			{
				inf[i][A65_ABS] = A65_REL;
				ops[i][A65_ABS] = s[1];
				inf[i][A65_ZP] = A65_REL;
				ops[i][A65_ZP] = s[1];
			}
		}
		while (*(s += 2) != (char)END65);

		s++;
	}

	// Set up first org section (set to zero)
	orgmap[0][0] = 0;

	SwitchSection(M6502);	// Switch to 6502 section

	// Initialise string conversion table(s)
	char * p = a8internal;
	memset(strtoa8, 31, 128);   // 31=fallback value ("?")

	for(; p<a8internal+sizeof(a8internal); p+=2)
		strtoa8[p[0]] = p[1];

	if (challoc == 0)
	{
		// Allocate and clear 64K of space for the 6502 section
		chcheck(UPSEG_SIZE);
		memset(sect[M6502].scode->chptr, 0, UPSEG_SIZE);
	}

	SwitchSection(TEXT);    // Go back to TEXT
}


//
// .6502 --- enter 6502 mode
//
int d_6502()
{
	SaveSection();			// Save curent section
	SwitchSection(M6502);	// Switch to 6502 section

	return 0;
}


//
// Do 6502 code generation
//
void m6502cg(int op)
{
	register int amode;		// (Parsed) addressing mode
	register int i;
	VALUE eval;				// Expression value
	WORD eattr;				// Expression attributes
	int zpreq;				// 1, optimize instr to zero-page form
	register char * p;		// (Temp) string usage
	ch_size = 0;			// Reset chunk size on every instruction

	//
	// Parse 6502 addressing mode
	//
	zpreq = 0;

	switch ((int)*tok)
	{
	case EOL:
		amode = A65_IMPL;
		break;

	case '#':
		tok++;

		if (*tok == '>')
		{
			tok++;
			if (expr(exprbuf, &eval, &eattr, NULL) < 0)
				return;

			amode = A65_IMMEDH;
			break;
		}
		else if (*tok == '<')
		{
			tok++;
			if (expr(exprbuf, &eval, &eattr, NULL) < 0)
				return;

			amode = A65_IMMEDL;
			break;
		}

		if (expr(exprbuf, &eval, &eattr, NULL) < 0)
			return;

		amode = A65_IMMED;
		break;

	case '(':
		tok++;

		if (expr(exprbuf, &eval, &eattr, NULL) < 0)
			return;

		if (*tok == ')')
		{
			// (foo) or (foo),y
			if (*++tok == ',')
			{
				// (foo),y
				tok++;
				p = string[tok[1]];

				if (*tok != SYMBOL || p[1] != EOS || (*p | 0x20) != 'y') // Sleazo tolower()
					goto badmode;

				tok += 2;
				amode = A65_INDY;
			}
			else
				amode = A65_IND;
		}
		else if (*tok == ',')
		{
			// (foo,x)
			tok++;
			p = string[tok[1]];

			if (*tok != SYMBOL || p[1] != EOS || (*p | 0x20) != 'x') // Sleazo tolower()
				goto badmode;

			tok += 2;

			if (*tok++ != ')')
				goto badmode;

			amode = A65_INDX;
		}
		else
			goto badmode;

		break;

	case '@':
		tok++;

		if (expr(exprbuf, &eval, &eattr, NULL) < 0)
			return;

		if (*tok == '(')
		{
			tok++;
			p = string[tok[1]];

			if (*tok != SYMBOL || p[1] != EOS || tok[2] != ')' || tok[3] != EOL)
				goto badmode;

			i = (*p | 0x20);	// Sleazo tolower()

			if (i == 'x')
				amode = A65_INDX;
			else if (i == 'y')
				amode = A65_INDY;
			else
				goto badmode;

			tok += 3;		// Past SYMBOL <string> ')' EOL
			zpreq = 1;		// Request zeropage optimization
		}
		else if (*tok == EOL)
			amode = A65_IND;
		else
			goto badmode;

		break;

	default:
		//
		// Short-circuit
		//   x,foo
		//   y,foo
		//
		p = string[tok[1]];
		// ggn: the following code is effectively disabled as it would make
		//      single letter labels not work correctly (would not identify the
		//      label properly). And from what I understand it's something to
		//      keep compatibility with the coinop assembler which is probably
		//      something we don't care about much :D
#if 0
		if (*tok == SYMBOL && p[1] == EOS && tok[2] == ',')
		{
			tok += 3;		// Past: SYMBOL <string> ','
			i = (*p | 0x20);

			if (i == 'x')
				amode = A65_ABSX;
			else if (i == 'y')
				amode = A65_ABSY;
			else
				goto not_coinop;

			if (expr(exprbuf, &eval, &eattr, NULL) < 0)
				return;

			if (*tok != EOL)
				goto badmode;

			zpreq = 1;
			break;
		}

not_coinop:
#endif
		if (expr(exprbuf, &eval, &eattr, NULL) < 0)
			return;

		zpreq = 1;

		if (*tok == EOL)
			amode = A65_ABS;
		else if (*tok == ',')
		{
			tok++;
			p = string[tok[1]];

			if (*tok != SYMBOL || p[1] != EOS)
				goto badmode;

			tok += 2;

			//
			// Check for X or Y index register;
			// the OR with 0x20 is a sleazo conversion
			// to lower-case that actually works.
			//
			i = *p | 0x20;	// Oooh, this is slimey (but fast!)

			if (i == 'x')
				amode = A65_ABSX;
			else if (i == 'y')
				amode = A65_ABSY;
			else
				goto badmode;
		}
		else
			goto badmode;

		break;

badmode:
		error("bad 6502 addressing mode");
		return;
	}

	//
	// Optimize ABS modes to zero-page when possible
	//   o  ZPX or ZPY is illegal, or
	//   o  expr is zeropage && zeropageRequest && expression is defined
	//
	if (inf[op][amode] == ILLEGAL	// If current op is illegal,
		|| (eval < 0x100			// or expr must be zero-page
		&& zpreq != 0				// amode must request zero-page opt.
		&& (eattr & DEFINED)))		// and the expression must be defined
	{
		i = abs2zp[amode];			// i = zero-page translation of amode
#ifdef DO_DEBUG
		DEBUG printf(" OPT: op=%d amode=%d i=%d inf[op][i]=%d\n",
					 op, amode, i, inf[op][i]);
#endif
		if (i >= 0 && (inf[op][i] & 0xFF) != ILLEGAL) // Use it if it's legal
			amode = i;
	}

#ifdef DO_DEBUG
	DEBUG printf("6502: op=%d amode=%d ", op, amode);
	DEBUG printf("inf[op][amode]=%d\n", (int)inf[op][amode]);
#endif

	switch (inf[op][amode])
	{
		case A65_IMPL:		// Just leave the instruction
			D_byte(ops[op][amode]);
			break;

		case A65_IMMEDH:
			D_byte(ops[op][amode]);

			if (!(eattr & DEFINED))
			{
				AddFixup(FU_BYTEH, sloc, exprbuf);
				eval = 0;
			}

			eval = (eval >> 8) & 0xFF; // Bring high byte to low
			D_byte(eval);				// Deposit byte following instr
			break;

		case A65_IMMEDL:
			D_byte(ops[op][amode]);

			if (!(eattr & DEFINED))
			{
				AddFixup(FU_BYTEL, sloc, exprbuf);
				eval = 0;
			}

			eval = eval & 0xFF; // Mask high byte
			D_byte(eval);		// Deposit byte following instr
			break;

		case A65_IMMED:
		case A65_INDX:
		case A65_INDY:
		case A65_ZP:
		case A65_ZPX:
		case A65_ZPY:
			D_byte(ops[op][amode]);

			if (!(eattr & DEFINED))
			{
				AddFixup(FU_BYTE, sloc, exprbuf);
				eval = 0;
			}
			else if (eval + 0x100 >= 0x200)
			{
				error(range_error);
				eval = 0;
			}

			D_byte(eval);		// Deposit byte following instr
			break;

		case A65_REL:
			D_byte(ops[op][amode]);

			if (eattr & DEFINED)
			{
				eval -= (sloc + 1);

				if (eval + 0x80 >= 0x100)
				{
					error(range_error);
					eval = 0;
				}

				D_byte(eval);
			}
			else
			{
				AddFixup(FU_6BRA, sloc, exprbuf);
				D_byte(0);
			}

			break;

		case A65_ABS:
		case A65_ABSX:
		case A65_ABSY:
		case A65_IND:
			D_byte(ops[op][amode]);

			if (!(eattr & DEFINED))
			{
				AddFixup(FU_WORD, sloc, exprbuf);
				eval = 0;
			}

			D_rword(eval);
			break;

			//
			// Deposit 3 NOPs for illegal things
			//
		default:
		case ILLEGAL:
			for(i=0; i<3; i++)
				D_byte(NOP);

			error("illegal 6502 addressing mode");
	}

	// Check for overflow of code region
	if (sloc > 0x10000L)
		fatal("6502 code pointer > 64K");

	if (*tok != EOL)
		error(extra_stuff);
}


//
// Generate 6502 object output file.
//
// ggn: converted into a com/exe/xex output format
void m6502obj(int ofd)
{
	uint16_t exeheader[3];
	int headsize = 6;
	uint16_t * headpoint = exeheader;

	CHUNK * ch = sect[M6502].scode;

	// If no 6502 code was generated, forget it
	if ((ch == NULL) || (ch->challoc == 0))
		return;

	exeheader[0] = 0xFFFF;		// Mandatory for first segment
	register uint8_t * p = ch->chptr;

	for(uint16_t * l=&orgmap[0][0]; l<currentorg; l+=2)
	{
		exeheader[1] = l[0];
		exeheader[2] = l[1] - 1;

		// Write header
		size_t unused = write(ofd, headpoint, headsize);
		unused = write(ofd, p + l[0], l[1] - l[0]);

		// Skip the $FFFF after first segment, it's not mandatory
		headpoint = &exeheader[1];
		headsize = 4;
	}
}

