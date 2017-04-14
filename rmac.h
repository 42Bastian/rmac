//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// RMAC.H - Main Application Code
// Copyright (C) 199x Landon Dyer, 2017 Reboot & Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source utilised with the kind permission of Landon Dyer
//

#ifndef __RMAC_H__
#define __RMAC_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

//
// TARGET SPECIFIC BUILD SETTINGS
//
#if defined(WIN32) || defined (WIN64)
	#include <io.h>
	#include <fcntl.h>
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
	#define inline __inline

	// usage:
	// WARNING(FIXME: Code removed because...)

	#else
	// If we're not compiling for Visual Studio let's assume that we're using
	// some flavour of gcc instead. So let's use the gcc compliant macro
	// instead. If some weirdo uses something else (I dunno, Intel compiler or
	// something?) this is probably going to explode spectacularly. Let's wait
	// for the fireworks!
	#define DO_PRAGMA(x) _Pragma (#x)
	#define WARNING(desc) DO_PRAGMA(message (#desc))

	#endif

#else

	#ifdef __GCCUNIX__
	#ifdef __MINGW32__
	#define off64_t long
	#define off_t long
	#endif

	#include <sys/fcntl.h>
	#include <unistd.h>
	// Release platform - mac OS-X or Linux
	#define PLATFORM        "OSX/Linux"
	#define _OPEN_FLAGS     O_TRUNC|O_CREAT|O_RDWR
	#define _OPEN_INC       O_RDONLY
	#define _PERM_MODE      S_IRUSR|S_IWUSR
	// WARNING WARNING WARNING
	#define DO_PRAGMA(x) _Pragma (#x)
	#define WARNING(desc) DO_PRAGMA(message (#desc))
#else
	// Release platform - not specified
	#include <sys/fcntl.h>
	#define PLATFORM        "Unknown"
	#define _OPEN_FLAGS     O_TRUNC|O_CREAT|O_RDWR
	#define _OPEN_INC       O_RDONLY
	#define _PERM_MODE      S_IREAD|S_IWRITE
	// Defined here, even though the platform may not support it...
	#define DO_PRAGMA(x) _Pragma (#x)
	#define WARNING(desc) DO_PRAGMA(message (#desc))
	#endif
#endif

//
// Endian related, for safe handling of endian-sensitive data
// USAGE: GETBExx() is *always* an rvalue, a = pointer to a uint8_t,
//        r = offset from 0. SETBExx(), v = value to write into 'a'
//
#define GETBE16(a, r) \
	(((uint16_t)(a)[(r + 0)] << 8) | ((uint16_t)(a)[(r + 1)]))

#define GETBE32(a, r) \
	(((uint32_t)(a)[(r + 0)] << 24) | ((uint32_t)(a)[(r + 1)] << 16) \
	| ((uint32_t)(a)[(r + 2)] << 8) | ((uint32_t)(a)[(r + 3)]))

#define GETBE64(a, r) \
	(((uint64_t)(a)[(r + 0)] << 56) | ((uint64_t)(a)[(r + 1)] << 48) \
	| ((uint64_t)(a)[(r + 2)] << 40) | ((uint64_t)(a)[(r + 3)] << 32) \
	| ((uint64_t)(a)[(r + 4)] << 24) | ((uint64_t)(a)[(r + 5)] << 16) \
	| ((uint64_t)(a)[(r + 6)] << 8) | ((uint64_t)(a)[(r + 7)]))

#define SETBE16(a, r, v) \
	{ (a)[(r + 0)] = (uint8_t)((v) >> 8); \
	(a)[(r + 1)] = (uint8_t)((v) & 0xFF); }

#define SETBE32(a, r, v) \
	{ (a)[(r + 0)] = (uint8_t)((v) >> 24); \
	(a)[(r + 1)] = (uint8_t)(((v) >> 16) & 0xFF); \
	(a)[(r + 2)] = (uint8_t)(((v) >> 8) & 0xFF); \
	(a)[(r + 3)] = (uint8_t)((v) & 0xFF); }

#define SETBE64(a, r, v) \
	{ (a)[(r + 0)] = (uint8_t)((v) >> 56); \
	(a)[(r + 1)] = (uint8_t)(((v) >> 48) & 0xFF); \
	(a)[(r + 2)] = (uint8_t)(((v) >> 40) & 0xFF); \
	(a)[(r + 3)] = (uint8_t)(((v) >> 32) & 0xFF); \
	(a)[(r + 4)] = (uint8_t)(((v) >> 24) & 0xFF); \
	(a)[(r + 5)] = (uint8_t)(((v) >> 16) & 0xFF); \
	(a)[(r + 6)] = (uint8_t)(((v) >> 8) & 0xFF); \
	(a)[(r + 7)] = (uint8_t)((v) & 0xFF); }

// Byteswap crap
#define BYTESWAP16(x) ((((x) & 0x00FF) << 8) | (((x) & 0xFF00) >> 8))
#define BYTESWAP32(x) ((((x) & 0x000000FF) << 24) | (((x) & 0x0000FF00) << 8) | (((x) & 0x00FF0000) >> 8) | (((x) & 0xFF000000) >> 24))
#define WORDSWAP32(x) ((((x) & 0x0000FFFF) << 16) | (((x) & 0xFFFF0000) >> 16))

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
#define ELF          3				// ELF object format

// Pointer type that can point to (almost) anything
#define PTR union _ptr
PTR
{
   uint8_t * cp;					// Char
   uint16_t * wp;					// WORD
   uint32_t * lp;					// LONG
   uint32_t lw;						// LONG
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

// Optimisation defines
enum
{
	OPT_ABS_SHORT     = 0,
	OPT_MOVEL_MOVEQ   = 1,
	OPT_BSR_BCC_S     = 2,
	OPT_INDIRECT_DISP = 3,
	OPT_COUNT   // Dummy, used to count number of optimisation switches
};

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
extern int prg_flag;	// 1 = write ".PRG" relocatable executable
extern LONG PRGFLAGS;
extern int optim_flags[OPT_COUNT];

// Exported functions
char * fext(char *, char *, int);
int nthpath(char *, int, char *);
int ParseOptimization(char * optstring);

#endif // __RMAC_H__

