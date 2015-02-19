//
// RMAC - Reboot's Macro Assembler for the Atari Jaguar Console System
// EAGEN0.C - Effective Address Code Generation
//            Generated Code for eaN (Included twice by "eagen.c")
// Copyright (C) 199x Landon Dyer, 2011 Reboot and Friends
// RMAC derived from MADMAC v1.07 Written by Landon Dyer, 1986
// Source utilised with the kind permission of Landon Dyer
//

int eaNgen(WORD siz)
{
	WORD w;
	VALUE v;
	WORD tdb;

	v = aNexval;
	w = (WORD)(aNexattr & DEFINED);
	tdb = (WORD)(aNexattr & TDB);

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
				rmark(cursect, sloc, tdb, MWORD, NULL);

			if (v + 0x8000 >= 0x18000)
				return error(range_error);

			D_word(v);
		}
		else
		{
			// Arrange for fixup later on 
			AddFixup(FU_WORD|FU_SEXT, sloc, aNexpr);
			D_word(0);
		}

		break;
	case PCDISP:
		if (w)
		{
			// Just deposit it 
			if ((aNexattr & TDB) == cursect)
				v -= (VALUE)sloc;
			else if ((aNexattr & TDB) != ABS)
				error(rel_error);

			if (v + 0x8000 >= 0x10000)
				return error(range_error);

			D_word(v);
		}
		else
		{
			// Arrange for fixup later on 
			AddFixup(FU_WORD|FU_SEXT|FU_PCREL, sloc, aNexpr);
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

			w |= v & 0xff;
			D_word(w);
		}
		else
		{
			// Fixup the byte later
			AddFixup(FU_BYTE|FU_SEXT, sloc+1, aNexpr);
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
				v -= (VALUE)sloc;
			else if ((aNexattr & TDB) != ABS)
				error(rel_error);

			if (v + 0x80 >= 0x100)
				return error(range_error);

			w |= v & 0xff;
			D_word(w);
		}
		else
		{
			// Fixup the byte later 
			AddFixup(FU_WBYTE|FU_SEXT|FU_PCREL, sloc, aNexpr);
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

				D_word(v);
			}
			else
			{
				AddFixup(FU_BYTE|FU_SEXT, sloc+1, aNexpr);
				D_word(0);
			}

			break;
		case SIZW:
		case SIZN:
			if (w)
			{
				if (tdb)
					rmark(cursect, sloc, tdb, MWORD, NULL);

				if (v + 0x10000 >= 0x20000)
					return error(range_error);

				D_word(v);
			}
			else
			{
				AddFixup(FU_WORD|FU_SEXT, sloc, aNexpr);
				D_word(0);
			}

			break;
		case SIZL:
			if (w)
			{
				if (tdb)
					rmark(cursect, sloc, tdb, MLONG, NULL);

				D_long(v);
			}
			else
			{
				AddFixup(FU_LONG, sloc, aNexpr);
				D_long(0);
			}

			break;
		default:
			// IMMED size problem
			interror(1);
		}

		break;
	case ABSW:
		if (w)
		{
			if (tdb)
				rmark(cursect, sloc, tdb, MWORD, NULL);

			if (v + 0x8000 >= 0x10000)
				return error(range_error);

			D_word(v);
		}
		else
		{
			AddFixup(FU_WORD|FU_SEXT, sloc, aNexpr);
			D_word(0);
		}

		break;
	case ABSL:
		if (w)
		{
			if (tdb)
				rmark(cursect, sloc, tdb, MLONG, NULL);

			D_long(v);
		}
		else
		{
			AddFixup(FU_LONG, sloc, aNexpr);
			D_long(0);
		}

		break;
	case ABASE:
	case MEMPOST:
	case MEMPRE:
	case PCBASE:
	case PCMPOST:
	case PCMPRE:
		return error("unsupported 68020 addressing mode");
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

