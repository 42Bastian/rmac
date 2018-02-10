//
// Cross-platform floating point handling
//
// by James Hammons
// (C) 2018 Underground Software
//

#include <stdint.h>

uint32_t FloatToIEEE754(float f);
uint64_t DoubleToIEEE754(double d);
void DoubleToExtended(double d, uint8_t out[]);

