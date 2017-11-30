//
// RMAC - Reboot's Macro Assembler for all Atari computers
// RMAC.C - Main Application Code
// Copyright (C) 199x Landon Dyer, 2011-2017 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source utilised with the kind permission of Landon Dyer
//

#include "rmac.h"
#include "6502.h"
#include "debug.h"
#include "direct.h"
#include "error.h"
#include "expr.h"
#include "listing.h"
#include "mark.h"
#include "macro.h"
#include "object.h"
#include "procln.h"
#include "riscasm.h"
#include "sect.h"
#include "symbol.h"
#include "token.h"
#include "version.h"

int perm_verb_flag;				// Permanently verbose, interactive mode
int list_flag;					// "-l" listing flag on command line
int list_pag = 1;				// Enable listing pagination by default
int verb_flag;					// Be verbose about what's going on
int m6502;						// 1, assembling 6502 code
int as68_flag;					// as68 kludge mode
int glob_flag;					// Assume undefined symbols are global
int lsym_flag;					// Include local symbols in object file
int sbra_flag;					// Warn about possible short branches
int prg_flag;					// !=0, produce .PRG executable (2=symbols)
int legacy_flag;				// Do stuff like insert code in RISC assembler
int obj_format;					// Object format flag
int debug;						// [1..9] Enable debugging levels
int err_flag;					// '-e' specified
int err_fd;						// File to write error messages to
int rgpu, rdsp;					// Assembling Jaguar GPU or DSP code
int dsp56001;					// Assembling DSP 56001 code
int list_fd;					// File to write listing to
int regbank;					// RISC register bank
int segpadsize;					// Segment padding size
int endian;						// Host processor endianess (0 = LE, 1 = BE)
char * objfname;				// Object filename pointer
char * firstfname;				// First source filename
char * cmdlnexec;				// Executable name, pointer to ARGV[0]
char * searchpath;				// Search path for include files
char defname[] = "noname.o";	// Default output filename
int optim_flags[OPT_COUNT];		// Specific optimisations on/off matrix
int activecpu = CPU_68000;		// Active 68k CPU (68000 by default)
int activefpu = FPU_NONE;		// Active FPU (none by default)


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
		"  -dsymbol[=value]  Define symbol (with optional value, default=0)\n"
		"  -e[errorfile]     Send error messages to file, not stdout\n"
		"  -f[format]        Output object file format\n"
		"                    a: ALCYON (use this for ST)\n"
		"                    b: BSD (use this for Jaguar)\n"
		"                    e: ELF\n"
		"                    x: com/exe/xex (Atari 800)\n"
		"  -i[path]          Directory to search for include files\n"
		"  -l[filename]      Create an output listing file\n"
		"  -l*[filename]     Create an output listing file without pagination\n"
		"  -m[cpu]           Select default CPU. Available options:\n"
		"                    68000\n"
		"                    68020\n"
		"                    68030\n"
		"                    68040\n"
		"                    68060\n"
		"                    6502\n"
		"                    tom\n"
		"                    jerry\n"
		"  -n                Don't do things behind your back in RISC assembler\n"
		"  -o file           Output file name\n"
		"  +o[value]         Turn a specific optimisation on\n"
		"                    Available optimisation values and default settings:\n"
		"                    o0: Absolute long adddresses to word        (on)\n"
		"                    o1: move.l #x,dn/an to moveq                (on)\n"
		"                    o2: Word branches to short                  (on)\n"
		"                    o3: Outer displacement 0(an) to (an)        (off)\n"
		"                    o4: lea size(An),An to addq #size,An        (off)\n"
		"                    o5: Absolute long base displacement to word (off)\n"
		"                    o6: Null branches to NOP                    (off)\n"
		"                    o7: clr.l Dx to moveq #0,Dx                 (off)\n"
		"  ~o[value]         Turn a specific optimisation off\n"
		"  +oall             Turn all optimisations on\n"
		"  ~oall             Turn all optimisations off\n"
		"  -p                Create an ST .prg (without symbols)\n"
		"  -ps               Create an ST .prg (with symbols)\n"
		"                    Forces -fa\n"
		"  -r[size]          Pad segments to boundary size specified\n"
		"                    w: word (2 bytes, default alignment)\n"
		"                    l: long (4 bytes)\n"
		"                    p: phrase (8 bytes)\n"
		"                    d: double phrase (16 bytes)\n"
		"                    q: quad phrase (32 bytes)\n"
		"  -s                Warn about possible short branches\n"
		"                    and applied optimisations\n"
		"  -u                Force referenced and undefined symbols global\n"
		"  -v                Set verbose mode\n"
		"  -x                Turn on debugging mode\n"
		"  -y[pagelen]       Set page line length (default: 61)\n"
		"\n", cmdlnexec);
}


//
// Display version information
//
void DisplayVersion(void)
{
	printf("\n"
		" _ __ _ __ ___   __ _  ___ \n"
		"| '__| '_ ` _ \\ / _` |/ __|\n"
		"| |  | | | | | | (_| | (__ \n"
		"|_|  |_| |_| |_|\\__,_|\\___|\n"
		"\nReboot's Macro Assembler\n"
		"Copyright (C) 199x Landon Dyer, 2011-2017 Reboot\n"
		"V%01i.%01i.%01i %s (%s)\n\n", MAJOR, MINOR, PATCH, __DATE__, PLATFORM);
}


//
// Parse optimisation options
//
int ParseOptimization(char * optstring)
{
	int onoff = 0;

	if (*optstring == '+')
		onoff = 1;
	else if (*optstring != '~')
		return ERROR;

	if ((optstring[2] == 'a' || optstring[2] == 'A')
		&& (optstring[3] == 'l' || optstring[3] == 'L')
		&& (optstring[4] == 'l' || optstring[4] == 'L'))
	{
		memset(optim_flags, onoff, OPT_COUNT * sizeof(int));
		return OK;
	}
	else if (optstring[1] == 'o' || optstring[1] == 'O') // Turn on specific optimisation
	{
		int opt_no = atoi(&optstring[2]);

		if ((opt_no >= 0) && (opt_no < OPT_COUNT))
		{
			optim_flags[opt_no] = onoff;
			return OK;
		}
		else
			return ERROR;
	}
	else
	{
		return ERROR;
	}
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
	m6502 = 0;						// 6502 mode off by default

	// Initialise modules
	InitSymbolTable();				// Symbol table
	InitTokenizer();				// Tokenizer
	InitLineProcessor();			// Line processor
	InitExpression();				// Expression analyzer
	InitSection();					// Section manager / code generator
	InitMark();						// Mark tape-recorder
	InitMacro();					// Macro processor
	InitListing();					// Listing generator
	Init6502();						// 6502 assembler

	// Process command line arguments and assemble source files
	for(argno=0; argno<argc; argno++)
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
				sy->svalue = (*s ? (uint32_t)atoi(s) : 0);
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
				case 'a':			// -fa = Alcyon [the default]
				case 'A':
					obj_format = ALCYON;
					break;
				case 'b':			// -fb = BSD (Jaguar Recommended: 3 out 4 jaguars recommend it!)
				case 'B':
					obj_format = BSD;
					break;
				case 'e':			// -fe = ELF
				case 'E':
					obj_format = ELF;
					break;
				case 'x':			// -fx = COM/EXE/XEX
				case 'X':
					obj_format = XEX;
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
				if (*(argv[argno] + 2) == '*')
				{
					list_fname = argv[argno] + 3;
					list_pag = 0;    // Special case - turn off pagination
				}
				else
				{
					list_fname = argv[argno] + 2;
				}

				listing = 1;
				list_flag = 1;
				lnsave++;
				break;
			case 'm':
			case 'M':
				if (strcmp(argv[argno] + 2, "68000") == 0)
					d_68000();
				else if (strcmp(argv[argno] + 2, "68020") == 0)
					d_68020();
				else if (strcmp(argv[argno] + 2, "68030") == 0)
					d_68030();
				else if (strcmp(argv[argno] + 2, "68040") == 0)
					d_68040();
				else if (strcmp(argv[argno] + 2, "68060") == 0)
					d_68060();
				else if (strcmp(argv[argno] + 2, "68881") == 0)
					d_68881();
				else if (strcmp(argv[argno] + 2, "68882") == 0)
					d_68882();
				else if (strcmp(argv[argno] + 2, "56001") == 0)
					d_56001();
				else if (strcmp(argv[argno] + 2, "6502") == 0)
					d_6502();
				else if (strcmp(argv[argno] + 2, "tom") == 0)
					d_gpu();
				else if (strcmp(argv[argno] + 2, "jerry") == 0)
					d_dsp();
				else
				{
					printf("Unrecognized CPU '%s'\n", argv[argno] + 2);
					errcnt++;
					return errcnt;
				}
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
			case 'p':		/* -p: generate ".PRG" executable output */
			case 'P':
				/*
				 * -p		.PRG generation w/o symbols
				 * -ps	.PRG generation with symbols
				 */
				switch (argv[argno][2])
				{
					case EOS:
						prg_flag = 1;
						break;

					case 's':
					case 'S':
						prg_flag = 2;
						break;

					default:
						printf("-p: syntax error\n");
						errcnt++;
						return errcnt;
				}

				// Enforce Alcyon object format - kind of silly
				// to ask for .prg output without it!
				obj_format = ALCYON;
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
					errcnt++;
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
		else if (*argv[argno] == '+' || *argv[argno] == '~')
		{
			if (ParseOptimization(argv[argno]) != OK)
			{
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
    // o  save the last pc value for 6502 (if we used 6502 at all) and
    //    detemine if the last section contains anything
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

	SwitchSection(M6502);

	if (sloc != currentorg[0])
	{
		currentorg[1] = sloc;
		currentorg += 2;
	}

	if (objfname == NULL)
	{
		if (firstfname == NULL)
			firstfname = defname;

		strcpy(fnbuf, firstfname);
		fext(fnbuf, (prg_flag ? ".prg" : ".o"), 1);
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
			s = (prg_flag ? "executable" : "object");
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

	// Set legacy optimisation flags to on and everything else to off
	memset(optim_flags, 0, OPT_COUNT * sizeof(int));
	optim_flags[OPT_ABS_SHORT] =
		optim_flags[OPT_MOVEL_MOVEQ] =
		optim_flags[OPT_BSR_BCC_S] = 1;

	cmdlnexec = argv[0];			// Obtain executable name
	endian = GetEndianess();		// Get processor endianess

	// If commands were passed in, process them
	if (argc > 1)
		return Process(argc - 1, argv + 1);

	DisplayVersion();
	DisplayHelp();

	return 0;
}

