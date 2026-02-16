#pragma once

#include <cstdint>
#include "data/data.hpp"

namespace sprites {
constexpr uint8_t PLAYER[] = {
        0,0,0,0,0,1,0,0,0,0,0, // .....@.....
        0,0,0,0,1,1,1,0,0,0,0, // ....@@@....
        0,0,0,0,1,1,1,0,0,0,0, // ....@@@....
        0,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@.
        1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
        1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
        1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
    };

constexpr uint8_t BULLET[] = {
        1, // @
        1, // @
        1  // @
    };

extern const data::Sprite PLAYER_SPRITE;
extern const data::Sprite BULLET_SPRITE;
} // sprites
