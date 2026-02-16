#include "sprites/aliens.hpp"
#include "sprites/player.hpp"
#include "sprites/text.hpp"

namespace sprites {

static std::vector<data::SpriteFrames> animationFrames(3);

const data::Sprite ALIEN_SPRITES[6] {
    // Alien 1
    {
        {8, 8}, // width, height
        const_cast<uint8_t*>(ALIEN_SPRITE_1)
    },
    // Alien 2
    {
        {8, 8}, // width, height
        const_cast<uint8_t*>(ALIEN_SPRITE_2)
    },
    // Alien 3
    {
        {11, 8}, // width, height
        const_cast<uint8_t*>(ALIEN_SPRITE_3)
    },
    // Alien 4
    {
        {11, 8}, // width, height
        const_cast<uint8_t*>(ALIEN_SPRITE_4)
    },
    // Alien 5
    {
        {12, 8}, // width, height
        const_cast<uint8_t*>(ALIEN_SPRITE_5)
    },
    // Alien 6
    {
        {12, 8}, // width, height
        const_cast<uint8_t*>(ALIEN_SPRITE_6)
    },
};

const data::Sprite ALIEN_DEATH_SPRITE {
    {13, 7}, // width, height
    const_cast<uint8_t*>(ALIEN_DEATH)
};

data::SpriteAnimation ALIEN_ANIMATIONS[3] = {
     {true, 2, 10, 0, data::SpriteFrames()},
     {true, 2, 10, 0, data::SpriteFrames()},
     {true, 2, 10, 0, data::SpriteFrames()},
};

const data::Sprite PLAYER_SPRITE{
    {11, 7}, // width, height
    const_cast<uint8_t*>(PLAYER)
};

const data::Sprite BULLET_SPRITE{
    {1, 3}, // width, height
    const_cast<uint8_t*>(BULLET)
};

const data::Sprite TEXT_SPRITESHEET{
    {5, 7}, // width, height
    const_cast<uint8_t*>(TEXT_SP)
};

const data::Sprite NUMBER_SPRITESHEET{
    {5, 7}, // width, height
    const_cast<uint8_t*>(TEXT_SP + 16 * 35)
};

void initializeAliens()
{
    for (size_t i = 0; i < 3; ++i) {
        animationFrames[i] = data::SpriteFrames(2);
        animationFrames[i][0] = std::make_shared<data::Sprite>(ALIEN_SPRITES[2 * i]);
        animationFrames[i][1] = std::make_shared<data::Sprite>(ALIEN_SPRITES[2 * i + 1]);
        ALIEN_ANIMATIONS[i].frames = animationFrames[i];
    }
}

} // sprites
