//
// Jaguar Object Processor assembler
//
// by James Hammons
// (C) 2019 Underground Software
//

#include "op.h"
#include "direct.h"
#include "error.h"
#include "expr.h"
#include "fltpoint.h"
#include "mark.h"
#include "procln.h"
#include "riscasm.h"
#include "rmac.h"
#include "sect.h"
#include "token.h"

#define DEF_MO
#include "opkw.h"		// For MO_* macros

// Function prototypes
static int HandleBitmap(void);
static int HandleScaledBitmap(void);
static int HandleGPUObject(void);
static int HandleBranch(void);
static int HandleStop(void);
static int HandleNOP(void);
static int HandleJump(void);

// OP assembler vars.
static uint8_t lastObjType;
static uint32_t lastSloc;
static char scratchbuf[4096];
static TOKEN fixupExpr[4] = { CONST, 0, 0, ENDEXPR };
//static PTR fixupPtr = { .tk = (fixupExpr + 1) };		// C99 \o/
static PTR fixupPtr = { (uint8_t *)(fixupExpr + 1) };	// meh, it works

//
// The main Object Processor assembler. Basically just calls the sub functions
// to generate the appropriate code.
//
int GenerateOPCode(int state)
{
	if (!robjproc)
		return error("opcode only valid in OP mode");

	// It's OK to call this before validating state. If the state is invalid, an
	// error will be generated and no object file will be produced, so it
	// doesn't matter if the line number symbols are a little off.
	GENLINENOSYM();

	switch (state)
	{
	case MO_BITMAP:
		return HandleBitmap();
	case MO_SCBITMAP:
		return HandleScaledBitmap();
	case MO_GPUOBJ:
		return HandleGPUObject();
	case MO_BRANCH:
		return HandleBranch();
	case MO_STOP:
		return HandleStop();
	case MO_NOP:
		return HandleNOP();
	case MO_JUMP:
		return HandleJump();
	}

	return error("unknown OP opcode");
}

static inline void GetSymbolUCFromTokenStream(char * s)
{
	strcpy(s, string[tok[1]]);
	strtoupper(s);
	tok += 2;
}

static inline uint64_t CheckFlags(char * s)
{
	GetSymbolUCFromTokenStream(s);

	if (strcmp(scratchbuf, "REFLECT") == 0)
		return 0x01;
	else if (strcmp(scratchbuf, "RMW") == 0)
		return 0x02;
	else if (strcmp(scratchbuf, "TRANS") == 0)
		return 0x04;
	else if (strcmp(scratchbuf, "RELEASE") == 0)
		return 0x08;

	return 0;
}

//
// Define a bitmap object
// Form: bitmap <data>, <xloc>, <yloc>, <dwidth>, <iwidth>, <iheight>, <bpp>,
//              <pallete idx>, <flags>, <firstpix>, <pitch>
//
static int HandleBitmap(void)
{
	uint64_t xpos = 0;
	uint64_t ypos = 0;
	uint64_t iheight = 0;
	uint64_t dwidth = 0;
	uint64_t iwidth = 0;
	uint64_t bpp = 0;
	uint64_t index = 0;
	uint64_t flags = 0;
	uint64_t firstpix = 0;
	uint64_t pitch = 1;

	uint64_t eval;
	uint16_t eattr;
	SYM * esym = 0;

	if ((orgaddr & 0x0F) != 0)
	{
		warn("bitmap not on double phrase boundary");

		// Fixup org address (by emitting a NOP)...
		HandleNOP();

		// We don't need to do a fixup here, because we're guaranteed that the
		// last object, if it was a scaled bitmap *or* regular bitmap, will
		// already be on the correct boundary, so we won't have to fix it up.
	}

	if (expr(exprbuf, &eval, &eattr, &esym) != OK)
		return ERROR;

	if (!(eattr & DEFINED))
		AddFixup(FU_QUAD | FU_OBJDATA, sloc, exprbuf);
	else if (eattr & TDB)
		MarkRelocatable(cursect, sloc, (eattr & TDB), MQUAD, NULL);

	uint64_t dataAddr = eval & 0xFFFFF8;
	uint64_t linkAddr = (orgaddr + 16) & 0x3FFFF8;

	uint64_t * vars[10] = { &xpos, &ypos, &dwidth, &iwidth, &iheight, &bpp, &index, 0, &firstpix, &pitch };

	for(int i=0; i<10; i++)
	{
		// If there are less than 10 arguments after the data address, use
		// defaults for the rest of them
		// N.B.: Should there be a default minimum # of args? Like the first 5?
		if (tok[0] == EOL)
			break;

		CHECK_COMMA;

		if (i != 7)
		{
			if (expr(exprbuf, &eval, &eattr, &esym) != OK)
				return ERROR;

			if (!(eattr & DEFINED))
				return error("bad expression");

			*vars[i] = eval;
		}
		else
		{
			// Handle FLAGs. Can have at most four.
			for(int j=0; j<4; j++)
			{
				if (tok[0] != SYMBOL)
					return error("missing REFLECT, RMW, TRANS, and/or RELEASE");

				flags |= CheckFlags(scratchbuf);

				// Break out if no more symbols...
				if (tok[0] != SYMBOL)
					break;
			}
		}
	}

	ErrorIfNotAtEOL();

	uint64_t p1 = 0x00 | ((ypos * 2) << 3) | (iheight << 14) | (linkAddr << 21) | (dataAddr << 40);
	uint64_t p2 = xpos | (bpp << 12) | (pitch << 15) | (dwidth << 18) | (iwidth << 28) | (index << 38) | (flags << 45) | (firstpix << 49);

	lastSloc = sloc;
	lastObjType = 0;
	D_quad(p1);
	D_quad(p2);

	return OK;
}

//
// Define a scaled bitmap object
// Form: scbitmap <data>, <xloc>, <yloc>, <dwidth>, <iwidth>, <iheight>,
//                <xscale>, <yscale>, <remainder>, <bpp>, <pallete idx>,
//                <flags>, <firstpix>, <pitch>
//
static int HandleScaledBitmap(void)
{
	uint64_t xpos = 0;
	uint64_t ypos = 0;
	uint64_t iheight = 0;
	uint64_t dwidth = 0;
	uint64_t iwidth = 0;
	uint64_t bpp = 0;
	uint64_t index = 0;
	uint64_t flags = 0;
	uint64_t firstpix = 0;
	uint64_t pitch = 1;
	uint64_t xscale = 0;
	uint64_t yscale = 0;
	uint64_t remainder = 0;

	uint64_t eval;
	uint16_t eattr;
	SYM * esym = 0;

	if ((orgaddr & 0x1F) != 0)
	{
		warn("scaled bitmap not on quad phrase boundary");

		// We only have to do a fixup here if the previous object was a bitmap,
		// as it can live on a 16-byte boundary while scaled bitmaps can't. If
		// the previous object was a scaled bitmap, it is guaranteed to have
		// been aligned, therefore no fixup is necessary.
		if (lastObjType == 0)
		{
			*fixupPtr.u64 = (orgaddr + 0x18) & 0xFFFFFFE0;
			AddFixup(FU_QUAD | FU_OBJLINK, lastSloc, fixupExpr);
		}

		switch (orgaddr & 0x1F)
		{
		case 0x08: HandleNOP(); // Emit 3 NOPs
		case 0x10: HandleNOP(); // Emit 2 NOPs
		case 0x18: HandleNOP(); // Emit 1 NOP
		}
	}

	if (expr(exprbuf, &eval, &eattr, &esym) != OK)
		return ERROR;

	if (!(eattr & DEFINED))
		AddFixup(FU_QUAD | FU_OBJDATA, sloc, exprbuf);
	else if (eattr & TDB)
		MarkRelocatable(cursect, sloc, (eattr & TDB), MQUAD, NULL);

	uint64_t dataAddr = eval & 0xFFFFF8;
	uint64_t linkAddr = (orgaddr + 32) & 0x3FFFF8;

	uint64_t * vars[13] = { &xpos, &ypos, &dwidth, &iwidth, &iheight, &xscale, &yscale, &remainder, &bpp, &index, 0, &firstpix, &pitch };

	for(int i=0; i<13; i++)
	{
		// If there are less than 13 arguments after the data address, use
		// defaults for the rest of them
		// N.B.: Should there be a default minimum # of args? Like the first 5?
		if (tok[0] == EOL)
			break;

		CHECK_COMMA;

		if (i != 10)
		{
			if (expr(exprbuf, &eval, &eattr, &esym) != OK)
				return ERROR;

			if (!(eattr & DEFINED))
				return error("bad expression");

			// Handle 3.5 fixed point nums (if any)...
			if ((i >= 5) && (i <= 7))
			{
				if (eattr & FLOAT)			// Handle floats
				{
//					PTR p = { .u64 = &eval };		// C99 \o/
					PTR p = { (uint8_t *)&eval };	// Meh, it works
					eval = DoubleToFixedPoint(*p.dp, 3, 5);
				}
				else
					eval <<= 5;				// Otherwise, it's just an int...
			}

			*vars[i] = eval;
		}
		else
		{
			// Handle FLAGs. Can have at most four.
			for(int j=0; j<4; j++)
			{
				if (tok[0] != SYMBOL)
					return error("missing REFLECT, RMW, TRANS, and/or RELEASE");

				flags |= CheckFlags(scratchbuf);

				// Break out if no more symbols...
				if (tok[0] != SYMBOL)
					break;
			}
		}
	}

	ErrorIfNotAtEOL();

	uint64_t p1 = 0x01 | ((ypos * 2) << 3) | (iheight << 14) | (linkAddr << 21) | (dataAddr << 40);
	uint64_t p2 = xpos | (bpp << 12) | (pitch << 15) | (dwidth << 18) | (iwidth << 28) | (index << 38) | (flags << 45) | (firstpix << 49);
	uint64_t p3 = (xscale & 0xFF) | (yscale & 0xFF) << 8 | (remainder & 0xFF) << 16;

	lastSloc = sloc;
	lastObjType = 1;
	D_quad(p1);
	D_quad(p2);
	D_quad(p3);
	D_quad(0LL);

	return OK;
}

//
// Insert GPU object
// Form: gpuobj <userdata> (bits 3-63 of this object)
//
static int HandleGPUObject(void)
{
	uint64_t eval;
	uint16_t eattr;
	SYM * esym = 0;

	if (expr(exprbuf, &eval, &eattr, &esym) != OK)
		return ERROR;

	if (!(eattr & DEFINED))
		return error("bad expression in data");

	ErrorIfNotAtEOL();

	uint64_t p1 = 0x02 | (eval << 3);

	lastObjType = 2;
	D_quad(p1);

	return OK;
}

//
// Insert a branch object
// Form: branch VC <condition (<, =, >)> <line #>, <link addr>
//       branch OPFLAG, <link addr>
//       branch SECHALF, <link addr>
//
static int HandleBranch(void)
{
	char missingKeyword[] = "missing VC, OPFLAG, or SECHALF in branch";
	uint32_t cc = 0;
	uint32_t ypos = 0;
	uint64_t eval;
	uint16_t eattr;
	SYM * esym = 0;

	if (tok[0] != SYMBOL)
		return error(missingKeyword);

	GetSymbolUCFromTokenStream(scratchbuf);

	if (strcmp(scratchbuf, "VC") == 0)
	{
		switch (*tok++)
		{
		case '=': cc = 0; break;
		case '<': cc = 1; break;
		case '>': cc = 2; break;
		default:
			return error("missing '<', '>', or '='");
		}

		if (expr(exprbuf, &eval, &eattr, &esym) != OK)
			return ERROR;

		if (!(eattr & DEFINED))
			return error("bad expression");

		ypos = (uint32_t)eval;
	}
	else if (strcmp(scratchbuf, "OPFLAG") == 0)
		cc = 3;
	else if (strcmp(scratchbuf, "SECHALF") == 0)
		cc = 4;
	else
		return error(missingKeyword);

	CHECK_COMMA;

	if (expr(exprbuf, &eval, &eattr, &esym) != OK)
		return ERROR;

	if (!(eattr & DEFINED))
		AddFixup(FU_QUAD | FU_OBJLINK, sloc, exprbuf);

	ErrorIfNotAtEOL();

	uint64_t p1 = 0x03 | (cc << 14) | ((ypos * 2) << 3) | ((eval & 0x3FFFF8) << 21);

	lastObjType = 3;
	D_quad(p1);

	return OK;
}

//
// Insert a stop object
// Form: stop
//
static int HandleStop(void)
{
	lastObjType = 4;
	D_quad(4LL);

	return OK;
}

//
// Insert a phrase sized "NOP" in the object list (psuedo-op)
// Form: nop
//
static int HandleNOP(void)
{
	uint64_t eval = (orgaddr + 8) & 0x3FFFF8;
	// This is "branch if VC > 2047". Branch addr is next phrase, so either way
	// it will pass by this phrase.
	uint64_t p1 = 0x03 | (2 << 14) | (0x7FF << 3) | (eval << 21);

	lastObjType = 3;
	D_quad(p1);

	return OK;
}

//
// Insert an unconditional jump in the object list (psuedo-op)
// Form: jump <link addr>
//
static int HandleJump(void)
{
	uint64_t eval;
	uint16_t eattr;
	SYM * esym = 0;

	if (expr(exprbuf, &eval, &eattr, &esym) != OK)
		return ERROR;

	if (!(eattr & DEFINED))
		AddFixup(FU_QUAD | FU_OBJLINK, sloc, exprbuf);

	ErrorIfNotAtEOL();

	// This is "branch if VC < 2047", which pretty much guarantees the branch.
	uint64_t p1 = 0x03 | (1 << 14) | (0x7FF << 3) | ((eval & 0x3FFFF8) << 21);

	lastObjType = 3;
	D_quad(p1);

	return OK;
}
