//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// RISCA.C - GPU/DSP Assembler
// Copyright (C) 199x Landon Dyer, 2011 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source Utilised with the Kind Permission of Landon Dyer
//

#include "risca.h"
#include "error.h"
#include "sect.h"
#include "token.h"
#include "expr.h"
#include "direct.h"
#include "mark.h"
#include "amode.h"

#define DEF_MR							// Declar keyword values
#include "risckw.h"						// Incl generated risc keywords

#define DEF_KW							// Declare keyword values 
#include "kwtab.h"						// Incl generated keyword tables & defs

unsigned altbankok = 0;					// Ok to use alternate register bank
unsigned orgactive = 0;					// RISC org directive active
unsigned orgaddr = 0;					// Org'd address
unsigned orgwarning = 0;				// Has an ORG warning been issued
int jpad = 0;
unsigned previousop = 0;				// Used for NOP padding checks
unsigned currentop = 0;					// Used for NOP padding checks
unsigned mjump_defined, mjump_dest;		// mjump macro flags, values etc

char reg_err[] = "missing register R0...R31";

// Jaguar Jump Condition Names
char condname[MAXINTERNCC][5] = { 
	"NZ", "Z", "NC", "NCNZ", "NCZ", "C", "CNZ", "CZ", "NN", "NNNZ", "NNZ",
	"N", "N_NZ", "N_Z ", "T", "A", "NE", "EQ", "CC", "HS", "HI", "CS", "LO",
	"PL", "MI", "F"
};

// Jaguar Jump Condition Numbers
char condnumber[] = {1, 2, 4, 5, 6, 8, 9, 10, 20, 21, 22, 24, 25, 26,
                     0, 0, 1, 2, 4, 4, 5,  8,  8, 20, 24, 31};

struct opcoderecord roptbl[] = {
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
	{ MR_UNPACK,  RI_ONE,    63 + GPUONLY },
	{ MR_PACK,    RI_ONE,    63 + GPUONLY },
	{ MR_ADDQMOD, RI_NUM_32, 63 + DSPONLY },
	{ MR_MOVE,    RI_MOVE,   0 },
	{ MR_LOAD,    RI_LOAD,   0 },
	{ MR_STORE,   RI_STORE,  0 }
};


//
// Convert a String to Uppercase
//
void strtoupper(char * s)
{
	while (*s)
	{
		*s = (char)(toupper(*s));
		s++;
	}
}


//
// Build RISC Instruction Word
//
void risc_instruction_word(unsigned short parm, int reg1, int reg2)
{
	int value = 0xE400;

	previousop = currentop;                                  // Opcode tracking for nop padding
	currentop = parm;

	if (!orgwarning)
	{                                        // Check for absolute address setting
		if (!orgactive && !in_main)
		{
			warn("GPU/DSP code outside of absolute section");
			orgwarning = 1;
		}
	}

	if (jpad)
	{                                               // JPAD directive
		//                JUMP                   JR                    NOP
		if (((previousop == 52) || (previousop == 53)) && (currentop != 57))
			D_word(value);                                     // Insert NOP
	}
	else
	{
		//               JUMP                   JR
		if ((previousop == 52) || (previousop == 53))
		{
			switch (currentop)
			{
			case 38:
				warn("NOP inserted before MOVEI instruction.");
				D_word(value);
				break;
			case 53:
				warn("NOP inserted before JR instruction.");
				D_word(value);
				break;
			case 52:
				warn("NOP inserted before JUMP instruction.");
				D_word(value);
				break;
			case 51:
				warn("NOP inserted before MOVE PC instruction.");
				D_word(value);
				break;
			default:
				break;
			}
		}
	}

	if (currentop == 20)
	{                                    // IMACN checks
		if ((previousop != 18) && (previousop != 20))
		{
			error("IMULTN/IMACN instruction must preceed IMACN instruction");
		}
	}

	if (currentop == 19)
	{                                    // RESMAC checks
		if (previousop != 20)
		{
			error("IMACN instruction must preceed RESMAC instruction");
		}
	}

	value =((parm & 0x3F) << 10) + ((reg1 & 0x1F) << 5) + (reg2 & 0x1F);
	D_word(value);
}


//
// Get a RISC Register
//
int getregister(WORD rattr)
{
	VALUE eval;					// Expression value
	WORD eattr;					// Expression attributes
	SYM * esym;					// External symbol involved in expr.
	TOKEN r_expr[EXPRSIZE];		// Expression token list
	WORD defined;				// Symbol defined flag

	if (expr(r_expr, &eval, &eattr, &esym) != OK)
	{
		error("malformed opcode");
		return ERROR;
	}
	else
	{
		defined = (WORD)(eattr & DEFINED);

		if ((challoc - ch_size) < 4)
			chcheck(4L);

		if (!defined)
		{
			fixup((WORD)(FU_WORD|rattr), sloc, r_expr);      
			return 0;
		}
		else
		{
			// Check for specified register, r0->r31
			if ((eval >= 0) && (eval <= 31))
			{
				return eval;
			}
			else
			{
				error(reg_err);
				return ERROR;
			}
		}
	}

	return ERROR;
}


//
// Do RISC Code Generation
//
int risccg(int state)
{
	unsigned short parm;					// Opcode parameters
	unsigned type;							// Opcode type
	int reg1;								// Register 1
	int reg2;								// Register 2
	int val = 0;							// Constructed value
	char scratch[80];
	SYM * ccsym;
	SYM * sy;
	int i;									// Iterator
	int t, c;
	WORD tdb;
	unsigned locptr = 0;					// Address location pointer
	unsigned page_jump = 0;					// Memory page jump flag
	VALUE eval;								// Expression value
	WORD eattr;								// Expression attributes
	SYM * esym;								// External symbol involved in expr.
	TOKEN r_expr[EXPRSIZE];					// Expression token list
	WORD defined;							// Symbol defined flag
	WORD attrflg;
	int indexed;							// Indexed register flag

	parm = (WORD)(roptbl[state-3000].parm);	// Get opcode parameter and type
	type = roptbl[state-3000].typ;

	// Detect whether the opcode parmeter passed determines that the opcode is
	// specific to only one of the RISC processors and ensure it is legal in
	// the current code section. If not then error and return.
	if (((parm & GPUONLY) && rdsp) || ((parm & DSPONLY) && rgpu))
	{
		error("opcode is not valid in this code section");
		return ERROR;
	}

	// Process RISC opcode
	switch (type)
	{
	// No operand instructions
	// NOP
	case RI_NONE: 
		risc_instruction_word(parm, 0, 0);
		break;
	// Single operand instructions (Rd)
	// ABS, MIRROR, NEG, NOT, PACK, RESMAC, SAT8, SAT16, SAT16S, SAT24, SAT32S, UNPACK
	case RI_ONE:
		reg2 = getregister(FU_REGTWO);
		at_eol();
		risc_instruction_word(parm, parm >> 6, reg2);
		break;   
	// Two operand instructions (Rs,Rd)
	// ADD, ADDC, AND, CMP, DIV, IMACN, IMULT, IMULTN, MOVEFA, MOVETA, MULT, MMULT, 
	// MTOI, NORMI, OR, ROR, SH, SHA, SUB, SUBC, XOR
	case RI_TWO:                      
		if (parm == 37)
			altbankok = 1;                      // MOVEFA

		reg1 = getregister(FU_REGONE);
		CHECK_COMMA;         

		if (parm == 36)
			altbankok = 1;                      // MOVETA

		reg2 = getregister(FU_REGTWO);
		at_eol();
		risc_instruction_word(parm, reg1, reg2);
		break;
	// Numeric operand (n,Rd) where n = -16..+15
	// CMPQ
	case RI_NUM_15:                   
	// Numeric operand (n,Rd) where n = 0..31
	// BCLR, BSET, BTST, MOVEQ
	case RI_NUM_31:      
	// Numeric operand (n,Rd) where n = 1..32
	// ADDQ, ADDQMOD, ADDQT, SHARQ, SHLQ, SHRQ, SUBQ, SUBQMOD, SUBQT, ROLQ, RORQ
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

		if (parm & SUB32) attrflg |= FU_SUB32;
		{
			if (*tok == '#')
			{
				++tok;

				if (expr(r_expr, &eval, &eattr, &esym) != OK)
					goto malformed;
				else
				{
					defined = (WORD)(eattr & DEFINED);

					if ((challoc - ch_size) < 4)
						chcheck(4L);

					if (!defined)
					{
						fixup((WORD)(FU_WORD|attrflg), sloc, r_expr);
						reg1 = 0;
					}
					else
					{
						if ((int)eval < reg1 || (int)eval > reg2)
						{
							error("constant out of range");
							return ERROR;
						}

						if (parm & SUB32) 
							reg1 = 32 - eval; 
						else if (type == RI_NUM_32)
							reg1 = (reg1 == 32) ? 0 : eval;
						else
							reg1 = eval;
					}
				}
			}
			else
				goto malformed;
		}

		CHECK_COMMA;
		reg2 = getregister(FU_REGTWO);
		at_eol();
		risc_instruction_word(parm, reg1, reg2);
		break;
	// Move Immediate - n,Rn - n in Second Word
	case RI_MOVEI:       
		if (*tok == '#')
		{
			++tok;
			if (expr(r_expr, &eval, &eattr, &esym) != OK)
			{
				malformed:
				error("malformed opcode");
				return ERROR;
			}
			else
			{
				// Opcode tracking for nop padding
				previousop = currentop;                          
				currentop = parm;

				// JUMP or JR
				if ((previousop == 52) || (previousop == 53) && !jpad)
				{
					warn("NOP inserted before MOVEI instruction.");   
					D_word(0xE400);
				}

				tdb = (WORD)(eattr & TDB);
				defined = (WORD)(eattr & DEFINED);

				if ((challoc - ch_size) < 4)
					chcheck(4L);

				if (!defined)
				{
					fixup(FU_LONG|FU_MOVEI, sloc + 2, r_expr);
					eval = 0;
				}
				else
				{
					if (tdb)
					{
						rmark(cursect, sloc + 2, tdb, MLONG|MMOVEI, NULL);
					}
				}	

				val = eval;

				// Store the defined flags and value of the movei when used in mjump
				if (mjump_align)
				{
					mjump_defined = defined;
					mjump_dest = val;
				}
			}
		}
		else
			goto malformed;

		++tok;
		reg2 = getregister(FU_REGTWO);
		at_eol();
		D_word((((parm & 0x3F) << 10) + reg2));
		val = ((val >> 16) & 0x0000FFFF) | ((val << 16) & 0xFFFF0000);
		D_long(val);
		break;
	case RI_MOVE:                     // PC,Rd or Rs,Rd
		if (*tok == KW_PC)
		{
			parm = 51;
			reg1 = 0;
			++tok;
		}
		else
		{
			parm = 34;
			reg1 = getregister(FU_REGONE);
		}

		CHECK_COMMA;
		reg2 = getregister(FU_REGTWO);
		at_eol();
		risc_instruction_word(parm, reg1, reg2);
		break;
	// (Rn),Rn = 41 / (R14/R15+n),Rn = 43/44 / (R14/R15+Rn),Rn = 58/59
	case RI_LOAD:          
		indexed = 0;
		parm = 41;

		if (*tok != '(')
			goto malformed;

		tok++;

		if ((*tok == KW_R14 || *tok == KW_R15) && (*(tok+1) != ')')) 
			indexed = (*tok - KW_R0);

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
				if (((sy->svalue & 0x1F) == 14 || (sy->svalue & 0x1F) == 15)
					&& (*(tok+2) != ')'))
				{
					indexed = (sy->svalue & 0x1F);
					++tok;
				}
			}
		}

		if (!indexed)
		{
			reg1 = getregister(FU_REGONE);
		}
		else
		{
			reg1 = indexed;
			indexed = 0;
			++tok;

			if (*tok == '+')
			{
				parm = (WORD)(reg1 - 14 + 58);
				tok++;

				if (*tok >= KW_R0 && *tok <= KW_R31)
				{
					indexed = 1;
				}

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
					{
						indexed = 1;
					} 
				}

				if (indexed)
				{
					reg1 = getregister(FU_REGONE);
				}
				else
				{
					if (expr(r_expr, &eval, &eattr, &esym) != OK)
					{
						goto malformed;
					}
					else
					{
						tdb = (WORD)(eattr & TDB);
						defined = (WORD)(eattr & DEFINED);

						if ((challoc - ch_size) < 4)
							chcheck(4L);

						if (!defined)
						{
							error("constant expected");
							return ERROR;
							//fixup(FU_WORD|FU_REGONE, sloc, r_expr);
							reg1 = 0;
						}
						else
						{
							reg1 = eval;

							if (reg1 == 0)
							{
								reg1 = 14 + (parm - 58);
								parm = 41;
								warn("NULL offset removed");
							}
							else
							{
								if (reg1 < 1 || reg1 > 32)
								{
									error("constant out of range");
									return ERROR;
								}

								if (reg1 == 32)
									reg1 = 0;

								parm = (WORD)(parm - 58 + 43);
							}
						}
					}
				}
			}
			else
			{
				reg1 = getregister(FU_REGONE);
			}
		}

		if (*tok != ')')
			goto malformed;

		++tok;
		CHECK_COMMA;
		reg2 = getregister(FU_REGTWO);
		at_eol();
		risc_instruction_word(parm, reg1, reg2);
		break;
	// Rn,(Rn) = 47 / Rn,(R14/R15+n) = 49/50 / Rn,(R14/R15+Rn) = 60/61
	case RI_STORE:    
		parm = 47;
		reg1 = getregister(FU_REGONE);
		CHECK_COMMA;

		if (*tok != '(') goto malformed;

		++tok;
		indexed = 0;

		if ((*tok == KW_R14 || *tok == KW_R15) && (*(tok+1) != ')')) 
			indexed = (*tok - KW_R0);

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
				if (((sy->svalue & 0x1F) == 14 || (sy->svalue & 0x1F) == 15)
					&& (*(tok+2) != ')'))
				{
					indexed = (sy->svalue & 0x1F);
					++tok;
				}
			}
		}

		if (!indexed)
		{
			reg2 = getregister(FU_REGTWO);
		}
		else
		{
			reg2 = indexed;
			indexed = 0;
			++tok;

			if (*tok == '+')
			{
				parm = (WORD)(reg2 - 14 + 60);
				tok++;

				if (*tok >= KW_R0 && *tok <= KW_R31)
				{
					indexed = 1;
				}

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
					{
						indexed = 1;
					}
				}

				if (indexed)
				{
					reg2 = getregister(FU_REGTWO);
				}
				else
				{
					if (expr(r_expr, &eval, &eattr, &esym) != OK)
					{
						goto malformed;
					}
					else
					{
						tdb = (WORD)(eattr & TDB);
						defined = (WORD)(eattr & DEFINED);

						if ((challoc - ch_size) < 4)
							chcheck(4L);

						if (!defined)
						{
							fixup(FU_WORD|FU_REGTWO, sloc, r_expr);
							reg2 = 0;
						}
						else
						{
							reg2 = eval;

							if (reg2 == 0 )
							{
								reg2 = 14 + (parm - 60);
								parm = 47;
								warn("NULL offset removed");
							}
							else
							{
								if (reg2 < 1 || reg2 > 32)
								{
									error("constant out of range");
									return ERROR;
								}

								if (reg2 == 32)
									reg2 = 0;

								parm = (WORD)(parm - 60 + 49);
							}
						}	
					}
				}
			}
			else
			{
				reg2 = getregister(FU_REGTWO);
			}
		}

		if (*tok != ')')
			goto malformed;

		++tok;
		at_eol();
		risc_instruction_word(parm, reg2, reg1);
		break;
	// LOADB/LOADP/LOADW (Rn),Rn
	case RI_LOADN:                    
		if (*tok != '(')
			goto malformed;

		++tok;
		reg1 = getregister(FU_REGONE);

		if (*tok != ')')
			goto malformed;

		++tok;
		CHECK_COMMA;
		reg2 = getregister(FU_REGTWO);
		at_eol();
		risc_instruction_word(parm, reg1, reg2);
		break;
	// STOREB/STOREP/STOREW Rn,(Rn)
	case RI_STOREN:                   
		reg1 = getregister(FU_REGONE);
		CHECK_COMMA;

		if (*tok != '(')
			goto malformed;

		++tok;
		reg2 = getregister(FU_REGTWO);

		if (*tok != ')')
			goto malformed;

		++tok;
		at_eol();
		risc_instruction_word(parm, reg2, reg1);
		break;
	case RI_JR:                       // Jump Relative - cc,n - n=-16..+15 words, reg2=cc
	case RI_JUMP:                     // Jump Absolute - cc,(Rs) - reg2=cc
		// Check to see if there is a comma in the token string. If not then the JR or JUMP should
		// default to 0, Jump Always
		t = i = c = 0;
		while (t != EOL)
		{
			t = *(tok + i);
			if (t == ',') c = 1;
			i++;
		}

		if (c)
		{                                            // Comma present in token string
			if (*tok == CONST)
			{                             // CC using a constant number
				++tok;
				val = *tok;
				++tok;
				CHECK_COMMA;
			}
			else if (*tok == SYMBOL)
			{
				val = 99;

				for(i=0; i<MAXINTERNCC; i++)
				{
//					strcpy(scratch, (char *)tok[1]);
					strcpy(scratch, string[tok[1]]);
					strtoupper(scratch);

					if (!strcmp(condname[i], scratch)) 
						val = condnumber[i];
				}

				if (val == 99)
				{
//					ccsym = lookup((char *)tok[1], LABEL, 0);
					ccsym = lookup(string[tok[1]], LABEL, 0);

					if (ccsym && (ccsym->sattre & EQUATEDCC) && !(ccsym->sattre & UNDEF_CC))
					{
						val = ccsym->svalue;
					}
					else
					{
						error("unknown condition code");
						return ERROR;
					}
				}

				tok += 2;
				CHECK_COMMA;
			}
			else if (*tok == '(')
			{
				val = 0;                                     // Jump always
			}
		}
		else
		{
			val = 0;                                        // Jump always
		}

		if (val < 0 || val > 31)
		{
			error("condition constant out of range");
			return ERROR;
		}
		else
		{
			reg1 = val;                                     // Store condition code
		}

		if (type == RI_JR)
		{                                // JR cc,n
			if (expr(r_expr, &eval, &eattr, &esym) != OK)
				goto malformed;
			else
			{
				tdb = (WORD)(eattr & TDB);
				defined = (WORD)(eattr & DEFINED);

				if ((challoc - ch_size) < 4)
					chcheck(4L);

				if (!defined)
				{
					if (in_main)
					{
						fixup(FU_WORD|FU_MJR, sloc, r_expr);
					}
					else
					{
						fixup(FU_WORD|FU_JR, sloc, r_expr);
					}

					reg2 = 0;
				}
				else
				{
					val = eval;

					if (orgactive)
					{
						reg2 = ((int)(val - (orgaddr + 2))) / 2;
						if ((reg2 < -16) || (reg2 > 15))
						error("PC relative overflow");
						locptr = orgaddr;
					}
					else
					{
						reg2 = ((int)(val - (sloc + 2))) / 2;
						if ((reg2 < -16) || (reg2 > 15))
						error("PC relative overflow");
						locptr = sloc;
					}
				}	

				if (in_main)
				{
					if (defined)
					{
						if (((locptr >= 0xF03000) && (locptr < 0xF04000)
							&& (val < 0xF03000)) || ((val >= 0xF03000)
							&& (val < 0xF04000) && (locptr < 0xF03000)))
						{
							warn("* cannot jump relative between main memory and local gpu ram");
						}
						else
						{
							page_jump = (locptr & 0xFFFFFF00) - (val & 0xFFFFFF00);

							if (page_jump)
							{
								if (val % 4)
								{
									warn("* destination address not aligned for long page jump relative, "
									"insert a \'nop\' before the destination label/address");
								}
							}
							else
							{
								if ((val - 2) % 4)
								{
									warn("* destination address not aligned for short page jump relative, "
										"insert a \'nop\' before the destination label/address");
								}
							}
						}
					}
				}
			}

			risc_instruction_word(parm, reg2, reg1);
		}
		else
		{                                           // JUMP cc, (Rn)
			if (*tok != '(')
				goto malformed;

			++tok;
			reg2 = getregister(FU_REGTWO);

			if (*tok != ')')
				goto malformed;

			++tok;
			at_eol();

			if (in_main)
			{
				if (!mjump_align)
				{
					warn("* \'jump\' is not recommended for .gpumain as destination addresses "
						"cannot be validated for alignment, use \'mjump\'");
					locptr = (orgactive) ? orgaddr : sloc;

					if (locptr % 4)
					{
						warn("* source address not aligned for long or short jump, "
							"insert a \'nop\' before the \'jump\'");
					}          
				}
				else
				{
					if (mjump_defined)
					{
						locptr = (orgactive) ? orgaddr : sloc;
						page_jump = (locptr & 0xFFFFFF00) - (mjump_dest & 0xFFFFFF00);

						if (page_jump)
						{
							if (mjump_dest % 4)
							{
								warn("* destination address not aligned for long page jump, "
								"insert a \'nop\' before the destination label/address");
							}          
						}
						else
						{
							if (!(mjump_dest & 0x0000000F) || ((mjump_dest - 2) % 4))
							{
								warn("* destination address not aligned for short page jump, "
								"insert a \'nop\' before the destination label/address");
							}          
						}
					}
					else
					{
						locptr = (orgactive) ? orgaddr : sloc;
						fwdjump[fwindex++] = locptr;
					}
				}
			}

			risc_instruction_word(parm, reg2, reg1);
		}

		break;
	// Should never get here :D
	default:                                              
		error("unknown risc opcode type");
		return ERROR;
		break;
	}

	return 0;
}
