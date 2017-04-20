#
# RMAC - Reboot's Macro Assembler for the Atari Jaguar
# Copyright (C) 199x Landon Dyer, 2011 Reboot & Friends
# MAKEFILE for *nix
#

STD := c99

# Detect old, shitty platforms that aren't C99/POSIX compliant
OSTYPE := $(shell uname -a)

# Should catch MinGW
ifeq "$(findstring MINGW, $(OSTYPE))" "MINGW"
STD := gnu99
endif

# If we're cross compiling using MXE, we're still fooooooooooked
ifneq "$(CROSS)" ""
STD := gnu99
endif


rm = /bin/rm -f
CC = $(CROSS)gcc
HOSTCC = gcc

CFLAGS = -std=$(STD) -D_DEFAULT_SOURCE -g -D__GCCUNIX__ -I. -O2

SRCS = 6502.c amode.c debug.c direct.c eagen.c error.c expr.c listing.c mach.c macro.c mark.c object.c procln.c riscasm.c rmac.c sect.c symbol.c token.c

OBJS = 6502.o amode.o debug.o direct.o eagen.o error.o expr.o listing.o mach.o macro.o mark.o object.o procln.o riscasm.o rmac.o sect.o symbol.o token.o

#
# Build everything
#

all : mntab.h 68ktab.h kwtab.h risckw.h 6502.h rmac
	@echo
	@echo "Don't forget to bump the version number before commiting!"
	@echo

#
# Generated sources for state machines and keyword, directive and mnemonic
# definitions
#

mntab.h : mntab 68kmn kwgen
	cat mntab 68kmn | ./kwgen mn >mntab.h

68ktab.h 68kmn : 68ktab 68ktab 68kgen
	./68kgen 68kmn <68ktab >68ktab.h

kwtab.h : kwtab kwgen
	./kwgen kw <kwtab >kwtab.h

6502.h : 6502.tbl kwgen
	./kwgen mp <6502.tbl >6502kw.h

risckw.h : kwtab kwgen
	./kwgen mr <risctab >risckw.h

#
# Build tools
#

kwgen.o : kwgen.c
	$(HOSTCC) $(CFLAGS) -c kwgen.c

kwgen : kwgen.o
	$(HOSTCC) $(CFLAGS) -o kwgen kwgen.o

68kgen.o : 68kgen.c
	$(HOSTCC) $(CFLAGS) -c 68kgen.c

68kgen : 68kgen.o
	$(HOSTCC) $(CFLAGS) -o 68kgen 68kgen.o

#
# Build RMAC executable
#

6502.o : 6502.c 6502.h
	$(CC) $(CFLAGS) -c 6502.c

amode.o : amode.c amode.h
	$(CC) $(CFLAGS) -c amode.c

debug.o : debug.c debug.h
	$(CC) $(CFLAGS) -c debug.c

direct.o : direct.c direct.h
	$(CC) $(CFLAGS) -c direct.c

eagen.o : eagen.c eagen.h eagen0.c
	$(CC) $(CFLAGS) -c eagen.c

error.o : error.c error.h
	$(CC) $(CFLAGS) -c error.c

expr.o : expr.c expr.h
	$(CC) $(CFLAGS) -c expr.c

listing.o : listing.c listing.h
	$(CC) $(CFLAGS) -c listing.c

mach.o : mach.c mach.h
	$(CC) $(CFLAGS) -c mach.c

macro.o : macro.c macro.h
	$(CC) $(CFLAGS) -c macro.c

mark.o : mark.c mark.h
	$(CC) $(CFLAGS) -c mark.c

object.o : object.c object.h
	$(CC) $(CFLAGS) -c object.c

procln.o : procln.c procln.h
	$(CC) $(CFLAGS) -c procln.c

risca.o : risca.c risca.h
	$(CC) $(CFLAGS) -c risca.c

rmac.o : rmac.c rmac.h
	$(CC) $(CFLAGS) -c rmac.c

sect.o : sect.c sect.h
	$(CC) $(CFLAGS) -c sect.c

symbol.o : symbol.c symbol.h
	$(CC) $(CFLAGS) -c symbol.c

token.o : token.c token.h
	$(CC) $(CFLAGS) -c token.c

rmac : $(OBJS)
	$(CC) $(CFLAGS) -o rmac $(OBJS)

#
# Clean build environment
#

clean:
	$(rm) $(OBJS) kwgen.o 68kgen.o rmac kwgen 68kgen kwtab.h 68ktab.h mntab.h risckw.h 6502kw.h

#
# Dependencies
#
6502.o: 6502.c direct.h rmac.h symbol.h expr.h error.h mach.h amode.h \
 procln.h token.h riscasm.h sect.h
68kgen.o: 68kgen.c
amode.o: amode.c amode.h rmac.h symbol.h error.h token.h expr.h kwtab.h \
 mntab.h parmode.h
debug.o: debug.c debug.h rmac.h symbol.h amode.h direct.h mark.h sect.h \
 token.h
direct.o: direct.c direct.h rmac.h symbol.h 6502.h error.h expr.h \
 listing.h mach.h amode.h macro.h mark.h procln.h token.h riscasm.h \
 sect.h kwtab.h
eagen.o: eagen.c eagen.h rmac.h symbol.h amode.h sect.h mark.h error.h \
 mach.h riscasm.h eagen0.c
error.o: error.c error.h rmac.h symbol.h token.h listing.h
expr.o: expr.c expr.h rmac.h symbol.h direct.h error.h listing.h mach.h \
 amode.h procln.h token.h riscasm.h sect.h kwtab.h
kwgen.o: kwgen.c
listing.o: listing.c listing.h rmac.h symbol.h version.h token.h procln.h \
 sect.h error.h
mach.o: mach.c mach.h amode.h rmac.h symbol.h eagen.h error.h direct.h \
 procln.h token.h riscasm.h sect.h kwtab.h 68ktab.h
macro.o: macro.c macro.h rmac.h symbol.h debug.h direct.h error.h expr.h \
 listing.h procln.h token.h
mark.o: mark.c mark.h rmac.h symbol.h error.h object.h riscasm.h sect.h
object.o: object.c object.h rmac.h symbol.h error.h mark.h riscasm.h \
 sect.h
procln.o: procln.c procln.h rmac.h symbol.h token.h 6502.h amode.h \
 direct.h error.h expr.h listing.h mach.h macro.h riscasm.h sect.h \
 kwtab.h mntab.h risckw.h 6502kw.h
riscasm.o: riscasm.c riscasm.h rmac.h symbol.h amode.h direct.h error.h \
 expr.h mark.h procln.h token.h sect.h risckw.h kwtab.h
rmac.o: rmac.c rmac.h symbol.h 6502.h debug.h direct.h error.h expr.h \
 listing.h mark.h macro.h object.h procln.h token.h riscasm.h sect.h \
 version.h
sect.o: sect.c sect.h rmac.h symbol.h 6502.h direct.h error.h expr.h \
 listing.h mach.h amode.h mark.h riscasm.h token.h
symbol.o: symbol.c symbol.h error.h rmac.h listing.h object.h procln.h \
 token.h
token.o: token.c token.h rmac.h symbol.h error.h macro.h procln.h sect.h \
 kwtab.h

