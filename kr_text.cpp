#include "kr_text.h"
#include <U8g2lib.h>

static int16_t curX = 0, curY = 0;
static uint16_t curColor = 0x0000;
static bool fontReady = false;

void krTextBegin() { fontReady = true; }

void krSetCursor(Arduino_Canvas *gfx, int16_t x, int16_t y) {
  curX = x; curY = y;
  gfx->setCursor(x, y);
}

void krSetTextColor(Arduino_Canvas *gfx, uint16_t color) {
  curColor = color;
  gfx->setTextColor(color);
}

static bool containsKorean(const char *s) {
  for (const uint8_t *p = (const uint8_t *)s; *p;) {
    uint32_t cp;
    uint8_t c = *p++;
    if (c < 0x80) cp = c;
    else if ((c & 0xE0) == 0xC0) cp = ((c & 0x1F) << 6) | (*p++ & 0x3F);
    else if ((c & 0xF0) == 0xE0) cp = ((c & 0x0F) << 12) | ((*p++ & 0x3F) << 6) | (*p++ & 0x3F);
    else cp = 0;
    if (cp >= 0xAC00 && cp <= 0xD7A3) return true;
  }
  return false;
}

void krPrint(Arduino_Canvas *gfx, const char *s) {
  if (!s) return;
  if (!fontReady) krTextBegin();
  gfx->setTextColor(curColor);
  gfx->setCursor(curX, curY);

  if (containsKorean(s)) {
    gfx->setFont(u8g2_font_unifont_h_cjk);
    gfx->setUTF8Print(true);
  } else {
    gfx->setFont(nullptr);
    gfx->setUTF8Print(false);
  }

  gfx->print(s);
  curX = gfx->getCursorX();
  curY = gfx->getCursorY();
}
