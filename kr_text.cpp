#include "kr_text.h"
#include <U8g2lib.h>

static U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8X8_PIN_NONE, U8X8_PIN_NONE, U8X8_PIN_NONE);
static bool ready = false;
static int16_t curX = 0, curY = 0;
static uint16_t curColor = 0x0000;

static uint32_t nextUtf8(const char *&p) {
  uint8_t c = (uint8_t)*p++;
  if (c < 0x80) return c;
  if ((c & 0xE0) == 0xC0) return ((c & 0x1F) << 6) | ((uint8_t)*p++ & 0x3F);
  if ((c & 0xF0) == 0xE0) return ((c & 0x0F) << 12) | (((uint8_t)*p++ & 0x3F) << 6) | ((uint8_t)*p++ & 0x3F);
  if ((c & 0xF8) == 0xF0) return ((c & 7) << 18) | (((uint8_t)*p++ & 0x3F) << 12) | (((uint8_t)*p++ & 0x3F) << 6) | ((uint8_t)*p++ & 0x3F);
  return '?';
}

static void drawHangul(Arduino_Canvas *gfx, uint32_t cp, int16_t x, int16_t baseline) {
  char utf[5] = {0};
  utf[0] = (char)(0xE0 | (cp >> 12));
  utf[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
  utf[2] = (char)(0x80 | (cp & 0x3F));

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_t_korean1);
  u8g2.setDrawColor(1);
  u8g2.drawUTF8(0, 24, utf);
  const uint8_t *buf = u8g2.getBufferPtr();
  int16_t top = baseline - 24;
  for (int y = 0; y < 64; y++) {
    int dy = top + y;
    if (dy < 0 || dy >= 466) continue;
    for (int xx = 0; xx < 32; xx++) {
      if (buf[(y >> 3) * 128 + xx] & (1 << (y & 7)))
        gfx->drawPixel(x + xx, dy, curColor);
    }
  }
}

void krTextBegin() {
  if (ready) return;
  u8g2.setFont(u8g2_font_unifont_t_korean1);
  ready = true;
}

void krSetCursor(Arduino_Canvas *gfx, int16_t x, int16_t y) {
  curX = x; curY = y; gfx->setCursor(x, y);
}

void krSetTextColor(Arduino_Canvas *gfx, uint16_t color) {
  curColor = color; gfx->setTextColor(color);
}

void krPrint(Arduino_Canvas *gfx, const char *s) {
  if (!s) return;
  krTextBegin();
  bool hasHangul = false;
  for (const char *p = s; *p;) {
    const char *q = p;
    uint32_t cp = nextUtf8(q);
    p = q;
    if (cp >= 0xAC00 && cp <= 0xD7A3) { hasHangul = true; break; }
  }
  if (!hasHangul) {
    gfx->setCursor(curX, curY);
    gfx->print(s);
    curX = gfx->getCursorX();
    return;
  }

  const char *p = s;
  while (*p) {
    const char *q = p;
    uint32_t cp = nextUtf8(q);
    p = q;
    if (cp >= 0xAC00 && cp <= 0xD7A3) {
      drawHangul(gfx, cp, curX, curY);
      curX += 16;
    } else {
      char one[2] = {(char)cp, 0};
      gfx->setCursor(curX, curY);
      gfx->print(one);
      curX = gfx->getCursorX();
    }
  }
  gfx->setCursor(curX, curY);
}
