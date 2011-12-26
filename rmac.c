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
#include "risca.h"
#include "direct.h"
#include "version.h"
#include "debug.h"
#include "symbol.h"
#include "object.h"

int perm_verb_flag;                                         // Permanently verbose, interactive mode
int list_flag;                                              // "-l" Listing flag on command line
int verb_flag;                                              // Be verbose about what's going on
int as68_flag;                                              // as68 kludge mode
int glob_flag;                                              // Assume undefined symbols are global
int lsym_flag;                                              // Include local symbols in object file
int sbra_flag;                                              // Warn about possible short branches
int obj_format;                                             // Object format flag
int debug;                                                  // [1..9] Enable debugging levels
int err_flag;                                               // '-e' specified
int err_fd;                                                 // File to write error messages to 
int rgpu, rdsp;                                             // Assembling Jaguar GPU or DSP code
int list_fd;                                                // File to write listing to
int regbank;                                                // RISC register bank
int segpadsize;                                             // Segment padding size
int in_main;                                                // In main memory flag for GPUMAIN
int endian;                                                 // Processor endianess
char *objfname;                                             // Object filename pointer
char *firstfname;                                           // First source filename
char *cmdlnexec;                                            // Executable name, pointer to ARGV[0]
char *searchpath;                                           // Search path for include files 
char defname[] = "noname.o";                                // Default output filename

// Under Windows and UNIX malloc() is an expensive call, so for small amounts of memory we allocate
// from a previously allocated buffer.

#define A_AMOUNT        4096                                // Amount to malloc() at a time
#define A_THRESH        64                                  // Use malloc() for amounts >= A_THRESH

static LONG a_amount;                                       // Amount left at a_ptr 
static char *a_ptr;                                         // Next free chunk
LONG amemtot;                                               // amem() total of requests

// Qsort; The THRESHold below is the insertion sort threshold, and has been adjusted
// for records of size 48 bytes.The MTHREShold is where we stop finding a better median.
 
#define THRESH          4                                   // Threshold for insertion
#define MTHRESH         6                                   // Threshold for median

static int (*qcmp)();                                       // The comparison routine
static int qsz;                                             // Size of each record
static int thresh;                                          // THRESHold in chars 
static int mthresh;                                         // MTHRESHold in chars

// qst: Do a quicksort. First, find the median element, and put that one in the first place as 
// the discriminator.  (This "median" is just the median of the first, last and middle elements).
// (Using this median instead of the first element is a big win).  Then, the usual 
// partitioning/swapping, followed by moving the discriminator into the right place.  Then, 
// figure out the sizes of the two partions, do the smaller one recursively and the larger one 
// via a repeat of this code.  Stopping when there are less than THRESH elements in a partition
// and cleaning up with an insertion sort (in our caller) is a huge win. All data swaps are done 
// in-line, which is space-losing but time-saving. (And there are only three places where 
// this is done).

static int qst(char *base, char *max) {
   char c, *i, *j, *jj;
   int ii;
   char *mid, *tmp;
   long lo, hi;

   /*
	 * At the top here, lo is the number of characters of elements in the
	 * current partition.  (Which should be max - base).
	 * Find the median of the first, last, and middle element and make
	 * that the middle element.  Set j to largest of first and middle.
	 * If max is larger than that guy, then it's that guy, else compare
	 * max with loser of first and take larger.  Things are set up to
	 * prefer the middle, then the first in case of ties.
	 */
	lo = max - base;		/* number of elements as chars */
	do	{
		mid = i = base + qsz * ((lo / qsz) >> 1);
		if (lo >= mthresh) {
			j = (qcmp((jj = base), i) > 0 ? jj : i);
			if (qcmp(j, (tmp = max - qsz)) > 0) {
				/* switch to first loser */
				j = (j == jj ? i : jj);
				if (qcmp(j, tmp) < 0)
					j = tmp;
			}
			if (j != i) {
				ii = qsz;
				do	{
					c = *i;
					*i++ = *j;
					*j++ = c;
				} while (--ii);
			}
		}
		/*
		 * Semi-standard quicksort partitioning/swapping
		 */
		for (i = base, j = max - qsz; ; ) {
			while (i < mid && qcmp(i, mid) <= 0)
				i += qsz;
			while (j > mid) {
				if (qcmp(mid, j) <= 0) {
					j -= qsz;
					continue;
				}
				tmp = i + qsz;	/* value of i after swap */
				if (i == mid) {
					/* j <-> mid, new mid is j */
					mid = jj = j;
				} else {
					/* i <-> j */
					jj = j;
					j -= qsz;
				}
				goto swap;
			}
			if (i == mid) {
				break;
			} else {
				/* i <-> mid, new mid is i */
				jj = mid;
				tmp = mid = i;	/* value of i after swap */
				j -= qsz;
			}
swap:
			ii = qsz;
			do	{
				c = *i;
				*i++ = *jj;
				*jj++ = c;
			} while (--ii);
			i = tmp;
		}
		/*
		 * Look at sizes of the two partitions, do the smaller
		 * one first by recursion, then do the larger one by
		 * making sure lo is its size, base and max are update
		 * correctly, and branching back.  But only repeat
		 * (recursively or by branching) if the partition is
		 * of at least size THRESH.
		 */
		i = (j = mid) + qsz;
		if ((lo = j - base) <= (hi = max - i)) {
			if (lo >= thresh)
				qst(base, j);
			base = i;
			lo = hi;
		} else {
			if (hi >= thresh)
				qst(i, max);
			max = j;
		}
	} while (lo >= thresh);

   return(0);
}

/*
 * qsort:
 * First, set up some global parameters for qst to share.  Then, quicksort
 * with qst(), and then a cleanup insertion sort ourselves.  Sound simple?
 * It's not...
 */

int rmac_qsort(char *base, int n, int size, int	(*compar)()) {
	register char c, *i, *j, *lo, *hi;
	char *min, *max;

	if (n <= 1)
		return(0);
	qsz = size;
	qcmp = compar;
	thresh = qsz * THRESH;
	mthresh = qsz * MTHRESH;
	max = base + n * qsz;
	if (n >= THRESH) {
		qst(base, max);
		hi = base + thresh;
	} else {
		hi = max;
	}
	/*
	 * First put smallest element, which must be in the first THRESH, in
	 * the first position as a sentinel.  This is done just by searching
	 * the first THRESH elements (or the first n if n < THRESH), finding
	 * the min, and swapping it into the first position.
	 */
	for (j = lo = base; (lo += qsz) < hi; )
		if (qcmp(j, lo) > 0)
			j = lo;
	if (j != base) {
		/* swap j into place */
		for (i = base, hi = base + qsz; i < hi; ) {
			c = *j;
			*j++ = *i;
			*i++ = c;
		}
	}
	/*
	 * With our sentinel in place, we now run the following hyper-fast
	 * insertion sort.  For each remaining element, min, from [1] to [n-1],
	 * set hi to the index of the element AFTER which this one goes.
	 * Then, do the standard insertion sort shift on a character at a time
	 * basis for each element in the frob.
	 */
	for (min = base; (hi = min += qsz) < max; ) {
		while (qcmp(hi -= qsz, min) > 0)
			/* void */;
		if ((hi += qsz) != min) {
			for (lo = min + qsz; --lo >= min; ) {
				c = *lo;
				for (i = j = lo; (j -= qsz) >= hi; i = j)
					*i = *j;
				*i = c;
			}
		}
	}
   return(0);
}

//
// --- Allocate memory; Panic and Quit if we Run Out -----------------------------------------------
//

char * amem(LONG amount)
{
	char * p;

	if (amount & 1)                                           // Keep word alignment
		++amount;

	if (amount < A_THRESH)
	{                                  // Honor *small* request
		if (a_amount < amount)
		{
			a_ptr = amem(A_AMOUNT);
			a_amount = A_AMOUNT;
		}

		p = a_ptr;
		a_ptr += amount;
		a_amount -= amount;
	}
	else
	{
		amemtot += amount;                                    // Bump total alloc
		p = (char *)malloc(amount);                           // Get memory from malloc

		if ((LONG)p == (LONG)NULL)
			fatal("memory exhausted");

		memset(p, 0, amount);
	}

	return p;
}

//
// --- Copy stuff around, return pointer to dest+count+1 (doesn't handle overlap) ------------------
//

char * copy(char * dest, char * src, LONG count)
{
   while (count--)
      *dest++ = *src++;

   return dest;
}

//
// --- Clear a region of memory --------------------------------------------------------------------
//

void clear(char *dest, LONG count)
{
	while(count--)
		*dest++ = 0;
}

//
// --- Check to see if the string is a keyword. Returns -1, or a value from the 'accept[]' table ---
//

int kmatch(char * p, int * base, int * check, int * tab, int * accept)
{
	int state;
	int j;

	for(state=0; state>=0;)
	{
		j = base[state] + (int)tolowertab[*p];

		if (check[j] != state)
		{                               // Reject, character doesn't match
			state = -1;                                        // No match 
			break;
		}

		if (!*++p)
		{                                           // Must accept or reject at EOS
			state = accept[j];                                 // (-1 on no terminal match) 
			break;
		}

		state = tab[j];
	}

	return state;
}

//
// --- Auto-even a section -------------------------------------------------------------------------
//

void autoeven(int sect)
{
	switchsect(sect);
	d_even();
	savsect();
}

//
// -------------------------------------------------------------------------------------------------
// Manipulate file extension.
// `name' must be large enough to hold any possible filename.
// If `stripp' is nonzero, any old extension is removed.
// Then, if the file does not already have an extension,
// `extension' is appended to the filename.
// -------------------------------------------------------------------------------------------------
//

char *fext(char *name, char *extension, int stripp) {
   char *s, *beg;                                           // String pointers

   // Find beginning of "real" name
   beg = name + strlen(name) - 1;
   for(; beg > name; --beg) {
      if(*beg == SLASHCHAR) {
         ++beg;
         break;
      }
   }

   if(stripp) {                                             // Clobber any old extension
      for(s = beg; *s && *s != '.'; ++s) 
         ;
      *s = '\0';
   }

   for(s = beg; *s != '.'; ++s) {
      if(!*s) {                                             // Append the new extension
         strcat(beg, extension);
         break;
      }
   }

   return(name);
}

//
// -------------------------------------------------------------------------------------------------
// Return `item'nth element of semicolon-seperated pathnames specified in the enviroment string `s'.
// Copy the pathname to `buf'.  Return 0 if the `item'nth path doesn't exist.
// 
// [`item' ranges from 0 to N-1, where N = #elements in search path]
// -------------------------------------------------------------------------------------------------
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
// --- Display Command Line Help -----------------------------------------------
//

void display_help(void)
{
	printf("Usage:\n");
	printf("    %s [options] srcfile\n", cmdlnexec);
	printf("\n");
	printf("Options:\n");
	printf("   -? or -h              display usage information\n");
	printf("   -dsymbol[=value]      define symbol\n");
	printf("   -e[errorfile]         send error messages to file, not stdout\n");
	printf("   -f[format]            output object file format\n");
	printf("                         b: BSD (use this for Jaguar)\n");
	printf("   -i[path]              directory to search for include files\n");
	printf("   -l[filename]          create an output listing file\n");
	printf("   -o file               output file name\n");
	printf("   -r[size]              pad segments to boundary size specified\n");
	printf("                         w: word (2 bytes, default alignment)\n");
	printf("                         l: long (4 bytes)\n");
	printf("                         p: phrase (8 bytes)\n");
	printf("                         d: double phrase (16 bytes)\n");
	printf("                         q: quad phrase (32 bytes)\n");
	printf("   -s                    warn about possible short branches\n");
	printf("   -u                    force referenced and undefined symbols global\n");
	printf("   -v                    set verbose mode\n");
	printf("   -y[pagelen]           set page line length (default: 61)\n");
	printf("\n");
}

//
// --- Display Version Information ---------------------------------------------
//

void display_version(void)
{
	printf("\nReboot's Macro Assembler for Atari Jaguar\n"); 
	printf("Copyright (C) 199x Landon Dyer, 2011 Reboot\n"); 
	printf("V%01i.%01i.%01i %s (%s)\n\n", MAJOR, MINOR, PATCH, __DATE__, PLATFORM);
}

// 
// --- Process Command Line Arguments and do an Assembly -------------------------------------------
//

int process(int argc, char **argv) {
   int argno;                                               // Argument number
   SYM *sy;                                                 // Pointer to a symbol record
   char *s;                                                 // String pointer
   int fd;                                                  // File descriptor
   char fnbuf[FNSIZ];                                       // Filename buffer 
   int i;                                                   // Iterator

   errcnt = 0;                                              // Initialise error count
   listing = 0;                                             // Initialise listing level
   list_flag = 0;                                           // Initialise listing flag
   verb_flag = perm_verb_flag;                              // Initialise verbose flag
   as68_flag = 0;                                           // Initialise as68 kludge mode
   glob_flag = 0;                                           // Initialise .globl flag
   sbra_flag = 0;                                           // Initialise short branch flag
   debug = 0;                                               // Initialise debug flag
   searchpath = NULL;                                       // Initialise search path
   objfname = NULL;                                         // Initialise object filename
   list_fname = NULL;                                       // Initialise listing filename
   err_fname = NULL;                                        // Initialise error filename
   obj_format = BSD;                                        // Initialise object format
   firstfname = NULL;                                       // Initialise first filename
   err_fd = ERROUT;                                         // Initialise error file descriptor
   err_flag = 0;                                            // Initialise error flag
   rgpu = 0;                                                // Initialise GPU assembly flag
   rdsp = 0;                                                // Initialise DSP assembly flag
   lsym_flag = 1;                                           // Include local symbols in object file
   regbank = BANK_N;                                        // No RISC register bank specified
   orgactive = 0;                                           // Not in RISC org section
   orgwarning = 0;                                          // No ORG warning issued
   a_amount = 0;
   segpadsize = 2;                                          // Initialise segment padding size
   in_main = 0;

   // Initialise modules
	init_sym();                                              // Symbol table
   init_token();                                            // Tokenizer
   init_procln();                                           // Line processor
   init_expr();                                             // Expression analyzer
   init_sect();                                             // Section manager / code generator
   init_mark();                                             // Mark tape-recorder
	init_macro();                                            // Macro processor
   init_list();                                             // Listing generator

   // Process command line arguments and assemble source files
   for(argno = 0; argno < argc; ++argno) {
      if(*argv[argno] == '-') {
         switch(argv[argno][1]) {
            case 'd':                                       // Define symbol
            case 'D':
               for(s = argv[argno] + 2; *s != EOS;) {
                  if(*s++ == '=') {
                     s[-1] = EOS;
                     break;
						}
               }
               if(argv[argno][2] == EOS) {
                  printf("-d: empty symbol\n");
                  ++errcnt;
                  return(errcnt);
               }
               sy = lookup(argv[argno] + 2, 0, 0);
               if(sy == NULL) {
                  sy = newsym(argv[argno] + 2, LABEL, 0);
                  sy->svalue = 0;
               }
               sy->sattr = DEFINED | EQUATED | ABS;
               if(*s)
                  sy->svalue = (VALUE)atoi(s);
               else
                  sy->svalue = 0;
               break;
            case 'e':                                       // Redirect error message output
            case 'E':
               err_fname = argv[argno] + 2;
               break;
            case 'f':                                       // -f<format>
            case 'F':
               switch(argv[argno][2]) {
                  case EOS:
                  case 'b':                                 // -fb = BSD (Jaguar Recommended)
                  case 'B':
                     obj_format = BSD;
                     break;
                  default:
                     printf("-f: unknown object format specified\n");
                     ++errcnt;
                     return(errcnt);
               }
               break;
            case 'g':                                       // Debugging flag
            case 'G':
               printf("Debugging flag (-g) not yet implemented\n");
               break;
            case 'i':                                       // Set directory search path
            case 'I':
               searchpath = argv[argno] + 2;
               break;
            case 'l':                                       // Produce listing file
            case 'L':
               list_fname = argv[argno] + 2;
               listing = 1;
               list_flag = 1;
               ++lnsave;
               break;
            case 'o':                                       // Direct object file output
            case 'O':
               if(argv[argno][2] != EOS) objfname = argv[argno] + 2;
               else {
                  if(++argno >= argc) {
                     printf("Missing argument to -o");
                     ++errcnt;
                     return(errcnt);
                  }
                  objfname = argv[argno];
               }
               break;
            case 'r':                                       // Pad seg to requested boundary size
            case 'R':
               switch(argv[argno][2]) {
                  case 'w': case 'W': segpadsize = 2;  break;  
                  case 'l': case 'L': segpadsize = 4;  break;
                  case 'p': case 'P': segpadsize = 8;  break;
                  case 'd': case 'D': segpadsize = 16; break;
                  case 'q': case 'Q': segpadsize = 32; break;
                  default: segpadsize = 2; break;           // Effective autoeven();
               }
               break;
            case 's':                                       // Warn about possible short branches
            case 'S':
               sbra_flag = 1;
               break;
            case 'u':                                       // Make undefined symbols .globl
            case 'U':
               glob_flag = 1;
               break;
            case 'v':                                       // Verbose flag
            case 'V':
               verb_flag++;
               if(verb_flag > 1) display_version();
               break;
            case 'x':                                       // Turn on debugging
            case 'X':
               debug = 1;
               printf("~ Debugging ON\n");
               break;
            case 'y':                                       // -y<pagelen>
            case 'Y':
               pagelen = atoi(argv[argno] + 2);
               if(pagelen < 10) {
                  printf("-y: bad page length\n");
                  ++errcnt;
                  return(errcnt);
               }
               break;
            case EOS:                                       // Input is stdin
               if(firstfname == NULL)                       // Kludge first filename
                  firstfname = defname;
               include(0, "(stdin)");
               assemble();
               break;
            case 'h':                                       // Display command line usage
            case 'H':
            case '?':
               display_version();
               display_help();
               ++errcnt;
               break;
            default:
               display_version();
               printf("Unknown switch: %s\n\n", argv[argno]);
               display_help();
               ++errcnt;
               break;
         }
      } else {
         // Record first filename.
         if(firstfname == NULL)
            firstfname = argv[argno];
         strcpy(fnbuf, argv[argno]);
         fext(fnbuf, ".s", 0);
         fd = open(fnbuf, 0);
         if(fd < 0) {
            printf("Cannot open: %s\n", fnbuf);
            ++errcnt;
            continue;
         }
         include(fd, fnbuf);
         assemble();
      }
   }

   // Wind-up processing;
   // o  save current section (no more code generation)
   // o  do auto-even of all sections (or boundary alignment as requested through '-r')
   // o  determine name of object file:
   //    -  "foo.o" for linkable output;
   //    -  "foo.prg" for GEMDOS executable (-p flag).
   savsect();
   for(i = TEXT; i <= BSS; i <<= 1) {
      switchsect(i);
      switch(segpadsize) {
         case 2:  d_even();    break;
         case 4:  d_long();    break;
         case 8:  d_phrase();  break;
         case 16: d_dphrase(); break;
         case 32: d_qphrase(); break;
      }
      savsect();
   }

   if(objfname == NULL) {
      if(firstfname == NULL)
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
   fixups();                                                // Do all fixups
   stopmark();                                              // Stop mark tape-recorder
   if(errcnt == 0) {
      if((fd = open(objfname, _OPEN_FLAGS, _PERM_MODE)) < 0)
         cantcreat(objfname);
      if(verb_flag) {
         s = "object";
         printf("[Writing %s file: %s]\n", s, objfname);
      }
      object((WORD)fd);
      close(fd);
      if(errcnt != 0)
         unlink(objfname);
   }

   if(list_flag) {
      if(verb_flag) printf("[Wrapping-up listing file]\n");
      listing = 1;
      symtable();
      close(list_fd);
   }

   if(err_flag)
      close(err_fd);

   DEBUG dump_everything();

   return(errcnt);
}

//
// --- Interactive Mode ----------------------------------------------------------------------------
//

void interactive(void)
{
	char * s;                                                 // String pointer for banner
	char ln[LNSIZ];                                          // Input line
	char * argv[MAXARGV];                                     // Argument values
	int argcnt;                                              // Argument count

   // As there is no command line, print a copyright message and prompt for command line
	s = "*****************************************************\n";
	printf("\n%s* RMAC - Reboot's Macro Assembler for Atari Jaguar  *\n", s);
	printf("* Copyright (C) 199x Landon Dyer, 2011 Reboot       *\n");
	printf("* Version %01i.%01i.%01i                Platform: %-9s  *\n",MAJOR,MINOR,PATCH,PLATFORM);
	printf("* ------------------------------------------------- *\n");
	printf("* INTERACTIVE MODE (press ENTER by itself to quit)  *\n%s\n", s);

	perm_verb_flag = 1;                                      // Enter permanent verbose mode

	// Handle commandlines until EOF or we get an empty one
	for(;;)
	{
		loop:
		printf("* ");
		fflush(stdout);                                       // Make prompt visible

//		if (gets(ln) == NULL || !*ln)                          // Get input line
		if (fgets(ln, LNSIZ, stdin) == NULL || !*ln)                          // Get input line
			break;

		argcnt = 0;                                           // Process input line
		s = ln;

		while (*s)
		{
			if (isspace(*s))
			{                                  // Skip whitespace
				++s;
			}
			else
			{
				if (argcnt >= MAXARGV)
				{
					printf("Too many arguments\n");
					goto loop;
				}

				argv[argcnt++] = s;

				while (*s && !isspace(*s))
					++s;

				if (isspace(*s))
					*s++ = EOS;
			}
		}

		if (argcnt == 0)                                       // Exit if no arguments
			break;

		process(argcnt, argv);                                // Process arguments

		if (errcnt)
			printf("%d assembly error%s\n", errcnt, (errcnt > 1) ? "s" : "");
	}
}

//
// --- Determine Processor Endianess ---------------------------------------------------------------
//

int get_endianess(void)
{
	int i = 1;
	char * p = (char *)&i;

	if (p[0] == 1)
		return 0;

	return 1;
}

//
// --- Application Entry Point; Handle the Command Line --------------------------------------------
//

int main(int argc, char ** argv)
{
	int status;                                              // Status flag
	int i;

	perm_verb_flag = 0;                                      // Clobber "permanent" verbose flag
	cmdlnexec = argv[0];                                     // Obtain executable name

	endian = get_endianess();                                // Get processor endianess

	for(i=0; i<MAXFWDJUMPS; i++)
		fwdjump[i] = 0;

	if (argc > 1)
	{                                           // Full command line passed
		status = process(argc - 1, argv + 1);              
	}
	else
	{                                                 // Interactive mode 
		status = 0;
		interactive();
	}

	return status;
}
