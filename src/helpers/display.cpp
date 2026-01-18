#include "display.h"

void drawText(
  Display& display,
  const char* text,
  int16_t posX,
  int16_t posY,
  HAlign hAlign,
  VAlign vAlign
) {
  int16_t x1, y1;
  uint16_t textWidth, textHeight;

  display.getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);

  int16_t cursorX;
  switch (hAlign) {
    case HAlign::Left:
      cursorX = posX - x1;
      break;
    case HAlign::Center:
      cursorX = posX - (textWidth / 2) - x1;
      break;
    case HAlign::Right:
      cursorX = posX - textWidth - x1;
      break;
  }

  int16_t cursorY;
  switch (vAlign) {
    case VAlign::Top:
      cursorY = posY - y1;
      break;
    case VAlign::Center:
      cursorY = posY - (textHeight / 2) - y1;
      break;
    case VAlign::Bottom:
      cursorY = posY - textHeight - y1;
      break;
  }

  display.setCursor(cursorX, cursorY);
  display.print(text);
}

void drawTriangle(
  Display& display,
  int16_t centerX,
  int16_t centerY,
  int16_t size,
  bool pointUp,
  uint16_t color
) {
  int16_t halfWidth = size * 0.6; // width is ~1.2x height for nice proportions
  int16_t halfHeight = size / 2;

  int16_t x0, y0, x1, y1, x2, y2;

  if (pointUp) {
    x0 = centerX;
    y0 = centerY - halfHeight; // top point
    x1 = centerX - halfWidth;
    y1 = centerY + halfHeight; // bottom left
    x2 = centerX + halfWidth;
    y2 = centerY + halfHeight; // bottom right
  } else {
    x0 = centerX;
    y0 = centerY + halfHeight; // bottom point
    x1 = centerX - halfWidth;
    y1 = centerY - halfHeight; // top left
    x2 = centerX + halfWidth;
    y2 = centerY - halfHeight; // top right
  }

  display.fillTriangle(x0, y0, x1, y1, x2, y2, color);
}
