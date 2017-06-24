//
// RMAC - Reboot's Macro Assembler for all Atari computers
// ERROR.C - Error Handling
// Copyright (C) 199x Landon Dyer, 2011-2017 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source utilised with the kind permission of Landon Dyer
//

#include "error.h"
#include <stdarg.h>
#include "listing.h"
#include "token.h"

int errcnt;						// Error count
char * err_fname;				// Name of error message file

static long unused;				// For supressing 'write' warnings


//
// Report error if not at EOL
//
int at_eol(void)
{
	if (*tok != EOL)
		error("syntax error. expected EOL, found $%X ('%c')", *tok, *tok);

	return 0;
}


//
// Cannot create a file
//
void cantcreat(const char * fn)
{
	printf("cannot create: '%s'\n", fn);
	exit(1);
}


//
// Setup for error message
//  o  Create error listing file (if necessary)
//  o  Set current filename
//
void err_setup(void)
{
	char fnbuf[FNSIZ];

	if (err_fname != NULL)
	{
		strcpy(fnbuf, err_fname);

		if (*fnbuf == EOS)
			strcpy(fnbuf, firstfname);

		err_fname = NULL;

		if ((err_fd = open(fnbuf, _OPEN_FLAGS, _PERM_MODE)) < 0)
			cantcreat(fnbuf);

		err_flag = 1;
	}
}


//
// Display error message (uses printf() style variable arguments)
//
int error(const char * text, ...)
{
	char buf[EBUFSIZ];
	char buf1[EBUFSIZ];

	err_setup();

	va_list arg;
	va_start(arg, text);
	vsprintf(buf, text, arg);
	va_end(arg);

	if (listing > 0)
		ship_ln(buf);

	sprintf(buf1, "%s %d: Error: %s\n", curfname, curlineno, buf);

	if (err_flag)
		unused = write(err_fd, buf1, (LONG)strlen(buf1));
	else
		printf("%s", buf1);

	taglist('E');
	errcnt++;

	return ERROR;
}


//
// Display warning message (uses printf() style variable arguments)
//
int warn(const char * text, ...)
{
	char buf[EBUFSIZ];
	char buf1[EBUFSIZ];

	err_setup();
	va_list arg;
	va_start(arg, text);
	vsprintf(buf, text, arg);
	va_end(arg);

	if (listing > 0)
		ship_ln(buf);

	sprintf(buf1, "%s %d: Warning: %s\n", curfname, curlineno, buf);

	if (err_flag)
		unused = write(err_fd, buf1, (LONG)strlen(buf1));
	else
		printf("%s", buf1);

	taglist('W');

	return OK;
}


int fatal(const char * s)
{
	char buf[EBUFSIZ];

	err_setup();

	if (listing > 0)
		ship_ln(s);

	sprintf(buf, "%s %d: Fatal: %s\n", curfname, curlineno, s);

	if (err_flag)
		unused = write(err_fd, buf, (LONG)strlen(buf));
	else
		printf("%s", buf);

	exit(1);
}


int interror(int n)
{
	char buf[EBUFSIZ];

	err_setup();
	sprintf(buf, "%s %d: Internal error #%d\n", curfname, curlineno, n);

	if (listing > 0)
		ship_ln(buf);

	if (err_flag)
		unused = write(err_fd, buf, (LONG)strlen(buf));
	else
		printf("%s", buf);

	exit(1);
}

