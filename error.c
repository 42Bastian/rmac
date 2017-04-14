//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// ERROR.C - Error Handling
// Copyright (C) 199x Landon Dyer, 2017 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source utilised with the kind permission of Landon Dyer
//

#include "error.h"
#include "token.h"
#include "listing.h"

int errcnt;						// Error count
char * err_fname;				// Name of error message file

static const char nl[] = "\n";
static long unused;				// For supressing 'write' warnings


//
// Report error if not at EOL
//
int at_eol(void)
{
	char msg[256];

	if (*tok != EOL)
	{
		sprintf(msg, "syntax error. expected EOL, found $%X ('%c')", *tok, *tok);
		error(msg);
	}

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
// o  Create error listing file (if necessary)
// o  Set current filename
//
void err_setup(void)
{
	char fnbuf[FNSIZ];

// This seems like it's unnecessary, as token.c seems to take care of this all by itself.
// Can restore if it's really needed. If not, into the bit bucket it goes. :-)
//	setfnum(cfileno);

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
// Display error message
//
int error(const char * s)
{
	char buf[EBUFSIZ];
	unsigned int length;

	err_setup();

	if (listing > 0)
		ship_ln(s);

	sprintf(buf, "%s %d: Error: %s%s", curfname, curlineno, s, nl);
	length = strlen(buf);

	if (err_flag)
		unused = write(err_fd, buf, length);
	else
		printf("%s", buf);

	taglist('E');
	errcnt++;

	return ERROR;
}


int errors(const char * s, char * s1)
{
	char buf[EBUFSIZ];
	char buf1[EBUFSIZ];

	err_setup();
	sprintf(buf, s, s1);

	if (listing > 0)
		ship_ln(buf);

	sprintf(buf1, "%s %d: Error: %s%s", curfname, curlineno, buf, nl);

	if (err_flag)
		unused = write(err_fd, buf1, (LONG)strlen(buf1));
	else
		printf("%s", buf1);

	taglist('E');
	++errcnt;

	return ERROR;
}


int warn(const char * s)
{
	char buf[EBUFSIZ];

	err_setup();

	if (listing > 0)
		ship_ln(s);

	sprintf(buf, "%s %d: Warning: %s%s", curfname, curlineno, s, nl);

	if (err_flag)
		unused = write(err_fd, buf, (LONG)strlen(buf));
	else
		printf("%s", buf);

	taglist('W');

	return OK;
}


int warns(const char * s, char * s1)
{
	char buf[EBUFSIZ];
	char buf1[EBUFSIZ];

	err_setup();
	sprintf(buf, s, s1);

	if (listing > 0)
		ship_ln(s);

	sprintf(buf1, "%s %d: Warning: %s%s", curfname, curlineno, buf, nl);

	if (err_flag)
		unused = write(err_fd, buf1, (LONG)strlen(buf1));
	else
		printf("%s", buf1);

	taglist('W');

	return OK;
}


int warni(const char * s, unsigned i)
{
	char buf[EBUFSIZ];
	char buf1[EBUFSIZ];

	err_setup();
	sprintf(buf, s, i);

	if (listing > 0)
		ship_ln(buf);

	sprintf(buf1, "%s %d: Warning: %s%s", curfname, curlineno, buf, nl);

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

	sprintf(buf, "%s %d: Fatal: %s%s", curfname, curlineno, s, nl);

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
	sprintf(buf, "%s %d: Internal Error Number %d%s", curfname, curlineno, n, nl);

	if (listing > 0)
		ship_ln(buf);

	if (err_flag)
		unused = write(err_fd, buf, (LONG)strlen(buf));
	else
		printf("%s", buf);

	exit(1);
}

