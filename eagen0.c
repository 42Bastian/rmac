//
// RMAC - Renamed Macro Assembler for all Atari computers
// EAGEN0.C - Effective Address Code Generation
//            Generated Code for eaN (Included twice by "eagen.c")
// Copyright (C) 199x Landon Dyer, 2011-2021 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source utilised with the kind permission of Landon Dyer
//

int eaNgen(WORD siz)
{
	uint32_t v = (uint32_t)aNexval;
	WORD w = (WORD)(aNexattr & DEFINED);
	WORD tdb = (WORD)(aNexattr & TDB);
	uint32_t vbd = (uint32_t)aNbdexval;
	WORD wbd = (WORD)(aNbdexattr & DEFINED);
	WORD tdbbd = (WORD)(aNbdexattr & TDB);
	uint8_t extDbl[12];

	switch (amN)
	{
	// "Do nothing" - they're in the opword
	case DREG:
	case AREG:
	case AIND:
	case APOSTINC:
	case APREDEC:
	case AM_USP:
	case AM_CCR:
	case AM_SR:
	case AM_NONE:
		// This is a performance hit, though
		break;

	case ADISP:
		// expr(An)
		if (w)
		{
			// Just deposit it
			if (tdb)
				MarkRelocatable(cursect, sloc, tdb, MWORD, NULL);

			if ((v == 0) && CHECK_OPTS(OPT_OUTER_DISP) && !movep)
			{
				// If expr is 0, size optimise the opcode. Generally the lower
				// 6 bits of the opcode for expr(ax) are 101rrr where rrr=the
				// number of the register, then followed by a word containing
				// 'expr'. We need to change that to 010rrr.
				if ((siz & 0x8000) == 0)
				{
					chptr_opcode[0] &= ((0xFFC7 >> 8) & 255); // mask off bits
					chptr_opcode[1] &= 0xFFC7 & 255;          // mask off bits
					chptr_opcode[0] |= ((0x0010 >> 8) & 255); // slap in 010 bits
					chptr_opcode[1] |= 0x0010 & 255;          // slap in 010 bits
				}
				else
				{
					// Special case for move ea,ea: there are two ea fields
					// there and we get a signal if it's the second ea field
					// from m_ea - siz's 16th bit is set
					chptr_opcode[0] &= ((0xFE3F >> 8) & 255); // mask off bits
					chptr_opcode[1] &= 0xFE3F & 255;          // mask off bits
					chptr_opcode[0] |= ((0x0080 >> 8) & 255); // slap in 010 bits
					chptr_opcode[1] |= 0x0080 & 255;          // slap in 010 bits
				}

				if (optim_warn_flag)
					warn("o3: 0(An) converted to (An)");

				return OK;
			}

			if ((v + 0x8000) >= 0x18000)
				return error(range_error);

			D_word(v);
		}
		else
		{
			// Arrange for fixup later on
			AddFixup(FU_WORD | FU_SEXT, sloc, aNexpr);
			D_word(0);
		}

		break;

	case PCDISP:
		if (w)
		{
			// Just deposit it
			if ((aNexattr & TDB) == cursect)
				v -= (uint32_t)sloc;
			else if ((aNexattr & TDB) != ABS)
				error(rel_error);

			if (v + 0x8000 >= 0x10000)
				return error(range_error);

			D_word(v);
		}
		else
		{
			// Arrange for fixup later on
			AddFixup(FU_WORD | FU_SEXT | FU_PCREL, sloc, aNexpr);
			D_word(0);
		}

		break;

	case AINDEXED:
		// Compute ixreg and size+scale
		w = (WORD)((aNixreg << 12) | aNixsiz);

		if (aNexattr & DEFINED)
		{
			// Deposit a byte...
			if (tdb)
				// Can't mark bytes
				return error(abs_error);

			if (v + 0x80 >= 0x180)
				return error(range_error);

			w |= v & 0xFF;
			D_word(w);
		}
		else
		{
			// Fixup the byte later
			AddFixup(FU_BYTE | FU_SEXT, sloc + 1, aNexpr);
			D_word(w);
		}

		break;

	case PCINDEXED:
		// Compute ixreg and size+scale
		w = (WORD)((aNixreg << 12) | aNixsiz);

		if (aNexattr & DEFINED)
		{
			// Deposit a byte...
			if ((aNexattr & TDB) == cursect)
				v -= (uint32_t)sloc;
			else if ((aNexattr & TDB) != ABS)
				error(rel_error);

			if (v + 0x80 >= 0x100)
				return error(range_error);

			w |= v & 0xFF;
			D_word(w);
		}
		else
		{
			// Fixup the byte later
			AddFixup(FU_WBYTE | FU_SEXT | FU_PCREL, sloc, aNexpr);
			D_word(w);
		}

		break;

	case IMMED:
		switch (siz)
		{
		case SIZB:
			if (w)
			{
				if (tdb)
					return error("illegal byte-sized relative reference");

				if (v + 0x100 >= 0x200)
					return error(range_error);

				D_word(v & 0xFF);
			}
			else
			{
				AddFixup(FU_BYTE | FU_SEXT, sloc + 1, aNexpr);
				D_word(0);
			}

			break;

		case SIZW:
		case SIZN:
			if (w)
			{
				if (v + 0x10000 >= 0x20000)
					return error(range_error);

				if (tdb)
					MarkRelocatable(cursect, sloc, tdb, MWORD, NULL);

				D_word(v);
			}
			else
			{
				AddFixup(FU_WORD | FU_SEXT, sloc, aNexpr);
				D_word(0);
			}

			break;

		case SIZL:
			if (w)
			{
				if (tdb)
					MarkRelocatable(cursect, sloc, tdb, MLONG, NULL);

				D_long(v);
			}
			else
			{
				AddFixup(FU_LONG, sloc, aNexpr);
				D_long(0);
			}

			break;

		case SIZS:
			// 68881/68882/68040 only
			if (w)
			{
//Would a floating point value *ever* need to be fixed up as if it were an address? :-P
//				if (tdb)
//					MarkRelocatable(cursect, sloc, tdb, MSINGLE, NULL);

				// The value passed back from expr() is an internal C double;
				// so we have to access it as such then convert it to an
				// IEEE-754 float so we can store it as such in the instruction
				// stream here.
				PTR p;
				p.u64 = &aNexval;
				float f = (float)*p.dp;
				uint32_t ieee754 = FloatToIEEE754(f);
				D_long(ieee754);
			}
			else
			{
				AddFixup(FU_FLOATSING, sloc, aNexpr);
				D_long(0);	// IEEE-754 zero is all zeroes
			}

    		break;

		case SIZD:
			// 68881/68882/68040 only
			if (w)
			{
//Would a floating point value *ever* need to be fixed up as if it were an address? :-P
//				if (tdb)
//					MarkRelocatable(cursect, sloc, tdb, MDOUBLE, NULL);

				PTR p;
				p.u64 = &aNexval;
				double d = *p.dp;
				uint64_t ieee754 = DoubleToIEEE754(d);
				D_quad(ieee754);
			}
			else
			{
				AddFixup(FU_FLOATDOUB, sloc, aNexpr);
				D_quad(0LL);	// IEEE-754 zero is all zeroes
			}

			break;

		case SIZX:
			// 68881/68882/68040 only
			if (w)
			{
//Would a floating point value *ever* need to be fixed up as if it were an address? :-P
//				if (tdb)
//					MarkRelocatable(cursect, sloc, tdb, MEXTEND, NULL);

				PTR p;
				p.u64 = &aNexval;
				DoubleToExtended(*p.dp, extDbl);
				D_extend(extDbl);
			}
			else
			{
				// Why would this be anything other than a floating point
				// expression???  Even if there were an undefined symbol in
				// the expression, how would that be relevant?  I can't see
				// any use case where this would make sense.
				AddFixup(FU_FLOATDOUB, sloc, aNexpr);
				memset(extDbl, 0, 12);
				D_extend(extDbl);
			}

			break;

		default:
			// IMMED size problem
			interror(1);
		}

		break;

    case SIZP:
		// 68881/68882/68040 only
		return error("Sorry, .p constant format is not implemented yet!");
		break;

	case ABSW:
		if (w) // Defined
		{
			if (tdb)
				MarkRelocatable(cursect, sloc, tdb, MWORD, NULL);

			if (v + 0x8000 >= 0x10000)
				return error(range_error);

			D_word(v);
		}
		else
		{
			AddFixup(FU_WORD | FU_SEXT, sloc, aNexpr);
			D_word(0);
		}

		break;

	case ABSL:
		if (w) // Defined
		{
			if (CHECK_OPTS(OPT_PC_RELATIVE))
			{
				if ((aNexattr & (DEFINED | REFERENCED | EQUATED)) == (DEFINED | REFERENCED))
					return error("relocation not allowed when o10 is enabled");
			}

			if (tdb)
				MarkRelocatable(cursect, sloc, tdb, MLONG, NULL);

			D_long(v);
		}
		else
		{
			AddFixup(FU_LONG, sloc, aNexpr);
			D_long(0);
		}

		break;

	case DINDW:
		D_word((0x190 | (aNixreg << 12)));
		break;

	case DINDL:
		D_word((0x990 | (aNixreg << 12)));
		break;

	case ABASE:
	case MEMPOST:
	case MEMPRE:
	case PCBASE:
	case PCMPOST:
	case PCMPRE:
		D_word(aNexten);

		// Deposit bd (if not suppressed)
		if ((aNexten & 0x0030) == EXT_BDSIZE0)
		{
			// Don't deposit anything (suppressed)
		}
		else if ((aNexten & 0x0030) == EXT_BDSIZEW)
		{
			// Deposit word bd
			if (wbd)
			{
				// Just deposit it
				if (tdb)
					MarkRelocatable(cursect, sloc, tdbbd, MWORD, NULL);

				if (vbd + 0x8000 >= 0x10000)
					return error(range_error);

				D_word(vbd);
			}
			else
			{
				// Arrange for fixup later on
				AddFixup(FU_WORD | FU_SEXT | FU_PCRELX, sloc, aNbexpr);
				D_word(0);
			}
		}
		else
		{
			// Deposit long bd
			if (wbd)
			{
				// Just deposit it
				if (tdbbd)
					MarkRelocatable(cursect, sloc, tdbbd, MLONG, NULL);

				D_long(vbd);
			}
			else
			{
				// Arrange for fixup later on
				AddFixup(FU_LONG, sloc, aNbexpr);
				D_long(0);
			}
		}

		// Deposit od (if not suppressed)
		if ((aNexten & 7) == EXT_IISPRE0 || (aNexten & 7) == EXT_IISPREN
			|| (aNexten & 7) == EXT_IISNOIN || (aNexten & 7) == EXT_IISPOSN)
		{
			// Don't deposit anything (suppressed)
		}
		else if ((aNexten & 7) == EXT_IISPREW
			|| (aNexten & 7) == EXT_IISPOSW || (aNexten & 7) == EXT_IISNOIW)
		{
			// Deposit word od
			if (w)
			{
				// Just deposit it
				if (tdb)
					MarkRelocatable(cursect, sloc, tdb, MWORD, NULL);

				if (v + 0x8000 >= 0x10000)
					return error(range_error);

				D_word(v);
			}
			else
			{
				// Arrange for fixup later on
				AddFixup(FU_WORD | FU_SEXT, sloc, aNexpr);
				D_word(0);
			}
		}
		else
		{
			// Deposit long od
			if (w)
			{
				// Just deposit it
				if (tdb)
					MarkRelocatable(cursect, sloc, tdb, MLONG, NULL);

				D_long(v);
			}
			else
			{
				// Arrange for fixup later on
				AddFixup(FU_LONG | FU_SEXT, sloc, aNexpr);
				D_long(0);
			}
		}

		break;
		//return error("unsupported 68020 addressing mode");

	default:
		// Bad addressing mode in ea gen
		interror(3);
	}

	return OK;
}

// Undefine dirty macros
#undef eaNgen
#undef amN
#undef aNexattr
#undef aNexval
#undef aNexpr
#undef aNixreg
#undef aNixsiz
#undef aNexten
#undef aNbexpr
#undef aNbdexval
#undef aNbdexattr
#undef AnESYM

