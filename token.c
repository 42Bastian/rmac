//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// TOKEN.C - Token Handling
// Copyright (C) 199x Landon Dyer, 2011-2012 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source Utilised with the Kind Permission of Landon Dyer
//

#include "token.h"
#include "error.h"
#include "macro.h"
#include "procln.h"
#include "symbol.h"

#define DECL_KW				// Declare keyword arrays
#define DEF_KW				// Declare keyword values 
#include "kwtab.h"			// Incl generated keyword tables & defs


int lnsave;					// 1; strcpy() text of current line
int curlineno;				// Current line number
int totlines;				// Total # of lines
int mjump_align = 0;		// mjump alignment flag
char lntag;					// Line tag
char * curfname;			// Current filename
char tolowertab[128];		// Uppercase ==> lowercase 
char hextab[128];			// Table of hex values
char dotxtab[128];			// Table for ".b", ".s", etc.
char irbuf[LNSIZ];			// Text for .rept block line
char lnbuf[LNSIZ];			// Text of current line
WORD filecount;				// Unique file number counter
WORD cfileno;				// Current file number
TOKEN * tok;				// Ptr to current token
TOKEN * etok;				// Ptr past last token in tokbuf[]
TOKEN tokeol[1] = {EOL};	// Bailout end-of-line token
char * string[TOKBUFSIZE*2];	// Token buffer string pointer storage

// File record, used to maintain a list of every include file ever visited
#define FILEREC struct _filerec
FILEREC
{
   FILEREC * frec_next;
   char * frec_name;
};

FILEREC * filerec;
FILEREC * last_fr;

INOBJ * cur_inobj;						// Ptr current input obj (IFILE/IMACRO)
static INOBJ * f_inobj;					// Ptr list of free INOBJs
static IFILE * f_ifile;					// Ptr list of free IFILEs
static IMACRO * f_imacro;				// Ptr list of free IMACROs

static TOKEN tokbuf[TOKBUFSIZE];		// Token buffer (stack-like, all files)

char chrtab[] = {
	ILLEG, ILLEG, ILLEG, ILLEG,			// NUL SOH STX ETX 
	ILLEG, ILLEG, ILLEG, ILLEG,			// EOT ENQ ACK BEL 
	ILLEG, WHITE, ILLEG, ILLEG,			// BS HT LF VT 
	WHITE, ILLEG, ILLEG, ILLEG,			// FF CR SO SI 

	ILLEG, ILLEG, ILLEG, ILLEG,			// DLE DC1 DC2 DC3 
	ILLEG, ILLEG, ILLEG, ILLEG,			// DC4 NAK SYN ETB 
	ILLEG, ILLEG, ILLEG, ILLEG,			// CAN EM SUB ESC 
	ILLEG, ILLEG, ILLEG, ILLEG,			// FS GS RS US 

	WHITE, MULTX, MULTX, SELF,			// SP ! " #
	MULTX+CTSYM, MULTX, SELF, MULTX,	// $ % & '
	SELF, SELF, SELF, SELF,				// ( ) * +
	SELF, SELF, STSYM, SELF,			// , - . /

	DIGIT+HDIGIT+CTSYM, DIGIT+HDIGIT+CTSYM,		// 0 1 
	DIGIT+HDIGIT+CTSYM, DIGIT+HDIGIT+CTSYM,		// 2 3 
	DIGIT+HDIGIT+CTSYM, DIGIT+HDIGIT+CTSYM,		// 4 5 
	DIGIT+HDIGIT+CTSYM, DIGIT+HDIGIT+CTSYM,		// 6 7 
	DIGIT+HDIGIT+CTSYM, DIGIT+HDIGIT+CTSYM,		// 8 9 
	MULTX, MULTX,								// : ; 
	MULTX, MULTX, MULTX, STSYM+CTSYM,			// < = > ? 

	MULTX, STSYM+CTSYM+HDIGIT,									// @ A
	(char)((BYTE)DOT)+STSYM+CTSYM+HDIGIT, STSYM+CTSYM+HDIGIT,	// B C
	STSYM+CTSYM+HDIGIT, STSYM+CTSYM+HDIGIT,						// D E
	STSYM+CTSYM+HDIGIT, STSYM+CTSYM,							// F G
	STSYM+CTSYM, STSYM+CTSYM, STSYM+CTSYM, STSYM+CTSYM,			// H I J K
	(char)((BYTE)DOT)+STSYM+CTSYM, STSYM+CTSYM, STSYM+CTSYM, STSYM+CTSYM,	// L M N O

	STSYM+CTSYM, STSYM+CTSYM, STSYM+CTSYM, (char)((BYTE)DOT)+STSYM+CTSYM,	// P Q R S
	STSYM+CTSYM, STSYM+CTSYM, STSYM+CTSYM, (char)((BYTE)DOT)+STSYM+CTSYM,	// T U V W
	STSYM+CTSYM, STSYM+CTSYM, STSYM+CTSYM, SELF,				// X Y Z [
	SELF, SELF, MULTX, STSYM+CTSYM,								// \ ] ^ _

	ILLEG, STSYM+CTSYM+HDIGIT,									// ` a
	(char)((BYTE)DOT)+STSYM+CTSYM+HDIGIT, STSYM+CTSYM+HDIGIT,	// b c
	STSYM+CTSYM+HDIGIT, STSYM+CTSYM+HDIGIT,						// d e
	STSYM+CTSYM+HDIGIT, STSYM+CTSYM,							// f g
	STSYM+CTSYM, STSYM+CTSYM, STSYM+CTSYM, STSYM+CTSYM,			// h i j k
	(char)((BYTE)DOT)+STSYM+CTSYM, STSYM+CTSYM, STSYM+CTSYM, STSYM+CTSYM,	// l m n o

	STSYM+CTSYM, STSYM+CTSYM, STSYM+CTSYM, (char)((BYTE)DOT)+STSYM+CTSYM,	// p q r s 
	STSYM+CTSYM, STSYM+CTSYM, STSYM+CTSYM, (char)((BYTE)DOT)+STSYM+CTSYM,	// t u v w 
	STSYM+CTSYM, STSYM+CTSYM, STSYM+CTSYM, SELF,				// x y z { 
	SELF, SELF, SELF, ILLEG										// | } ~ DEL 
};

// Names of registers
static char * regname[] = {
	"d0", "d1",  "d2",  "d3", "d4", "d5", "d6", "d7",
	"a0", "a1",  "a2",  "a3", "a4", "a5", "a6", "a7",
	"pc", "ssp", "usp", "sr", "ccr"
};

static char * riscregname[] = {
	 "r0",  "r1",  "r2",  "r3",  "r4", "r5",   "r6",  "r7", 
	 "r8",  "r9", "r10", "r11", "r12", "r13", "r14", "r15",
	"r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
	"r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31"
};


//
// Initialize tokenizer
//
void InitTokenizer(void)
{
	int i;									// Iterator
	char * htab = "0123456789abcdefABCDEF";	// Hex character table

	lnsave = 0;								// Don't save lines
	curfname = "";							// No file, empty filename
	filecount = (WORD)-1;
	cfileno = (WORD)-1;						// cfileno gets bumped to 0
	curlineno = 0;
	totlines = 0;
	etok = tokbuf;
	f_inobj = NULL;
	f_ifile = NULL;
	f_imacro = NULL;
	cur_inobj = NULL;
	filerec = NULL;
	last_fr = NULL;
	lntag = SPACE;

	// Initialize hex, "dot" and tolower tables
	for(i=0; i<128; i++)
	{
		hextab[i] = -1;
		dotxtab[i] = 0;
		tolowertab[i] = (char)i;
	}

	for(i=0; htab[i]!=EOS; i++)
		hextab[htab[i]] = (char)((i < 16) ? i : i - 6);

	for(i='A'; i<='Z'; i++)
		tolowertab[i] |= 0x20;

	// These characters are legal immediately after a period
	dotxtab['b'] = DOTB;					// .b .B .s .S 
	dotxtab['B'] = DOTB;
	dotxtab['s'] = DOTB;
	dotxtab['S'] = DOTB;
	dotxtab['w'] = DOTW;					// .w .W 
	dotxtab['W'] = DOTW;
	dotxtab['l'] = DOTL;					// .l .L 
	dotxtab['L'] = DOTL;
	dotxtab['i'] = DOTI;					// .i .I (???) 
	dotxtab['I'] = DOTI;
}


void SetFilenameForErrorReporting(void)
{
	WORD fnum = cfileno;

	// Check for absolute top filename (this should never happen)
	if (fnum == -1)
	{
		curfname = "(*top*)";
		return;
	}

	FILEREC * fr = filerec;

	// Advance to the correct record...
	while (fr != NULL && fnum != 0)
	{
		fr = fr->frec_next;
		fnum--;
	}

	// Check for file # record not found (this should never happen either)
	if (fr == NULL)
	{
		curfname = "(*NOT FOUND*)";
		return;
	}

	curfname = fr->frec_name;
}


//
// Allocate an IFILE or IMACRO
//
INOBJ * a_inobj(int typ)
{
	INOBJ * inobj;
	IFILE * ifile;
	IMACRO * imacro;

	// Allocate and initialize INOBJ first
	if (f_inobj == NULL)
		inobj = malloc(sizeof(INOBJ));
	else
	{
		inobj = f_inobj;
		f_inobj = f_inobj->in_link;
	}

	switch (typ)
	{
	case SRC_IFILE:							// Alloc and init an IFILE
		if (f_ifile == NULL)
			ifile = malloc(sizeof(IFILE));
		else
		{
			ifile = f_ifile;
			f_ifile = f_ifile->if_link;
		}

		inobj->inobj.ifile = ifile;
		break;
	case SRC_IMACRO:						// Alloc and init an IMACRO 
		if (f_imacro == NULL)
			imacro = malloc(sizeof(IMACRO));
		else
		{
			imacro = f_imacro;
			f_imacro = f_imacro->im_link;
		}

		inobj->inobj.imacro = imacro;
		break;
	case SRC_IREPT:							// Alloc and init an IREPT
		inobj->inobj.irept = malloc(sizeof(IREPT));
		DEBUG printf("alloc IREPT\n");
		break;
	}

	// Install INOBJ on top of input stack
	inobj->in_ifent = ifent;				// Record .if context on entry
	inobj->in_type = (WORD)typ;
	inobj->in_otok = tok;
	inobj->in_etok = etok;
	inobj->in_link = cur_inobj;
	cur_inobj = inobj;

	return inobj;
}


//
// Perform macro substitution from 'orig' to 'dest'. Return OK or some error.
// A macro reference is in one of two forms:
// \name <non-name-character>
// \{name}
// A doubled backslash (\\) is compressed to a single backslash (\).
// Argument definitions have been pre-tokenized, so we have to turn them back
// into text. This means that numbers, in particular, become hex, regardless of
// their representation when the macro was invoked. This is a hack.
// A label may appear at the beginning of the line:
// :<name><whitespace>
// (the colon must be in the first column). These labels are stripped before
// macro expansion takes place.
//
int ExpandMacro(char * src, char * dest, int destsiz)
{
	int i;
	int questmark;			// \? for testing argument existence
	char mname[128];		// Assume max size of a formal arg name
	char numbuf[20];		// Buffer for text of CONSTs
	TOKEN * tk;
	SYM * arg;
	char ** symbolString;

	DEBUG { printf("ExM: src=\"%s\"\n", src); }

	IMACRO * imacro = cur_inobj->inobj.imacro;
	int macnum = (int)(imacro->im_macro->sattr);

//	destsiz--;
	char * dst = dest;						// Next dest slot
	char * edst = dest + destsiz - 1;		// End + 1(?) of dest buffer

	// Check for (and skip over) any "label" on the line
	char * s = src;
	char * d = NULL;

	if (*s == ':')
	{
		while (*s != EOS && !(chrtab[*s] & WHITE))
			s++;

		if (*s != EOS)
			s++;							// Skip first whitespace
	}

	// Expand the rest of the line
	while (*s != EOS)
	{
		// Copy single character
		if (*s != '\\')
		{
			if (dst >= edst)
				goto overflow;

			*dst++ = *s++;
		}
		// Do macro expansion
		else
		{
			questmark = 0;

			// Do special cases
			switch (*++s)
			{
			case '\\':						// \\, \ (collapse to single backslash)
				if (dst >= edst)
					goto overflow;

				*dst++ = *s++;
				continue;
			case '?':						// \? <macro>  set `questmark' flag 
				++s;
				questmark = 1;
				break;
			case '#':						// \#, number of arguments 
				sprintf(numbuf, "%d", (int)imacro->im_nargs);
				goto copystr;
			case '!':						// \! size suffix supplied on invocation
				switch ((int)imacro->im_siz)
				{
				case SIZN: d = "";   break;
				case SIZB: d = ".b"; break;
				case SIZW: d = ".w"; break;
				case SIZL: d = ".l"; break;
				}

				goto copy_d;
			case '~':						// ==> unique label string Mnnnn... 
				sprintf(numbuf, "M%u", curuniq);
copystr:
				d = numbuf;
copy_d:
				s++;

				while (*d != EOS)
				{
					if (dst >= edst)
						goto overflow;
					else
						*dst++ = *d++;
				}

				continue;
			case EOS:
				return error("missing argument name");
			}

			// \n ==> argument number 'n', 0..9
			if (chrtab[*s] & DIGIT)
			{
				i = *s++ - '1';

				if (i < 0)
					i = 9;

				goto arg_num;
			}

			// Get argument name: \name, \{name}
			d = mname;

			// \label
			if (*s != '{')
			{
				do
				{
					*d++ = *s++;
				}
				while (chrtab[*s] & CTSYM);
			}
			// \\{label}
			else
			{
				for(++s; *s != EOS && *s != '}';)
					*d++ = *s++;

				if (*s != '}')
					return error("missing '}'");
				else
					s++;
			}

			*d = EOS;

			// Lookup the argument and copy its (string) value into the
			// destination string
			DEBUG printf("argument='%s'\n", mname);

			if ((arg = lookup(mname, MACARG, macnum)) == NULL)
				return errors("undefined argument: '%s'", mname);
			else
			{
				// Convert a string of tokens (terminated with EOL) back into
				// text. If an argument is out of range (not specified in the
				// macro invocation) then it is ignored.
				i = (int)arg->svalue;
arg_num:
				DEBUG printf("~argnumber=%d (argBase=%u)\n", i, imacro->argBase);
				tk = NULL;

				if (i < imacro->im_nargs)
				{
#if 0
//					tk = argp[i];
//					tk = argPtrs[i];
					tk = argPtrs[imacro->argBase + i];
#else
					tk = imacro->argument[i].token;
					symbolString = imacro->argument[i].string;
//DEBUG
//{
//	printf("ExM: Preparing to parse argument #%u...\n", i);
//	dumptok(tk);
//}
#endif
				}

				// \?arg yields:
				//    0  if the argument is empty or non-existant,
				//    1  if the argument is not empty
				if (questmark)
				{
					if (tk == NULL || *tk == EOL)
						questmark = 0;

					if (dst >= edst)
						goto overflow;

					*dst++ = (char)(questmark + '0');
					continue;
				}

				// Argument # is in range, so expand it
				if (tk != NULL)
				{
					while (*tk != EOL)
					{
						// Reverse-translation from a token number to a string.
						// This is a hack. It might be better table-driven.
						d = NULL;

						if ((*tk >= KW_D0) && !rdsp && !rgpu)
						{
							d = regname[(int)*tk++ - KW_D0];
							goto strcopy;
						}
						else if ((*tk >= KW_R0) && (*tk <= KW_R31))
						{
							d = riscregname[(int)*tk++ - KW_R0];
							goto strcopy;
						}
						else
						{
							switch ((int)*tk++)
							{
							case SYMBOL:
#if 0
//								d = (char *)*tk++;
								d = string[*tk++];
#else
								// This fix should be done for strings too
								d = symbolString[*tk++];
DEBUG printf("ExM: SYMBOL=\"%s\"", d);
#endif
								break;
							case STRING:
#if 0
//								d = (char *)*tk++;
								d = string[*tk++];
#else
								d = symbolString[*tk++];
#endif
								if (dst >= edst)
									goto overflow;

								*dst++ = '"';

								while (*d != EOS)
								{
									if (dst >= edst)
										goto overflow;
									else
										*dst++ = *d++;
								}

								if (dst >= edst)
									goto overflow;

								*dst++ = '"';
								continue;
								break;
// Shamus: Changing the format specifier from %lx to %ux caused
//         the assembler to choke on legitimate code... Need to investigate
//         this further before changing anything else here!
							case CONST:
								sprintf(numbuf, "$%lx", (LONG)*tk++);
								d = numbuf;
								break;
							case DEQUALS:
								d = "==";
								break;
							case SET:
								d = "set";
								break;
							case COLON:
								d = ":";
								break;
							case DCOLON:
								d = "::";
								break;
							case GE:
								d = ">=";
								break;
							case LE:
								d = "<=";
								break;
							case NE:
								d = "<>";
								break;
							case SHR:
								d = ">>";
								break;
							case SHL:
								d = "<<";
								break;
							case DOTB:
								d = ".b";
								break;
							case DOTW:
								d = ".w";
								break;
							case DOTL:
								d = ".l";
								break;
							case CR_DATE:
								d = "^^date";
								break;
							case CR_TIME:
								d = "^^time";
								break;
							case CR_DEFINED:
								d = "^^defined ";
								break;
							case CR_REFERENCED:
								d = "^^referenced ";
								break;
							case CR_STREQ:
								d = "^^streq ";
								break;
							case CR_MACDEF:
								d = "^^macdef ";
								break;
							default:
								if (dst >= edst)
									goto overflow;

								*dst++ = (char)*(tk - 1);
								break;
							}
						}

						// If 'd' != NULL, copy string to destination
						if (d != NULL)
						{
strcopy:
							DEBUG printf("d='%s'\n", d);

							while (*d != EOS)
							{
								if (dst >= edst)
									goto overflow;
								else
									*dst++ = *d++;
							}
						}
					}
				}
			}
		}
	}

	*dst = EOS;
	DEBUG { printf("ExM: dst=\"%s\"\n", dest); }
	return OK;

overflow:
	*dst = EOS;
	DEBUG printf("*** OVERFLOW LINE ***\n%s\n", dest);
	return fatal("line too long as a result of macro expansion");
}


//
// Get next line of text from a macro
//
char * GetNextMacroLine(void)
{
	unsigned source_addr;

	IMACRO * imacro = cur_inobj->inobj.imacro;
//	LONG * strp = imacro->im_nextln;
	struct LineList * strp = imacro->im_nextln;

	if (strp == NULL)						// End-of-macro
		return NULL;

//	imacro->im_nextln = (LONG *)*strp;
	imacro->im_nextln = strp->next;
//	ExpandMacro((char *)(strp + 1), imacro->im_lnbuf, LNSIZ);
	ExpandMacro(strp->line, imacro->im_lnbuf, LNSIZ);

	return imacro->im_lnbuf;
}


//
// Get next line of text from a repeat block
//
char * GetNextRepeatLine(void)
{

	IREPT * irept = cur_inobj->inobj.irept;
	LONG * strp = irept->ir_nextln;			// initial null

	// Do repeat at end of .rept block's string list
	if (strp == NULL)
	{
		DEBUG printf("back-to-top-of-repeat-block count=%d\n", (int)irept->ir_count);
		irept->ir_nextln = irept->ir_firstln;	// copy first line

		if (irept->ir_count-- == 0)
		{
			DEBUG printf("end-repeat-block\n");
			return NULL;
		}

		strp = irept->ir_nextln;
	}

	strcpy(irbuf, (char *)(irept->ir_nextln + 1));
	DEBUG printf("repeat line='%s'\n", irbuf);
	irept->ir_nextln = (LONG *)*strp;

	return irbuf;
}


//
// Include a source file used at the root, and for ".include" files
//
int include(int handle, char * fname)
{
	IFILE * ifile;
	INOBJ * inobj;
	FILEREC * fr;

	// Verbose mode
	if (verb_flag)
		printf("[include: %s, cfileno=%u]\n", fname, cfileno);

	// Alloc and initialize include-descriptors
	inobj = a_inobj(SRC_IFILE);
	ifile = inobj->inobj.ifile;

	ifile->ifhandle = handle;				// Setup file handle
	ifile->ifind = ifile->ifcnt = 0;		// Setup buffer indices
	ifile->ifoldlineno = curlineno;			// Save old line number
	ifile->ifoldfname = curfname;			// Save old filename
	ifile->ifno = cfileno;					// Save old file number

//	cfileno = filecount++;					// Compute new file number
	// NB: This *must* be preincrement, we're adding one to the filecount here!
	cfileno = ++filecount;					// Compute NEW file number
	curfname = strdup(fname);				// Set current filename (alloc storage)
	curlineno = 0;							// Start on line zero

	// Add another file to the file-record
	fr = (FILEREC *)malloc(sizeof(FILEREC));
	fr->frec_next = NULL;
	fr->frec_name = curfname;

	if (last_fr == NULL)
		filerec = fr;						// Add first filerec 
	else
		last_fr->frec_next = fr;			// Append to list of filerecs 

	last_fr = fr;
	DEBUG printf("[include: curfname: %s, cfileno=%u]\n", curfname, cfileno);

	return OK;
}


//
// Pop the current input level
//
int fpop(void)
{
	IFILE * ifile;
	IMACRO * imacro;
	LONG * p, * p1;
	INOBJ * inobj = cur_inobj;

	if (inobj != NULL)
	{
		// Pop IFENT levels until we reach the conditional assembly context we
		// were at when the input object was entered.
		while (ifent != inobj->in_ifent)
			d_endif();

		tok = inobj->in_otok;				// Restore tok and otok
		etok = inobj->in_etok;

		switch (inobj->in_type)
		{
		case SRC_IFILE:						// Pop and release an IFILE
			if (verb_flag)
				printf("[Leaving: %s]\n", curfname);

			ifile = inobj->inobj.ifile;
			ifile->if_link = f_ifile;
			f_ifile = ifile;
			close(ifile->ifhandle);			// Close source file
if (verb_flag) printf("[fpop (pre):  curfname=%s]\n", curfname);
			curfname = ifile->ifoldfname;	// Set current filename
if (verb_flag) printf("[fpop (post): curfname=%s]\n", curfname);
if (verb_flag) printf("[fpop: (pre)  cfileno=%d ifile->ifno=%d]\n", (int)cfileno, (int)ifile->ifno);
			curlineno = ifile->ifoldlineno;	// Set current line# 
			DEBUG printf("cfileno=%d ifile->ifno=%d\n", (int)cfileno, (int)ifile->ifno);
			cfileno = ifile->ifno;			// Restore current file number
if (verb_flag) printf("[fpop: (post) cfileno=%d ifile->ifno=%d]\n", (int)cfileno, (int)ifile->ifno);
			break;
		case SRC_IMACRO:					// Pop and release an IMACRO
			imacro = inobj->inobj.imacro;
			imacro->im_link = f_imacro;
			f_imacro = imacro;
			break;
		case SRC_IREPT:						// Pop and release an IREPT
			DEBUG printf("dealloc IREPT\n");
			p = inobj->inobj.irept->ir_firstln;

			while (p != NULL)
			{
				p1 = (LONG *)*p;
				p = p1;
			}

			break;
		}

		cur_inobj = inobj->in_link;
		inobj->in_link = f_inobj;
		f_inobj = inobj;
	}

	return 0;
}


//
// Get line from file into buf, return NULL on EOF or ptr to the start of a
// null-term line
//
char * GetNextLine(void)
{
	int i, j;
	char * p, * d;
	int readamt = -1;						// 0 if last read() yeilded 0 bytes
	IFILE * fl = cur_inobj->inobj.ifile;

	for(;;)
	{
		// Scan for next end-of-line; handle stupid text formats by treating
		// \r\n the same as \n. (lone '\r' at end of buffer means we have to
		// check for '\n').
		d = &fl->ifbuf[fl->ifind];

		for(p=d, i=0, j=fl->ifcnt; i<j; i++, p++)
		{
			if (*p == '\r' || *p == '\n')
			{
				i++;

				if (*p == '\r')
				{
					if (i >= j)
						break;	// Need to read more, then look for '\n' to eat 
					else if (p[1] == '\n')
						i++;
				}

				// Cover up the newline with end-of-string sentinel
				*p = '\0';

				fl->ifind += i;
				fl->ifcnt -= i;
				return d;
			}
		}

		// Handle hanging lines by ignoring them (Input file is exhausted, no
		// \r or \n on last line)
		// Shamus: This is retarded. Never ignore any input!
		if (!readamt && fl->ifcnt)
		{
#if 0
			fl->ifcnt = 0;
			*p = '\0';
			return NULL;
#else
			// Really should check to see if we're at the end of the buffer!
			// :-P
			fl->ifbuf[fl->ifind + fl->ifcnt] = '\0';
			fl->ifcnt = 0;
			return &fl->ifbuf[fl->ifind];
#endif
		}

		// Truncate and return absurdly long lines.
		if (fl->ifcnt >= QUANTUM)
		{
			fl->ifbuf[fl->ifind + fl->ifcnt - 1] = '\0';
			fl->ifcnt = 0;
			return &fl->ifbuf[fl->ifind];
		}

		// Relocate what's left of a line to the beginning of the buffer, and
		// read some more of the file in; return NULL if the buffer's empty and
		// on EOF.
		if (fl->ifind != 0)
		{
			p = &fl->ifbuf[fl->ifind];
			d = &fl->ifbuf[fl->ifcnt & 1];

			for(i=0; i<fl->ifcnt; i++)
				*d++ = *p++;

			fl->ifind = fl->ifcnt & 1;
		}

		readamt = read(fl->ifhandle, &fl->ifbuf[fl->ifind + fl->ifcnt], QUANTUM);

		if (readamt < 0)
			return NULL;

		if ((fl->ifcnt += readamt) == 0)
			return NULL;
	}
}


//
// Tokenize a line
//
int TokenizeLine(void)
{
	char * ln = NULL;			// Ptr to current position in line
	char * p;					// Random character ptr
	TOKEN * tk;					// Token-deposit ptr
	int state = 0;				// State for keyword detector
	int j = 0;					// Var for keyword detector
	char c;						// Random char
	VALUE v;					// Random value
	char * nullspot = NULL;		// Spot to clobber for SYMBOL termination
	int stuffnull;				// 1:terminate SYMBOL '\0' at *nullspot
	char c1;
	int stringNum = 0;			// Pointer to string locations in tokenized line

retry:

	if (cur_inobj == NULL)					// Return EOF if input stack is empty
		return TKEOF;

	// Get another line of input from the current input source: a file, a
	// macro, or a repeat-block
	switch (cur_inobj->in_type)
	{
	// Include-file:
	// o  handle EOF;
	// o  bump source line number;
	// o  tag the listing-line with a space;
	// o  kludge lines generated by Alcyon C.
	case SRC_IFILE:
		if ((ln = GetNextLine()) == NULL)
		{
if (verb_flag) printf("TokenizeLine: Calling fpop() from SRC_IFILE...\n");
			fpop();							// Pop input level
			goto retry;						// Try for more lines 
		}

		curlineno++;						// Bump line number
		lntag = SPACE;

		if (as68_flag)
		{
			// AS68 compatibility, throw away all lines starting with
			// back-quotes, tildes, or '*'
			// On other lines, turn the first '*' into a semi-colon.
			if (*ln == '`' || *ln == '~' || *ln == '*')
				*ln = ';';
			else
			{
				for(p=ln; *p!=EOS; p++)
				{
					if (*p == '*')
					{
						*p = ';';
						break;
					}
				}
			}
		}

		break;
	// Macro-block:
	// o  Handle end-of-macro;
	// o  tag the listing-line with an at (@) sign.
	case SRC_IMACRO:
		if ((ln = GetNextMacroLine()) == NULL)
		{
			ExitMacro();					// Exit macro (pop args, do fpop(), etc)
			goto retry;						// Try for more lines...
		}

		lntag = '@';
		break;
	// Repeat-block:
	// o  Handle end-of-repeat-block;
	// o  tag the listing-line with a pound (#) sign.
	case SRC_IREPT:
		if ((ln = GetNextRepeatLine()) == NULL)
		{
if (verb_flag) printf("TokenizeLine: Calling fpop() from SRC_IREPT...\n");
			fpop();
			goto retry;
		}

		lntag = '#';
		break;
	}

	// Save text of the line.  We only do this during listings and within
	// macro-type blocks, since it is expensive to unconditionally copy every
	// line.
	if (lnsave)
		strcpy(lnbuf, ln);

	// General house-keeping
	tok = tokeol;			// Set "tok" to EOL in case of error
	tk = etok;				// Reset token ptr
	stuffnull = 0;			// Don't stuff nulls
	totlines++;				// Bump total #lines assembled

	// See if the entire line is a comment. This is a win if the programmer
	// puts in lots of comments
	if (*ln == '*' || *ln == ';' || ((*ln == '/') && (*(ln + 1) == '/')))
		goto goteol;

	// Main tokenization loop;
	// o  skip whitespace;
	// o  handle end-of-line;
	// o  handle symbols;
	// o  handle single-character tokens (operators, etc.);
	// o  handle multiple-character tokens (constants, strings, etc.).
	for(; *ln!=EOS;)
	{
		// Skip whitespace, handle EOL
		while ((int)chrtab[*ln] & WHITE)
			ln++;

		// Handle EOL, comment with ';'
		if (*ln == EOS || *ln == ';'|| ((*ln == '/') && (*(ln + 1) == '/'))) 
			break;

		// Handle start of symbol. Symbols are null-terminated in place. The
		// termination is always one symbol behind, since there may be no place
		// for a null in the case that an operator immediately follows the name.
		c = chrtab[*ln];

		if (c & STSYM)
		{
			if (stuffnull)			// Terminate old symbol from previous pass
				*nullspot = EOS;

			v = 0;					// Assume no DOT attrib follows symbol
			stuffnull = 1;
			p = nullspot = ln++;	// Nullspot -> start of this symbol

			// Find end of symbol (and compute its length)
			for(j=1; (int)chrtab[*ln]&CTSYM; j++)
				ln++;

			// Handle "DOT" special forms (like ".b") that follow a normal
			// symbol or keyword:
			if (*ln == '.')
			{
				*ln++ = EOS;		// Terminate symbol
				stuffnull = 0;		// And never try it again 

				// Character following the `.' must have a DOT attribute, and
				// the chararacter after THAT one must not have a start-symbol
				// attribute (to prevent symbols that look like, for example,
				// "zingo.barf", which might be a good idea anyway....)
				if ((((int)chrtab[*ln] & DOT) == 0) || ((int)dotxtab[*ln] <= 0))
					return error("[bwsl] must follow `.' in symbol");

				v = (VALUE)dotxtab[*ln++];

				if ((int)chrtab[*ln] & CTSYM)
					return error("misuse of `.', not allowed in symbols");
			}

			// If the symbol is small, check to see if it's really the name of
			// a register.
			if (j <= KWSIZE)
			{
				for(state=0; state>=0;)
				{
					j = (int)tolowertab[*p++];
					j += kwbase[state];

					if (kwcheck[j] != state)
					{
						j = -1;
						break;
					}

					if (*p == EOS || p == ln)
					{
						j = kwaccept[j];
						break;
					}

					state = kwtab[j];
				}
			}
			else
			{
				j = -1;
			}

			// Make j = -1 if user tries to use a RISC register while in 68K mode
			if (!(rgpu || rdsp) && ((TOKEN)j >= KW_R0 && (TOKEN)j <= KW_R31))
			{
				j = -1;
			}

			// Make j = -1 if time, date etc with no preceeding ^^
			// defined, referenced, streq, macdef, date and time
			switch ((TOKEN)j)
			{
			case 112:   // defined
			case 113:   // referenced
			case 118:   // streq
			case 119:   // macdef
			case 120:   // time
			case 121:   // date
				j = -1;
			}

			// If not tokenized keyword OR token was not found
			if (j < 0 || state < 0)
			{
				*tk++ = SYMBOL;
//#warning
//problem here: nullspot is a char * but TOKEN is a uint32_t. On a 64-bit
//system, this will cause all kinds of mischief.
#if 0
				*tk++ = (TOKEN)nullspot;
#else
				string[stringNum] = nullspot;
				*tk++ = stringNum;
				stringNum++;
#endif
			}
			else
			{
				*tk++ = (TOKEN)j;
				stuffnull = 0;
			}

			if (v)							// Record attribute token (if any)
				*tk++ = (TOKEN)v;

			if (stuffnull)					// Arrange for string termination on next pass
				nullspot = ln;

			continue;
		}

		// Handle identity tokens
		if (c & SELF)
		{
			*tk++ = *ln++;
			continue;
		}

		// Handle multiple-character tokens
		if (c & MULTX)
		{
			switch (*ln++)
			{
			case '!':		// ! or != 
				if (*ln == '=')
				{
					*tk++ = NE;
					++ln;
				}
				else
					*tk++ = '!';

				continue;
			case '\'':		// 'string' 
			case '\"':		// "string" 
				c1 = ln[-1];
				*tk++ = STRING;
//#warning
// More char * stuffing (8 bytes) into the space of 4 (TOKEN).
// Need to figure out how to fix this crap.
#if 0
				*tk++ = (TOKEN)ln;
#else
				string[stringNum] = ln;
				*tk++ = stringNum;
				stringNum++;
#endif

				for(p=ln; *ln!=EOS && *ln!=c1;)
				{
					c = *ln++;

					if (c == '\\')
					{
						switch (*ln++)
						{
						case EOS:
							return(error("unterminated string"));
						case 'e':
							c = '\033';
							break;
						case 'n':
							c = '\n';
							break;
						case 'b':
							c = '\b';
							break;
						case 't':
							c = '\t';
							break;
						case 'r':
							c = '\r';
							break;
						case 'f':
							c = '\f';
							break;
						case '\"':
							c = '\"';
							break;
						case '\'':
							c = '\'';
							break;
						case '\\':
							c = '\\';
							break;
						default:
							warn("bad backslash code in string");
							ln--;
							break;
						}
					}

					*p++ = c;
				}

				if (*ln++ != c1)
					return error("unterminated string");

				*p++ = EOS;
				continue;
			case '$':		// $, hex constant
				if ((int)chrtab[*ln] & HDIGIT)
				{
					v = 0;

					while ((int)hextab[*ln] >= 0)
						v = (v << 4) + (int)hextab[*ln++];

					if (*ln == '.')
					{
						if ((*(ln + 1) == 'b') || (*(ln + 1) == 'B'))
						{
							v &= 0x000000FF;
							ln += 2;
						}

						if ((*(ln + 1) == 'w') || (*(ln + 1) == 'W'))
						{
							v &= 0x0000FFFF;
							ln += 2;
						}

						if ((*(ln + 1) == 'l') || (*(ln + 1) == 'L'))
						{
							ln += 2;
						}
					}

					*tk++ = CONST;
					*tk++ = v;
				}
				else
					*tk++ = '$';

				continue;
			case '<':		// < or << or <> or <= 
				switch (*ln)
				{
				case '<':
					*tk++ = SHL;
					++ln;
					continue;
				case '>':
					*tk++ = NE;
					++ln;
					continue;
				case '=':
					*tk++ = LE;
					++ln;
					continue;
				default:
					*tk++ = '<';
					continue;
				}
			case ':':		// : or ::
				if (*ln == ':')
				{
					*tk++ = DCOLON;
					++ln;
				}
				else
					*tk++ = ':';

				continue;
			case '=':		// = or == 
				if (*ln == '=')
				{
					*tk++ = DEQUALS;
					++ln;
				}
				else
					*tk++ = '=';

				continue;
			case '>':		// > or >> or >= 
				switch (*ln)
				{
				case '>':
					*tk++ = SHR;
					ln++;
					continue;
				case '=':
					*tk++ = GE;
					ln++;
					continue;
				default:
					*tk++ = '>';
					continue;
				}
			case '%':		// % or binary constant 
				if (*ln < '0' || *ln > '1')
				{
					*tk++ = '%';
					continue;
				}

				v = 0;

				while (*ln >= '0' && *ln <= '1')
					v = (v << 1) + *ln++ - '0';

				if (*ln == '.')
				{
					if ((*(ln + 1) == 'b') || (*(ln + 1) == 'B'))
					{
						v &= 0x000000FF;
						ln += 2;
					}

					if ((*(ln + 1) == 'w') || (*(ln + 1) == 'W'))
					{
						v &= 0x0000FFFF;
						ln += 2;
					}

					if ((*(ln + 1) == 'l') || (*(ln + 1) == 'L'))
					{
						ln += 2;
					}
				}

				*tk++ = CONST;
				*tk++ = v;
				continue;
			case '@':		// @ or octal constant 
				if (*ln < '0' || *ln > '7')
				{
					*tk++ = '@';
					continue;
				}

				v = 0;

				while (*ln >= '0' && *ln <= '7')
					v = (v << 3) + *ln++ - '0';

				if (*ln == '.')
				{
					if ((*(ln+1) == 'b') || (*(ln+1) == 'B'))
					{
						v &= 0x000000FF;
						ln += 2;
					}

					if ((*(ln+1) == 'w') || (*(ln+1) == 'W'))
					{
						v &= 0x0000FFFF;
						ln += 2;
					}

					if ((*(ln+1) == 'l') || (*(ln+1) == 'L'))
					{
						ln += 2;
					}
				}

				*tk++ = CONST;
				*tk++ = v;
				continue;
			case '^':		// ^ or ^^ <operator-name>
				if (*ln != '^')
				{
					*tk++ = '^';
					continue;
				}

				if (((int)chrtab[*++ln] & STSYM) == 0)
				{
					error("invalid symbol following ^^");
					continue;
				}

				p = ln++;

				while ((int)chrtab[*ln] & CTSYM)
					++ln;

				for(state=0; state>=0;)
				{
					// Get char, convert to lowercase 
					j = *p++;

					if (j >= 'A' && j <= 'Z')
						j += 0x20;

					j += kwbase[state];

					if (kwcheck[j] != state)
					{
						j = -1;
						break;
					}

					if (*p == EOS || p == ln)
					{
						j = kwaccept[j];
						break;
					}

					state = kwtab[j];
				}

				if (j < 0 || state < 0)
				{
					error("unknown symbol following ^^");
					continue;
				}

				*tk++ = (TOKEN)j;
				continue;
			default:
				interror(2);	// Bad MULTX entry in chrtab
				continue;
			}
		}

		// Handle decimal constant
		if (c & DIGIT)
		{
			v = 0;

			while ((int)chrtab[*ln] & DIGIT)
				v = (v * 10) + *ln++ - '0';

			// See if there's a .[bwl] after the constant & deal with it if so
			if (*ln == '.')
			{
				if ((*(ln + 1) == 'b') || (*(ln + 1) == 'B'))
				{
					v &= 0x000000FF;
					ln += 2;
				}
				else if ((*(ln + 1) == 'w') || (*(ln + 1) == 'W'))
				{
					v &= 0x0000FFFF;
					ln += 2;
				}
				else if ((*(ln + 1) == 'l') || (*(ln + 1) == 'L'))
				{
					ln += 2;
				}
			}

			*tk++ = CONST;
			*tk++ = v;
//printf("CONST: %i\n", v);
			continue;
		}

		// Handle illegal character
		return error("illegal character");
	}

	// Terminate line of tokens and return "success."

goteol:
	tok = etok;								// Set tok to beginning of line

	if (stuffnull)							// Terminate last SYMBOL
		*nullspot = EOS;

	*tk++ = EOL;

	return OK;
}


//
// .GOTO <label>	goto directive
// 
// The label is searched for starting from the first line of the current,
// enclosing macro definition. If no enclosing macro exists, an error is
// generated.
// 
// A label is of the form:
// 
// :<name><whitespace>
// 
// The colon must appear in column 1.  The label is stripped prior to macro
// expansion, and is NOT subject to macro expansion.  The whitespace may also
// be EOL.
//
//int d_goto(WORD siz) {
//int d_goto(void)
int d_goto(WORD unused)
{
	char * s1, * s2;

	// Setup for the search
	if (*tok != SYMBOL)
		return error("missing label");

//	sym = (char *)tok[1];
	char * sym = string[tok[1]];
	tok += 2;

	if (cur_inobj->in_type != SRC_IMACRO)
		return error("goto not in macro");

	IMACRO * imacro = cur_inobj->inobj.imacro;
//	defln = (LONG *)imacro->im_macro->svalue;
	struct LineList * defln = imacro->im_macro->lineList;

	// Find the label, starting with the first line.
//	for(; defln!=NULL; defln=(LONG *)*defln)
	for(; defln!=NULL; defln=defln->next)
	{
//		if (*(char *)(defln + 1) == ':')
		if (defln->line[0] == ':')
		{
			// Compare names (sleazo string compare)
			// This string compare is not right. Doesn't check for lengths.
			// (actually it does, but in a crappy, unclear way.)
#warning "!!! Bad string comparison !!!"
			s1 = sym;
//			s2 = (char *)(defln + 1) + 1;
			s2 = defln->line;

			while (*s1 == *s2)
			{
				if (*s1 == EOS)
					break;
				else
				{
					s1++;
					s2++;
				}
			}

			// Found the label, set new macro next-line and return.
			if ((*s2 == EOS) || ((int)chrtab[*s2] & WHITE))
			{
				imacro->im_nextln = defln;
				return 0;
			}
		}
	}

	return error("goto label not found");
}


void DumpTokenBuffer(void)
{
	TOKEN * t;
	printf("Tokens [%X]: ", sloc);

	for(t=tokbuf; *t!=EOL; t++)
	{
		if (*t == COLON)
			printf("[COLON]");
		else if (*t == CONST)
		{
			t++;
			printf("[CONST: $%X]", (uint32_t)*t);
		}
		else if (*t == ACONST)
			printf("[ACONST]");
		else if (*t == STRING)
		{
			t++;
			printf("[STRING:\"%s\"]", string[*t]);
		}
		else if (*t == SYMBOL)
		{
			t++;
			printf("[SYMBOL:\"%s\"]", string[*t]);
		}
		else if (*t == EOS)
			printf("[EOS]");
		else if (*t == TKEOF)
			printf("[TKEOF]");
		else if (*t == DEQUALS)
			printf("[DEQUALS]");
		else if (*t == SET)
			printf("[SET]");
		else if (*t == REG)
			printf("[REG]");
		else if (*t == DCOLON)
			printf("[DCOLON]");
		else if (*t == GE)
			printf("[GE]");
		else if (*t == LE)
			printf("[LE]");
		else if (*t == NE)
			printf("[NE]");
		else if (*t == SHR)
			printf("[SHR]");
		else if (*t == SHL)
			printf("[SHL]");
		else if (*t == UNMINUS)
			printf("[UNMINUS]");
		else if (*t == DOTB)
			printf("[DOTB]");
		else if (*t == DOTW)
			printf("[DOTW]");
		else if (*t == DOTL)
			printf("[DOTL]");
		else if (*t == DOTI)
			printf("[DOTI]");
		else if (*t == ENDEXPR)
			printf("[ENDEXPR]");
		else if (*t == CR_DEFINED)
			printf("[CR_DEFINED]");
		else if (*t == CR_REFERENCED)
			printf("[CR_REFERENCED]");
		else if (*t == CR_STREQ)
			printf("[CR_STREQ]");
		else if (*t == CR_MACDEF)
			printf("[CR_MACDEF]");
		else if (*t == CR_TIME)
			printf("[CR_TIME]");
		else if (*t == CR_DATE)
			printf("[CR_DATE]");
		else if (*t >= 0x20 && *t <= 0x2F)
			printf("[%c]", (char)*t);
		else if (*t >= 0x3A && *t <= 0x3F)
			printf("[%c]", (char)*t);
		else if (*t >= 0x80 && *t <= 0x87)
			printf("[D%u]", ((uint32_t)*t) - 0x80);
		else if (*t >= 0x88 && *t <= 0x8F)
			printf("[A%u]", ((uint32_t)*t) - 0x88);
		else
			printf("[%X:%c]", (uint32_t)*t, (char)*t);
	}

	printf("[EOL]\n");
}

