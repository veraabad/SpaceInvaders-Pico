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
