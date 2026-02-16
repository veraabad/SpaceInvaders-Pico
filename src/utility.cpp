#include "util/utility.hpp"
#include <cstdio>

namespace util {

uint32_t rgbToUint32(uint8_t r, uint8_t g, uint8_t b)
{
    // Setting alpha to full opacity ----------|
    //                                         v
    return (r << 24) | (g << 16) | (b << 8) | 0xFF;
}

bool spriteOverlapCheck(
    const data::Sprite& spA, size_t xA, size_t yA,
    const data::Sprite& spB, size_t xB, size_t yB
){
    if (xA < xB + spB.width && xA + spA.width > xB
        && yA < yB + spB.height && yA + spB.height > yB) {
        return true;
    }
    return false;
}

void errorCallback(int error, const char* description)
{
    fprintf(stderr, "Error %d: %s\n", error, description);
}

} // util
