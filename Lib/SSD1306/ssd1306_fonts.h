/**
  ******************************************************************************
  * @file    ssd1306_fonts.h
  * @brief   Font definitions for SSD1306
  ******************************************************************************
  */

#ifndef SSD1306_FONTS_H
#define SSD1306_FONTS_H

#include <stdint.h>

/* Font structure definition */
typedef struct {
    const uint8_t width;
    const uint8_t height;
    const uint8_t *data;
    const uint8_t format;  // 0 = column-major (default), 1 = row-major
} FontDef;

extern FontDef Font_7x10;
extern FontDef Font_19x26;

#endif // SSD1306_FONTS_H