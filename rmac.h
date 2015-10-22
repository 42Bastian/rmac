//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// RMAC.H - Main Application Code
// Copyright (C) 199x Landon Dyer, 2011 Reboot & Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source utilised with the kind permission of Landon Dyer
//

#ifndef __RMAC_H__
#define __RMAC_H__

//
// TARGET SPECIFIC BUILD SETTINGS
//
#if defined(WIN32) || defined (WIN64)
	// Release platform - windows
	#define PLATFORM        "Win32"
	#define _OPEN_FLAGS     _O_TRUNC|_O_CREAT|_O_BINARY|_O_RDWR
	#define _OPEN_INC       _O_RDONLY|_O_BINARY
	#define _PERM_MODE      _S_IREAD|_S_IWRITE 
	#ifdef _MSC_VER
		#if _MSC_VER > 1000
			#pragma warning(disable:4996)
		#endif

	// Makes warnings double clickable on visual studio (ggn)
	#define STRINGIZE_HELPER(x) #x
	#define STRINGIZE(x) STRINGIZE_HELPER(x)
	#define WARNING(desc) __pragma(message(__FILE__ "(" STRINGIZE(__LINE__) ") : Warning: " #desc))

	// usage:
	// WARNING(FIXME: Code removed because...)

	#else
	//If we're not compiling for Visual Studio let's assume that we're using
	//some flavour of gcc instead. So let's use the gcc compliant macro instead.
	//If some weirdo uses something else (I dunno, Intel compiler or something?)
	//this is probably going to explode spectacularly. Let's wait for the fireworks!
	#define DO_PRAGMA(x) _Pragma (#x)
	#define WARNING(desc) DO_PRAGMA(message (#desc))

	#endif
	#include <io.h>
	#include <fcntl.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <ctype.h>
	#include <sys/types.h>
	#include <sys/stat.h>

#else 
	#ifdef __GCCUNIX__
	// Release platform - mac OS-X or linux
	#define PLATFORM        "OSX/Linux"
	#define _OPEN_FLAGS     O_TRUNC|O_CREAT|O_RDWR
	#define _OPEN_INC       O_RDONLY
	#define _PERM_MODE      S_IREAD|S_IWRITE 
	// WARNING WARNING WARNING
	#define DO_PRAGMA(x) _Pragma (#x)
	#define WARNING(desc) DO_PRAGMA(message (#desc))
	#include <sys/fcntl.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <ctype.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <unistd.h>
#else
	// Release platform - not specified 
	#define PLATFORM        "Unknown"
	#define _OPEN_FLAGS     O_TRUNC|O_CREAT|O_RDWR
	#define _OPEN_INC       O_RDONLY
	#define _PERM_MODE      S_IREAD|S_IWRITE 
	// Defined here, even though the platfrom may not support it...
	#define DO_PRAGMA(x) _Pragma (#x)
	#define WARNING(desc) DO_PRAGMA(message (#desc))
	#include <sys/fcntl.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <ctype.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#endif
#endif

//
// Non-target specific stuff
//
#include <inttypes.h>
#include "symbol.h"

#define BYTE         uint8_t
#define WORD         uint16_t
#define LONG         uint32_t
#define VOID         void

#define ERROR        (-1)			// Generic error return
#define EOS          '\0'			// End of string
#define SPACE        ' '			// ASCII space 
#define SLASHCHAR    '/'
#define SLASHSTRING  "/"
#define VALUE        LONG			// Assembler value
#define TOKEN        LONG			// Assembler token
#define FNSIZ        128			// Maximum size of a filename
#define OK           0				// OK return
#define DEBUG        if (debug)		// Debug conditional
#define MAXARGV      100			// Maximum number of commandline args
#define STDOUT       1				// Standard output
#define ERROUT       2				// Error output
#define CREATMASK    0

// (Normally) non-printable tokens
#define COLON        ':'			// : (grumble: GNUmacs hates ':')
#define CONST        'a'			// CONST <value>
#define ACONST       'A'			// ACONST <value> <attrib>
#define STRING       'b'			// STRING <address>
#define SYMBOL       'c'			// SYMBOL <address>
#define EOL          'e'			// End of line
#define TKEOF        'f'			// End of file (or macro)
#define DEQUALS      'g'			// ==
#define SET          149			// set
#define REG          'R'			// reg
#define EQUREG       148			// equreg
#define CCDEF        183			// ccdef
#define DCOLON       'h'			// ::
#define GE           'i'			// >=
#define LE           'j'			// <=
#define NE           'k'			// <> or !=
#define SHR          'l'			// >>
#define SHL          'm'			// <<
#define UNMINUS      'n'			// Unary '-'
#define DOTB         'B'			// .b or .B or .s or .S
#define DOTW         'W'			// .w or .W
#define DOTL         'L'			// .l or .L
#define DOTI         'I'			// .i or .I
#define ENDEXPR      'E'			// End of expression

// Object code formats
#define ALCYON       0				// Alcyon/DRI C object format
#define MWC          1				// Mark Williams object format
#define BSD          2				// BSD object format

// Pointer type that can point to (almost) anything
#define PTR union _ptr
PTR
{
   char * cp;						// Char
   WORD * wp;						// WORD
   LONG * lp;						// LONG
   LONG lw;							// LONG
   SYM ** sy;						// SYM
   TOKEN * tk;						// TOKEN
};

// Symbol spaces
#define LABEL        0				// User-defined symbol
#define MACRO        1				// Macro definition
#define MACARG       2				// Macro argument
#define SY_UNDEF     -1				// Undefined (lookup never matches it)

// Symbol and expression attributes
#define DEFINED      0x8000			// Symbol has been defined
#define GLOBAL       0x4000			// Symbol has been .GLOBL'd
#define COMMON       0x2000			// Symbol has been .COMM'd
#define REFERENCED   0x1000			// Symbol has been referenced
#define EQUATED      0x0800			// Symbol was equated
#define SDECLLIST    0x0400			// Symbol is on 'sdecl'-order list

// Expression spaces, ORed with symbol and expression attributes above
#define ABS          0x0000			// In absolute space
#define TEXT         0x0001			// Relative to text
#define DATA         0x0002			// Relative to data
#define BSS          0x0004			// Relative to BSS
//#define M6502        0x0008		// 6502/microprocessor (absolute)
#define TDB          (TEXT|DATA|BSS)	// Mask for text+data+bss

// Sizes 
#define SIZB         0x0001			// .b 
#define SIZW         0x0002			// .w 
#define SIZL         0x0004			// .l 
#define SIZN         0x0008			// no .(size) specifier

// RISC register bank definitions (used in extended symbol attributes also)
#define BANK_N       0x0000			// No register bank specified
#define BANK_0       0x0001			// Register bank zero specified
#define BANK_1       0x0002			// Register bank one specified
#define EQUATEDREG   0x0008			// Equated register symbol
#define UNDEF_EQUR   0x0010
#define EQUATEDCC    0x0020
#define UNDEF_CC     0x0040

//#define RISCSYM      0x00010000

// Globals, externals, etc.
extern int verb_flag;
extern int debug;
extern int rgpu, rdsp;
extern int err_flag;
extern int err_fd;
extern int regbank;
extern char * firstfname;
extern int list_fd;
extern int as68_flag;
extern int list_flag;
extern int glob_flag;
extern int lsym_flag;
extern int sbra_flag;
extern int obj_format;
extern int legacy_flag;

// Exported functions
char * fext(char *, char *, int);
int nthpath(char *, int, char *);

#endif // __RMAC_H__

