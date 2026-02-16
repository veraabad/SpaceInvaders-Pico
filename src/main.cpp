/**
 * @file  example/main.cpp
 * @brief ILI9163C demo – exercises all drawing primitives, bitmap push,
 *        text rendering, rotation, and colour utilities.
 *
 * Default wiring (SPI0):
 *   MOSI → GP19    SCLK → GP18
 *   CS   → GP17    DC   → GP16
 *   RST  → GP20    BL   → GP15
 */

#include <cstdio>
#include "pico/stdlib.h"
#include "ili9163c.hpp"
#include "font5x7.hpp"
#include "sprites/aliens.hpp"
#include "sprites/player.hpp"
#include "sprites/text.hpp"
#include "data/data.hpp"
#include "util/utility.hpp"

// ── Pin definitions ──────────────────────────────────────────────────────────
static constexpr uint8_t PIN_SCK    = 18;
static constexpr uint8_t PIN_MOSI   = 19;
static constexpr uint8_t PIN_CS     = 17;
static constexpr uint8_t PIN_DC     = 16;
static constexpr uint8_t PIN_RST    = 20;
static constexpr uint8_t PIN_BL     = 12;
static constexpr uint8_t PIN_LEFT   = 13;
static constexpr uint8_t PIN_RIGHT  = 15;
static constexpr uint8_t PIN_FIRE   = 14;

// ── Global display object ────────────────────────────────────────────────────
ILI9163C tft(spi0, PIN_SCK, PIN_MOSI, PIN_CS, PIN_DC, PIN_RST, PIN_BL);
bool gameRunning = false;
int moveDir = 0;
bool firePressed = 0;
size_t score = 0;

static void clear_screen(data::Buffer* buffer) {
    buffer->clear(Color::rgb(0, 128, 0));
    tft.drawBuffer(buffer);
    sleep_ms(2000);
}

static void init_keys() {
    gpio_init(PIN_RIGHT);
    gpio_set_dir(PIN_RIGHT, GPIO_IN);
    gpio_pull_up(PIN_RIGHT);

    gpio_init(PIN_LEFT);
    gpio_set_dir(PIN_LEFT, GPIO_IN);
    gpio_pull_up(PIN_LEFT);

    gpio_init(PIN_FIRE);
    gpio_set_dir(PIN_FIRE, GPIO_IN);
    gpio_pull_up(PIN_FIRE);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Entry point
// ─────────────────────────────────────────────────────────────────────────────
int main() {
    stdio_init_all();
    sleep_ms(200);  // settle

    printf("ILI9163C library demo starting...\n");

    tft.begin();
    tft.setBacklight(true);

    printf("Setting up key GPIOs\n");
    init_keys();

    // Game display buffer
    data::Buffer buffer(ILI9163C::DISPLAY_WIDTH, ILI9163C::DISPLAY_HEIGHT);

    // Prepare game
    sprites::initializeAliens();

    size_t credit_y = buffer.getHeight() - sprites::TEXT_SPRITESHEET.height - 7;
    size_t credit_x = buffer.getWidth() - (sprites::TEXT_SPRITESHEET.width * 9) - 10;

    data::Game game;
    game.width = buffer.getWidth();
    game.height = buffer.getHeight();
    game.numAliens = 40;
    game.rowAliens = 5;
    game.colAliens = 8;
    game.numBullets = 0;
    game.aliens = std::vector<data::Alien>(game.numAliens);

    game.player.x = (buffer.getWidth() / 2) - 5;
    game.player.y = credit_y - 20;

    game.player.life = 3;

    for (size_t yi = 0; yi < game.rowAliens; ++yi) {
        for (size_t xi = 0; xi < game.colAliens; ++xi) {
            data::Alien& alien = game.aliens[yi * game.colAliens + xi];
            alien.type = (game.rowAliens - yi) / 2 + 1;

            const data::Sprite& sprite = sprites::ALIEN_SPRITES[2 * (alien.type - 1)];

            alien.x = 14 * xi + 8 + (sprites::ALIEN_DEATH_SPRITE.width - sprite.width) / 2;
            alien.y = 16 * yi + (2 * sprites::TEXT_SPRITESHEET.height + 18);
        }
    }
    size_t score_x = 4 + 2 * sprites::NUMBER_SPRITESHEET.width;
    size_t score_y = 2 * sprites::NUMBER_SPRITESHEET.height + 6;

    std::vector<uint8_t> deathCounters(game.numAliens, 10);

    uint16_t clearColor = Color::rgb(0, 128, 0);

    gameRunning = true;

    int playerMoveDir = 0;
    // Game loop
    while (true) {
        buffer.clear(clearColor);

        buffer.drawText(
            sprites::TEXT_SPRITESHEET, "SCORE",
            4, sprites::TEXT_SPRITESHEET.height,
            Color::rgb(128, 0, 0)
        );

        buffer.drawNumber(
            sprites::NUMBER_SPRITESHEET, score,
            score_x, score_y,
            Color::rgb(128, 0, 0)
        );

        buffer.drawText(
            sprites::TEXT_SPRITESHEET, "CREDIT 00",
            credit_x, credit_y,
            Color::rgb(128, 0, 0)
        );

        // Line at bottom
        for (size_t i = 0; i < game.width; ++i) {
            buffer.getVector()[game.width * (credit_y - 5) + i] = Color::rgb(128, 0, 0);
        }

        // Draw aliens
        for (size_t ai = 0; ai < game.numAliens; ++ai) {
            if (!deathCounters[ai]) {
                // Dead alien; don't draw
                continue;
            }

            const data::Alien& alien = game.aliens[ai];
            if (alien.type == data::ALIEN_DEAD) {
                buffer.drawSprite(sprites::ALIEN_DEATH_SPRITE, alien.x, alien.y, Color::rgb(128, 0, 0));
            } else {
                const data::SpriteAnimation& animation = sprites::ALIEN_ANIMATIONS[alien.type - 1];
                size_t current_frame = animation.time / animation.frameDuration;
                const data::Sprite& sprite = *animation.frames[current_frame];
                buffer.drawSprite(sprite, alien.x, alien.y, Color::rgb(128, 0, 0));
            }
        }

        // Draw bullets
        for (size_t bi = 0; bi < game.numBullets; ++bi) {
            const data::Bullet& bullet = game.bullets[bi];
            const data::Sprite& sprite = sprites::BULLET_SPRITE;
            buffer.drawSprite(sprite, bullet.x, bullet.y, Color::rgb(128, 0, 0));
        }

        // Draw player
        buffer.drawSprite(sprites::PLAYER_SPRITE, game.player.x, game.player.y, Color::rgb(128, 0, 0));

        // Update animations
        for (size_t i = 0; i < 3; ++i) {
            ++sprites::ALIEN_ANIMATIONS[i].time;
            if (sprites::ALIEN_ANIMATIONS[i].time == sprites::ALIEN_ANIMATIONS[i].numFrames * sprites::ALIEN_ANIMATIONS[i].frameDuration) {
                sprites::ALIEN_ANIMATIONS[i].time = 0;
            }
        }

        tft.drawBuffer(&buffer);

        // Update deathCounters
        for (size_t ai = 0; ai < game.numAliens; ++ai) {
            const data::Alien& alien = game.aliens[ai];
            if (alien.type == data::ALIEN_DEAD && deathCounters[ai]) {
                --deathCounters[ai];
            }
        }

        // Set direction of bullets
        for (size_t bi = 0; bi < game.numBullets;) {
            game.bullets[bi].y += game.bullets[bi].dir;
            if (game.bullets[bi].y >= game.height || game.bullets[bi].y < sprites::BULLET_SPRITE.height) {
                game.bullets[bi] = game.bullets[game.numBullets - 1];
                --game.numBullets;
                continue;
            }
            // Check for alien collision
            for (size_t ai = 0; ai < game.numAliens; ++ai) {
                const data::Alien& alien = game.aliens[ai];
                if (alien.type == data::ALIEN_DEAD) {
                    continue;
                }
                const data::SpriteAnimation& animation = sprites::ALIEN_ANIMATIONS[alien.type - 1];
                size_t current_frame = animation.time / animation.frameDuration;
                const data::Sprite& alien_sprite = *animation.frames[current_frame];
                bool overlap = util::spriteOverlapCheck(
                    sprites::BULLET_SPRITE, game.bullets[bi].x, game.bullets[bi].y,
                    alien_sprite, alien.x, alien.y
                );
                if (overlap) {
                    score += 10 * (4 - game.aliens[ai].type);
                    game.aliens[ai].type = data::ALIEN_DEAD;
                    game.aliens[ai].x -= (sprites::ALIEN_DEATH_SPRITE.width - alien_sprite.width) / 2;
                    game.bullets[bi] = game.bullets[game.numBullets - 1];
                    --game.numBullets;
                    continue;
                }
            }

            ++bi;
        }

        playerMoveDir = 2 * moveDir;

        if (playerMoveDir != 0) {
            if (game.player.x + sprites::PLAYER_SPRITE.width + playerMoveDir >= game.width) {
                game.player.x = game.width - sprites::PLAYER_SPRITE.width;
            } else if ((int)game.player.x + playerMoveDir <= 0) {
                game.player.x = 0;
            } else {
                game.player.x += playerMoveDir;
            }
        }

        if (firePressed && game.numBullets < GAME_MAX_BULLETS) {
            game.bullets[game.numBullets].x = game.player.x + sprites::PLAYER_SPRITE.width / 2;
            game.bullets[game.numBullets].y = game.player.y + sprites::PLAYER_SPRITE.height;
            game.bullets[game.numBullets].dir = 2;
            ++game.numBullets;
        }
        firePressed = false;

        // TODO: add polling for key press
    }

    uint8_t sequence = 0;

    while (true) {
        if (!gpio_get(PIN_LEFT)) {
            if (sequence == 0 ) {
                sequence = 8;
            } else {
                sequence = (sequence - 1) % 8;
            }
        } else if (!gpio_get(PIN_RIGHT)) {
            sequence = (sequence + 1) % 8;
        } else if (!gpio_get(PIN_FIRE)) {
            sequence = 0;
        }
        clear_screen(&buffer);
    }
}
