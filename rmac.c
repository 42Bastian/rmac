//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// RMAC.C - Main Application Code
// Copyright (C) 199x Landon Dyer, 2011 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source Utilised with the Kind Permission of Landon Dyer
//

#include "rmac.h"
#include "error.h"
#include "listing.h"
#include "procln.h"
#include "token.h"
#include "expr.h"
#include "sect.h"
#include "mark.h"
#include "macro.h"
#include "riscasm.h"
#include "direct.h"
#include "version.h"
#include "debug.h"
#include "symbol.h"
#include "object.h"

int perm_verb_flag;				// Permanently verbose, interactive mode
int list_flag;					// "-l" listing flag on command line
int verb_flag;					// Be verbose about what's going on
int as68_flag;					// as68 kludge mode
int glob_flag;					// Assume undefined symbols are global
int lsym_flag;					// Include local symbols in object file
int sbra_flag;					// Warn about possible short branches
int legacy_flag;				// Do stuff like insert code in RISC assembler
int obj_format;					// Object format flag
int debug;						// [1..9] Enable debugging levels
int err_flag;					// '-e' specified
int err_fd;						// File to write error messages to 
int rgpu, rdsp;					// Assembling Jaguar GPU or DSP code
int list_fd;					// File to write listing to
int regbank;					// RISC register bank
int segpadsize;					// Segment padding size
int endian;						// Host processor endianess
char * objfname;				// Object filename pointer
char * firstfname;				// First source filename
char * cmdlnexec;				// Executable name, pointer to ARGV[0]
char * searchpath;				// Search path for include files 
char defname[] = "noname.o";	// Default output filename


//
// Manipulate file extension.
//
// 'name' must be large enough to hold any possible filename. If 'stripp' is
// nonzero, any old extension is removed. If the file does not already have an
// extension, 'extension' is appended to the filename.
//
char * fext(char * name, char * extension, int stripp)
{
	char * s;

	// Find beginning of "real" name (strip off path)
	char * beg = strrchr(name, SLASHCHAR);

	if (beg == NULL)
		beg = name;

	// Clobber any old extension, if requested
	if (stripp)
	{
		for(s=beg; *s && *s!='.'; ++s) 
			;

		*s = '\0';
	}

	if (strrchr(beg, '.') == NULL)
		strcat(beg, extension);

	return name;
}


//
// Return 'item'nth element of semicolon-seperated pathnames specified in the
// enviroment string 's'. Copy the pathname to 'buf'.  Return 0 if the 'item'
// nth path doesn't exist.
// 
// ['item' ranges from 0 to N-1, where N = #elements in search path]
//
int nthpath(char * env_var, int itemno, char * buf)
{
	char * s = searchpath;

	if (s == NULL)
		s = getenv(env_var);

	if (s == NULL)
		return 0;

	while (itemno--)
		while (*s != EOS && *s++ != ';')
			;

	if (*s == EOS)
		return 0;

	while (*s != EOS && *s != ';')
		*buf++ = *s++;

	*buf++ = EOS;

	return 1;
}


//
// Display command line help
//
void DisplayHelp(void)
{
	printf("Usage:\n"
		"    %s [options] srcfile\n"
		"\n"
		"Options:\n"
		"  -? or -h          Display usage information\n"
		"  -dsymbol[=value]  Define symbol\n"
		"  -e[errorfile]     Send error messages to file, not stdout\n"
		"  -f[format]        Output object file format\n"
		"                    b: BSD (use this for Jaguar)\n"
		"  -i[path]          Directory to search for include files\n"
		"  -l[filename]      Create an output listing file\n"
		"  -n                Don't do things behind your back in RISC assembler\n"
		"  -o file           Output file name\n"
		"  -r[size]          Pad segments to boundary size specified\n"
		"                    w: word (2 bytes, default alignment)\n"
		"                    l: long (4 bytes)\n"
		"                    p: phrase (8 bytes)\n"
		"                    d: double phrase (16 bytes)\n"
		"                    q: quad phrase (32 bytes)\n"
		"  -s                Warn about possible short branches\n"
		"  -u                Force referenced and undefined symbols global\n"
		"  -v                Set verbose mode\n"
		"  -y[pagelen]       Set page line length (default: 61)\n"
		"\n", cmdlnexec);
}


//
// Display version information
//
void DisplayVersion(void)
{
	printf("\nReboot's Macro Assembler for Atari Jaguar\n"
		"Copyright (C) 199x Landon Dyer, 2011-2015 Reboot\n"
		"V%01i.%01i.%01i %s (%s)\n\n", MAJOR, MINOR, PATCH, __DATE__, PLATFORM);
}


// 
// Process command line arguments and do an assembly
//
int Process(int argc, char ** argv)
{
	int argno;						// Argument number
	SYM * sy;						// Pointer to a symbol record
	char * s;						// String pointer
	int fd;							// File descriptor
	char fnbuf[FNSIZ];				// Filename buffer 
	int i;							// Iterator

	errcnt = 0;						// Initialise error count
	listing = 0;					// Initialise listing level
	list_flag = 0;					// Initialise listing flag
	verb_flag = perm_verb_flag;		// Initialise verbose flag
	as68_flag = 0;					// Initialise as68 kludge mode
	glob_flag = 0;					// Initialise .globl flag
	sbra_flag = 0;					// Initialise short branch flag
	debug = 0;						// Initialise debug flag
	searchpath = NULL;				// Initialise search path
	objfname = NULL;				// Initialise object filename
	list_fname = NULL;				// Initialise listing filename
	err_fname = NULL;				// Initialise error filename
	obj_format = BSD;				// Initialise object format
	firstfname = NULL;				// Initialise first filename
	err_fd = ERROUT;				// Initialise error file descriptor
	err_flag = 0;					// Initialise error flag
	rgpu = 0;						// Initialise GPU assembly flag
	rdsp = 0;						// Initialise DSP assembly flag
	lsym_flag = 1;					// Include local symbols in object file
	regbank = BANK_N;				// No RISC register bank specified
	orgactive = 0;					// Not in RISC org section
	orgwarning = 0;					// No ORG warning issued
	segpadsize = 2;					// Initialise segment padding size

	// Initialise modules
	InitSymbolTable();				// Symbol table
	InitTokenizer();				// Tokenizer
	InitLineProcessor();			// Line processor
	InitExpression();				// Expression analyzer
	InitSection();					// Section manager / code generator
	InitMark();						// Mark tape-recorder
	InitMacro();					// Macro processor
	InitListing();					// Listing generator

	// Process command line arguments and assemble source files
	for(argno=0; argno<argc; ++argno)
	{
		if (*argv[argno] == '-')
		{
			switch (argv[argno][1])
			{
			case 'd':				// Define symbol
			case 'D':
				for(s=argv[argno]+2; *s!=EOS;)
				{
					if (*s++ == '=')
					{
						s[-1] = EOS;
						break;
					}
				}

				if (argv[argno][2] == EOS)
				{
					printf("-d: empty symbol\n");
					errcnt++;
					return errcnt;
				}

				sy = lookup(argv[argno] + 2, 0, 0);

				if (sy == NULL)
				{
					sy = NewSymbol(argv[argno] + 2, LABEL, 0);
					sy->svalue = 0;
				}

				sy->sattr = DEFINED | EQUATED | ABS;
				sy->svalue = (*s ? (VALUE)atoi(s) : 0);
				break;
			case 'e':				// Redirect error message output
			case 'E':
				err_fname = argv[argno] + 2;
				break;
			case 'f':				// -f<format>
			case 'F':
				switch (argv[argno][2])
				{
				case EOS:
				case 'b':			// -fb = BSD (Jaguar Recommended)
				case 'B':
					obj_format = BSD;
					break;
				default:
					printf("-f: unknown object format specified\n");
					errcnt++;
					return errcnt;
				}
				break;
			case 'g':				// Debugging flag
			case 'G':
				printf("Debugging flag (-g) not yet implemented\n");
				break;
			case 'i':				// Set directory search path
			case 'I':
				searchpath = argv[argno] + 2;
				break;
			case 'l':				// Produce listing file
			case 'L':
				list_fname = argv[argno] + 2;
				listing = 1;
				list_flag = 1;
				lnsave++;
				break;
			case 'o':				// Direct object file output
			case 'O':
				if (argv[argno][2] != EOS)
					objfname = argv[argno] + 2;
				else
				{
					if (++argno >= argc)
					{
						printf("Missing argument to -o");
						errcnt++;
						return errcnt;
					}
					objfname = argv[argno];
				}

				break;
			case 'r':				// Pad seg to requested boundary size
			case 'R':
				switch(argv[argno][2])
				{
				case 'w': case 'W': segpadsize = 2;  break;  
				case 'l': case 'L': segpadsize = 4;  break;
				case 'p': case 'P': segpadsize = 8;  break;
				case 'd': case 'D': segpadsize = 16; break;
				case 'q': case 'Q': segpadsize = 32; break;
				default: segpadsize = 2; break;	// Effective autoeven();
				}
				break;
			case 's':				// Warn about possible short branches
			case 'S':
				sbra_flag = 1;
				break;
			case 'u':				// Make undefined symbols .globl
			case 'U':
				glob_flag = 1;
				break;
			case 'v':				// Verbose flag
			case 'V':
				verb_flag++;

				if (verb_flag > 1)
					DisplayVersion();

				break;
			case 'x':				// Turn on debugging
			case 'X':
				debug = 1;
				printf("~ Debugging ON\n");
				break;
			case 'y':				// -y<pagelen>
			case 'Y':
				pagelen = atoi(argv[argno] + 2);

				if (pagelen < 10)
				{
					printf("-y: bad page length\n");
					++errcnt;
					return errcnt;
				}

				break;
			case EOS:				// Input is stdin
				if (firstfname == NULL)	// Kludge first filename
					firstfname = defname;

				include(0, "(stdin)");
				Assemble();
				break;
			case 'h':				// Display command line usage
			case 'H':
			case '?':
				DisplayVersion();
				DisplayHelp();
				errcnt++;
				break;
			case 'n':				// Turn off legacy mode
			case 'N':
				legacy_flag = 0;
				printf("Legacy mode OFF\n");
				break;
			default:
				DisplayVersion();
				printf("Unknown switch: %s\n\n", argv[argno]);
				DisplayHelp();
				errcnt++;
				break;
			}
		}
		else
		{
			// Record first filename.
			if (firstfname == NULL)
				firstfname = argv[argno];

			strcpy(fnbuf, argv[argno]);
			fext(fnbuf, ".s", 0);
			fd = open(fnbuf, 0);

			if (fd < 0)
			{
				printf("Cannot open: %s\n", fnbuf);
				errcnt++;
				continue;
			}

			include(fd, fnbuf);
			Assemble();
		}
	}

	// Wind-up processing;
	// o  save current section (no more code generation)
	// o  do auto-even of all sections (or boundary alignment as requested
	//    through '-r')
	// o  determine name of object file:
	//    -  "foo.o" for linkable output;
	//    -  "foo.prg" for GEMDOS executable (-p flag).
	SaveSection();

	for(i=TEXT; i<=BSS; i<<=1)
	{
		SwitchSection(i);

		switch(segpadsize)
		{
		case 2:  d_even();    break;
		case 4:  d_long();    break;
		case 8:  d_phrase();  break;
		case 16: d_dphrase(); break;
		case 32: d_qphrase(); break;
		}

		SaveSection();
	}

	if (objfname == NULL)
	{
		if (firstfname == NULL)
			firstfname = defname;

		strcpy(fnbuf, firstfname);
		//fext(fnbuf, prg_flag ? ".prg" : ".o", 1);
		fext(fnbuf, ".o", 1);
		objfname = fnbuf;
	}

	// With one pass finished, go back and:
	// (1)   run through all the fixups and resolve forward references;
	// (1.5) ensure that remaining fixups can be handled by the linker
	//       (`lo68' format, extended (postfix) format....)
	// (2)   generate the output file image and symbol table;
	// (3)   generate relocation information from left-over fixups.
	ResolveAllFixups();						// Do all fixups
	StopMark();								// Stop mark tape-recorder

	if (errcnt == 0)
	{
		if ((fd = open(objfname, _OPEN_FLAGS, _PERM_MODE)) < 0)
			cantcreat(objfname);

		if (verb_flag)
		{
			s = "object";
			printf("[Writing %s file: %s]\n", s, objfname);
		}

		WriteObject(fd);
		close(fd);

		if (errcnt != 0)
			unlink(objfname);
	}

	if (list_flag)
	{
		if (verb_flag)
			printf("[Wrapping-up listing file]\n");

		listing = 1;
		symtable();
		close(list_fd);
	}

	if (err_flag)
		close(err_fd);

	DEBUG dump_everything();

	return errcnt;
}


//
// Determine processor endianess
//
int GetEndianess(void)
{
	int i = 1;
	char * p = (char *)&i;

	if (p[0] == 1)
		return 0;

	return 1;
}


//
// Application entry point
//
int main(int argc, char ** argv)
{
	perm_verb_flag = 0;				// Clobber "permanent" verbose flag
	legacy_flag = 1;				// Default is legacy mode on (:-P)
	cmdlnexec = argv[0];			// Obtain executable name

	endian = GetEndianess();		// Get processor endianess

	// If commands were passed in, process them
	if (argc > 1)
		return Process(argc - 1, argv + 1);              

	DisplayVersion();
	DisplayHelp();

	return 0;
}

