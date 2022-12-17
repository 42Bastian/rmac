//
// Floating point to IEEE-754 conversion routines
//
// by James Hammons
// (C) 2019 Underground Software
//
// Since there are no guarantees vis-a-vis floating point numbers in C, we have
// to utilize routines like the following in order to guarantee that the thing
// we get out of the C compiler is an honest-to-God IEEE-754 style floating
// point number (since that's what the Motorola processors that we target
// expect).
//

#include "fltpoint.h"
#include <float.h>
#include <math.h>
#include <stdio.h>
#include "error.h"

//
// Check for IEEE-754 conformance (C99 compilers should be OK here)
//
// The reason we do this is mainly to ensure consistency across all platforms,
// even those that still haven't implemented C99 compliance after other
// compilers have had them for decades. The long and the short of it is, there
// are no guarantees for floating point implementations across platforms the
// way there is for ints (in <stdint.h>, for example) and so we have to be
// careful that bad assumptions vis-a-vis floating point numbers don't creep
// into the codebase and cause problems similar to the ones we had when adding
// proper 64-bit support. Hence, the following ugliness...
//
// IEEE-745 expects the following for floats and doubles:
//  float: exponent is 8 bits, mantissa is 24 bits
// double: exponent is 11 bits, mantissa is 53 bits
// FLT_RADIX should be 2
#ifdef FLT_RADIX
	#if FLT_RADIX != 2
	#error "FLT_RADIX: Your compiler sucks. Get a real one."
	#endif
#endif
#ifdef FLT_MANT_DIG
	#if FLT_MANT_DIG != 24
	#error "FLT_MANT_DIG: Your compiler sucks. Get a real one."
	#endif
#endif
#ifdef DBL_MANT_DIG
	#if DBL_MANT_DIG != 53
	#error "DBL_MANT_DIG: Your compiler sucks. Get a real one."
	#endif
#endif
#ifdef FLT_MAX_EXP
	#if FLT_MAX_EXP != 128
	#error "FLT_MAX_EXP: Your compiler sucks. Get a real one."
	#endif
#endif
#ifdef DBL_MAX_EXP
	#if DBL_MAX_EXP != 1024
	#error "DBL_MAX_EXP: Your compiler sucks. Get a real one."
	#endif
#endif
//
// So if we get here, we can be pretty sure that a float is 4 bytes and a
// double is 8. IEEE-754? Maaaaaaaaybe. But we don't have to worry about that
// so much, as long as the token stream is OK (floats are 4 bytes, doubles are
// 8).
//


uint32_t FloatToIEEE754(float f)
{
	uint32_t sign = (signbit(f) ? 0x80000000 : 0);

	// Split the float into normalized mantissa (range: (-1, -0.5], 0,
	// [+0.5, +1)) and base-2 exponent
	// d = mantissa * (2 ^ exponent) *exactly* for FLT_RADIX=2
	// Also, since we want the mantissa to be non-inverted (2's complemented),
	// we make sure to pass in a positive number (floats/doubles are *not* 2's
	// complemented) as we already captured the sign bit above.
	int32_t exponent;
	float mantissa = frexpf((f < 0 ? -f : f), &exponent);

	// Set the exponent bias for IEEE-754 floats
	exponent += 0x7E;

	// Check for zero, set the proper exponent if so (zero exponent means no
	// implied leading one)
	if (f == 0)
		exponent = 0;

	// Extract most significant 24 bits of mantissa
	mantissa = ldexpf(mantissa, 24);

	// Convert to an unsigned int
	uint32_t ieeeVal = truncf(mantissa);

	// ieeeVal now has the mantissa in binary format, *including* the leading 1
	// bit; so we have to strip that bit out, since in IEEE-754, it's implied.
	ieeeVal &= 0x007FFFFF;

	// Finally, add in the other parts to make a proper IEEE-754 float
	ieeeVal |= sign | ((exponent & 0xFF) << 23);

	return ieeeVal;
}


uint64_t DoubleToIEEE754(double d)
{
	uint64_t sign = (signbit(d) ? 0x8000000000000000LL : 0);
	int32_t exponent;

	// Split double into normalized mantissa (range: (-1, -0.5], 0, [+0.5, +1))
	// and base-2 exponent
	// d = mantissa * (2 ^ exponent) *exactly* for FLT_RADIX=2
	// Also, since we want the mantissa to be non-inverted (2's complemented),
	// we make sure to pass in a positive number (floats/doubles are *not* 2's
	// complemented) as we already captured the sign bit above.
	double mantissa = frexp((d < 0 ? -d : d), &exponent);

	// Set the exponent bias for IEEE-754 doubles
	exponent += 0x3FE;

	// Check for zero, set the proper exponent if so
	if (d == 0)
		exponent = 0;

	// Extract most significant 53 bits of mantissa
	mantissa = ldexp(mantissa, 53);

	// Convert to an unsigned int
	uint64_t ieeeVal = trunc(mantissa);

	// ieeeVal now has the mantissa in binary format, *including* the leading 1
	// bit; so we have to strip that bit out, since in IEEE-754, it's implied.
	ieeeVal &= 0x000FFFFFFFFFFFFF;

	// Finally, add in the other parts to make a proper IEEE-754 double
	ieeeVal |= sign | ((uint64_t)(exponent & 0x7FF) << 52);

	return ieeeVal;
}


void DoubleToExtended(double d, uint8_t out[])
{
	int8_t sign = (signbit(d) ? 0x80 : 0);
	int32_t exponent;
	double mantissa = frexp((d < 0 ? -d : d), &exponent);
	exponent += 0x3FFE;

	if (d == 0)
		exponent = 0;

	mantissa = ldexp(mantissa, 64);
	uint64_t intMant = trunc(mantissa);

	// Motorola extended floating point is 96 bits, so we pack it into the
	// 12-byte array that's passed in. The format is as follows: 1 bit (sign),
	// 15 bits (exponent w/$3FFF bias), 16 bits of zero, 64 bits of mantissa.
	out[0] = sign | ((exponent >> 8) & 0x7F);
	out[1] = exponent & 0xFF;
	out[2] = 0;
	out[3] = 0;
	out[4] = (intMant >> 56) & 0xFF;
	out[5] = (intMant >> 48) & 0xFF;
	out[6] = (intMant >> 40) & 0xFF;
	out[7] = (intMant >> 32) & 0xFF;
	out[8] = (intMant >> 24) & 0xFF;
	out[9] = (intMant >> 16) & 0xFF;
	out[10] = (intMant >> 8) & 0xFF;
	out[11] = intMant & 0xFF;
}


//
// Convert a double to a DSP56001 style fixed point float.
// Seems to be 23 bits of float value with 1 bit (MSB) for the sign.
//
uint32_t DoubleToDSPFloat(double d)
{
	if (d >= 1)
	{
		warn("DSP value clamped to +1.");
		return 0x7FFFFF;
	}
	else if (d <= -1)
	{
		warn("DSP value clamped to -1.");
		return 0x800000;
	}

	// The casts are here because some compilers do weird shit.  See bug #149.
	return (uint32_t)((int32_t)trunc(round(ldexp(d, 23))));
}


//
// Convert a host native floating point number to a fixed point number.
//
uint64_t DoubleToFixedPoint(double d, int intBits, int fracBits)
{
	uint8_t signBit = (signbit(d) ? 1 : 0);

	// Ensure what we're working on is positive...
	if (d < 0)
		d *= -1;

	double scaleFactor = (double)(1 << fracBits);
	uint64_t result = (uint64_t)(d * scaleFactor);

	// Invert the result, if necessary
	if (signBit == 1)
		result = (result ^ 0xFFFFFFFFFFFFFFFFLL) + 1;

	return result;
}

