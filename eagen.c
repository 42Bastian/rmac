//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// EAGEN.C - Effective Address Code Generation
// Copyright (C) 199x Landon Dyer, 2017 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source utilised with the kind permission of Landon Dyer
//

#include "rmac.h"
#include "amode.h"
#include "sect.h"
#include "mark.h"
#include "error.h"
#include "mach.h"
#include "riscasm.h"

#define eaNgen    ea0gen
#define amN       am0
#define aNexattr  a0exattr
#define aNexval   a0exval
#define aNexpr    a0expr
#define aNixreg   a0ixreg
#define aNixsiz   a0ixsiz
#define AnESYM    a0esym
#include "eagen0.c"

#define eaNgen    ea1gen
#define amN       am1
#define aNexattr  a1exattr
#define aNexval   a1exval
#define aNexpr    a1expr
#define aNixreg   a1ixreg
#define aNixsiz   a1ixsiz
#define AnESYM    a1esym
#include "eagen0.c"
