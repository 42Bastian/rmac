//
// RMAC - Reboot's Macro Assembler for all Atari computers
// PARMODE.C - Addressing Modes Parser Include
// Copyright (C) 199x Landon Dyer, 2011-2020 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source utilised with the kind permission of Landon Dyer
//

// This file is included (twice) to parse two addressing modes, into slightly
// different var names
{
	uint64_t scaleval;			// Expression's value
	TOKEN scaleexpr[EXPRSIZE];	// Expression
	WORD scaleattr;				// Expression's attribute
	SYM * scaleesym;			// External symbol involved in expr

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
	// ([bd,An],Xn[.siz][*scale],od)
	// ([bd,An,Xn[.siz][*scale]],od)
	// ([bd,PC],Xn[.siz][*scale],od)
	// ([bd,PC,Xn[.siz][*scale]],od)
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
			// Since index register isn't used here, store register number in this field
			AnIXREG = *tok++ & 7;                                // (Dn)

			if (*tok == ')')
			{
				tok++;
				AnEXTEN |= EXT_FULLWORD;    // Definitely using full extension format, so set bit 8
				AnEXTEN |= EXT_BS;          // Base register suppressed
				AnEXTEN |= EXT_BDSIZE0;     // Base displacement null
				AnEXTEN |= EXT_IISPOSN;     // Indirect Postindexed with Null Outer Displacement
				AMn = MEMPOST;
				AnREG = 6 << 3;		// stuff 110 to mode field
				goto AnOK;
			}
			else if (*tok == 'L')
			{
				AMn = DINDL;									 // (Dn.l)
				AnEXTEN = 1 << 11;   // Long index size
				tok++;
			}
			else if (*tok == 'W')                                // (Dn.w)
			{
				AMn = DINDW;
				AnEXTEN = 0 << 11;   // Word index size
				tok++;
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
			{
				return error("(Dn) error");
			}

			if (*tok == '*')
			{                        // scale: *1, *2, *4, *8
				tok++;

				if (*tok == SYMBOL)
				{
					if (expr(scaleexpr, &scaleval, &scaleattr, &scaleesym) != OK)
						return error("scale factor expression must evaluate");

					switch (scaleval)
					{
					case 1:
						break;
					case 2:
						AnIXSIZ |= TIMES2;
						AnEXTEN |= 1 << 9;
						break;
					case 4:
						AnIXSIZ |= TIMES4;
						AnEXTEN |= 2 << 9;
						break;
					case 8:
						AnIXSIZ |= TIMES8;
						AnEXTEN |= 3 << 9;
						break;
					default:
						goto badmode;
					}
				}
				else if (*tok++ != CONST)
					goto badmode;
				else
				{
					switch ((int)*tok++)
					{
					case 1:
						break;
					case 2:
						AnIXSIZ |= TIMES2;
						AnEXTEN |= 1 << 9;
						break;
					case 4:
						AnIXSIZ |= TIMES4;
						AnEXTEN |= 2 << 9;
						break;
					case 8:
						AnIXSIZ |= TIMES8;
						AnEXTEN |= 3 << 9;
						break;
					default:
						goto badmode;
					}

					tok++;  // Take into account that constants are 64-bit
				}
			}

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
			else if (*tok == ',')
			{
				tok++;  // eat the comma
				// It might be (Dn[.wl][*scale],od)
				// Maybe this is wrong and we have to write some code here
				// instead of reusing that path...
				AnEXTEN |= EXT_FULLWORD;	// Definitely using full extension format, so set bit 8
				AnEXTEN |= EXT_BS;	 // Base displacement null - suppressed
				AnEXTEN |= AnIXREG << 12;
				goto CHECKODn;
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

				if (*tok == SYMBOL)
				{
					if (expr(scaleexpr, &scaleval, &scaleattr, &scaleesym) != OK)
						return error("scale factor expression must evaluate");

					switch (scaleval)
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
				else if (*tok++ != CONST)
					goto badmode;
				else
				{
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

					tok++;  // Take into account that constants are 64-bit
				}
			}

			if (*tok == ',')
			{
				// If we got here we didn't get any [] stuff
				// so let's suppress base displacement before
				// branching off
				tok++;
				AnEXTEN |= EXT_BDSIZE0;     // Base displacement null - suppressed
				goto CHECKODn;
			}
			if (*tok++ != ')')         // final ")"
				goto badmode;

			goto AnOK;
		}
		else if (*tok == '[')
		{                              // ([...
			tok++;
			AnEXTEN |= EXT_FULLWORD;     // Definitely using full extension format, so set bit 8

			// Check to see if base displacement is present
			if (*tok != CONST && *tok != SYMBOL)
			{
				AnEXTEN |= EXT_BDSIZE0;
			}
			else
			{
				expr(AnBEXPR, &AnBEXVAL, &AnBEXATTR, &AnESYM);

				if (CHECK_OPTS(OPT_020_DISP) && (AnBEXVAL == 0) && (AnEXATTR != 0))
				{
					// bd = 0 so let's optimise it out
					AnEXTEN |= EXT_BDSIZE0;
				}
				else if (*tok == DOTL)
				{
					// ([bd.l,...
					AnEXTEN |= EXT_BDSIZEL;
					tok++;
				}
				else
				{
					// ([bd[.w],... or ([bd,...
					// Is .W forced here?
					if (*tok == DOTW)
					{
						AnEXTEN |= EXT_BDSIZEW;
						tok++;
					}
					else
					{
						// Defined, absolute values from $FFFF8000..$00007FFF
						// get optimized to absolute short
						if (CHECK_OPTS(OPT_020_DISP)
							&& ((AnBEXATTR & (TDB | DEFINED)) == DEFINED)
							&& (((uint32_t)AnBEXVAL + 0x8000) < 0x10000))
						{
							AnEXTEN |= EXT_BDSIZEW;

							if (optim_warn_flag)
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
				AnREG = (6 << 3) | (*tok & 7);
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

				// Check for scale
				if (*tok == '*')			// ([bd,An/PC],Xn*...)
				{                           // scale: *1, *2, *4, *8
					tok++;

					if (*tok == SYMBOL)
					{
						if (expr(scaleexpr, &scaleval, &scaleattr, &scaleesym) != OK)
							return error("scale factor expression must evaluate");

						switch (scaleval)
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
					else if (*tok++ != CONST)
						goto badmode;
					else
					{
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

						tok++;  // Take into account that constants are 64-bit
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
				AnREG = 6 << 3;		// stuff 110b to mode field
				AnEXTEN |= EXT_BS;
			}
			else
			{
				goto badmode;
			}

			// At a crossroads here. We can accept either ([bd,An/PC],... or ([bd,An/PC,Xn*scale],...
			if (*tok == ']')
			{
				// ([bd,An/PC],Xn,od)
				// Check for Xn
				tok++;

				if (*tok == ')')
				{
					// Xn and od are non existent, get out of jail free card
					tok++;
					AMn = MEMPRE;			// ([bc,An,Xn],od) with no Xn and od
					AnEXTEN |= EXT_IS | EXT_IISPREN;	// Suppress Xn and od
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
					// No index found, suppress it
					AnEXTEN |= EXT_IS;
					tok--;					// Rewind tok to point to the comma
					goto IS_SUPPRESSEDn;	// https://xkcd.com/292/ - what does he know anyway?
				}

				// Check for size
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

				// Check for scale
				if (*tok == '*')                   // ([bd,An/PC],Xn*...)
				{                                  // scale: *1, *2, *4, *8
					tok++;

					if (*tok == SYMBOL)
					{
						if (expr(scaleexpr, &scaleval, &scaleattr, &scaleesym) != OK)
							return error("scale factor expression must evaluate");

						switch (scaleval)
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
					else if (*tok++ != CONST)
						goto badmode;
					else
					{
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

						tok++;  // Take into account that constants are 64-bit
					}
				}

				// Check for od
				if (*tok == ')')	// ([bd,An/PC],Xn)
				{
					// od is non existent, get out of jail free card
					AMn = MEMPOST;		// let's say it's ([bd,An],Xn,od) with od=0 then
					AnEXTEN |= EXT_IISPOSN; // No outer displacement
					tok++;
					goto AnOK;
				}
				else if (*tok != ',')
					return error("comma expected");
				else
					tok++;	// eat the comma

CHECKODn:
				if (expr(AnEXPR, &AnEXVAL, &AnEXATTR, &AnESYM) != OK)
					goto badmode;

				if (CHECK_OPTS(OPT_020_DISP) && (AnEXATTR & DEFINED) && (AnEXVAL == 0))
				{
					// od = 0 so optimise it out
					AMn = MEMPOST;		 // let's say it's ([bd,An],Xn,od) with od=0 then
					AnEXTEN |= EXT_IISPOSN; // No outer displacement
					tok++;
					goto AnOK;
				}

				// ([bd,An/PC],Xn,od)
					// Is .W forced here?
				if (*tok == DOTW)
				{
					tok++;
					// od[.W]
					AnEXTEN |= EXT_IISPOSW; // Word outer displacement
					AMn = MEMPOST;
				}
				else
				{
					// Is .L forced here?
					if (*tok == DOTL)
						tok++;				// Doesn't matter, we're going for .L anyway

					// od.L
					if (!(AnEXTEN & EXT_BS))
						AnEXTEN |= EXT_IISPOSL; // Long outer displacement
					else
					{
						// bd is suppressed, so sticking the od size in bd
						AnEXTEN |= EXT_BDSIZEL;
						// And of course the expression has to be copied to
						// AnBEXPR instead of AnEXPR. Yay. :-/
						int i = 0;

						do
						{
							AnBEXPR[i] = AnEXPR[i];
							i++;
						} while (AnEXPR[i] != 'E');

						AnBEXPR[i] = 'E';
					}

					AMn = MEMPOST;

					// Defined, absolute values from $FFFF8000..$00007FFF get
					// optimized to absolute short
					if (CHECK_OPTS(OPT_020_DISP)
						&& ((AnEXATTR & (TDB | DEFINED)) == DEFINED)
						&& (((uint32_t)AnEXVAL + 0x8000) < 0x10000))
					{
						AnEXTEN |= EXT_IISPOSW; // Word outer displacement
						AMn = MEMPOST;
						if (optim_warn_flag)
							warn("absolute value in outer displacement ranging $FFFF8000..$00007FFF optimised to absolute short");
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
					// od is non existent, get out of jail free card
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

				if (CHECK_OPTS(OPT_020_DISP) && (AnEXVAL == 0))
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
					else if (CHECK_OPTS(OPT_020_DISP)
						&& ((AnEXATTR & (TDB | DEFINED)) == DEFINED)
						&& (((uint32_t)AnEXVAL + 0x8000) < 0x10000))
					{
						//AnEXTEN|=EXT_IISNOIW; // Word outer displacement with IS suppressed
						if (optim_warn_flag)
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
				tok++;			// ([bd,An,Xn.size*scale],od)

				// Check for Xn
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

				// Check for scale
				if (*tok == '*')				// ([bd,An/PC],Xn*...)
				{								// scale: *1, *2, *4, *8
					tok++;

					if (*tok == SYMBOL)
					{
						if (expr(scaleexpr, &scaleval, &scaleattr, &scaleesym) != OK)
							return error("scale factor expression must evaluate");

						switch (scaleval)
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
					else if (*tok++ != CONST)
						goto badmode;
					else
					{
						switch ((int)*tok++)
						{
						case 1:
							break;
						case 2:
							AnIXSIZ |= TIMES2;
							AnEXTEN |= 1 << 9;
							break;
						case 4:
							AnIXSIZ |= TIMES4;
							AnEXTEN |= 2 << 9;
							break;
						case 8:
							AnIXSIZ |= TIMES8;
							AnEXTEN |= 3 << 9;
							break;
						default:
							goto badmode;
						}

						tok++;  // Take into account that constants are 64-bit
					}
				}

				// Check for ]
				if (*tok != ']')
					return error("Expected closing bracket ]");
				tok++;			// Eat the bracket

				// Check for od
				if (*tok == ')')	// ([bd,An/PC,Xn]...
				{
					// od is non existent, get out of jail free card
					//AnEXVAL=0;		// zero outer displacement
					AMn = MEMPRE;			// let's say it's ([bd,An,Xn],od) with od suppressed then
					AnEXTEN |= EXT_IISPREN; // No outer displacement
					tok++;
					goto AnOK;
				}
				else if (*tok++ != ',')
					return error("comma expected after ]");

				if (*tok == SYMBOL || *tok == CONST)
				{
					if (expr(AnEXPR, &AnEXVAL, &AnEXATTR, &AnESYM) != OK)
						goto badmode;

					if (CHECK_OPTS(OPT_020_DISP) && (AnEXVAL == 0) && (AnEXATTR & DEFINED))
					{
						// od=0 so optimise it out
						AMn = MEMPRE;		 // let's say it's ([bd,An],Xn,od) with od=0 then
						AnEXTEN |= EXT_IISPRE0; // No outer displacement
						tok++;
						goto AnOK;
					}
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
					AMn = MEMPRE;
					int expr_size = EXT_IISPREW; // Assume we have a .w value

					if ((AnEXVAL + 0x8000) > 0x10000)
					{
						// Long value, so mark it as such for now
						expr_size = EXT_IISPREL;

						// Defined, absolute values from $FFFF8000..$00007FFF
						// get optimized to absolute short
						if (CHECK_OPTS(OPT_020_DISP)
							&& ((AnEXATTR & (TDB | DEFINED)) == DEFINED)
							&& (((uint32_t)AnEXVAL + 0x8000) < 0x10000))
						{
							expr_size = EXT_IISPREW;

							if (optim_warn_flag)
								warn("outer displacement absolute value from $FFFF8000..$00007FFF optimised to absolute short");
						}
					}

					AnEXTEN |= expr_size; // Assume we have a .w value

					// Is .W forced here?
					if (*tok == DOTW)
					{
						tok++;

						if (expr_size == EXT_IISPREL)
							return error("outer displacement value does not fit in .w size");
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
					// Check if we're actually doing d8(An,Dn) or
					// (d16,An,Dn[.size][*scale])
					// TODO: not a very clear cut case from what I can think.
					// The only way to distinguish between the two is to check
					// AnEXVAL and see if it's >127 or <-128. But this doesn't
					// work if AnEXVAL isn't defined yet. For now we fall
					// through to d8(An,Dn) but this might bite us in the arse
					// during fixups...
					if ((AnEXATTR & DEFINED) && (AnEXVAL + 0x80 > 0x100))
					{
						// We're going to treat it as a full extension format
						// with no indirect access and no base displacement/
						// index register suppression
						AnEXTEN |= EXT_FULLWORD;	// Definitely using full extension format, so set bit 8
						AnEXTEN |= EXT_IISPRE0;		// No Memory Indirect Action
						AnEXTEN |= EXT_BDSIZEL;		// Base Displacement Size Long
						tok++;						// Get past the comma

						// Our expression is techically a base displacement,
						// so let's copy it to the relevant variables so
						// eagen0.c can pick it up properly
						//AnBEXPR = AnEXPR;
						AnBEXVAL = AnEXVAL;
						AnBEXATTR = AnEXATTR;

						if ((*tok >= KW_D0) && (*tok <= KW_D7))
						{
							AnEXTEN |= ((*tok++) & 7) << 12;
							// Check for size
							{
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
							if (*tok == '*')		// ([bd,An/PC],Xn*...)
							{						// scale: *1, *2, *4, *8
								tok++;

								if (*tok == SYMBOL)
								{
									if (expr(scaleexpr, &scaleval, &scaleattr, &scaleesym) != OK)
										return error("scale factor expression must evaluate");

									switch (scaleval)
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
								else if (*tok++ != CONST)
									goto badmode;
								else
								{
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

									tok++;  // Take into account that constants are 64-bit
								}
							}

							if (*tok++ != ')')
								return error("Closing parenthesis missing on addressing mode");

							// Let's say that this is the closest to our case
							AMn = MEMPOST;
							goto AnOK;
						}
						else
							goto badmode;
					}

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
					AMn = PCDISP;             // expr(PC)
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
		AnREG = 2;	// Added this for the case of USP used in movec (see CREGlut in mach.c). Hopefully nothing gets broken!
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

			// When PC relative is enforced, check for any symbols that aren't
			// EQU'd, in this case it's an illegal mode
			if ((CHECK_OPTS(OPT_PC_RELATIVE)) && (AnEXATTR & REFERENCED) && (AnEXATTR & DEFINED) && (!(AnEXATTR & EQUATED)))
				return error("relocation not allowed");

			// .L is forced here
			if (*tok == DOTL)
			{
				tok++;
				AMn = ABSL;
			}
			else
			{
				// Defined, absolute values from $FFFF8000..$00007FFF get
				// optimized to absolute short
				if (CHECK_OPTS(OPT_ABS_SHORT)
					&& ((AnEXATTR & (TDB | DEFINED)) == DEFINED)
					&& (((uint32_t)AnEXVAL + 0x8000) < 0x10000))
				{
					AMn = ABSW;

					if (optim_warn_flag)
						warn("absolute value from $FFFF8000..$00007FFF optimised to absolute short");
				}
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
