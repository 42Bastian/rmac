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
	if ((*tok.u32 >= KW_D0) && (*tok.u32 <= KW_D7))
	{
		AMn = DREG;
		AnREG = *tok.u32++ & 7;
	}
	else if ((*tok.u32 >= KW_A0) && (*tok.u32 <= KW_A7))
	{
		AMn = AREG;
		AnREG = *tok.u32++ & 7;
	}
	else if (*tok.u32 == '#')
	{
		tok.u32++;

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
	else if (*tok.u32 == '(')
	{
		tok.u32++;

		if ((*tok.u32 >= KW_A0) && (*tok.u32 <= KW_A7))
		{
			AnREG = *tok.u32++ & 7;

			if (*tok.u32 == ')')
			{
				tok.u32++;

				if (*tok.u32 == '+')
				{
					tok.u32++;
					AMn = APOSTINC;
				}
				else
					AMn = AIND;

				goto AnOK;
			}

			AMn = AINDEXED;
			goto AMn_IX0;            // Handle ",Xn[.siz][*scale])"
		}
		else if ((*tok.u32 >= KW_D0) && (*tok.u32 <= KW_D7))
		{
			//Since index register isn't used here, store register number in this field
			AnIXREG = *tok.u32++ & 7;                                // (Dn)

			if (*tok.u32 == ')')
			{
				tok.u32++;
				AnEXTEN |= EXT_FULLWORD;    // Definitely using full extension format, so set bit 8
				AnEXTEN |= EXT_BS;          // Base register suppressed
				AnEXTEN |= EXT_BDSIZE0;     // Base displacement null
				AnEXTEN |= EXT_IISPOSN;     // Indirect Postindexed with Null Outer Displacement
				AMn = MEMPOST;
				AnREG = 6 << 3;		// stuff 110 to mode field
				goto AnOK;
			}
			else if (*tok.u32 == 'L')
			{
				// TODO: does DINDL gets used at all?
				AMn = DINDL;                                     // (Dn.l)
				AnEXTEN = 1 << 1;   // Long index size
				tok.u32++;
			}
			else if (*tok.u32 == 'W')                                // (Dn.w)
			{
				// TODO: does DINDW gets used at all?
				AMn = DINDW;
				AnEXTEN = 1 << 1;   // Word index size
				tok.u32++;
			}
			else if (*tok.u32 == ',')
			{
				// ([bd,An],Xn..) without bd, An
				// Base displacement is suppressed
				AnEXTEN |= EXT_FULLWORD;    // Definitely using full extension format, so set bit 8
				AnEXTEN |= EXT_BS;          // Base register suppressed
				AnEXTEN |= EXT_BDSIZE0;
				AnREG = 6 << 3;		// stuff 110 to mode field
				tok.u32++;
				goto CHECKODn;
			}
			else
			{
				return error("(Dn) error");
			}

			if (*tok.u32 == '*')
			{                        // scale: *1, *2, *4, *8
				tok.u32++;

				if (*tok.u32 == SYMBOL)
				{
					if (expr(AnEXPR, &AnEXVAL, &AnEXATTR, &AnESYM) != OK)
						return error("scale factor expression must evaluate");

					switch (AnEXVAL)
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
				else if (*tok.u32++ != CONST || *tok.u32 > 8)
					goto badmode;
				else
				{
					switch ((int)*tok.u32++)
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
			}

			if (*tok.u32 == ')')
			{
				tok.u32++;
				AnEXTEN |= EXT_FULLWORD;    // Definitely using full extension format, so set bit 8
				AnEXTEN |= EXT_BS;          // Base register suppressed
				AnEXTEN |= EXT_BDSIZE0;     // Base displacement null
				AnEXTEN |= EXT_IISPOSN;     // Indirect Postindexed with Null Outer Displacement
				AnREG = 6 << 3;		// stuff 110 to mode field
				AMn = MEMPOST;
				goto AnOK;
			}
			else if (*tok.u32 == ',')
			{
				tok.u32++;  // eat the comma
				// It might be (Dn[.wl][*scale],od)
				// Maybe this is wrong and we have to write some code here
				// instead of reusing that path...
				AnEXTEN |= EXT_BDSIZE0;     // Base displacement null - suppressed
				goto CHECKODn;
			}
			else
				return error("unhandled so far");
		}
		else if (*tok.u32 == KW_PC)
		{                            // (PC,Xn[.siz][*scale])
			tok.u32++;
			AMn = PCINDEXED;

			// Common index handler; enter here with 'tok' pointing at the
			// comma.

			AMn_IX0:                 // Handle indexed with missing expr

			AnEXVAL = 0;
			AnEXATTR = ABS | DEFINED;

			AMn_IXN:                 // Handle any indexed (tok -> a comma)

			if (*tok.u32++ != ',')
				goto badmode;

			if (*tok.u32 < KW_D0 || *tok.u32 > KW_A7)
				goto badmode;

			AnIXREG = *tok.u32++ & 15;

			switch ((int)*tok.u32)
			{                        // Index reg size: <empty> | .W | .L
			case DOTW:
				tok.u32++;
			default:
				AnIXSIZ = 0;
				break;
			case DOTL:
				AnIXSIZ = 0x0800;
				tok.u32++;
				break;
			case DOTB:               // .B not allowed here...
				goto badmode;
			}

			if (*tok.u32 == '*')
			{                        // scale: *1, *2, *4, *8
				tok.u32++;

				if (*tok.u32 == SYMBOL)
				{
					if (expr(AnEXPR, &AnEXVAL, &AnEXATTR, &AnESYM) != OK)
						return error("scale factor expression must evaluate");
					switch (AnEXVAL)
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
				else if (*tok.u32++ != CONST || *tok.u32 > 8)
					goto badmode;
				else
				{
					switch ((int)*tok.u32++)
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
			}

			if (*tok.u32 == ',')
			{
				// If we got here we didn't get any [] stuff
				// so let's suppress base displacement before
				// branching off
				tok.u32++;
				AnEXTEN |= EXT_BDSIZE0;     // Base displacement null - suppressed
				goto CHECKODn;
			}
			if (*tok.u32++ != ')')         // final ")"
				goto badmode;

			goto AnOK;
		}
		else if (*tok.u32 == '[')
		{                              // ([...
			tok.u32++;
			AnEXTEN |= EXT_FULLWORD;     // Definitely using full extension format, so set bit 8

			// Check to see if base displacement is present
			if (*tok.u32 != CONST && *tok.u32 != SYMBOL)
			{
				AnEXTEN |= EXT_BDSIZE0;
			}
			else
			{
				expr(AnBEXPR, &AnBEXVAL, &AnBEXATTR, &AnESYM);
				if (CHECK_OPTS(OPT_BASE_DISP) && AnBEXVAL == 0 && AnEXATTR != 0)
				{
					// bd=0 so let's optimise it out
					AnEXTEN|=EXT_BDSIZE0;
				}
				else if (*tok.u32 == DOTL)
				{						// ([bd.l,...
						AnEXTEN |= EXT_BDSIZEL;
						tok.u32++;
				}
				else
				{						// ([bd[.w],... or ([bd,...
					// Is .W forced here?
					if (*tok.u32 == DOTW)
					{
						AnEXTEN |= EXT_BDSIZEW;
						tok.u32++;
					}
					else
					{
						// Defined, absolute values from $FFFF8000..$00007FFF get optimized
						// to absolute short
						if (CHECK_OPTS(OPT_ABS_SHORT)
							&& ((AnBEXATTR & (TDB | DEFINED)) == DEFINED)
							&& (((uint32_t)AnBEXVAL + 0x8000) < 0x10000))
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

				if (*tok.u32 == ',')
					tok.u32++;
				//else
				//	return error("Comma expected after base displacement");
			}

			// Check for address register or PC, suppress base register
			// otherwise

			if (*tok.u32 == KW_PC)
			{					// ([bd,PC,...
				AnREG = (7 << 3) | 3;	// PC is special case - stuff 011 to register field and 111 to the mode field
				tok.u32++;
			}
			else if ((*tok.u32 >= KW_A0) && (*tok.u32 <= KW_A7))
			{					// ([bd,An,...
				AnREG = (6 << 3) | *tok.u32 & 7;
				tok.u32++;
			}
			else if ((*tok.u32 >= KW_D0) && (*tok.u32 <= KW_D7))
			{
				// ([bd,Dn,...
				AnREG = (6 << 3);
				AnEXTEN |= ((*tok.u32 & 7) << 12);
				AnEXTEN |= EXT_D;
				AnEXTEN |= EXT_BS; // Oh look, a data register! Which means that base register is suppressed
				tok.u32++;

				// Check for size
				{
				// ([bd,An/PC],Xn.W/L...)
				switch ((int)*tok.u32)
				{
				// Index reg size: <empty> | .W | .L
				case DOTW:
					tok.u32++;
					break;
				default:
					break;
				case DOTL:
					AnEXTEN |= EXT_L;
					tok.u32++;
					break;
				case DOTB:
					// .B not allowed here...
					goto badmode;
				}
				}

				// Check for scale
				if (*tok.u32 == '*')			// ([bd,An/PC],Xn*...)
				{                           // scale: *1, *2, *4, *8
					tok.u32++;

					if (*tok.u32 == SYMBOL)
					{
						if (expr(AnEXPR, &AnEXVAL, &AnEXATTR, &AnESYM) != OK)
							return error("scale factor expression must evaluate");

						switch (AnEXVAL)
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
					else if (*tok.u32++ != CONST || *tok.u32 > 8)
						goto badmode;
					else
					{
						switch ((int)*tok.u32++)
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
				}
				if (*tok.u32 == ']')  // ([bd,Dn]...
				{
					tok.u32++;
					goto IS_SUPPRESSEDn;
				}
			}
			else if (*tok.u32 == ']')
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
			if (*tok.u32 == ']')
			{
				//([bd,An/PC],Xn,od)
				// Check for Xn
				tok.u32++;

				if (*tok.u32 == ')')
				{
					//Xn and od are non existent, get out of jail free card
					tok.u32++;
					AMn = MEMPRE;			// ([bc,An,Xn],od) with no Xn and od
					AnEXTEN |= EXT_IS | EXT_IISPREN;	//Suppress Xn and od
					goto AnOK;
				}
				else if (*tok.u32 != ',')
					return error("comma expected after ]");
				else
					tok.u32++;				// eat the comma

				if ((*tok.u32 >= KW_A0) && (*tok.u32 <= KW_A7))
				{
					AnIXREG = ((*tok.u32 & 7) << 12);
					AnEXTEN |= EXT_A;
					tok.u32++;
				}
				else if ((*tok.u32 >= KW_D0) && (*tok.u32 <= KW_D7))
				{
					AnEXTEN |= ((*tok.u32 & 7) << 12);
					AnEXTEN |= EXT_D;
					tok.u32++;
				}
				else
				{
					//No index found, suppress it
					AnEXTEN |= EXT_IS;
					tok.u32--;					// Rewind tok to point to the comma
					goto IS_SUPPRESSEDn;	// https://xkcd.com/292/ - what does he know anyway?
				}

				// Check for size
				{
					// ([bd,An/PC],Xn.W/L...)
					switch ((int)*tok.u32)
					{
					// Index reg size: <empty> | .W | .L
					case DOTW:
						tok.u32++;
						break;
					default:
						break;
					case DOTL:
						AnEXTEN |= EXT_L;
						tok.u32++;
						break;
					case DOTB:
						// .B not allowed here...
						goto badmode;
					}
				}

				// Check for scale
				if (*tok.u32 == '*')                   // ([bd,An/PC],Xn*...)
				{                                  // scale: *1, *2, *4, *8
					tok.u32++;

					if (*tok.u32 == SYMBOL)
					{
						if (expr(AnEXPR, &AnEXVAL, &AnEXATTR, &AnESYM) != OK)
							return error("scale factor expression must evaluate");

						switch (AnEXVAL)
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
					else if (*tok.u32++ != CONST || *tok.u32 > 8)
						goto badmode;
					else
					{
						switch ((int)*tok.u32++)
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
				}

				// Check for od
				if (*tok.u32 == ')')	// ([bd,An/PC],Xn)
				{
					//od is non existant, get out of jail free card
					AMn = MEMPOST;		// let's say it's ([bd,An],Xn,od) with od=0 then
					AnEXTEN |= EXT_IISPOSN; // No outer displacement
					tok.u32++;
					goto AnOK;
				}
				else if (*tok.u32 != ',')
					return error("comma expected");
				else
					tok.u32++;	// eat the comma

				CHECKODn:
				if (expr(AnEXPR, &AnEXVAL, &AnEXATTR, &AnESYM) != OK)
					goto badmode;

				if (CHECK_OPTS(OPT_BASE_DISP) && (AnEXVAL == 0))
				{
					// od=0 so optimise it out
					AMn = MEMPOST;		 // let's say it's ([bd,An],Xn,od) with od=0 then
					AnEXTEN |= EXT_IISPOSN; // No outer displacement
					tok.u32++;
					goto AnOK;
				}

				// ([bd,An/PC],Xn,od)
				if (*tok.u32 == DOTL)
				{
					// expr.L
					AnEXTEN |= EXT_IISPOSL; // Long outer displacement
					AMn = MEMPOST;
					tok.u32++;

					// Defined, absolute values from $FFFF8000..$00007FFF get
					// optimized to absolute short
					if (CHECK_OPTS(OPT_ABS_SHORT)
						&& ((AnEXATTR & (TDB | DEFINED)) == DEFINED)
						&& (((uint32_t)AnEXVAL + 0x8000) < 0x10000))
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
					if (*tok.u32 == DOTW)
					{
						tok.u32++;
					}
				}

				// Check for final closing parenthesis
				if (*tok.u32 == ')')
				{
					tok.u32++;
					goto AnOK;
				}
				else
					return error("Closing parenthesis missing on addressing mode");

IS_SUPPRESSEDn:

				// Check for od
				if (*tok.u32 == ')')	// ([bd,An/PC],Xn)
				{
					//od is non existant, get out of jail free card
					AMn = MEMPOST;		// let's say it's ([bd,An],Xn,od) with od=0 then
					AnEXTEN |= EXT_IISNOIN; // No outer displacement
					tok.u32++;
					goto AnOK;
				}
				else if (*tok.u32!=',')
					return error("comma expected");
				else
					tok.u32++;	// eat the comma

                if ((*tok.u32 != CONST) && (*tok.u32 != SYMBOL))
					goto badmode;

				expr(AnEXPR, &AnEXVAL, &AnEXATTR, &AnESYM);

				if (CHECK_OPTS(OPT_BASE_DISP) && (AnEXVAL == 0))
				{
					// od=0 so optimise it out
					AMn = MEMPOST;		 // let's say it's ([bd,An],Xn,od) with od=0 then
					AnEXTEN |= EXT_IISNOIN; // No outer displacement
					tok.u32++;
					goto AnOK;
				}

				// ([bd,An/PC],Xn,od)
				if (*tok.u32 == DOTL)
				{
					// expr.L
					tok.u32++;
					AMn = MEMPOST;
					AnEXTEN |= EXT_IISNOIL; // Long outer displacement with IS suppressed
				}
				else
				{
					// expr[.W][]
					AnEXTEN |= EXT_IISNOIW; // Word outer displacement with IS suppressed
					AMn = MEMPRE;

					if (*tok.u32 == DOTW)
					{
						//AnEXTEN|=EXT_IISNOIW; // Word outer displacement
						AMn = MEMPOST;
						tok.u32++;
					}
					// Defined, absolute values from $FFFF8000..$00007FFF get
					// optimized to absolute short
					else if (CHECK_OPTS(OPT_BASE_DISP)
						&& ((AnEXATTR & (TDB | DEFINED)) == DEFINED)
						&& (((uint32_t)AnEXVAL + 0x8000) < 0x10000))
					{
						//AnEXTEN|=EXT_IISNOIW; // Word outer displacement with IS suppressed
						warn("outer displacement absolute value from $FFFF8000..$00007FFF optimised to absolute short");
					}
				}

				// Check for final closing parenthesis
				if (*tok.u32 == ')')
				{
					tok.u32++;
					goto AnOK;
				}
				else
					return error("Closing parenthesis missing on addressing mode");
			}
			else if (*tok.u32 == ',')
			{
				*tok.u32++;			// ([bd,An,Xn.size*scale],od)

				//Check for Xn
				if ((*tok.u32 >= KW_A0) && (*tok.u32 <= KW_A7))
				{
					AnEXTEN |= ((*tok.u32 & 7) << 12);
					AnEXTEN |= EXT_A;
					tok.u32++;
				}
				else if ((*tok.u32 >= KW_D0) && (*tok.u32 <= KW_D7))
				{
					AnEXTEN |= ((*tok.u32 & 7) << 12);
					AnEXTEN |= EXT_D;
					tok.u32++;
				}

				// Check for size
				{
				// ([bd,An/PC],Xn.W/L...)
				switch ((int)*tok.u32)
				{
				// Index reg size: <empty> | .W | .L
				case DOTW:
					tok.u32++;
					break;
				default:
					break;
				case DOTL:
					tok.u32++;
					AnEXTEN |= EXT_L;
					break;
				case DOTB:
					// .B not allowed here...
					goto badmode;
				}
				}

				// Check for scale
				if (*tok.u32 == '*')			// ([bd,An/PC],Xn*...)
				{                           // scale: *1, *2, *4, *8
					tok.u32++;

					if (*tok.u32 == SYMBOL)
					{
						if (expr(AnEXPR, &AnEXVAL, &AnEXATTR, &AnESYM) != OK)
							return error("scale factor expression must evaluate");
						switch (AnEXVAL)
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
					else if (*tok.u32++ != CONST || *tok.u32 > 8)
						goto badmode;
					else
					{
						switch ((int)*tok.u32++)
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
				}

				//Check for ]
				if (*tok.u32 != ']')
					return error("Expected closing bracket ]");
				tok.u32++;			// Eat the bracket

				//Check for od
				if (*tok.u32 == ')')	// ([bd,An/PC,Xn]...
				{
					//od is non existant, get out of jail free card
					//AnEXVAL=0;		// zero outer displacement
					AMn = MEMPRE;			// let's say it's ([bd,An,Xn],od) with od suppressed then
					AnEXTEN |= EXT_IISPREN; // No outer displacement
					tok.u32++;
					goto AnOK;
				}
                else if (*tok.u32++ != ',')
					return error("comma expected after ]");

				if (*tok.u32 == SYMBOL || *tok.u32 == CONST)
				{
					if (expr(AnEXPR, &AnEXVAL, &AnEXATTR, &AnESYM) != OK)
						goto badmode;

					if (CHECK_OPTS(OPT_BASE_DISP) && (AnEXVAL == 0) && (AnEXATTR & DEFINED))
					{
						// od=0 so optimise it out
						AMn = MEMPRE;		 // let's say it's ([bd,An],Xn,od) with od=0 then
						AnEXTEN |= EXT_IISPRE0; // No outer displacement
						tok.u32++;
						goto AnOK;
					}
				}

				// ([bd,An/PC,Xn],od)
				if (*tok.u32 == DOTL)
				{
					// expr.L
					AMn = MEMPRE;
					tok.u32++;
					AnEXTEN |= EXT_IISPREL;
				}
				else
				{
					// expr.[W]
					AMn = MEMPRE;
					int expr_size = EXT_IISPREW; // Assume we have a .w value

					if ((AnEXVAL + 0x8000) > 0x10000)
					{
						// Long value, so mark it as such for now
						expr_size = EXT_IISPREL;

						// Defined, absolute values from $FFFF8000..$00007FFF
						// get optimized to absolute short
                        if (CHECK_OPTS(OPT_BASE_DISP)
							&& ((AnEXATTR & (TDB | DEFINED)) == DEFINED)
							&& (((uint32_t)AnEXVAL + 0x8000) < 0x10000))
						{
							expr_size = EXT_IISPREW;
							warn("outer displacement absolute value from $FFFF8000..$00007FFF optimised to absolute short");
						}
					}

					AnEXTEN |= expr_size; // Assume we have a .w value

					// Is .W forced here?
					if (*tok.u32 == DOTW)
					{
						tok.u32++;

						if (expr_size == EXT_IISPREL)
							return error("outer displacement value does not fit in .w size");
					}
				}

				// Check for final closing parenthesis
				if (*tok.u32 == ')')
				{
					tok.u32++;
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
			if (*tok.u32 == ')')
			{
				tok.u32++;
				goto CHK_FOR_DISPn;
			}

			// Otherwise, check for PC & etc displacements...
			if (*tok.u32++ != ',')
				goto badmode;

			if ((*tok.u32 >= KW_A0) && (*tok.u32 <= KW_A7))
			{
				AnREG = *tok.u32 & 7;
				tok.u32++;

				if (*tok.u32 == ',')
				{
					AMn = AINDEXED;
					goto AMn_IXN;
				}
				else if (*tok.u32 == ')')
				{
					AMn = ADISP;
					tok.u32++;
					goto AnOK;
				}
				else
					goto badmode;
			}
			else if (*tok.u32 == KW_PC)
			{
				if (*++tok.u32 == ',')
				{                             // expr(PC,Xn...)
					AMn = PCINDEXED;
					goto AMn_IXN;
				}
				else if (*tok.u32 == ')')
				{
					AMn = PCDISP;             // expr(PC)
					tok.u32++;
					goto AnOK;
				}
				else
					goto badmode;
			}
			else
				goto badmode;
		}
	}
	else if (*tok.u32 == '-' && tok.u32[1] == '(' && ((tok.u32[2] >= KW_A0) && (tok.u32[2] <= KW_A7)) && tok.u32[3] == ')')
	{
		AMn = APREDEC;
		AnREG = tok.u32[2] & 7;
		tok.u32 += 4;
	}
	else if (*tok.u32 == KW_CCR)
	{
		AMn = AM_CCR;
		tok.u32++;
		goto AnOK;
	}
	else if (*tok.u32 == KW_SR)
	{
		AMn = AM_SR;
		tok.u32++;
		goto AnOK;
	}
	else if (*tok.u32 == KW_USP)
	{
		AMn = AM_USP;
		tok.u32++;
		AnREG = 2;	//Added this for the case of USP used in movec (see CREGlut in mach.c). Hopefully nothing gets broken!
		goto AnOK;
	}
	else if ((*tok.u32 >= KW_IC40) && (*tok.u32 <= KW_BC40))
	{
		AMn = CACHES;
		AnREG = *tok.u32++ - KW_IC40;

		// After a cache keyword only a comma or EOL is allowed
		if ((*tok.u32 != ',') && (*tok.u32 != EOL))
			return ERROR;
		goto AnOK;
	}
	else if ((*tok.u32 >= KW_SFC) && (*tok.u32 <= KW_CRP))
	{
		AMn = CREG;
		AnREG = (*tok.u32++) - KW_SFC;
		goto AnOK;
	}
	else if ((*tok.u32 >= KW_FP0) && (*tok.u32 <= KW_FP7))
	{
		AMn = FREG;
		AnREG = (*tok.u32++ & 7);
	}
	else if ((*tok.u32 >= KW_FPIAR) && (*tok.u32 <= KW_FPCR))
	{
		AMn = FPSCR;
		AnREG = (1 << ((*tok.u32++) - KW_FPIAR + 10));
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
		if (*tok.u32 == DOTW)
		{
			// expr.W
			tok.u32++;
			AMn = ABSW;

			if (((AnEXATTR & (TDB | DEFINED)) == DEFINED) && (AnEXVAL < 0x10000))
				AnEXVAL = (int32_t)(int16_t)AnEXVAL;  // Sign extend value

			goto AnOK;
		}
		else if (*tok.u32 != '(')
		{
			// expr[.L]
			AMn = ABSL;

			// Defined, absolute values from $FFFF8000..$00007FFF get optimized
			// to absolute short
			if (CHECK_OPTS(OPT_ABS_SHORT)
				&& ((AnEXATTR & (TDB | DEFINED)) == DEFINED)
				&& (((uint32_t)AnEXVAL + 0x8000) < 0x10000))
			{
				AMn = ABSW;

				if (sbra_flag)
					warn("absolute value from $FFFF8000..$00007FFF optimised to absolute short");
			}

			// Is .L forced here?
			if (*tok.u32 == DOTL)
			{
				tok.u32++;
				AMn = ABSL;
			}

			goto AnOK;
		}

		tok.u32++;

		if ((*tok.u32 >= KW_A0) && (*tok.u32 <= KW_A7))
		{
			AnREG = *tok.u32++ & 7;

			if (*tok.u32 == ')')
			{
				AMn = ADISP;
				tok.u32++;
				goto AnOK;
			}

			AMn = AINDEXED;
			goto AMn_IXN;
		}
		else if (*tok.u32 == KW_PC)
		{
			if (*++tok.u32 == ')')
			{
				AMn = PCDISP;
				tok.u32++;
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
