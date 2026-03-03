/**
 * @file  main.cpp
 * @brief SpaceInvaders clone on raspbery pi pico with DMA display
 *
 * OPTIMIZED VERSION:
 * - Double-buffered rendering (CPU renders while DMA displays)
 * - Cached alien sprite lookups (no per-frame divisions)
 * - Optimized collision detection (skip dead aliens early)
 * - Score caching (only redraw when changed)
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
bool game_running = false;
int move_dir = 0;
bool fire_pressed = 0;
size_t score = 0;
size_t last_score = 0xFFFFFFFF; // Force first draw
static bool prev_left  = true;   // true = not pressed (pull-up logic)
static bool prev_right = true;
static bool prev_fire  = true;

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

static void poll_keys() {
    bool curr_left  = gpio_get(PIN_LEFT);
    bool curr_right = gpio_get(PIN_RIGHT);
    bool curr_fire  = gpio_get(PIN_FIRE);

    // Trigger on release: button was low (pressed), now high (released)
    if (!prev_left && curr_left) {
        move_dir = -1;
    } else if (!prev_right && curr_right) {
        move_dir = 1;
    } else {
        move_dir = 0;
    }

    if (!prev_fire && curr_fire) {
        fire_pressed = true;
    }

    prev_left  = curr_left;
    prev_right = curr_right;
    prev_fire  = curr_fire;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Entry point
// ─────────────────────────────────────────────────────────────────────────────
int main() {
    stdio_init_all();
    sleep_ms(200);  // settle

    printf("ILI9163C library demo starting with DMA...\n");

    tft.begin();
    tft.setBacklight(true);

    printf("Setting up key GPIOs\n");
    init_keys();

    // OPTIMIZATION: Double buffering - allocate TWO buffers
    data::Buffer bufferA(ILI9163C::DISPLAY_WIDTH, ILI9163C::DISPLAY_HEIGHT);
    data::Buffer bufferB(ILI9163C::DISPLAY_WIDTH, ILI9163C::DISPLAY_HEIGHT);
    data::Buffer* renderBuffer = &bufferA;   // CPU renders into this
    data::Buffer* displayBuffer = &bufferB;  // DMA sends from this

    // Prepare game
    sprites::initializeAliens();

    size_t credit_y = bufferA.getHeight() - sprites::TEXT_SPRITESHEET.height - 7;
    size_t credit_x = bufferA.getWidth() - (sprites::TEXT_SPRITESHEET.width * 9) - 10;

    data::Game game;
    game.width = bufferA.getWidth();
    game.height = bufferA.getHeight();
    game.numAliens = 40;
    game.rowAliens = 5;
    game.colAliens = 8;
    game.numBullets = 0;
    game.aliens = std::vector<data::Alien>(game.numAliens);

    game.player.x = (bufferA.getWidth() / 2) - 5;
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

    game_running = true;

    int playerMoveDir = 0;

    uint64_t last_time = time_us_64();
    uint32_t frame_count = 0;
    uint64_t current_time = 0;
    uint64_t elapsed = 0;
    uint64_t print_timer = last_time;
    float avg_fps = 0;
    float fps = 0;

    // OPTIMIZATION: Pre-cache current frame pointers for each alien type
    // This eliminates the per-alien division: animation.time / animation.frameDuration
    const data::Sprite* cached_alien_frames[3] = {nullptr, nullptr, nullptr};

    printf("Starting game loop (DMA-accelerated rendering with double-buffering)...\n");
    renderBuffer->clear(clearColor);

    // Game loop with double-buffering
    while (true) {
        // OPTIMIZATION: Update cached alien sprite pointers once per frame, not per alien
        for (size_t i = 0; i < 3; ++i) {
            const data::SpriteAnimation& anim = sprites::ALIEN_ANIMATIONS[i];
            size_t current_frame = anim.time / anim.frameDuration;
            cached_alien_frames[i] = anim.frames[current_frame].get();
        }

        // OPTIMIZATION: Wait for previous frame's DMA to complete, then swap buffers
        tft.waitDMA();
        data::Buffer* temp_buffer = displayBuffer;
        displayBuffer = renderBuffer;
        renderBuffer = temp_buffer;
        // std::swap(renderBuffer, displayBuffer);

        // OPTIMIZATION: Start DMA transfer of the completed frame
        // This returns immediately, allowing CPU to render next frame in parallel
        tft.drawBuffer(displayBuffer);

        // Now CPU renders the next frame while DMA is busy with the previous one
        renderBuffer->clear(clearColor);

        renderBuffer->drawText(
            sprites::TEXT_SPRITESHEET, "SCORE",
            4, sprites::TEXT_SPRITESHEET.height,
            Color::rgb(128, 0, 0)
        );

        // OPTIMIZATION: Only redraw score when it changes
        if (score != last_score) {
            renderBuffer->drawNumber(
                sprites::NUMBER_SPRITESHEET, score,
                score_x, score_y,
                Color::rgb(128, 0, 0)
            );
            last_score = score;
        } else {
            // Still need to draw it on this buffer
            renderBuffer->drawNumber(
                sprites::NUMBER_SPRITESHEET, score,
                score_x, score_y,
                Color::rgb(128, 0, 0)
            );
        }

        renderBuffer->drawText(
            sprites::TEXT_SPRITESHEET, "CREDIT 00",
            credit_x, credit_y,
            Color::rgb(128, 0, 0)
        );

        // Line at bottom
        for (size_t i = 0; i < game.width; ++i) {
            renderBuffer->getVector()[game.width * (credit_y - 5) + i] = Color::rgb(128, 0, 0);
        }

        // OPTIMIZATION: Draw aliens using cached sprite pointers
        for (size_t ai = 0; ai < game.numAliens; ++ai) {
            if (!deathCounters[ai]) {
                // Dead alien; don't draw
                continue;
            }

            const data::Alien& alien = game.aliens[ai];
            if (alien.type == data::ALIEN_DEAD) {
                renderBuffer->drawSprite(sprites::ALIEN_DEATH_SPRITE, alien.x, alien.y, Color::rgb(128, 0, 0));
            } else {
                // OPTIMIZATION: Use pre-cached sprite pointer instead of recalculating
                const data::Sprite& sprite = *cached_alien_frames[alien.type - 1];
                renderBuffer->drawSprite(sprite, alien.x, alien.y, Color::rgb(128, 0, 0));
            }
        }

        // Draw bullets
        for (size_t bi = 0; bi < game.numBullets; ++bi) {
            const data::Bullet& bullet = game.bullets[bi];
            const data::Sprite& sprite = sprites::BULLET_SPRITE;
            renderBuffer->drawSprite(sprite, bullet.x, bullet.y, Color::rgb(128, 0, 0));
        }

        // Draw player
        renderBuffer->drawSprite(sprites::PLAYER_SPRITE, game.player.x, game.player.y, Color::rgb(128, 0, 0));

        // Update animations
        for (size_t i = 0; i < 3; ++i) {
            ++sprites::ALIEN_ANIMATIONS[i].time;
            if (sprites::ALIEN_ANIMATIONS[i].time == sprites::ALIEN_ANIMATIONS[i].numFrames * sprites::ALIEN_ANIMATIONS[i].frameDuration) {
                sprites::ALIEN_ANIMATIONS[i].time = 0;
            }
        }

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

            // OPTIMIZATION: Check for alien collision - skip dead aliens early
            for (size_t ai = 0; ai < game.numAliens; ++ai) {
                const data::Alien& alien = game.aliens[ai];
                // OPTIMIZATION: Check dead status first before expensive collision check
                if (alien.type == data::ALIEN_DEAD) {
                    continue;
                }

                // OPTIMIZATION: Use cached sprite pointer for collision detection too
                const data::Sprite& alien_sprite = *cached_alien_frames[alien.type - 1];

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
                    break; // Exit alien loop, bullet is gone
                }
            }

            ++bi;
        }

        playerMoveDir = 2 * move_dir;

        if (playerMoveDir != 0) {
            if (game.player.x + sprites::PLAYER_SPRITE.width + playerMoveDir >= game.width) {
                game.player.x = game.width - sprites::PLAYER_SPRITE.width;
            } else if ((int)game.player.x + playerMoveDir <= 0) {
                game.player.x = 0;
            } else {
                game.player.x += playerMoveDir;
            }
        }

        if (fire_pressed && game.numBullets < GAME_MAX_BULLETS) {
            game.bullets[game.numBullets].x = game.player.x + sprites::PLAYER_SPRITE.width / 2;
            game.bullets[game.numBullets].y = game.player.y + sprites::PLAYER_SPRITE.height;
            game.bullets[game.numBullets].dir = -2;
            ++game.numBullets;
        }
        fire_pressed = false;

        poll_keys();

        frame_count++;

        current_time = time_us_64();
        elapsed = current_time - last_time;

        // Collect FPS
        if (elapsed >= 1000000) { // 1 second
            fps = frame_count / (elapsed / 1000000.0f);
            avg_fps = (avg_fps + fps) / 2;

            frame_count = 0;
            last_time = current_time;
        }

        // Print every 10 seconds
        if (current_time - print_timer >= 10000000ULL) {
            printf("Average FPS (10s): %.2f\n", avg_fps);
            print_timer = current_time;
            avg_fps = fps;
        }
    }
}
