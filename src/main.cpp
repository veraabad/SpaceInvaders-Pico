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

// ─────────────────────────────────────────────────────────────────────────────
//  Demo helpers
// ─────────────────────────────────────────────────────────────────────────────

static void demo_primitives() {
    tft.fillScreen(Color::BLACK);

    // Coloured rectangles
    tft.fillRect(0,  0,  63, 63, Color::RED);
    tft.fillRect(65, 0,  63, 63, Color::GREEN);
    tft.fillRect(0,  65, 63, 63, Color::BLUE);
    tft.fillRect(65, 65, 63, 63, Color::YELLOW);

    // Overlaid outline rects
    tft.drawRect(10, 10, 44, 44, Color::WHITE);
    tft.drawRect(74, 10, 44, 44, Color::WHITE);
    tft.drawRect(10, 74, 44, 44, Color::WHITE);
    tft.drawRect(74, 74, 44, 44, Color::WHITE);

    sleep_ms(2000);
}

static void demo_circles() {
    tft.fillScreen(Color::BLACK);

    tft.fillCircle(32,  32,  30, Color::CYAN);
    tft.fillCircle(96,  32,  30, Color::MAGENTA);
    tft.fillCircle(32,  96,  30, Color::ORANGE);
    tft.fillCircle(96,  96,  30, Color::PURPLE);

    tft.drawCircle(64, 64, 28, Color::WHITE);
    tft.drawCircle(64, 64, 20, Color::YELLOW);
    tft.drawCircle(64, 64, 12, Color::RED);

    sleep_ms(2000);
}

static void demo_lines() {
    tft.fillScreen(Color::BLACK);

    const int16_t cx = 64, cy = 64;
    for (int i = 0; i < 360; i += 20) {
        float rad = i * 3.14159f / 180.0f;
        int16_t x = static_cast<int16_t>(cx + 60 * __builtin_cosf(rad));
        int16_t y = static_cast<int16_t>(cy + 60 * __builtin_sinf(rad));
        uint16_t col = Color::rgb(
            static_cast<uint8_t>(128 + 127 * __builtin_sinf(rad)),
            static_cast<uint8_t>(128 + 127 * __builtin_cosf(rad)),
            200
        );
        tft.drawLine(cx, cy, x, y, col);
    }

    sleep_ms(2000);
}

static void demo_triangles() {
    tft.fillScreen(Color::BLACK);

    // Filled triangles
    tft.fillTriangle(64,  5,  5,  123, 123, 123, Color::CYAN);
    tft.fillTriangle(64,  20, 20, 108, 108, 108, Color::BLUE);
    tft.fillTriangle(64,  35, 35,  93,  93,  93, Color::GREEN);

    // Outline
    tft.drawTriangle(64, 5, 5, 123, 123, 123, Color::WHITE);

    sleep_ms(2000);
}

static void demo_roundrects() {
    tft.fillScreen(Color::BLACK);

    for (int r = 2; r <= 20; r += 3) {
        uint16_t col = Color::rgb(
            static_cast<uint8_t>(r * 10),
            static_cast<uint8_t>(200 - r * 5),
            180
        );
        tft.drawRoundRect(64 - r * 3, 64 - r * 3,
                          r * 6, r * 6, r, col);
    }

    tft.fillRoundRect(44, 44, 40, 40, 8, Color::ORANGE);
    tft.drawRoundRect(44, 44, 40, 40, 8, Color::WHITE);

    sleep_ms(2000);
}

static void demo_text() {
    tft.fillScreen(Color::BLACK);
    tft.setFont(&Font5x7);

    // Title
    tft.setTextColor(Color::YELLOW, Color::BLACK);
    tft.setCursor(2, 2);
    tft.println("ILI9163C Demo");

    // Coloured rows
    const uint16_t colours[] = {
        Color::RED, Color::GREEN, Color::BLUE,
        Color::CYAN, Color::MAGENTA, Color::ORANGE
    };
    const char* lines[] = {
        "Hello, Pico!",
        "RGB565 colour",
        "SPI @ 32 MHz",
        "128x128 TFT",
        "5x7 Font",
        "by ILI9163C lib"
    };

    for (int i = 0; i < 6; ++i) {
        tft.setTextColor(colours[i], Color::BLACK);
        tft.setCursor(2, 18 + i * 10);
        tft.println(lines[i]);
    }

    // Bottom bar
    tft.fillRect(0, 114, 128, 14, Color::GRAY);
    tft.setTextColor(Color::WHITE, Color::GRAY);
    tft.setCursor(4, 117);
    tft.print("Raspberry Pi Pico");

    sleep_ms(3000);
}

static void demo_bitmap() {
    tft.fillScreen(Color::BLACK);

    // Generate a small gradient bitmap on the stack (32×32)
    static uint16_t bmp[32 * 32];
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 32; ++x) {
            bmp[y * 32 + x] = Color::rgb(
                static_cast<uint8_t>(x * 8),
                static_cast<uint8_t>(y * 8),
                128
            );
        }
    }

    // Tile the bitmap across the display
    for (int ty = 0; ty < 4; ++ty) {
        for (int tx = 0; tx < 4; ++tx) {
            tft.drawBitmap(tx * 32, ty * 32, 32, 32, bmp);
        }
    }

    sleep_ms(2000);
}

static void demo_rotation() {
    const Rotation rots[] = {
        Rotation::Deg0, Rotation::Deg90,
        Rotation::Deg180, Rotation::Deg270
    };
    const char* labels[] = { "0 deg", "90 deg", "180 deg", "270 deg" };

    for (int i = 0; i < 4; ++i) {
        tft.setRotation(rots[i]);
        tft.fillScreen(Color::BLACK);
        tft.setFont(&Font5x7);
        tft.setTextColor(Color::WHITE, Color::BLACK);
        tft.setCursor(4, 4);
        tft.print("Rotation: ");
        tft.println(labels[i]);

        // Arrow pointing "up"
        tft.fillTriangle(
            tft.width() / 2, 20,
            tft.width() / 2 - 12, 40,
            tft.width() / 2 + 12, 40,
            Color::GREEN
        );

        sleep_ms(1500);
    }

    // Reset to portrait
    tft.setRotation(Rotation::Deg0);
}

static void clear_screen() {
    tft.fillScreen(Color::rgb(0, 128, 0));
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
        switch (sequence) {
            case 0:
                printf("Game Screen\n");  clear_screen();
                break;
            case 1:
                printf("Primitives demo\n");  demo_primitives();
                break;
            case 2:
                printf("Circles demo\n");     demo_circles();
                break;
            case 3:
                printf("Lines demo\n");       demo_lines();
                break;
            case 4:
                printf("Triangles demo\n");   demo_triangles();
                break;
            case 5:
                printf("Round rects demo\n"); demo_roundrects();
                break;
            case 6:
                printf("Text demo\n");        demo_text();
                break;
            case 7:
                printf("Bitmap demo\n");      demo_bitmap();
                break;
            case 8:
                printf("Rotation demo\n");    demo_rotation();
                break;
            default:
                printf("Unknown section");
                break;
        }
    }
}
