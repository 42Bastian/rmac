////////////////////////////////////////////////////////////////////////////////////////////////////
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// EAGEN.C - Effective Address Code Generation
// Copyright (C) 199x Landon Dyer, 2011 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source Utilised with the Kind Permission of Landon Dyer

#include "rmac.h"
#include "amode.h"
#include "sect.h"
#include "mark.h"
#include "error.h"
#include "mach.h"
#include "risca.h"

#define eaNgen    ea0gen
#define amN       am0
#define aNexattr  a0exattr
#define aNexval   a0exval
#define aNexpr    a0expr
#define aNixreg   a0ixreg
#define aNixsiz   a0ixsiz
#include "eagen0.c"

#define eaNgen    ea1gen
#define amN       am1
#define aNexattr  a1exattr
#define aNexval   a1exval
#define aNexpr    a1expr
#define aNixreg   a1ixreg
#define aNixsiz   a1ixsiz
#include "eagen0.c"
