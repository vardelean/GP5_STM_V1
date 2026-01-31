/**
  ******************************************************************************
  * @file    ssd1306.h
  * @brief   SSD1306 OLED display driver for STM32
  ******************************************************************************
  */
#ifndef __SSD1306_H__
#define __SSD1306_H__


#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "ssd1306_fonts.h"

// Include main.h which properly includes all HAL headers in correct order
#include "main.h"


/* SSD1306 settings */
#define SSD1306_WIDTH           128
#define SSD1306_HEIGHT          64
#define SSD1306_BUFFER_SIZE     (SSD1306_WIDTH * SSD1306_HEIGHT / 8)

/* I2C address */
#define SSD1306_I2C_ADDR        0x78  // 0x3C << 1

/* Colors */
typedef enum {
    Black = 0x00,
    White = 0x01
} SSD1306_COLOR;

/* Function prototypes */
bool SSD1306_Init(I2C_HandleTypeDef *hi2c);
void SSD1306_Fill(SSD1306_COLOR color);
void SSD1306_UpdateScreen(void);
void SSD1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color);
void SSD1306_WriteChar(char ch, FontDef Font, SSD1306_COLOR color);
void SSD1306_WriteString(const char *str, FontDef Font, SSD1306_COLOR color);
void SSD1306_SetCursor(uint8_t x, uint8_t y);
void SSD1306_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, SSD1306_COLOR color);
void SSD1306_DrawRectangle(uint8_t x, uint8_t y, uint8_t w, uint8_t h, SSD1306_COLOR color);
void SSD1306_DrawFilledRectangle(uint8_t x, uint8_t y, uint8_t w, uint8_t h, SSD1306_COLOR color);
void SSD1306_DrawCircle(uint8_t x, uint8_t y, uint8_t radius, SSD1306_COLOR color);
void SSD1306_DrawBitmap(uint8_t x, uint8_t y, const uint8_t *bitmap, uint8_t w, uint8_t h, SSD1306_COLOR color);
void SSD1306_ScrollRight(uint8_t start_row, uint8_t end_row);
void SSD1306_ScrollLeft(uint8_t start_row, uint8_t end_row);
void SSD1306_StopScroll(void);
void SSD1306_InvertDisplay(bool invert);
void SSD1306_SetContrast(uint8_t value);

#endif // SSD1306_H
