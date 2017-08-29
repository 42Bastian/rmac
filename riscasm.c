//
// RMAC - Reboot's Macro Assembler for all Atari computers
// RISCA.C - GPU/DSP Assembler
// Copyright (C) 199x Landon Dyer, 2011-2017 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source utilised with the kind permission of Landon Dyer
//

#include "riscasm.h"
#include "amode.h"
#include "direct.h"
#include "error.h"
#include "expr.h"
#include "mark.h"
#include "procln.h"
#include "sect.h"
#include "token.h"

#define DEF_MR				// Declare keyword values
#include "risckw.h"			// Incl. generated risc keywords

#define DEF_KW				// Declare keyword values
#include "kwtab.h"			// Incl. generated keyword tables & defs


unsigned altbankok = 0;		// Ok to use alternate register bank
unsigned orgactive = 0;		// RISC/6502 org directive active
unsigned orgaddr = 0;		// Org'd address
unsigned orgwarning = 0;	// Has an ORG warning been issued
int lastOpcode = -1;		// Last RISC opcode assembled
uint8_t riscImmTokenSeen;	// The '#' (immediate) token was seen

const char reg_err[] = "missing register R0...R31";

// Jaguar jump condition names
const char condname[MAXINTERNCC][5] = {
	"NZ", "Z", "NC", "NCNZ", "NCZ", "C", "CNZ", "CZ", "NN", "NNNZ", "NNZ",
	"N", "N_NZ", "N_Z", "T", "A", "NE", "EQ", "CC", "HS", "HI", "CS", "LO",
	"PL", "MI", "F"
};

// Jaguar jump condition numbers
const char condnumber[] = {
	1, 2, 4, 5, 6, 8, 9, 10, 20, 21, 22, 24, 25, 26,
	0, 0, 1, 2, 4, 4, 5,  8,  8, 20, 24, 31
};

const struct opcoderecord roptbl[] = {
	{ MR_ADD,     RI_TWO,    0 },
	{ MR_ADDC,    RI_TWO,    1 },
	{ MR_ADDQ,    RI_NUM_32, 2 },
	{ MR_ADDQT,   RI_NUM_32, 3 },
	{ MR_SUB,     RI_TWO,    4 },
	{ MR_SUBC,    RI_TWO,    5 },
	{ MR_SUBQ,    RI_NUM_32, 6 },
	{ MR_SUBQT,   RI_NUM_32, 7 },
	{ MR_NEG,     RI_ONE,    8 },
	{ MR_AND,     RI_TWO,    9 },
	{ MR_OR,      RI_TWO,    10 },
	{ MR_XOR,     RI_TWO,    11 },
	{ MR_NOT,     RI_ONE,    12 },
	{ MR_BTST,    RI_NUM_31, 13 },
	{ MR_BSET,    RI_NUM_31, 14 },
	{ MR_BCLR,    RI_NUM_31, 15 },
	{ MR_MULT,    RI_TWO,    16 },
	{ MR_IMULT,   RI_TWO,    17 },
	{ MR_IMULTN,  RI_TWO,    18 },
	{ MR_RESMAC,  RI_ONE,    19 },
	{ MR_IMACN,   RI_TWO,    20 },
	{ MR_DIV,     RI_TWO,    21 },
	{ MR_ABS,     RI_ONE,    22 },
	{ MR_SH,      RI_TWO,    23 },
	{ MR_SHLQ,    RI_NUM_32, 24 + SUB32 },
	{ MR_SHRQ,    RI_NUM_32, 25 },
	{ MR_SHA,     RI_TWO,    26 },
	{ MR_SHARQ,   RI_NUM_32, 27 },
	{ MR_ROR,     RI_TWO,    28 },
	{ MR_RORQ,    RI_NUM_32, 29 },
	{ MR_ROLQ,    RI_NUM_32, 29 + SUB32 },
	{ MR_CMP,     RI_TWO,    30 },
	{ MR_CMPQ,    RI_NUM_15, 31 },
	{ MR_SAT8,    RI_ONE,    32 + GPUONLY },
	{ MR_SUBQMOD, RI_NUM_32, 32 + DSPONLY },
	{ MR_SAT16,   RI_ONE,    33 + GPUONLY },
	{ MR_SAT16S,  RI_ONE,    33 + DSPONLY },
	{ MR_MOVEQ,   RI_NUM_31, 35 },
	{ MR_MOVETA,  RI_TWO,    36 },
	{ MR_MOVEFA,  RI_TWO,    37 },
	{ MR_MOVEI,   RI_MOVEI,  38 },
	{ MR_LOADB,   RI_LOADN,  39 },
	{ MR_LOADW,   RI_LOADN,  40 },
	{ MR_LOADP,   RI_LOADN,  42 + GPUONLY },
	{ MR_SAT32S,  RI_ONE,    42 + DSPONLY },
	{ MR_STOREB,  RI_STOREN, 45 },
	{ MR_STOREW,  RI_STOREN, 46 },
	{ MR_STOREP,  RI_STOREN, 48 + GPUONLY },
	{ MR_MIRROR,  RI_ONE,    48 + DSPONLY },
	{ MR_JUMP,    RI_JUMP,   52 },
	{ MR_JR,      RI_JR,     53 },
	{ MR_MMULT,   RI_TWO,    54 },
	{ MR_MTOI,    RI_TWO,    55 },
	{ MR_NORMI,   RI_TWO,    56 },
	{ MR_NOP,     RI_NONE,   57 },
	{ MR_SAT24,   RI_ONE,    62 },
	{ MR_UNPACK,  RI_ONE,    63 + GPUONLY | (0 << 6) },
	{ MR_PACK,    RI_ONE,    63 + GPUONLY | (1 << 6) },
	{ MR_ADDQMOD, RI_NUM_32, 63 + DSPONLY },
	{ MR_MOVE,    RI_MOVE,   0 },
	{ MR_LOAD,    RI_LOAD,   0 },
	{ MR_STORE,   RI_STORE,  0 }
};


//
// Convert a string to uppercase
//
void strtoupper(char * s)
{
	while (*s)
		*s++ &= 0xDF;
}


//
// Function to return "malformed expression" error
// This is done mainly to remove a bunch of GOTO statements in the parser
//
static inline int MalformedOpcode(int signal)
{
	return error("Malformed opcode [internal $%02X]", signal);
}


//
// Function to return "Illegal Indexed Register" error
// Anyone trying to index something other than R14 or R15
//
static inline int IllegalIndexedRegister(int reg)
{
	return error("Attempted index reference with non-indexable register (r%d)", reg - KW_R0);
}


//
// Function to return "Illegal Indexed Register" error for EQUR scenarios
// Trying to use register value within EQUR that isn't 14 or 15
//
static inline int IllegalIndexedRegisterEqur(SYM * sy)
{
	return error("Attempted index reference with non-indexable register within EQUR (%s = r%d)", sy->sname, sy->svalue);
}


//
// Build RISC instruction word
//
void BuildRISCIntructionWord(unsigned short opcode, int reg1, int reg2)
{
	// Check for absolute address setting
	if (!orgwarning && !orgactive)
	{
		warn("RISC code generated with no origin defined");
		orgwarning = 1;
	}

	int value = ((opcode & 0x3F) << 10) + ((reg1 & 0x1F) << 5) + (reg2 & 0x1F);
	D_word(value);
//printf("BuildRISC: opcode=$%X, reg1=$%X, reg2=$%X, final=$%04X\n", opcode, reg1, reg2, value);
}


//
// Get a RISC register
//
int GetRegister(WORD rattr)
{
	uint32_t eval;					// Expression value
	WORD eattr;					// Expression attributes
	SYM * esym;					// External symbol involved in expr.
	TOKEN r_expr[EXPRSIZE];		// Expression token list

	// Evaluate what's in the global "tok" buffer
	if (expr(r_expr, &eval, &eattr, &esym) != OK)
		return ERROR;

	if ((challoc - ch_size) < 4)
		chcheck(4L);

	if (!(eattr & DEFINED))
	{
		AddFixup((WORD)(FU_WORD | rattr), sloc, r_expr);
		return 0;
	}

	// If we got a register in range (0-31), return it
	if ((eval >= 0) && (eval <= 31))
		return eval;

	// Otherwise, it's out of range & we flag an error
	return error(reg_err);
}


//
// Do RISC code generation
//
int GenerateRISCCode(int state)
{
	int reg1;					// Register 1
	int reg2;					// Register 2
	int val = 0;				// Constructed value
	char scratch[80];
	SYM * ccsym;
	SYM * sy;
	int i, commaFound;
	TOKEN * t;
	WORD attrflg;
	int indexed;				// Indexed register flag

	uint32_t eval;					// Expression value
	WORD eattr;					// Expression attributes
	SYM * esym;					// External symbol involved in expr.
	TOKEN r_expr[EXPRSIZE];		// Expression token list

	// Get opcode parameter and type
	unsigned short parm = (WORD)(roptbl[state - 3000].parm);
	unsigned type = roptbl[state - 3000].typ;
	riscImmTokenSeen = 0;		// Set to "token not seen yet"

	// Detect whether the opcode parmeter passed determines that the opcode is
	// specific to only one of the RISC processors and ensure it is legal in
	// the current code section. If not then show error and return.
	if (((parm & GPUONLY) && rdsp) || ((parm & DSPONLY) && rgpu))
		return error("Opcode is not valid in this code section");

	// Process RISC opcode
	switch (type)
	{
	// No operand instructions
	// NOP (57)
	case RI_NONE:
		BuildRISCIntructionWord(parm, 0, 0);
		break;

	// Single operand instructions (Rd)
	// ABS, MIRROR, NEG, NOT, PACK, RESMAC, SAT8, SAT16, SAT16S, SAT24, SAT32S,
	// UNPACK
	case RI_ONE:
		reg2 = GetRegister(FU_REGTWO);
		at_eol();
		BuildRISCIntructionWord(parm, parm >> 6, reg2);
		break;

	// Two operand instructions (Rs,Rd)
	// ADD, ADDC, AND, CMP, DIV, IMACN, IMULT, IMULTN, MOVEFA, MOVETA, MULT,
	// MMULT, MTOI, NORMI, OR, ROR, SH, SHA, SUB, SUBC, XOR
	case RI_TWO:
		if (parm == 37)
			altbankok = 1;                      // MOVEFA

		reg1 = GetRegister(FU_REGONE);
		CHECK_COMMA;

		if (parm == 36)
			altbankok = 1;                      // MOVETA

		reg2 = GetRegister(FU_REGTWO);
		at_eol();
		BuildRISCIntructionWord(parm, reg1, reg2);
		break;

	// Numeric operand (n,Rd) where n = -16..+15
	// CMPQ
	case RI_NUM_15:

	// Numeric operand (n,Rd) where n = 0..31
	// BCLR, BSET, BTST, MOVEQ
	case RI_NUM_31:

	// Numeric operand (n,Rd) where n = 1..32
	// ADDQ, ADDQMOD, ADDQT, SHARQ, SHLQ, SHRQ, SUBQ, SUBQMOD, SUBQT, ROLQ,
	// RORQ
	case RI_NUM_32:
		switch (type)
		{
		case RI_NUM_15:
			reg1 = -16; reg2 = 15; attrflg = FU_NUM15;
			break;
		default:
		case RI_NUM_31:
			reg1 =   0; reg2 = 31; attrflg = FU_NUM31;
			break;
		case RI_NUM_32:
			reg1 =   1; reg2 = 32; attrflg = FU_NUM32;
			break;
		}

		if (parm & SUB32)
			attrflg |= FU_SUB32;

		if (*tok != '#')
			return MalformedOpcode(0x01);

		tok++;
		riscImmTokenSeen = 1;

		if (expr(r_expr, &eval, &eattr, &esym) != OK)
			return MalformedOpcode(0x02);

		if ((challoc - ch_size) < 4)
			chcheck(4L);

		if (!(eattr & DEFINED))
		{
			AddFixup((WORD)(FU_WORD | attrflg), sloc, r_expr);
			reg1 = 0;
		}
		else
		{
			if ((int)eval < reg1 || (int)eval > reg2)
				return error("constant out of range");

			if (parm & SUB32)
				reg1 = 32 - eval;
			else if (type == RI_NUM_32)
				reg1 = (reg1 == 32 ? 0 : eval);
			else
				reg1 = eval;
		}

		CHECK_COMMA;
		reg2 = GetRegister(FU_REGTWO);
		at_eol();
		BuildRISCIntructionWord(parm, reg1, reg2);
		break;

	// Move Immediate--n,Rn--n in Second Word
	case RI_MOVEI:
		if (*tok != '#')
			return MalformedOpcode(0x03);

		tok++;
		riscImmTokenSeen = 1;

		// Check for equated register after # and return error if so
		if (*tok == SYMBOL)
		{
			sy = lookup(string[tok[1]], LABEL, 0);

			if (sy && (sy->sattre & EQUATEDREG))
				return error("equated register in 1st operand of MOVEI instruction");
		}

		if (expr(r_expr, &eval, &eattr, &esym) != OK)
			return MalformedOpcode(0x04);

		if (lastOpcode == RI_JUMP || lastOpcode == RI_JR)
		{
			if (legacy_flag)
			{
				// User doesn't care, emit a NOP to fix
				BuildRISCIntructionWord(57, 0, 0);
				warn("MOVEI following JUMP, inserting NOP to fix your BROKEN CODE");
			}
			else
				warn("MOVEI immediately follows JUMP");
		}

		if ((challoc - ch_size) < 4)
			chcheck(4L);

		if (!(eattr & DEFINED))
		{
			AddFixup(FU_LONG | FU_MOVEI, sloc + 2, r_expr);
			eval = 0;
		}
		else
		{
			if (eattr & TDB)
//{
//printf("RISCASM: Doing MarkRelocatable for RI_MOVEI (tdb=$%X)...\n", eattr & TDB);
				MarkRelocatable(cursect, sloc + 2, (eattr & TDB), (MLONG | MMOVEI), NULL);
//}
		}

//		val = ((eval >> 16) & 0x0000FFFF) | ((eval << 16) & 0xFFFF0000);
		val = WORDSWAP32(eval);
		CHECK_COMMA;
		reg2 = GetRegister(FU_REGTWO);
		at_eol();
		D_word((((parm & 0x3F) << 10) + reg2));
		D_long(val);
		break;

	// PC,Rd or Rs,Rd
	case RI_MOVE:
		if (*tok == KW_PC)
		{
			parm = 51;
			reg1 = 0;
			tok++;
		}
		else
		{
			parm = 34;
			reg1 = GetRegister(FU_REGONE);
		}

		CHECK_COMMA;
		reg2 = GetRegister(FU_REGTWO);
		at_eol();
		BuildRISCIntructionWord(parm, reg1, reg2);
		break;

	// (Rn),Rn = 41 / (R14/R15+n),Rn = 43/44 / (R14/R15+Rn),Rn = 58/59
	case RI_LOAD:
		indexed = 0;
		parm = 41;

		if (*tok != '(')
			return MalformedOpcode(0x05);

		tok++;

        if ((*(tok + 1) == '+') || (*(tok + 1) == '-')) {
            // Trying to make indexed call
            if ((*tok == KW_R14 || *tok == KW_R15)) {
                indexed = (*tok - KW_R0);
            } else {
                return IllegalIndexedRegister(*tok);
            }
        }

		if (*tok == SYMBOL)
		{
//			sy = lookup((char *)tok[1], LABEL, 0);
			sy = lookup(string[tok[1]], LABEL, 0);

			if (!sy)
			{
				error(reg_err);
				return ERROR;
			}

			if (sy->sattre & EQUATEDREG)
			{
				if ((*(tok + 2) == '+') || (*(tok + 2) == '-')) {
				    if ((sy->svalue & 0x1F) == 14 || (sy->svalue & 0x1F) == 15) {
				        indexed = (sy->svalue & 0x1F);
                        tok++;
				    } else {
				        return IllegalIndexedRegisterEqur(sy);
				    }
				}
			}
		}

		if (!indexed)
		{
			reg1 = GetRegister(FU_REGONE);
		}
		else
		{
			reg1 = indexed;
			indexed = 0;
			tok++;

			if (*tok == '+')
			{
				parm = (WORD)(reg1 - 14 + 58);
				tok++;

				if (*tok >= KW_R0 && *tok <= KW_R31)
					indexed = 1;

				if (*tok == SYMBOL)
				{
//					sy = lookup((char *)tok[1], LABEL, 0);
					sy = lookup(string[tok[1]], LABEL, 0);

					if (!sy)
					{
						error(reg_err);
						return ERROR;
					}

					if (sy->sattre & EQUATEDREG)
						indexed = 1;
				}

				if (indexed)
				{
					reg1 = GetRegister(FU_REGONE);
				}
				else
				{
					if (expr(r_expr, &eval, &eattr, &esym) != OK)
						return MalformedOpcode(0x06);

					if ((challoc - ch_size) < 4)
						chcheck(4L);

					if (!(eattr & DEFINED))
						return error("constant expected after '+'");

					reg1 = eval;

					if (reg1 == 0)
					{
						reg1 = 14 + (parm - 58);
						parm = 41;
						warn("NULL offset in LOAD ignored");
					}
					else
					{
						if (reg1 < 1 || reg1 > 32)
							return error("constant in LOAD out of range");

						if (reg1 == 32)
							reg1 = 0;

						parm = (WORD)(parm - 58 + 43);
					}
				}
			}
			else
			{
				reg1 = GetRegister(FU_REGONE);
			}
		}

		if (*tok != ')')
			return MalformedOpcode(0x07);

		tok++;
		CHECK_COMMA;
		reg2 = GetRegister(FU_REGTWO);
		at_eol();
		BuildRISCIntructionWord(parm, reg1, reg2);
		break;

	// Rn,(Rn) = 47 / Rn,(R14/R15+n) = 49/50 / Rn,(R14/R15+Rn) = 60/61
	case RI_STORE:
		parm = 47;
		reg1 = GetRegister(FU_REGONE);
		CHECK_COMMA;

		if (*tok != '(')
			return MalformedOpcode(0x08);

		tok++;
		indexed = 0;

		if ((*tok == KW_R14 || *tok == KW_R15) && (*(tok + 1) != ')'))
			indexed = (*tok - KW_R0);

		if (*tok == SYMBOL)
		{
			sy = lookup(string[tok[1]], LABEL, 0);

			if (!sy)
			{
				error(reg_err);
				return ERROR;
			}

			if (sy->sattre & EQUATEDREG)
			{
				if (((sy->svalue & 0x1F) == 14 || (sy->svalue & 0x1F) == 15)
					&& (*(tok + 2) != ')'))
				{
					indexed = (sy->svalue & 0x1F);
					tok++;
				}
			}
		}

		if (!indexed)
		{
			reg2 = GetRegister(FU_REGTWO);
		}
		else
		{
			reg2 = indexed;
			indexed = 0;
			tok++;

			if (*tok == '+')
			{
				parm = (WORD)(reg2 - 14 + 60);
				tok++;

				if (*tok >= KW_R0 && *tok <= KW_R31)
					indexed = 1;

				if (*tok == SYMBOL)
				{
					sy = lookup(string[tok[1]], LABEL, 0);

					if (!sy)
					{
						error(reg_err);
						return ERROR;
					}

					if (sy->sattre & EQUATEDREG)
						indexed = 1;
				}

				if (indexed)
				{
					reg2 = GetRegister(FU_REGTWO);
				}
				else
				{
					if (expr(r_expr, &eval, &eattr, &esym) != OK)
						return MalformedOpcode(0x09);

					if ((challoc - ch_size) < 4)
						chcheck(4L);

					if (!(eattr & DEFINED))
					{
						AddFixup(FU_WORD | FU_REGTWO, sloc, r_expr);
						reg2 = 0;
					}
					else
					{
						reg2 = eval;

						if (reg2 == 0)
						{
							reg2 = 14 + (parm - 60);
							parm = 47;
							warn("NULL offset in STORE ignored");
						}
						else
						{
							if (reg2 < 1 || reg2 > 32)
								return error("constant in STORE out of range");

							if (reg2 == 32)
								reg2 = 0;

							parm = (WORD)(parm - 60 + 49);
						}
					}
				}
			}
			else
			{
				reg2 = GetRegister(FU_REGTWO);
			}
		}

		if (*tok != ')')
			return MalformedOpcode(0x0A);

		tok++;
		at_eol();
		BuildRISCIntructionWord(parm, reg2, reg1);
		break;

	// LOADB/LOADP/LOADW (Rn),Rn
	case RI_LOADN:
		if (*tok != '(')
			return MalformedOpcode(0x0B);

		tok++;
		reg1 = GetRegister(FU_REGONE);

		if (*tok != ')')
			return MalformedOpcode(0x0C);

		tok++;
		CHECK_COMMA;
		reg2 = GetRegister(FU_REGTWO);
		at_eol();
		BuildRISCIntructionWord(parm, reg1, reg2);
		break;

	// STOREB/STOREP/STOREW Rn,(Rn)
	case RI_STOREN:
		reg1 = GetRegister(FU_REGONE);
		CHECK_COMMA;

		if (*tok != '(')
			return MalformedOpcode(0x0D);

		tok++;
		reg2 = GetRegister(FU_REGTWO);

		if (*tok != ')')
			return MalformedOpcode(0x0E);

		tok++;
		at_eol();
		BuildRISCIntructionWord(parm, reg2, reg1);
		break;

	// Jump Relative - cc,n - n=-16..+15 words, reg2=cc
	case RI_JR:

	// Jump Absolute - cc,(Rs) - reg2=cc
	case RI_JUMP:
		// Check to see if there is a comma in the token string. If not then
		// the JR or JUMP should default to 0, Jump Always
		commaFound = 0;

		for(t=tok; *t!=EOL; t++)
		{
			if (*t == ',')
			{
				commaFound = 1;
				break;
			}
		}

		if (commaFound)
		{
			if (*tok == CONST)
			{
				// CC using a constant number
				tok++;
				val = *tok;
				tok++;
				CHECK_COMMA;
			}
			else if (*tok == SYMBOL)
			{
				val = 99;
//				strcpy(scratch, (char *)tok[1]);
				strcpy(scratch, string[tok[1]]);
				strtoupper(scratch);

				for(i=0; i<MAXINTERNCC; i++)
				{
					// Look for the condition code & break if found
					if (strcmp(condname[i], scratch) == 0)
					{
						val = condnumber[i];
						break;
					}
				}

				// Standard CC was not found, look for an equated one
				if (val == 99)
				{
//					ccsym = lookup((char *)tok[1], LABEL, 0);
					ccsym = lookup(string[tok[1]], LABEL, 0);

					if (ccsym && (ccsym->sattre & EQUATEDCC) && !(ccsym->sattre & UNDEF_CC))
						val = ccsym->svalue;
					else
						return error("unknown condition code");
				}

				tok += 2;
				CHECK_COMMA;
			}
			else if (*tok == '(')
			{
				// Set CC to "Jump Always"
				val = 0;
			}
		}
		else
		{
			// Set CC to "Jump Always"
			val = 0;
		}

		if (val < 0 || val > 31)
			return error("condition constant out of range");

		// Store condition code
		reg1 = val;

		if (type == RI_JR)
		{
			// JR cc,n
			if (expr(r_expr, &eval, &eattr, &esym) != OK)
				return MalformedOpcode(0x0F);

			if ((challoc - ch_size) < 4)
				chcheck(4L);

			if (!(eattr & DEFINED))
			{
				AddFixup(FU_WORD | FU_JR, sloc, r_expr);
				reg2 = 0;
			}
			else
			{
				reg2 = ((int)(eval - ((orgactive ? orgaddr : sloc) + 2))) / 2;

				if ((reg2 < -16) || (reg2 > 15))
					error("PC relative overflow (outside of -16 to 15)");
			}

			BuildRISCIntructionWord(parm, reg2, reg1);
		}
		else
		{
			// JUMP cc, (Rn)
			if (*tok != '(')
				return MalformedOpcode(0x10);

			tok++;
			reg2 = GetRegister(FU_REGTWO);

			if (*tok != ')')
				return MalformedOpcode(0x11);

			tok++;
			at_eol();
			BuildRISCIntructionWord(parm, reg2, reg1);
		}

		break;

	// Should never get here :-D
	default:
		return error("Unknown RISC opcode type");
	}

	lastOpcode = type;
	return 0;
}

