//
// Cross-platform floating point handling (fixed point handling too!)
//
// by James Hammons
// (C) 2018 Underground Software
//

#ifndef __FLTPOINT_H__
#define __FLTPOINT_H__

#include <stdint.h>

uint32_t FloatToIEEE754(float f);
uint64_t DoubleToIEEE754(double d);
void DoubleToExtended(double d, uint8_t out[]);

uint64_t DoubleToFixedPoint(double d, int intBits, int fracBits);

#endif	// __FLTPOINT_H__

