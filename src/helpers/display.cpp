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

void drawSparkLine(
  Display& display,
  int16_t x,
  int16_t y,
  int16_t width,
  int16_t height,
  int32_t* values,
  int count
) {
  if (count < 2) {
    return;
  }

  // find min and max for scaling
  int32_t minVal = values[0];
  int32_t maxVal = values[0];

  for (int i = 0; i < count; i++) {
    if (values[i] < minVal) minVal = values[i];
    if (values[i] > maxVal) maxVal = values[i];
  }

  int32_t range = maxVal - minVal;

  // avoid division by zero if all values are the same
  if (range == 0) {
    range = 1;
  }

  int32_t refVal = values[0]; // use first (oldest) value as reference point for coloring

  // helper lambda to convert a value to Y coordinate
  // min maps to bottom (y + height), max maps to top (y)
  auto valueToY = [&](int32_t val) -> float {
    float normalized = (float)(val - minVal) / (float)range;
    return y + height - (normalized * height);
  };

  // reference line Y position (represents starting value)
  float refY = valueToY(refVal);
  display.drawLine(x, (int16_t)refY, x + width, (int16_t)refY, GxEPD_BLACK);

  float segmentWidth = (float)width / (float)(count - 1);
  for (int i = 0; i < count - 1; i++) {
    float x1 = x + (i * segmentWidth);
    float y1 = valueToY(values[i]);
    float x2 = x + ((i + 1) * segmentWidth);
    float y2 = valueToY(values[i + 1]);

    // break each segment into tiny sub-segments for proper color transitions
    int steps = (int)segmentWidth;
    if (steps < 2) {
      steps = 2;
    }

    for (int s = 0; s < steps; s++) {
      float t1 = (float)s / steps;
      float t2 = (float)(s + 1) / steps;

      // interpolate sub-segment endpoints
      float sx1 = x1 + (x2 - x1) * t1;
      float sy1 = y1 + (y2 - y1) * t1;
      float sx2 = x1 + (x2 - x1) * t2;
      float sy2 = y1 + (y2 - y1) * t2;

      // use midpoint Y to determine color (above or below reference line)
      float midY = (sy1 + sy2) / 2.0f;
      uint16_t color = (midY <= refY) ? GxEPD_GREEN : GxEPD_RED;

      // draw line twice with 1px Y offset for 2px thickness
      display.drawLine((int16_t)sx1, (int16_t)sy1, (int16_t)sx2, (int16_t)sy2, color);
      display.drawLine((int16_t)sx1, (int16_t)sy1 + 1, (int16_t)sx2, (int16_t)sy2 + 1, color);
    }
  }
}