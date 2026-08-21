#include "kr_text.h"

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

void krPrint(Arduino_Canvas *gfx, const char *s) {
  if (!s) return;
  if (!fontReady) krTextBegin();

  // Arduino_GFX has native U8g2 font support. With U8g2 installed,
  // unifont_h_cjk provides UTF-8 Korean without a separate bitmap font.
  gfx->setFont(u8g2_font_unifont_h_cjk);
  gfx->setUTF8Print(true);
  gfx->setTextColor(curColor);
  gfx->setCursor(curX, curY);
  gfx->print(s);
  curX = gfx->getCursorX();
  curY = gfx->getCursorY();
}
