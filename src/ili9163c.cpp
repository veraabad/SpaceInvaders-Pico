/**
 * @file  ili9163c.cpp
 * @brief ILI9163C 128×128 TFT driver for Raspberry Pi Pico (hardware SPI).
 */

#include "ili9163c.hpp"
#include <algorithm>   // std::swap (stdlib already pulled in by header)
#include <cstdlib>     // abs

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────────────────────
ILI9163C::ILI9163C(spi_inst_t* spi_inst,
                   uint8_t     sck_pin,
                   uint8_t     mosi_pin,
                   uint8_t     cs_pin,
                   uint8_t     dc_pin,
                   uint8_t     rst_pin,
                   uint8_t     bl_pin,
                   uint32_t    spi_speed)
    : _spi(spi_inst),
      _sck(sck_pin), _mosi(mosi_pin),
      _cs(cs_pin),   _dc(dc_pin),
      _rst(rst_pin), _bl(bl_pin),
      _spi_speed(spi_speed)
{}

// ─────────────────────────────────────────────────────────────────────────────
//  Lifecycle
// ─────────────────────────────────────────────────────────────────────────────

void ILI9163C::begin() {
    // Initialise SPI peripheral
    spi_init(_spi, _spi_speed);
    spi_set_format(_spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    // Configure GPIO
    gpio_set_function(_sck,  GPIO_FUNC_SPI);
    gpio_set_function(_mosi, GPIO_FUNC_SPI);

    gpio_init(_cs);  gpio_set_dir(_cs, GPIO_OUT);  gpio_put(_cs, 1);
    gpio_init(_dc);  gpio_set_dir(_dc, GPIO_OUT);  gpio_put(_dc, 1);

    if (_rst != 0xFF) {
        gpio_init(_rst); gpio_set_dir(_rst, GPIO_OUT); gpio_put(_rst, 1);
    }
    if (_bl != 0xFF) {
        gpio_init(_bl);  gpio_set_dir(_bl,  GPIO_OUT); gpio_put(_bl,  1);
    }

    reset();
    _initRegisters();
    displayOn(true);
    fillScreen(Color::BLACK);
}

void ILI9163C::reset() {
    if (_rst != 0xFF) {
        gpio_put(_rst, 1); sleep_ms(10);
        gpio_put(_rst, 0); sleep_ms(50);
        gpio_put(_rst, 1); sleep_ms(150);
    } else {
        _sendCmd(ILI9163C_CMD::SWRESET);
        sleep_ms(150);
    }
}

void ILI9163C::sleep(bool enable) {
    _sendCmd(enable ? ILI9163C_CMD::SLPIN : ILI9163C_CMD::SLPOUT);
    sleep_ms(enable ? 5 : 120);
}

void ILI9163C::displayOn(bool on) {
    _sendCmd(on ? ILI9163C_CMD::DISPON : ILI9163C_CMD::DISPOFF);
}

void ILI9163C::setBacklight(bool on) {
    if (_bl != 0xFF) gpio_put(_bl, on ? 1 : 0);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Register initialisation sequence
// ─────────────────────────────────────────────────────────────────────────────
void ILI9163C::_initRegisters() {
    // Exit sleep
    _sendCmd(ILI9163C_CMD::SLPOUT);
    sleep_ms(120);

    // Frame-rate control – Normal mode
    {
        const uint8_t d[] = {0x08, 0x02};
        _sendCmdData(ILI9163C_CMD::FRMCTR1, d, 2);
    }
    // Frame-rate control – Idle mode
    {
        const uint8_t d[] = {0x08, 0x02};
        _sendCmdData(ILI9163C_CMD::FRMCTR2, d, 2);
    }
    // Frame-rate control – Partial mode
    {
        const uint8_t d[] = {0x08, 0x02, 0x08, 0x02};
        _sendCmdData(ILI9163C_CMD::FRMCTR3, d, 4);
    }

    // Display inversion control – no inversion
    _sendCmdData(ILI9163C_CMD::INVCTR, (const uint8_t[]){0x07}, 1);

    // Power control 1
    _sendCmdData(ILI9163C_CMD::PWCTR1, (const uint8_t[]){0x0A, 0x02}, 2);
    // Power control 2
    _sendCmdData(ILI9163C_CMD::PWCTR2, (const uint8_t[]){0x02}, 1);
    // Power control 3 (normal mode)
    _sendCmdData(ILI9163C_CMD::PWCTR3, (const uint8_t[]){0x8A, 0x2A}, 2);
    // Power control 4 (idle mode)
    _sendCmdData(ILI9163C_CMD::PWCTR4, (const uint8_t[]){0x8A, 0xEE}, 2);
    // Power control 5 (partial mode / full colours)
    _sendCmdData(ILI9163C_CMD::PWCTR5, (const uint8_t[]){0x8A, 0xAA}, 2);

    // VCOM control
    _sendCmdData(ILI9163C_CMD::VMCTR1, (const uint8_t[]){0x0E}, 1);

    // Memory access control – portrait, RGB order
    _sendCmdData(ILI9163C_CMD::MADCTL,
                 (const uint8_t[]){MADCTL::MX | MADCTL::MY | MADCTL::BGR}, 1);

    // Interface pixel format – 16-bit (RGB565)
    _sendCmdData(ILI9163C_CMD::COLMOD, (const uint8_t[]){0x05}, 1);

    // Gamma curve select
    _sendCmdData(ILI9163C_CMD::GAMSET, (const uint8_t[]){0x01}, 1);

    // Positive gamma correction
    {
        const uint8_t d[] = {
            0x0F, 0x1A, 0x0F, 0x18, 0x2F, 0x28, 0x20,
            0x22, 0x1F, 0x1B, 0x23, 0x37, 0x00, 0x07, 0x02, 0x10
        };
        _sendCmdData(ILI9163C_CMD::GMCTRP1, d, 16);
    }
    // Negative gamma correction
    {
        const uint8_t d[] = {
            0x0F, 0x1B, 0x0F, 0x17, 0x33, 0x2C, 0x29,
            0x2E, 0x30, 0x30, 0x39, 0x3F, 0x00, 0x07, 0x03, 0x10
        };
        _sendCmdData(ILI9163C_CMD::GMCTRN1, d, 16);
    }

    // Column and row address set (full 128×128)
    _setWindow(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);

    // Normal display mode on
    _sendCmd(ILI9163C_CMD::NORON);
    sleep_ms(10);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Configuration
// ─────────────────────────────────────────────────────────────────────────────

void ILI9163C::setRotation(Rotation r) {
    _rotation = r;
    uint8_t madctl = MADCTL::BGR;

    switch (r) {
        case Rotation::Deg0:
            madctl |= MADCTL::MX | MADCTL::MY;
            _width  = DISPLAY_WIDTH;
            _height = DISPLAY_HEIGHT;
            break;
        case Rotation::Deg90:
            madctl |= MADCTL::MY | MADCTL::MV;
            _width  = DISPLAY_HEIGHT;
            _height = DISPLAY_WIDTH;
            break;
        case Rotation::Deg180:
            // no extra flags
            _width  = DISPLAY_WIDTH;
            _height = DISPLAY_HEIGHT;
            break;
        case Rotation::Deg270:
            madctl |= MADCTL::MX | MADCTL::MV;
            _width  = DISPLAY_HEIGHT;
            _height = DISPLAY_WIDTH;
            break;
    }
    _sendCmdData(ILI9163C_CMD::MADCTL, &madctl, 1);
}

void ILI9163C::invertDisplay(bool invert) {
    _sendCmd(invert ? ILI9163C_CMD::INVON : ILI9163C_CMD::INVOFF);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Low-level SPI helpers
// ─────────────────────────────────────────────────────────────────────────────

void ILI9163C::_writeByte(uint8_t b) {
    spi_write_blocking(_spi, &b, 1);
}

void ILI9163C::_writeBytes(const uint8_t* buf, size_t len) {
    spi_write_blocking(_spi, buf, len);
}

void ILI9163C::_writeWord(uint16_t w) {
    uint8_t buf[2] = { static_cast<uint8_t>(w >> 8),
                       static_cast<uint8_t>(w & 0xFF) };
    spi_write_blocking(_spi, buf, 2);
}

void ILI9163C::_writeWords(const uint16_t* buf, size_t count) {
    // Byte-swap inline; use a small staging buffer for efficiency
    static constexpr size_t CHUNK = 64;
    uint8_t stage[CHUNK * 2];

    while (count > 0) {
        size_t n = (count < CHUNK) ? count : CHUNK;
        for (size_t i = 0; i < n; ++i) {
            stage[i * 2]     = static_cast<uint8_t>(buf[i] >> 8);
            stage[i * 2 + 1] = static_cast<uint8_t>(buf[i] & 0xFF);
        }
        spi_write_blocking(_spi, stage, n * 2);
        buf   += n;
        count -= n;
    }
}

void ILI9163C::_fillWords(uint16_t value, size_t count) {
    // Build a two-byte pattern and blast it out
    uint8_t hi = static_cast<uint8_t>(value >> 8);
    uint8_t lo = static_cast<uint8_t>(value & 0xFF);

    static constexpr size_t CHUNK = 64;
    uint8_t stage[CHUNK * 2];
    for (size_t i = 0; i < CHUNK; ++i) { stage[i*2] = hi; stage[i*2+1] = lo; }

    while (count >= CHUNK) {
        spi_write_blocking(_spi, stage, CHUNK * 2);
        count -= CHUNK;
    }
    if (count > 0) spi_write_blocking(_spi, stage, count * 2);
}

void ILI9163C::_sendCmd(uint8_t cmd) {
    _cs_low();
    _dc_low();
    _writeByte(cmd);
    _cs_high();
}

void ILI9163C::_sendData8(uint8_t data) {
    _cs_low();
    _dc_high();
    _writeByte(data);
    _cs_high();
}

void ILI9163C::_sendData16(uint16_t data) {
    _cs_low();
    _dc_high();
    _writeWord(data);
    _cs_high();
}

void ILI9163C::_sendCmdData(uint8_t cmd, const uint8_t* data, size_t len) {
    _cs_low();
    _dc_low();
    _writeByte(cmd);
    if (len > 0) {
        _dc_high();
        _writeBytes(data, len);
    }
    _cs_high();
}

void ILI9163C::_setWindow(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
    // Column address set
    {
        const uint8_t d[] = {
            static_cast<uint8_t>(x0 >> 8), static_cast<uint8_t>(x0 & 0xFF),
            static_cast<uint8_t>(x1 >> 8), static_cast<uint8_t>(x1 & 0xFF)
        };
        _sendCmdData(ILI9163C_CMD::CASET, d, 4);
    }
    // Row address set
    {
        const uint8_t d[] = {
            static_cast<uint8_t>(y0 >> 8), static_cast<uint8_t>(y0 & 0xFF),
            static_cast<uint8_t>(y1 >> 8), static_cast<uint8_t>(y1 & 0xFF)
        };
        _sendCmdData(ILI9163C_CMD::RASET, d, 4);
    }
    // RAM write
    _sendCmd(ILI9163C_CMD::RAMWR);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Drawing Primitives
// ─────────────────────────────────────────────────────────────────────────────

void ILI9163C::fillScreen(uint16_t colour) {
    fillRect(0, 0, _width, _height, colour);
}

void ILI9163C::drawPixel(int16_t x, int16_t y, uint16_t colour) {
    if (x < 0 || x >= _width || y < 0 || y >= _height) return;
    _setWindow(x, y, x, y);
    _cs_low();
    _dc_high();
    _writeWord(colour);
    _cs_high();
}

void ILI9163C::drawHLine(int16_t x, int16_t y, int16_t w, uint16_t colour) {
    if (y < 0 || y >= _height || x >= _width || w <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (x + w > _width)  w = _width - x;
    if (w <= 0) return;

    _setWindow(x, y, x + w - 1, y);
    _cs_low();
    _dc_high();
    _fillWords(colour, static_cast<size_t>(w));
    _cs_high();
}

void ILI9163C::drawVLine(int16_t x, int16_t y, int16_t h, uint16_t colour) {
    if (x < 0 || x >= _width || y >= _height || h <= 0) return;
    if (y < 0) { h += y; y = 0; }
    if (y + h > _height) h = _height - y;
    if (h <= 0) return;

    _setWindow(x, y, x, y + h - 1);
    _cs_low();
    _dc_high();
    _fillWords(colour, static_cast<size_t>(h));
    _cs_high();
}

// Bresenham line
void ILI9163C::drawLine(int16_t x0, int16_t y0,
                         int16_t x1, int16_t y1,
                         uint16_t colour) {
    // Use fast H/V paths where possible
    if (y0 == y1) { drawHLine(x0 < x1 ? x0 : x1, y0, abs(x1 - x0) + 1, colour); return; }
    if (x0 == x1) { drawVLine(x0, y0 < y1 ? y0 : y1, abs(y1 - y0) + 1, colour); return; }

    bool steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep)   { _swap(x0, y0); _swap(x1, y1); }
    if (x0 > x1) { _swap(x0, x1); _swap(y0, y1); }

    int16_t dx = x1 - x0;
    int16_t dy = abs(y1 - y0);
    int16_t err = dx / 2;
    int16_t ystep = (y0 < y1) ? 1 : -1;

    for (; x0 <= x1; ++x0) {
        if (steep) drawPixel(y0, x0, colour);
        else        drawPixel(x0, y0, colour);
        err -= dy;
        if (err < 0) { y0 += ystep; err += dx; }
    }
}

void ILI9163C::drawRect(int16_t x, int16_t y, int16_t w, int16_t h,
                         uint16_t colour) {
    drawHLine(x,         y,         w, colour);
    drawHLine(x,         y + h - 1, w, colour);
    drawVLine(x,         y,         h, colour);
    drawVLine(x + w - 1, y,         h, colour);
}

void ILI9163C::fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                         uint16_t colour) {
    if (x >= _width || y >= _height || w <= 0 || h <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > _width)  w = _width  - x;
    if (y + h > _height) h = _height - y;
    if (w <= 0 || h <= 0) return;

    _setWindow(x, y, x + w - 1, y + h - 1);
    _cs_low();
    _dc_high();
    _fillWords(colour, static_cast<size_t>(w) * static_cast<size_t>(h));
    _cs_high();
}

// ── Circle (Midpoint / Bresenham) ─────────────────────────────────────────────

void ILI9163C::_drawCircleHelper(int16_t cx, int16_t cy, int16_t r,
                                   uint8_t corners, uint16_t colour) {
    int16_t f     = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x     = 0;
    int16_t y     = r;

    while (x < y) {
        if (f >= 0) { --y; ddF_y += 2; f += ddF_y; }
        ++x; ddF_x += 2; f += ddF_x;
        if (corners & 0x4) { drawPixel(cx + x, cy + y, colour); drawPixel(cx + y, cy + x, colour); }
        if (corners & 0x2) { drawPixel(cx + x, cy - y, colour); drawPixel(cx + y, cy - x, colour); }
        if (corners & 0x8) { drawPixel(cx - y, cy + x, colour); drawPixel(cx - x, cy + y, colour); }
        if (corners & 0x1) { drawPixel(cx - y, cy - x, colour); drawPixel(cx - x, cy - y, colour); }
    }
}

void ILI9163C::drawCircle(int16_t cx, int16_t cy, int16_t r, uint16_t colour) {
    drawPixel(cx,     cy + r, colour);
    drawPixel(cx,     cy - r, colour);
    drawPixel(cx + r, cy,     colour);
    drawPixel(cx - r, cy,     colour);
    _drawCircleHelper(cx, cy, r, 0x0F, colour);
}

void ILI9163C::_fillCircleHelper(int16_t cx, int16_t cy, int16_t r,
                                   uint8_t sides, int16_t delta, uint16_t colour) {
    int16_t f     = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x     = 0;
    int16_t y     = r;

    while (x < y) {
        if (f >= 0) { --y; ddF_y += 2; f += ddF_y; }
        ++x; ddF_x += 2; f += ddF_x;
        if (sides & 0x1) {
            drawVLine(cx + x, cy - y, 2 * y + 1 + delta, colour);
            drawVLine(cx + y, cy - x, 2 * x + 1 + delta, colour);
        }
        if (sides & 0x2) {
            drawVLine(cx - x, cy - y, 2 * y + 1 + delta, colour);
            drawVLine(cx - y, cy - x, 2 * x + 1 + delta, colour);
        }
    }
}

void ILI9163C::fillCircle(int16_t cx, int16_t cy, int16_t r, uint16_t colour) {
    drawVLine(cx, cy - r, 2 * r + 1, colour);
    _fillCircleHelper(cx, cy, r, 0x03, 0, colour);
}

// ── Triangles ─────────────────────────────────────────────────────────────────

void ILI9163C::drawTriangle(int16_t x0, int16_t y0,
                              int16_t x1, int16_t y1,
                              int16_t x2, int16_t y2,
                              uint16_t colour) {
    drawLine(x0, y0, x1, y1, colour);
    drawLine(x1, y1, x2, y2, colour);
    drawLine(x2, y2, x0, y0, colour);
}

void ILI9163C::fillTriangle(int16_t x0, int16_t y0,
                              int16_t x1, int16_t y1,
                              int16_t x2, int16_t y2,
                              uint16_t colour) {
    // Sort vertices by y
    if (y0 > y1) { _swap(y0, y1); _swap(x0, x1); }
    if (y1 > y2) { _swap(y1, y2); _swap(x1, x2); }
    if (y0 > y1) { _swap(y0, y1); _swap(x0, x1); }

    if (y0 == y2) {                          // degenerate – horizontal line
        int16_t a = x0, b = x0;
        if (x1 < a) a = x1; if (x1 > b) b = x1;
        if (x2 < a) a = x2; if (x2 > b) b = x2;
        drawHLine(a, y0, b - a + 1, colour);
        return;
    }

    int16_t dx01 = x1 - x0, dy01 = y1 - y0;
    int16_t dx02 = x2 - x0, dy02 = y2 - y0;
    int16_t dx12 = x2 - x1, dy12 = y2 - y1;
    int32_t sa = 0, sb = 0;

    int16_t last = (y1 == y2) ? y1 : y1 - 1;

    for (int16_t y = y0; y <= last; ++y) {
        int16_t a = x0 + sa / dy01;
        int16_t b = x0 + sb / dy02;
        sa += dx01; sb += dx02;
        if (a > b) _swap(a, b);
        drawHLine(a, y, b - a + 1, colour);
    }

    sa = dx12 * (y1 - y0); // NOTE: re-init sa for bottom half   (intentional reuse)
    sb = dx02 * (y1 - y0);
    for (int16_t y = y1; y <= y2; ++y) {
        int16_t a = x1 + sa / dy12;
        int16_t b = x0 + sb / dy02;
        sa += dx12; sb += dx02;
        if (a > b) _swap(a, b);
        drawHLine(a, y, b - a + 1, colour);
    }
}

// ── Rounded rectangles ────────────────────────────────────────────────────────

void ILI9163C::drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h,
                               int16_t r, uint16_t colour) {
    drawHLine(x + r,     y,         w - 2 * r, colour);
    drawHLine(x + r,     y + h - 1, w - 2 * r, colour);
    drawVLine(x,         y + r,     h - 2 * r, colour);
    drawVLine(x + w - 1, y + r,     h - 2 * r, colour);
    _drawCircleHelper(x + r,         y + r,         r, 0x1, colour);
    _drawCircleHelper(x + w - r - 1, y + r,         r, 0x2, colour);
    _drawCircleHelper(x + w - r - 1, y + h - r - 1, r, 0x4, colour);
    _drawCircleHelper(x + r,         y + h - r - 1, r, 0x8, colour);
}

void ILI9163C::fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h,
                               int16_t r, uint16_t colour) {
    fillRect(x + r, y, w - 2 * r, h, colour);
    _fillCircleHelper(x + w - r - 1, y + r, r, 0x1, h - 2 * r - 1, colour);
    _fillCircleHelper(x + r,         y + r, r, 0x2, h - 2 * r - 1, colour);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Bitmap
// ─────────────────────────────────────────────────────────────────────────────

void ILI9163C::drawBitmap(int16_t x, int16_t y, int16_t w, int16_t h,
                           const uint16_t* bitmap) {
    if (x >= _width || y >= _height || w <= 0 || h <= 0) return;
    _setWindow(x, y, x + w - 1, y + h - 1);
    _cs_low();
    _dc_high();
    _writeWords(bitmap, static_cast<size_t>(w) * static_cast<size_t>(h));
    _cs_high();
}

void ILI9163C::drawBuffer(data::Buffer* buffer) {
    drawBitmap(0, 0, buffer->getWidth(), buffer->getHeight(), buffer->getData());
}

// ─────────────────────────────────────────────────────────────────────────────
//  Text
// ─────────────────────────────────────────────────────────────────────────────

void ILI9163C::setFont(const Font* font) { _font = font; }

void ILI9163C::setTextColor(uint16_t fg, uint16_t bg) {
    _text_fg = fg;
    _text_bg = bg;
}

void ILI9163C::setCursor(int16_t x, int16_t y) {
    _cursor_x = x;
    _cursor_y = y;
}

void ILI9163C::print(char c) {
    if (_font == nullptr) return;
    if (c == '\n') {
        _cursor_x  = 0;
        _cursor_y += _font->height;
        return;
    }
    if (c == '\r') { _cursor_x = 0; return; }

    if (c < _font->first || c >= static_cast<char>(_font->first + _font->count))
        c = '?';  // substitute unknown glyph

    const uint8_t glyph_idx = static_cast<uint8_t>(c) - _font->first;
    const size_t  bytes_per_row = (_font->width + 7) / 8;
    const uint8_t* glyph = _font->data + glyph_idx * (_font->height * bytes_per_row);

    // Wrap at right edge
    if (_cursor_x + _font->width > _width) {
        _cursor_x  = 0;
        _cursor_y += _font->height;
    }

    _setWindow(_cursor_x, _cursor_y,
               _cursor_x + _font->width - 1,
               _cursor_y + _font->height - 1);

    _cs_low();
    _dc_high();

    for (uint8_t row = 0; row < _font->height; ++row) {
        for (uint8_t col = 0; col < _font->width; ++col) {
            bool set = glyph[row * bytes_per_row + col / 8] & (0x80 >> (col % 8));
            uint16_t px = set ? _text_fg : _text_bg;
            uint8_t hi = static_cast<uint8_t>(px >> 8);
            uint8_t lo = static_cast<uint8_t>(px & 0xFF);
            spi_write_blocking(_spi, &hi, 1);
            spi_write_blocking(_spi, &lo, 1);
        }
    }

    _cs_high();
    _cursor_x += _font->width;
}

void ILI9163C::print(const char* str) {
    while (str && *str) print(*str++);
}

void ILI9163C::println(const char* str) {
    print(str);
    print('\n');
}
