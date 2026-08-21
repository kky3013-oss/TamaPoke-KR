#pragma once
#include <Arduino.h>
#include "Arduino_GFX_Library.h"

void krTextBegin();
void krSetCursor(Arduino_Canvas *gfx, int16_t x, int16_t y);
void krSetTextColor(Arduino_Canvas *gfx, uint16_t color);
void krPrint(Arduino_Canvas *gfx, const char *s);

template <typename T>
inline void krPrint(Arduino_Canvas *gfx, const T &v) { gfx->print(v); }
