//
// Cross-platform floating point handling
//

#include <stdint.h>

uint32_t FloatToIEEE754(float f);
uint64_t DoubleToIEEE754(double d);
void DoubleToExtended(double d, uint8_t out[]);

