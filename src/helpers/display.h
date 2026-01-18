#ifndef HELPERS_DISPLAY_H
#define HELPERS_DISPLAY_H

#include <GxEPD2_7C.h>
#include <epd7c/GxEPD2_730c_GDEP073E01.h>

using Display = GxEPD2_7C<GxEPD2_730c_GDEP073E01, GxEPD2_730c_GDEY073D46::HEIGHT / 8>;

enum class HAlign {
  Left,
  Center,
  Right
};

enum class VAlign {
  Top,
  Center,
  Bottom
};

// draw text with configurable alignment
void drawText(
  Display& display,
  const char* text,
  int16_t posX,
  int16_t posY,
  HAlign hAlign = HAlign::Left,
  VAlign vAlign = VAlign::Top
);

// draw an up or down triangle (arrow indicator)
void drawTriangle(
  Display& display,
  int16_t centerX,
  int16_t centerY,
  int16_t size,
  bool pointUp,
  uint16_t color
);

#endif
