//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// DEBUG.C - Debugging Messages
// Copyright (C) 199x Landon Dyer, 2011-2012 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source Utilised with the Kind Permission of Landon Dyer
//

#include "debug.h"
#include "amode.h"
#include "direct.h"
#include "mark.h"
#include "sect.h"
#include "token.h"


static int siztab[4] = { 3, 5, 9, 9 };


//
// Print 'c' Visibly
//
int visprt(char c)
{
	if (c < 0x20 || c >= 0x7F)
		putchar('.');
	else
		putchar(c);

	return 0;
}


//
// Print expression, return ptr to just past the ENDEXPR
//
TOKEN * printexpr(TOKEN * tp)
{
	if (tp != NULL)
	{
		while (*tp != ENDEXPR)
		{
			switch ((int)*tp++)
			{
			case SYMBOL:
//				printf("`%s' ", ((SYM *)*tp)->sname);
				printf("`%s' ", symbolPtr[*tp]->sname);
				tp++;
				break;
			case CONST:
				printf("$%X ", *tp++);
				break;
			case ACONST:
				printf("ACONST=($%X,$%X) ", *tp, tp[1]);
				tp += 2;
				break;
			default:
				printf("%c ", (char)tp[-1]);
				break;
			}
		}
	}

//	printf(";\n");
	return tp + 1;
}


//
// Dump data in a chunk (and maybe others) in the appropriate format
//
int chdump(CHUNK * ch, int format)
{
	while (ch != NULL)
	{
		printf("chloc=$%08X, chsize=$%X\n", ch->chloc, ch->ch_size);
		mdump(ch->chptr, ch->ch_size, format, ch->chloc);
		ch = ch->chnext;
	}

	return 0;
}


//
// Dump fixup records in printable format
//
int fudump(CHUNK * ch)
{
	PTR p;
	char * ep;
	WORD attr, esiz;
	WORD line, file;
	LONG loc;

	for(; ch!=NULL;)
	{
		p.cp = ch->chptr;
		ep = ch->chptr + ch->ch_size;

		while (p.cp < ep)
		{
			attr = *p.wp++;
			loc = *p.lp++;
			file = *p.wp++;
			line = *p.wp++;

			printf("$%04X $%08X %d.%d: ", (int)attr, loc, (int)file, (int)line);

			if (attr & FU_EXPR)
			{
				esiz = *p.wp++;
				printf("(%d long) ", (int)esiz);
				p.tk = printexpr(p.tk);
			}
			else
			{
//				printf("`%s' ;\n", (*p.sy)->sname);
				printf("`%s' ;", (*p.sy)->sname);
				p.sy++;
			}

			if ((attr & 0x0F00) == FU_JR)
			{
				printf(" *=$%X", *p.lp);
				p.lp++;
			}

			printf("\n");
		}

		ch = ch->chnext;
	}

	return 0;
}


//
// Dump marks
//
int mudump(void)
{
	MCHUNK * mch;
	PTR p;
	WORD from;
	WORD w;
	LONG loc;
	SYM * symbol;

	from = 0;

	for(mch=firstmch; mch!=NULL; mch=mch->mcnext)
	{
		printf("mch=$%08X mcptr=$%08X mcalloc=$%X mcused=$%X\n",
			(uint32_t)mch,
			(mch->mcptr.lw),
			mch->mcalloc,
			(mch->mcused));

		p = mch->mcptr;
		
		for(;;)
		{
			w = *p.wp++;

			if (w & MCHEND)
				break;

			symbol = NULL;
			loc = *p.lp++;

			if (w & MCHFROM)
				from = *p.wp++;

			if (w & MSYMBOL)
				symbol = *p.sy++;

			printf("m=$%04X to=%d loc=$%X from=%d siz=%s",
					w, w & 0x00FF, loc, from, (w & MLONG) ? "long" : "word");

			if (symbol != NULL)
				printf(" sym=`%s'", symbol->sname);

			printf("\n");
		}
	}

	return 0;
}


//
// Dump memory from 'start' for 'count' bytes; `flg' is the following ORed together:
// 0 - bytes
// 1 - words
// 2 - longwords
// 
// if `base' is not -1, then print it at the start of each line, incremented accordingly.
//
int mdump(char * start, LONG count, int flg, LONG base)
{
	int i, j, k;
	j = 0;

	for(i=0; i<(int)count;)
	{
		if ((i & 15) == 0)
		{
			if (j < i)
			{
				printf("  ");

				while(j < i)
				visprt(start[j++]);

				putchar('\n');
			}

			j = i;

			if (base != -1)
				printf("%08X  ", base);
		}

		switch (flg & 3)
		{
		case 0:
			printf("%02X ", start[i] & 0xff);
			++i;
			break;
		case 1:
			printf("%02X%02X ", start[i] & 0xff, start[i+1] & 0xff);
			i += 2;
			break;
		case 2:
			printf("%02X%02X%02X%02X ", start[i] & 0xff, start[i+1] & 0xff,
				start[i+2] & 0xff, start[i+3] & 0xff);
			i += 4;
			break;
		case 3:
			break;
		}

		if (base != -1)
			base += 1 << (flg & 3);
	}

	// Print remaining bit of ascii; the hairy expression computes the number of
	// spaces to print to make the ascii line up nicely.
	if (j != i)
	{
		k = ((16 - (i - j)) / (1 << (flg & 3))) * siztab[flg & 3];

		while(k--)
			putchar(' ');

		printf("  ");

		while(j < i)
			visprt(start[j++]);

		putchar('\n');
	}

	return 0;
}


//
// Dump list of tokens on stdout in printable form
//
int dumptok(TOKEN * tk)
{
	int flg = 0;

	while (*tk != EOL)
	{
		if (flg++)
			printf(" ");

		if (*tk >= 128)
		{
			printf("REG=%u", *tk++ - 128);
			continue;
		}

		switch ((int)*tk++)
		{
		case CONST:                                        // CONST <value>
			printf("CONST=%u", *tk++);
			break;
		case STRING:                                       // STRING <address>
			printf("STRING='%s'", string[*tk++]);
			break;
		case SYMBOL:                                       // SYMBOL <address> 
			printf("SYMBOL='%s'", string[*tk++]);
			break;
		case EOL:                                          // End of line 
			printf("EOL");
			break;
		case TKEOF:                                        // End of file (or macro)
			printf("TKEOF");
			break;
		case DEQUALS:                                      // == 
			printf("DEQUALS");
			break;
		case DCOLON:                                       // :: 
			printf("DCOLON");
			break;
		case GE:                                           // >= 
			printf("GE");
			break;
		case LE:                                           // <= 
			printf("LE");
			break;
		case NE:                                           // <> or != 
			printf("NE");
			break;
		case SHR:                                          // >> 
			printf("SHR");
			break;
		case SHL:                                          // << 
			printf("SHL");
			break;
		default:
			printf("%c", (int)tk[-1]);
			break;
		}
	}

	printf("\n");

	return 0;
}


//
// Dump Everything
//
int dump_everything(void)
{
	int i;

	for(i=1; i<NSECTS; i++)
	{
		if (sect[i].scattr & SUSED)
		{
			printf("Section %d sloc=$%X\n", i, sect[i].sloc);
			printf("Code:\n");
			chdump(sect[i].sfcode, 1);

			printf("Fixup:\n");
			fudump(sect[i].sffix);

			printf("\n");
		}
	}

	printf("\nMarks:\n");
	mudump();								// Dump marks
//	printf("Total memory allocated=$%X\n", amemtot);

	return 0;
}
