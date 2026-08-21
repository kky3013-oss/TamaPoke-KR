#pragma once
#include <Arduino.h>
#include "Arduino_GFX_Library.h"

void krKeyboardBegin();
void krKeyboardOpen(const char *existing, char *buf, size_t cap);
void krKeyboardRender(Arduino_Canvas *gfx, const char *buf, bool koreanMode);
void krKeyboardTap(int16_t x, int16_t y, bool &koreanMode, char *buf, size_t cap, bool &closeKeyboard);
