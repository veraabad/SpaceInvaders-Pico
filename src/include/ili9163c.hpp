#pragma once

#include <cstdint>
#include <cstring>
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "data/data.hpp"

// ─────────────────────────────────────────────
//  ILI9163C Command Table
// ─────────────────────────────────────────────
namespace ILI9163C_CMD {
    static constexpr uint8_t NOP        = 0x00;
    static constexpr uint8_t SWRESET    = 0x01;
    static constexpr uint8_t RDDID      = 0x04;
    static constexpr uint8_t RDDST      = 0x09;
    static constexpr uint8_t SLPIN      = 0x10;
    static constexpr uint8_t SLPOUT     = 0x11;
    static constexpr uint8_t PTLON      = 0x12;
    static constexpr uint8_t NORON      = 0x13;
    static constexpr uint8_t INVOFF     = 0x20;
    static constexpr uint8_t INVON      = 0x21;
    static constexpr uint8_t GAMSET     = 0x26;
    static constexpr uint8_t DISPOFF    = 0x28;
    static constexpr uint8_t DISPON     = 0x29;
    static constexpr uint8_t CASET      = 0x2A;
    static constexpr uint8_t RASET      = 0x2B;
    static constexpr uint8_t RAMWR      = 0x2C;
    static constexpr uint8_t RAMRD      = 0x2E;
    static constexpr uint8_t PTLAR      = 0x30;
    static constexpr uint8_t MADCTL     = 0x36;
    static constexpr uint8_t COLMOD     = 0x3A;
    static constexpr uint8_t FRMCTR1    = 0xB1;
    static constexpr uint8_t FRMCTR2    = 0xB2;
    static constexpr uint8_t FRMCTR3    = 0xB3;
    static constexpr uint8_t INVCTR     = 0xB4;
    static constexpr uint8_t DISSET5    = 0xB6;
    static constexpr uint8_t PWCTR1     = 0xC0;
    static constexpr uint8_t PWCTR2     = 0xC1;
    static constexpr uint8_t PWCTR3     = 0xC2;
    static constexpr uint8_t PWCTR4     = 0xC3;
    static constexpr uint8_t PWCTR5     = 0xC4;
    static constexpr uint8_t VMCTR1     = 0xC5;
    static constexpr uint8_t VMOFCTR    = 0xC7;
    static constexpr uint8_t WRID2      = 0xD1;
    static constexpr uint8_t WRID3      = 0xD2;
    static constexpr uint8_t RDID1      = 0xDA;
    static constexpr uint8_t RDID2      = 0xDB;
    static constexpr uint8_t RDID3      = 0xDC;
    static constexpr uint8_t RDID4      = 0xDD;
    static constexpr uint8_t GMCTRP1    = 0xE0;
    static constexpr uint8_t GMCTRN1    = 0xE1;
    static constexpr uint8_t PWCTR6     = 0xFC;
}

// ─────────────────────────────────────────────
//  MADCTL bit flags (Memory Access Control)
// ─────────────────────────────────────────────
namespace MADCTL {
    static constexpr uint8_t MY  = 0x80; // Row address order
    static constexpr uint8_t MX  = 0x40; // Column address order
    static constexpr uint8_t MV  = 0x20; // Row/column exchange
    static constexpr uint8_t ML  = 0x10; // Vertical refresh order
    static constexpr uint8_t RGB = 0x00; // RGB colour filter
    static constexpr uint8_t BGR = 0x08; // BGR colour filter
    static constexpr uint8_t MH  = 0x04; // Horizontal refresh order
}

// ─────────────────────────────────────────────
//  Common 16-bit (RGB565) colour constants
// ─────────────────────────────────────────────
namespace Color {
    static constexpr uint16_t BLACK   = 0x0000;
    static constexpr uint16_t WHITE   = 0xFFFF;
    static constexpr uint16_t RED     = 0xF800;
    static constexpr uint16_t GREEN   = 0x07E0;
    static constexpr uint16_t BLUE    = 0x001F;
    static constexpr uint16_t CYAN    = 0x07FF;
    static constexpr uint16_t MAGENTA = 0xF81F;
    static constexpr uint16_t YELLOW  = 0xFFE0;
    static constexpr uint16_t ORANGE  = 0xFD20;
    static constexpr uint16_t PURPLE  = 0x8010;
    static constexpr uint16_t GRAY    = 0x8410;

    /// Build an RGB565 colour from 8-bit r/g/b components.
    static constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
        return static_cast<uint16_t>(
            ((r & 0xF8u) << 8) | ((g & 0xFCu) << 3) | (b >> 3)
        );
    }
}

// ─────────────────────────────────────────────
//  Rotation enum
// ─────────────────────────────────────────────
enum class Rotation : uint8_t {
    Deg0   = 0,
    Deg90  = 1,
    Deg180 = 2,
    Deg270 = 3
};

// ─────────────────────────────────────────────
//  Minimal font descriptor
// ─────────────────────────────────────────────
struct Font {
    const uint8_t* data;   ///< Glyph bitmap data
    uint8_t        width;  ///< Pixels per glyph (fixed-width)
    uint8_t        height; ///< Pixels per glyph
    uint8_t        first;  ///< First ASCII codepoint stored
    uint8_t        count;  ///< Number of glyphs stored
};

// ─────────────────────────────────────────────
//  ILI9163C driver
// ─────────────────────────────────────────────

/**
 * @brief Hardware-SPI driver for the ILI9163C 128×128 TFT controller.
 *
 * Typical wiring (Pico default SPI0):
 *   MOSI → GP19  (SPI0 TX)
 *   SCLK → GP18  (SPI0 SCK)
 *   CS   → GP17  (active-low chip-select)
 *   DC   → GP16  (data=HIGH / command=LOW)
 *   RST  → GP20  (active-low reset, optional – pass 0xFF to skip)
 *   BL   → GP15  (backlight, optional  – pass 0xFF to skip)
 */
class ILI9163C {
public:
    // ── physical panel size ─────────────────────────────────────────────
    static constexpr uint16_t DISPLAY_WIDTH  = 128;
    static constexpr uint16_t DISPLAY_HEIGHT = 160;

    /**
     * @brief Construct an ILI9163C driver.
     *
     * @param spi_inst   Pico SPI peripheral (spi0 or spi1).
     * @param sck_pin    SPI clock GPIO.
     * @param mosi_pin   SPI MOSI GPIO.
     * @param cs_pin     Chip-select GPIO (active-low).
     * @param dc_pin     Data/Command GPIO.
     * @param rst_pin    Reset GPIO (active-low); pass 0xFF to omit.
     * @param bl_pin     Backlight GPIO; pass 0xFF to omit.
     * @param spi_speed  Bus frequency in Hz (default 32 MHz).
     */
    ILI9163C(spi_inst_t* spi_inst,
             uint8_t     sck_pin,
             uint8_t     mosi_pin,
             uint8_t     cs_pin,
             uint8_t     dc_pin,
             uint8_t     rst_pin  = 0xFF,
             uint8_t     bl_pin   = 0xFF,
             uint32_t    spi_speed = 32'000'000u);

    // ── lifecycle ───────────────────────────────────────────────────────
    void begin();
    void reset();
    void sleep(bool enable);
    void displayOn(bool on);
    void setBacklight(bool on);

    // ── configuration ───────────────────────────────────────────────────
    void setRotation(Rotation r);
    void invertDisplay(bool invert);

    // ── drawing primitives ──────────────────────────────────────────────
    void fillScreen(uint16_t colour);
    void drawPixel(int16_t x, int16_t y, uint16_t colour);
    void drawHLine(int16_t x, int16_t y, int16_t w, uint16_t colour);
    void drawVLine(int16_t x, int16_t y, int16_t h, uint16_t colour);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t colour);
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t colour);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t colour);
    void drawCircle(int16_t cx, int16_t cy, int16_t r, uint16_t colour);
    void fillCircle(int16_t cx, int16_t cy, int16_t r, uint16_t colour);
    void drawTriangle(int16_t x0, int16_t y0,
                      int16_t x1, int16_t y1,
                      int16_t x2, int16_t y2,
                      uint16_t colour);
    void fillTriangle(int16_t x0, int16_t y0,
                      int16_t x1, int16_t y1,
                      int16_t x2, int16_t y2,
                      uint16_t colour);
    void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h,
                       int16_t r, uint16_t colour);
    void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h,
                       int16_t r, uint16_t colour);

    // ── bitmap / raw pixel push ─────────────────────────────────────────
    /**
     * @brief Push a raw RGB565 bitmap into a window.
     * @param x, y      Top-left corner.
     * @param w, h      Dimensions.
     * @param bitmap    Pointer to w*h uint16_t pixels (big-endian on wire).
     */
    void drawBitmap(int16_t x, int16_t y, int16_t w, int16_t h,
                    const uint16_t* bitmap);

    void drawBuffer(data::Buffer* buffer);

    // ── text ────────────────────────────────────────────────────────────
    void setFont(const Font* font);
    void setTextColor(uint16_t fg, uint16_t bg = Color::BLACK);
    void setCursor(int16_t x, int16_t y);
    void print(char c);
    void print(const char* str);
    void println(const char* str);

    // ── accessors ───────────────────────────────────────────────────────
    int16_t  width()  const { return _width; }
    int16_t  height() const { return _height; }
    int16_t  cursorX() const { return _cursor_x; }
    int16_t  cursorY() const { return _cursor_y; }

private:
    // SPI peripheral & pin configuration
    spi_inst_t* _spi;
    uint8_t     _sck, _mosi, _cs, _dc, _rst, _bl;
    uint32_t    _spi_speed;

    // Logical dimensions (swap with rotation)
    int16_t  _width  = DISPLAY_WIDTH;
    int16_t  _height = DISPLAY_HEIGHT;
    Rotation _rotation = Rotation::Deg0;

    // Text state
    const Font* _font      = nullptr;
    uint16_t    _text_fg   = Color::WHITE;
    uint16_t    _text_bg   = Color::BLACK;
    int16_t     _cursor_x  = 0;
    int16_t     _cursor_y  = 0;

    // ── low-level SPI helpers ────────────────────────────────────────────
    void     _cs_low()   { gpio_put(_cs, 0); }
    void     _cs_high()  { gpio_put(_cs, 1); }
    void     _dc_low()   { gpio_put(_dc, 0); }  // command
    void     _dc_high()  { gpio_put(_dc, 1); }  // data

    void     _writeByte(uint8_t b);
    void     _writeBytes(const uint8_t* buf, size_t len);
    void     _writeWord(uint16_t w);
    void     _writeWords(const uint16_t* buf, size_t count);
    void     _fillWords(uint16_t value, size_t count);

    void     _sendCmd(uint8_t cmd);
    void     _sendData8(uint8_t data);
    void     _sendData16(uint16_t data);
    void     _sendCmdData(uint8_t cmd, const uint8_t* data, size_t len);

    void     _setWindow(int16_t x0, int16_t y0, int16_t x1, int16_t y1);

    // ── init helpers ─────────────────────────────────────────────────────
    void     _initRegisters();

    // ── geometry helpers ─────────────────────────────────────────────────
    void     _drawCircleHelper(int16_t cx, int16_t cy, int16_t r,
                                uint8_t corners, uint16_t colour);
    void     _fillCircleHelper(int16_t cx, int16_t cy, int16_t r,
                                uint8_t sides, int16_t delta, uint16_t colour);

    // Swap helper
    static void _swap(int16_t& a, int16_t& b) { int16_t t = a; a = b; b = t; }
};
