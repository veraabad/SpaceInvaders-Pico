#pragma once

#include <cstdint>
#include "data/data.hpp"

namespace util {

/**
 * @brief Converts RGB color components to a 32-bit unsigned integer.
 *
 * @param r Red component (0-255).
 * @param g Green component (0-255).
 * @param b Blue component (0-255).
 * @return uint32_t Packed color value in RGBA format with full opacity.
 */
uint32_t rgbToUint32(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Checks if two sprites overlap based on their positions and dimensions.
 *
 * @param spA First sprite to check.
 * @param xA X-coordinate of first sprite.
 * @param yA Y-coordinate of first sprite.
 * @param spB Second sprite to check.
 * @param xB X-coordinate of second sprite.
 * @param yB Y-coordinate of second sprite.
 * @return bool True if sprites overlap, false otherwise.
 */
bool spriteOverlapCheck(
    const data::Sprite& spA, size_t xA, size_t yA,
    const data::Sprite& spB, size_t xB, size_t yB
);

/**
 * @brief GLFW error callback that prints error messages to stderr.
 *
 * @param error Error code from GLFW.
 * @param description Human-readable error description.
 */
void errorCallback(int error, const char* description);

} // util
