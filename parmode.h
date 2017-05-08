//
// RMAC - Reboot's Macro Assembler for all Atari computers
// PARMODE.C - Addressing Modes Parser Include
// Copyright (C) 199x Landon Dyer, 2011-2017 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source utilised with the kind permission of Landon Dyer
//

// This file is included (twice) to parse two addressing modes, into slightly
// different var names
{
	// Dn
	// An
	// # expression
	if ((*tok >= KW_D0) && (*tok <= KW_D7))
	{
		AMn = DREG;
		AnREG = *tok++ & 7;
	}
	else if ((*tok >= KW_A0) && (*tok <= KW_A7))
	{
		AMn = AREG;
		AnREG = *tok++ & 7;
	}
	else if (*tok == '#')
	{
		tok++;

		if (expr(AnEXPR, &AnEXVAL, &AnEXATTR, &AnESYM) != OK)
			return ERROR;

		AMn = IMMED;
	}

	// Small problem with this is that the opening parentheses might be an
	// expression that's part of a displacement; this code will falsely flag
	// that as an error.

	// (An)
	// (An)+
	// (An,Xn[.siz][*scale])
	// (PC,Xn[.siz][*scale])
	// (d16,An)
	// (d8,An,Xn[.siz][*scale])
	// (d16,PC)
	// (d8,PC,Xn[.siz][*scale])
	// ([bd,An],Xn,od)
	// ([bd,An,Xn],od)
	// ([bd,PC],Xn,od)
	// ([bd,PC,Xn],od)
	else if (*tok == '(')
	{
		tok++;

		if ((*tok >= KW_A0) && (*tok <= KW_A7))
		{
			AnREG = *tok++ & 7;

			if (*tok == ')')
			{
				tok++;

				if (*tok == '+')
				{
					tok++;
					AMn = APOSTINC;
				}
				else
					AMn = AIND;

				goto AnOK;
			}

			AMn = AINDEXED;
			goto AMn_IX0;            // Handle ",Xn[.siz][*scale])"
		}
		else if ((*tok >= KW_D0) && (*tok <= KW_D7))
		{
			//Since index register isn't used here, store register number in this field
			AnIXREG = *tok++ & 7;                                // (Dn)
			if (*tok == ')')
			{
				tok++;
				AnEXTEN |= EXT_FULLWORD;    // Definitely using full extension format, so set bit 8
				AnEXTEN |= EXT_BS;          // Base register suppressed
				AnEXTEN |= EXT_BDSIZE0;     // Base displacement null
				AnEXTEN |= EXT_IISPOSN;     // Indirect Postindexed with Null Outer Displacement
				AMn= MEMPOST;
				AnREG = 6 << 3;		// stuff 110 to mode field
				goto AnOK;
			}
			else if (*tok == 'L')
			{
				// TODO: does DINDL gets used at all?
				//AMn=DINDL;                                     // (Dn.l)
				//AnEXTEN = 1 << 1;   // Long index size
				//tok++;
			}
			else if (*tok == 'W')                                // (Dn.w)
			{
				// TODO: does DINDW gets used at all?
				//AMn=DINDW;
				//AnEXTEN = 1 << 1;   // Word index size
				//tok++;
			}
			else if (*tok == ',')
			{
				// ([bd,An],Xn..) without bd, An
				// Base displacement is suppressed
				AnEXTEN |= EXT_FULLWORD;    // Definitely using full extension format, so set bit 8
				AnEXTEN |= EXT_BS;          // Base register suppressed
				AnEXTEN |= EXT_BDSIZE0;
				AnREG = 6 << 3;		// stuff 110 to mode field
				tok++;
				goto CHECKODn;
			}
			else
				return error("(Dn) error");

			if (*tok == ')')
			{
				tok++;
				AnEXTEN |= EXT_FULLWORD;    // Definitely using full extension format, so set bit 8
				AnEXTEN |= EXT_BS;          // Base register suppressed
				AnEXTEN |= EXT_BDSIZE0;     // Base displacement null
				AnEXTEN |= EXT_IISPOSN;     // Indirect Postindexed with Null Outer Displacement
				AnREG = 6 << 3;		// stuff 110 to mode field
				AMn = MEMPOST;
				goto AnOK;
			}
			else
				return error("unhandled so far");
		}
		else if (*tok == KW_PC)
		{                            // (PC,Xn[.siz][*scale])
			tok++;
			AMn = PCINDEXED;

			// Common index handler; enter here with 'tok' pointing at the
			// comma.

			AMn_IX0:                 // Handle indexed with missing expr

			AnEXVAL = 0;
			AnEXATTR = ABS | DEFINED;

			AMn_IXN:                 // Handle any indexed (tok -> a comma)

			if (*tok++ != ',')
				goto badmode;

			if (*tok < KW_D0 || *tok > KW_A7)
				goto badmode;

			AnIXREG = *tok++ & 15;

			switch ((int)*tok)
			{                        // Index reg size: <empty> | .W | .L
			case DOTW:
				tok++;
			default:
				AnIXSIZ = 0;
				break;
			case DOTL:
				AnIXSIZ = 0x0800;
				tok++;
				break;
			case DOTB:               // .B not allowed here...
				goto badmode;
			}

			if (*tok == '*')
			{                        // scale: *1, *2, *4, *8
				tok++;

				if (*tok++ != CONST || *tok > 8)
					goto badmode;

				switch ((int)*tok++)
				{
				case 1:
					break;
				case 2:
					AnIXSIZ |= TIMES2;
					break;
				case 4:
					AnIXSIZ |= TIMES4;
					break;
				case 8:
					AnIXSIZ |= TIMES8;
					break;
				default:
					goto badmode;
				}
			}

			if (*tok++ != ')')         // final ")"
				goto badmode;

			goto AnOK;
		}
		else if (*tok == '[')
		{                              // ([...
			tok++;
			AnEXTEN|=EXT_FULLWORD;     //Definitely using full extension format, so set bit 8
			// Check to see if base displacement is present
			//WARNING("expr will return a bad expression error here but this is expected, it needs to be silenced!");
			if (*tok!=CONST && *tok !=SYMBOL)
			//if (expr(AnBEXPR, &AnBEXVAL, &AnBEXATTR, &AnESYM) != OK)
			{
				AnEXTEN|=EXT_BDSIZE0;
				//tok++;
				//tok--;                 //Rewind tok since expr advances it forward
			}
			else
			{
				expr(AnBEXPR, &AnBEXVAL, &AnBEXATTR, &AnESYM);
				if (optim_flags[OPT_BASE_DISP] && AnBEXVAL==0 && AnEXATTR!=0)
				{
					// bd=0 so let's optimise it out
					AnEXTEN|=EXT_BDSIZE0;
				}
				else if (*tok==DOTL)
				{						   // ([bd.l,...
						AnEXTEN|=EXT_BDSIZEL;
						tok++;
				}
				else
				{						   // ([bd[.w],... or ([bd,...
					// Is .W forced here?
					if (*tok == DOTW)
					{
    					AnEXTEN|=EXT_BDSIZEW;
						tok++;
					}
					else
					{
						// Defined, absolute values from $FFFF8000..$00007FFF get optimized
						// to absolute short
						if (optim_flags[OPT_ABS_SHORT]
							&& ((AnBEXATTR & (TDB | DEFINED)) == DEFINED)
							&& ((AnBEXVAL + 0x8000) < 0x10000))
						{
							AnEXTEN |= EXT_BDSIZEW;
							warn("absolute value in base displacement ranging $FFFF8000..$00007FFF optimised to absolute short");
						}
						else
						{
							AnEXTEN |= EXT_BDSIZEL;
						}
					}
				}

				if (*tok == ',')
					tok++;
				//else
				//	return error("Comma expected after base displacement");
			}

			// Check for address register or PC, suppress base register
			// otherwise

			if (*tok == KW_PC)
			{					// ([bd,PC,...
				AnREG = (7 << 3) | 3;	// PC is special case - stuff 011 to register field and 111 to the mode field
				tok++;
			}
			else if ((*tok >= KW_A0) && (*tok <= KW_A7))
			{					// ([bd,An,...
				AnREG = (6<<3)|*tok & 7;
				tok++;
			}
			else if ((*tok >= KW_D0) && (*tok <= KW_D7))
			{
				// ([bd,Dn,...
				AnREG = (6 << 3);
				AnEXTEN |= ((*tok & 7) << 12);
				AnEXTEN |= EXT_D;
				AnEXTEN |= EXT_BS; // Oh look, a data register! Which means that base register is suppressed
				tok++;

				// Check for size
				{
					// ([bd,An/PC],Xn.W/L...)
					switch ((int)*tok)
					{
					// Index reg size: <empty> | .W | .L
					case DOTW:
						tok++;
						break;
					default:
						break;
					case DOTL:
						AnEXTEN |= EXT_L;
						tok++;
						break;
					case DOTB:
						// .B not allowed here...
						goto badmode;
					}
				}

				// Check for scale
				if (*tok == '*')			// ([bd,An/PC],Xn*...)
				{
					tok++;

					if (*tok == CONST)	// TODO: I suppose the scale is stored as a CONST and nothing else? So prolly the if is not needed?
						tok++;

					switch ((int)*tok++)
					{
					case 1:
						break;
					case 2:
						AnEXTEN |= EXT_TIMES2;
						break;
					case 4:
						AnEXTEN |= EXT_TIMES4;
						break;
					case 8:
						AnEXTEN |= EXT_TIMES8;
						break;
					default:
						goto badmode;
					}
				}

				if (*tok == ']')  // ([bd,Dn]...
				{
					tok++;
					goto IS_SUPPRESSEDn;
				}
			}
			else if (*tok == ']')
			{
				// PC and Xn is suppressed
				AnREG = 6 << 3;		// stuff 110 to mode field
				//AnEXTEN|=EXT_BS|EXT_IS;
				AnEXTEN |= EXT_BS;
			}
			else
			{
				goto badmode;
			}

			// At a crossroads here. We can accept either ([bd,An/PC],... or ([bd,An/PC,Xn*scale],...
			if (*tok == ']')
			{
				//([bd,An/PC],Xn,od)
				// Check for Xn
				tok++;

				if (*tok == ')')
				{
					//Xn and od are non existent, get out of jail free card
					AMn = MEMPRE;			// ([bc,An,Xn],od) with no Xn and od
					AnEXTEN |= EXT_IS | EXT_IISPREN;	//Suppress Xn and od
					tok++;
					goto AnOK;
				}
				else if (*tok != ',')
					return error("comma expected after ]");
				else
					tok++;				// eat the comma

				if ((*tok >= KW_A0) && (*tok <= KW_A7))
				{
					AnIXREG = ((*tok & 7) << 12);
					AnEXTEN |= EXT_A;
					tok++;
				}
				else if ((*tok >= KW_D0) && (*tok <= KW_D7))
				{
					AnEXTEN |= ((*tok & 7) << 12);
					AnEXTEN |= EXT_D;
					tok++;
				}
				else
				{
					//No index found, suppress it
					AnEXTEN |= EXT_IS;
					tok--;							// Rewind tok to point to the comma
					goto IS_SUPPRESSEDn;			// https://xkcd.com/292/ - what does he know anyway?
				}

				// Check for size
				{
					// ([bd,An/PC],Xn.W/L...)
					switch ((int)*tok)
					{
					// Index reg size: <empty> | .W | .L
					case DOTW:
						tok++;
						break;
					default:
						break;
					case DOTL:
						AnEXTEN |= EXT_L;
						tok++;
						break;
					case DOTB:
						// .B not allowed here...
						goto badmode;
					}
				}

				// Check for scale
				if (*tok == '*')
				{
					// ([bd,An/PC],Xn*...)
					tok++;

					if (*tok == CONST)	// TODO: I suppose the scale is stored as a CONST and nothing else? So prolly the if is not needed?
						tok++;

					switch ((int)*tok++)
					{
					case 1:
						break;
					case 2:
						AnEXTEN |= EXT_TIMES2;
						break;
					case 4:
						AnEXTEN |= EXT_TIMES4;
						break;
					case 8:
						AnEXTEN |= EXT_TIMES8;
						break;
					default:
						goto badmode;
					}
				}

				// Check for od
				if (*tok == ')')	// ([bd,An/PC],Xn)
				{
					//od is non existant, get out of jail free card
					AMn = MEMPOST;		// let's say it's ([bd,An],Xn,od) with od=0 then
					AnEXTEN |= EXT_IISPOSN; // No outer displacement
					tok++;
					goto AnOK;
				}
				else if (*tok!=',')
					return error("comma expected");
				else
					tok++;	// eat the comma

				CHECKODn:

				if (expr(AnEXPR, &AnEXVAL, &AnEXATTR, &AnESYM) != OK)
					goto badmode;

				if (optim_flags[OPT_BASE_DISP] && (AnEXVAL == 0))
				{
					// od=0 so optimise it out
					AMn = MEMPOST;		 // let's say it's ([bd,An],Xn,od) with od=0 then
					AnEXTEN |= EXT_IISPOSN; // No outer displacement
					tok++;
					goto AnOK;
				}

				// ([bd,An/PC],Xn,od)
				if (*tok == DOTL)
				{
					// expr.L
					AnEXTEN |= EXT_IISPOSL; // Long outer displacement
					AMn = MEMPOST;

					// Defined, absolute values from $FFFF8000..$00007FFF get
					// optimized to absolute short
					if (optim_flags[OPT_ABS_SHORT]
						&& ((AnEXATTR & (TDB | DEFINED)) == DEFINED)
						&& ((AnEXVAL + 0x8000) < 0x10000))
					{
						AnEXTEN |= EXT_IISPOSW; // Word outer displacement
						AMn = MEMPOST;
						warn("outer displacement absolute value from $FFFF8000..$00007FFF optimised to absolute short");
					}

				}
				else
				{
					// expr[.W]
					AnEXTEN |= EXT_IISPOSW; // Word outer displacement
					AMn = MEMPOST;

					// Is .W forced here?
					if (*tok == DOTW)
					{
						tok++;
					}
				}

				// Check for final closing parenthesis
				if (*tok == ')')
				{
					tok++;
					goto AnOK;
				}
				else
					return error("Closing parenthesis missing on addressing mode");

				IS_SUPPRESSEDn:

				// Check for od
				if (*tok == ')')	// ([bd,An/PC],Xn)
				{
					//od is non existant, get out of jail free card
					AMn = MEMPOST;		// let's say it's ([bd,An],Xn,od) with od=0 then
					AnEXTEN |= EXT_IISNOIN; // No outer displacement
					tok++;
					goto AnOK;
				}
				else if (*tok!=',')
					return error("comma expected");
				else
					tok++;	// eat the comma

                if ((*tok != CONST) && (*tok != SYMBOL))
					goto badmode;

				expr(AnEXPR, &AnEXVAL, &AnEXATTR, &AnESYM);

				if (optim_flags[OPT_BASE_DISP] && (AnEXVAL == 0))
				{
					// od=0 so optimise it out
					AMn = MEMPOST;		 // let's say it's ([bd,An],Xn,od) with od=0 then
					AnEXTEN |= EXT_IISNOIN; // No outer displacement
					tok++;
					goto AnOK;
				}

				// ([bd,An/PC],Xn,od)
				if (*tok == DOTL)
				{
					// expr.L
					tok++;
					AMn = MEMPOST;
					AnEXTEN |= EXT_IISNOIL; // Long outer displacement with IS suppressed
				}
				else
				{
					// expr[.W][]
					AnEXTEN |= EXT_IISNOIW; // Word outer displacement with IS suppressed
					AMn = MEMPRE;

					if (*tok == DOTW)
					{
						//AnEXTEN|=EXT_IISNOIW; // Word outer displacement
						AMn = MEMPOST;
						tok++;
					}

					// Defined, absolute values from $FFFF8000..$00007FFF get
					// optimized to absolute short
					else if (optim_flags[OPT_BASE_DISP]
						&& ((AnEXATTR & (TDB | DEFINED)) == DEFINED)
						&& ((AnEXVAL + 0x8000) < 0x10000))
					{
						//AnEXTEN|=EXT_IISNOIW; // Word outer displacement with IS suppressed
						warn("outer displacement absolute value from $FFFF8000..$00007FFF optimised to absolute short");
					}
				}

				// Check for final closing parenthesis
				if (*tok == ')')
				{
					tok++;
					goto AnOK;
				}
				else
					return error("Closing parenthesis missing on addressing mode");
			}
			else if (*tok == ',')
			{
				*tok++;			// ([bd,An,Xn.size*scale],od)

				//Check for Xn
				if ((*tok >= KW_A0) && (*tok <= KW_A7))
				{
					AnEXTEN |= ((*tok & 7) << 12);
					AnEXTEN |= EXT_A;
					tok++;
				}
				else if ((*tok >= KW_D0) && (*tok <= KW_D7))
				{
					AnEXTEN |= ((*tok & 7) << 12);
					AnEXTEN |= EXT_D;
					tok++;
				}

				// Check for size
				{
					// ([bd,An/PC],Xn.W/L...)
					switch ((int)*tok)
					{
					// Index reg size: <empty> | .W | .L
					case DOTW:
						tok++;
						break;
					default:
						break;
					case DOTL:
						tok++;
						AnEXTEN |= EXT_L;
						break;
					case DOTB:
						// .B not allowed here...
						goto badmode;
					}
				}

				// Check for scale
				if (*tok == '*')			// ([bd,An/PC],Xn*...)
				{
					tok++;

					if (*tok == CONST)	// TODO: I suppose the scale is stored as a CONST and nothing else? So prolly the if is not needed?
						tok++;

					switch ((int)*tok++)
					{
					case 1:
						break;
					case 2:
						AnEXTEN |= EXT_TIMES2;
						break;
					case 4:
						AnEXTEN |= EXT_TIMES4;
						break;
					case 8:
						AnEXTEN |= EXT_TIMES8;
						break;
					default:
						goto badmode;
					}
				}

				//Check for ]
				if (*tok != ']')
					return error("Expected closing bracket ]");

				tok++;			// Eat the bracket

				//Check for od
				if (*tok == ')')	// ([bd,An/PC,Xn]...
				{
					//od is non existant, get out of jail free card
					//AnEXVAL=0;		// zero outer displacement
					AMn = MEMPRE;			// let's say it's ([bd,An,Xn],od) with od suppressed then
					AnEXTEN |= EXT_IISPREN; // No outer displacement
					tok++;
					goto AnOK;
				}
				else if (*tok++!=',')
					return error("comma expected after ]");

				WARNING(Put symbol and constant checks here!)

				if (expr(AnEXPR, &AnEXVAL, &AnEXATTR, &AnESYM) != OK)
					goto badmode;

				if (optim_flags[OPT_BASE_DISP] && (AnEXVAL == 0))
				{
					// od=0 so optimise it out
					AMn = MEMPRE;		 // let's say it's ([bd,An],Xn,od) with od=0 then
					AnEXTEN |= EXT_IISPRE0; // No outer displacement
					tok++;
					goto AnOK;
				}

				// ([bd,An/PC,Xn],od)
				if (*tok == DOTL)
				{
					// expr.L
					AMn = MEMPRE;
					tok++;
					AnEXTEN |= EXT_IISPREL;
				}
				else
				{
					// expr.[W]
					//tok++;

					AnEXTEN |= EXT_IISPREW;
					AMn = MEMPRE;

                    // Is .W forced here?
					if (*tok == DOTW)
					{
						tok++;
					}

					// Defined, absolute values from $FFFF8000..$00007FFF get optimized
					// to absolute short
					else if (optim_flags[OPT_BASE_DISP]
						&& ((AnEXATTR & (TDB | DEFINED)) == DEFINED)
						&& ((AnEXVAL + 0x8000) < 0x10000))
					{
						AnEXTEN |= EXT_IISPREW;
						warn("outer displacement absolute value from $FFFF8000..$00007FFF optimised to absolute short");
					}
				}

				// Check for final closing parenthesis
				if (*tok == ')')
				{
					tok++;
					goto AnOK;
				}
				else
					return error("Closing parenthesis missing on addressing mode");
			}
			else
				goto badmode;
		}
		else
		{
			// (expr...
			if (expr(AnEXPR, &AnEXVAL, &AnEXATTR, &AnESYM) != OK)
				return ERROR;

			// It could be that this is really just an expression prefixing a
			// register as a displacement...
			if (*tok == ')')
			{
				tok++;
				goto CHK_FOR_DISPn;
			}

			// Otherwise, check for PC & etc displacements...
			if (*tok++ != ',')
				goto badmode;

			if ((*tok >= KW_A0) && (*tok <= KW_A7))
			{
				AnREG = *tok & 7;
				tok++;

				if (*tok == ',')
				{
					AMn = AINDEXED;
					goto AMn_IXN;
				}
				else if (*tok == ')')
				{
					AMn = ADISP;
					tok++;
					goto AnOK;
				}
				else
					goto badmode;
			}
			else if (*tok == KW_PC)
			{
				if (*++tok == ',')
				{                             // expr(PC,Xn...)
					AMn = PCINDEXED;
					goto AMn_IXN;
				}
				else if (*tok == ')')
				{
					AMn = PCDISP;                                // expr(PC)
					tok++;
					goto AnOK;
				}
				else
					goto badmode;
			}
			else
				goto badmode;
		}
	}
	else if (*tok == '-' && tok[1] == '(' && ((tok[2] >= KW_A0) && (tok[2] <= KW_A7)) && tok[3] == ')')
	{
		AMn = APREDEC;
		AnREG = tok[2] & 7;
		tok += 4;
	}
	else if (*tok == KW_CCR)
	{
		AMn = AM_CCR;
		tok++;
		goto AnOK;
	}
	else if (*tok == KW_SR)
	{
		AMn = AM_SR;
		tok++;
		goto AnOK;
	}
	else if (*tok == KW_USP)
	{
		AMn = AM_USP;
		tok++;
		AnREG = 2;	//Added this for the case of USP used in movec (see CREGlut in mach.c). Hopefully nothing gets broken!
		goto AnOK;
	}
	else if ((*tok >= KW_IC40) && (*tok <= KW_BC40))
	{
		AMn = CACHES;
		AnREG = *tok++ - KW_IC40;

		// After a cache keyword only a comma or EOL is allowed
		if ((*tok != ',') && (*tok != EOL))
			return ERROR;

		goto AnOK;
	}
	else if ((*tok >= KW_SFC) && (*tok <= KW_CRP))
	{
		AMn = CREG;
		AnREG = (*tok++) - KW_SFC;
		goto AnOK;
	}
	else if ((*tok >= KW_FP0) && (*tok <= KW_FP7))
	{
		AMn = FREG;
		AnREG = (*tok++ & 7);
	}
	else if ((*tok >= KW_FPIAR) && (*tok <= KW_FPCR))
	{
		AMn = FPSCR;
		AnREG = (1 << ((*tok++) - KW_FPIAR + 10));
	}
	// expr
	// expr.w
	// expr.l
	// d16(An)
	// d8(An,Xn[.siz])
	// d16(PC)
	// d8(PC,Xn[.siz])
	else
	{
		if (expr(AnEXPR, &AnEXVAL, &AnEXATTR, &AnESYM) != OK)
			return ERROR;

CHK_FOR_DISPn:
		if (*tok == DOTW)
		{
			// expr.W
			tok++;
			AMn = ABSW;

			if (((AnEXATTR & (TDB | DEFINED)) == DEFINED) && (AnEXVAL < 0x10000))
				AnEXVAL = (int32_t)(int16_t)AnEXVAL;  // Sign extend value

			goto AnOK;
		}
		else if (*tok != '(')
		{
			// expr[.L]
			AMn = ABSL;

			// Defined, absolute values from $FFFF8000..$00007FFF get optimized
			// to absolute short
			if (optim_flags[OPT_ABS_SHORT]
				&& ((AnEXATTR & (TDB | DEFINED)) == DEFINED)
				&& ((AnEXVAL + 0x8000) < 0x10000))
			{
				AMn = ABSW;

				if (sbra_flag)
					warn("absolute value from $FFFF8000..$00007FFF optimised to absolute short");
			}

			// Is .L forced here?
			if (*tok == DOTL)
			{
				tok++;
				AMn = ABSL;
			}

			goto AnOK;
		}

		tok++;

		if ((*tok >= KW_A0) && (*tok <= KW_A7))
		{
			AnREG = *tok++ & 7;

			if (*tok == ')')
			{
				AMn = ADISP;
				tok++;
				goto AnOK;
			}

			AMn = AINDEXED;
			goto AMn_IXN;
		}
		else if (*tok == KW_PC)
		{
			if (*++tok == ')')
			{
				AMn = PCDISP;
				tok++;
				goto AnOK;
			}

			AMn = PCINDEXED;
			goto AMn_IXN;
		}
		goto badmode;
	}

	// Addressing mode OK
AnOK:
	;
}

// Clean up dirty little macros
#undef AnOK
#undef AMn
#undef AnREG
#undef AnIXREG
#undef AnIXSIZ
#undef AnEXPR
#undef AnEXVAL
#undef AnEXATTR
#undef AnOEXPR
#undef AnOEXVAL
#undef AnOEXATTR
#undef AnESYM
#undef AMn_IX0
#undef AMn_IXN
#undef CHK_FOR_DISPn
#undef AnBEXPR
#undef AnBEXVAL
#undef AnBEXATTR
#undef AnBZISE
#undef AnEXTEN
#undef AMn_030
#undef IS_SUPPRESSEDn
#undef CHECKODn
