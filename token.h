//
// RMAC - Reboot's Macro Assembler for all Atari computers
// TOKEN.H - Token Handling
// Copyright (C) 199x Landon Dyer, 2011-2017 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source utilised with the kind permission of Landon Dyer
//

#ifndef __TOKEN_H__
#define __TOKEN_H__

#include "rmac.h"

// Include Files and Macros
#define SRC_IFILE       0			// Input source is IFILE
#define SRC_IMACRO      1			// Input source is IMACRO
#define SRC_IREPT       2			// Input source is IREPT

// Macros
#define INOBJ           struct _inobj
#define IUNION          union _iunion
#define IFILE           struct _incldfile
#define IMACRO          struct _imacro
#define IREPT           struct _irept
#define IFENT           struct _ifent

// Tunable definitions
#define LNSIZ           1024		// Maximum size of a line of text
#define TOKBUFSIZE      400			// Size of token-line buffer
#define QUANTUM         4096L		// # bytes to eat at a time from a file
#define LNBUFSIZ        (QUANTUM*2)	// Size of file's buffer
#define KWSIZE          7			// Maximum size of keyword in kwtab.h

// (Normally) non-printable tokens
#define COLON           ':'			// : (grumble: GNUmacs hates ':')
#define CONST           'a'			// CONST <value>
#define FCONST          'r'			// Floating CONST <value>
#define ACONST          'A'			// ACONST <value> <attrib>
#define STRING          'b'			// STRING <address>
#define STRINGA8        'S'			// Atari 800 internal STRING <address>
#define SYMBOL          'c'			// SYMBOL <address>
#define EOL             'e'			// End of line
#define TKEOF           'f'			// End of file (or macro)
#define DEQUALS         'g'			// ==
#define SET             0x95		// Set
#define REG             'R'			// Reg
#define EQUREG          0x94		// equreg
#define CCDEF           0xB7		// ccdef
#define DCOLON          'h'			// ::
#define GE              'i'			// >=
#define LE              'j'			// <=
#define NE              'k'			// <> or !=
#define SHR             'l'			// >>
#define SHL             'm'			// <<
#define UNMINUS         'n'			// Unary '-'
#define DOTB            'B'			// .b or .B or .s or .S
#define DOTW            'W'			// .w or .W
#define DOTL            'L'			// .l or .L
#define DOTI            'I'			// .l or .L
#define DOTX            'X'			// .x or .X
#define DOTD            'D'			// .d or .D
#define DOTP            'P'			// .p or .P
#define DOTQ            'Q'			// .q or .Q (essentially an alias for P)
#define DOTS            'S'			// .s or .S (FPU Single)
#define ENDEXPR         'E'			// End of expression

// ^^ operators
#define CR_DEFINED      'p'			// ^^defined - is symbol defined?
#define CR_REFERENCED   'q'			// ^^referenced - was symbol referenced?
#define CR_STREQ        'v'			// ^^streq - compare two strings
#define CR_MACDEF       'w'			// ^^macdef - is macro defined?
#define CR_TIME         'x'			// ^^time - DOS format time
#define CR_DATE         'y'			// ^^date - DOS format date
#define CR_ABSCOUNT     'z'			// ^^abscount - count the number of bytes
									// defined in curent .abs section

// Character Attributes
#define ILLEG           0			// Illegal character (unused)
#define DIGIT           1			// 0-9
#define HDIGIT          2			// A-F, a-f
#define STSYM           4			// A-Z, a-z, _~.
#define CTSYM           8			// A-Z, a-z, 0-9, _~$?
#define SELF            16			// Single-character tokens: ( ) [ ] etc
#define WHITE           32			// Whitespace (space, tab, etc.)
#define MULTX           64			// Multiple-character tokens
#define DOT             128			// [bwlsBWSL] for what follows a '.'

// Macro to check for specific optimizations or override
#define CHECK_OPTS(x)	(optim_flags[x] && !optimizeOff)

// Conditional assembly structures
IFENT {
	IFENT * if_prev;		// Ptr prev .if state block (or NULL)
	WORD if_state;			// 0:enabled, 1:disabled
};

// Pointer to IFILE or IMACRO or IREPT
IUNION {
	IFILE * ifile;
	IMACRO * imacro;
	IREPT * irept;
};

// Ptr to IFILEs, IMACROs, and so on; back-ptr to previous input objects
INOBJ {
	WORD in_type;			// 0=IFILE, 1=IMACRO, 2=IREPT
	IFENT * in_ifent;		// Pointer to .if context on entry
	INOBJ * in_link;		// Pointer to previous INOBJ
	TOKEN * in_otok;		// Old `tok' value
	TOKEN * in_etok;		// Old `etok' value
	IUNION inobj;			// IFILE or IMACRO or IREPT
};

// Information about a file
IFILE {
	IFILE * if_link;		// Pointer to ancient IFILEs
	char * ifoldfname;		// Old file's name
	int ifoldlineno;		// Old line number
	int ifind;				// Position in file buffer
	int ifcnt;				// #chars left in file buffer
	int ifhandle;			// File's descriptor
	WORD ifno;				// File number
	char ifbuf[LNBUFSIZ];	// Line buffer
};

#define TOKENSTREAM struct _tokenstream
TOKENSTREAM {
	TOKEN token[32];		// 32 ought to be enough for anybody (including XiA!)
	char * string[32];		// same for attached strings
};

// Information about a macro invocation
IMACRO {
	IMACRO * im_link;		// Pointer to ancient IMACROs
	LLIST * im_nextln;		// Next line to include
	WORD im_nargs;			// # of arguments supplied on invocation
	WORD im_siz;			// Size suffix supplied on invocation
	LONG im_olduniq;		// Old value of 'macuniq'
	SYM * im_macro;			// Pointer to macro we're in
	char im_lnbuf[LNSIZ];	// Line buffer
	TOKENSTREAM argument[20];	// Assume no more than 20 arguments in an invocation
};

// Information about a .rept invocation
IREPT {
	LLIST * ir_firstln;		// Pointer to first line
	LLIST * ir_nextln;		// Pointer to next line
	uint32_t ir_count;		// Repeat count (decrements)
};

// Exported variables
extern int lnsave;
extern uint16_t curlineno;
extern char * curfname;
extern WORD cfileno;
extern TOKEN * tok;
extern char lnbuf[];
extern char lntag;
extern char tolowertab[];
extern INOBJ * cur_inobj;
extern int mjump_align;
extern char * string[];
int optimizeOff;

// Exported functions
int include(int, char *);
void InitTokenizer(void);
void SetFilenameForErrorReporting(void);
int TokenizeLine(void);
int fpop(void);
int d_goto(WORD);
INOBJ * a_inobj(int);
void DumpToken(TOKEN);
void DumpTokenBuffer(void);

#endif // __TOKEN_H__

