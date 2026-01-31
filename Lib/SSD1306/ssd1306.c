/**
  ******************************************************************************
  * @file    ssd1306.c
  * @brief   SSD1306 OLED display driver implementation
  ******************************************************************************
  */

#include "ssd1306.h"
#include <string.h>

/* Private variables */
static uint8_t SSD1306_Buffer[SSD1306_BUFFER_SIZE];
static I2C_HandleTypeDef *ssd1306_i2c;
static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;

/* SSD1306 commands */
#define SSD1306_CMD_DISPLAY_OFF             0xAE
#define SSD1306_CMD_DISPLAY_ON              0xAF
#define SSD1306_CMD_SET_MEMORY_MODE         0x20
#define SSD1306_CMD_SET_COLUMN_ADDR         0x21
#define SSD1306_CMD_SET_PAGE_ADDR           0x22
#define SSD1306_CMD_SET_START_LINE          0x40
#define SSD1306_CMD_SET_CONTRAST            0x81
#define SSD1306_CMD_SET_CHARGE_PUMP         0x8D
#define SSD1306_CMD_SET_SEGMENT_REMAP       0xA1
#define SSD1306_CMD_SET_ENTIRE_DISPLAY_ON   0xA4
#define SSD1306_CMD_SET_NORMAL_DISPLAY      0xA6
#define SSD1306_CMD_SET_INVERSE_DISPLAY     0xA7
#define SSD1306_CMD_SET_MULTIPLEX_RATIO     0xA8
#define SSD1306_CMD_SET_DISPLAY_OFFSET      0xD3
#define SSD1306_CMD_SET_DISPLAY_CLK_DIV     0xD5
#define SSD1306_CMD_SET_PRECHARGE_PERIOD    0xD9
#define SSD1306_CMD_SET_COM_PINS            0xDA
#define SSD1306_CMD_SET_VCOM_DESELECT       0xDB
#define SSD1306_CMD_SET_COM_SCAN_DEC        0xC8
#define SSD1306_CMD_DEACTIVATE_SCROLL       0x2E
#define SSD1306_CMD_ACTIVATE_SCROLL         0x2F
#define SSD1306_CMD_RIGHT_HORIZONTAL_SCROLL 0x26
#define SSD1306_CMD_LEFT_HORIZONTAL_SCROLL  0x27

/* Private functions */
static bool SSD1306_WriteCommand(uint8_t command) {
    return HAL_I2C_Mem_Write(ssd1306_i2c, SSD1306_I2C_ADDR, 0x00, 1, &command, 1, HAL_MAX_DELAY) == HAL_OK;
}

static bool SSD1306_WriteData(uint8_t *data, uint16_t size) {
    return HAL_I2C_Mem_Write(ssd1306_i2c, SSD1306_I2C_ADDR, 0x40, 1, data, size, HAL_MAX_DELAY) == HAL_OK;
}

/* Initialize SSD1306 */
bool SSD1306_Init(I2C_HandleTypeDef *hi2c) {
    ssd1306_i2c = hi2c;
    
    /* Wait for display to boot */
    HAL_Delay(100);
    
    /* Init sequence */
    SSD1306_WriteCommand(SSD1306_CMD_DISPLAY_OFF);
    SSD1306_WriteCommand(SSD1306_CMD_SET_DISPLAY_CLK_DIV);
    SSD1306_WriteCommand(0x80);
    SSD1306_WriteCommand(SSD1306_CMD_SET_MULTIPLEX_RATIO);
    SSD1306_WriteCommand(SSD1306_HEIGHT - 1);
    SSD1306_WriteCommand(SSD1306_CMD_SET_DISPLAY_OFFSET);
    SSD1306_WriteCommand(0x00);
    SSD1306_WriteCommand(SSD1306_CMD_SET_START_LINE | 0x00);
    SSD1306_WriteCommand(SSD1306_CMD_SET_CHARGE_PUMP);
    SSD1306_WriteCommand(0x14);
    SSD1306_WriteCommand(SSD1306_CMD_SET_MEMORY_MODE);
    SSD1306_WriteCommand(0x00);
    SSD1306_WriteCommand(SSD1306_CMD_SET_SEGMENT_REMAP);
    SSD1306_WriteCommand(SSD1306_CMD_SET_COM_SCAN_DEC);
    SSD1306_WriteCommand(SSD1306_CMD_SET_COM_PINS);
    SSD1306_WriteCommand(0x12);
    SSD1306_WriteCommand(SSD1306_CMD_SET_CONTRAST);
    SSD1306_WriteCommand(0xCF);
    SSD1306_WriteCommand(SSD1306_CMD_SET_PRECHARGE_PERIOD);
    SSD1306_WriteCommand(0xF1);
    SSD1306_WriteCommand(SSD1306_CMD_SET_VCOM_DESELECT);
    SSD1306_WriteCommand(0x40);
    SSD1306_WriteCommand(SSD1306_CMD_SET_ENTIRE_DISPLAY_ON);
    SSD1306_WriteCommand(SSD1306_CMD_SET_NORMAL_DISPLAY);
    SSD1306_WriteCommand(SSD1306_CMD_DEACTIVATE_SCROLL);
    SSD1306_WriteCommand(SSD1306_CMD_DISPLAY_ON);
    
    /* Clear screen */
    SSD1306_Fill(Black);
    SSD1306_UpdateScreen();
    
    return true;
}

/* Fill entire screen with color */
void SSD1306_Fill(SSD1306_COLOR color) {
    memset(SSD1306_Buffer, (color == Black) ? 0x00 : 0xFF, SSD1306_BUFFER_SIZE);
}

/* Update screen with buffer */
void SSD1306_UpdateScreen(void) {
    SSD1306_WriteCommand(SSD1306_CMD_SET_COLUMN_ADDR);
    SSD1306_WriteCommand(0);
    SSD1306_WriteCommand(SSD1306_WIDTH - 1);
    SSD1306_WriteCommand(SSD1306_CMD_SET_PAGE_ADDR);
    SSD1306_WriteCommand(0);
    SSD1306_WriteCommand((SSD1306_HEIGHT / 8) - 1);
    
    SSD1306_WriteData(SSD1306_Buffer, SSD1306_BUFFER_SIZE);
}

/* Draw pixel */
void SSD1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color) {
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
        return;
    }
    
    if (color == White) {
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
    } else {
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
    }
}

/* Set cursor position */
void SSD1306_SetCursor(uint8_t x, uint8_t y) {
    cursor_x = x;
    cursor_y = y;
}

/* Helper function to map character to Font_19x26 index */
static uint16_t Font_19x26_GetIndex(char ch) {
    if (ch >= '0' && ch <= '9')        // Digits 0-9 at index 0-9
        return (ch - '0');
    return 0;                          // Default to space
}

/* Write character */
void SSD1306_WriteChar(char ch, FontDef Font, SSD1306_COLOR color) {
    uint32_t i, j;
    
    if (ch < 32 || ch > 126) {
        return;
    }
    
    if (cursor_x + Font.width > SSD1306_WIDTH) {
        cursor_x = 0;
        cursor_y += Font.height;
        if (cursor_y + Font.height > SSD1306_HEIGHT) {
            cursor_y = 0;
        }
    }
    
    if (Font.format == 1) {
        // Row-major format: each row is stored horizontally in bytes
        uint32_t bytes_per_row = (Font.width + 7) / 8;  // Number of bytes per row
        uint32_t char_index;
        
        // Special handling for Font_19x26 which has non-contiguous character set
        if (Font.width == 19 && Font.height == 26) {
            char_index = Font_19x26_GetIndex(ch);
        } else {
            char_index = (ch - 32);  // Standard ASCII mapping
        }
        
        uint32_t char_offset = char_index * Font.height * bytes_per_row;
        
        for (j = 0; j < Font.height; j++) {
            for (i = 0; i < Font.width; i++) {
                uint32_t byte_in_row = i / 8;
                uint32_t bit_in_byte = i % 8;
                uint32_t byte_idx = char_offset + j * bytes_per_row + byte_in_row;
                
                if ((Font.data[byte_idx] >> bit_in_byte) & 0x01) {
                    SSD1306_DrawPixel(cursor_x + i, cursor_y + j, color);
                } else {
                    SSD1306_DrawPixel(cursor_x + i, cursor_y + j, !color);
                }
            }
        }
    } else {
        // Column-major format: each byte represents a vertical column
        // Font_7x10 uses single byte per column
        uint32_t b;
        for (i = 0; i < Font.width; i++) {
            b = Font.data[(ch - 32) * Font.width + i];
            for (j = 0; j < Font.height; j++) {
                if ((b >> j) & 0x01) {
                    SSD1306_DrawPixel(cursor_x + i, cursor_y + j, color);
                } else {
                    SSD1306_DrawPixel(cursor_x + i, cursor_y + j, !color);
                }
            }
        }
    }
    
    cursor_x += Font.width;
}

/* Write string */
void SSD1306_WriteString(const char *str, FontDef Font, SSD1306_COLOR color) {
    while (*str) {
        if (*str == '\n') {
            cursor_x = 0;
            cursor_y += Font.height;
            if (cursor_y + Font.height > SSD1306_HEIGHT) {
                cursor_y = 0;
            }
        } else {
            SSD1306_WriteChar(*str, Font, color);
        }
        str++;
    }
}

/* Draw line */
void SSD1306_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, SSD1306_COLOR color) {
    int dx = (x1 >= x0) ? x1 - x0 : x0 - x1;
    int dy = (y1 >= y0) ? y1 - y0 : y0 - y1;
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    
    while (1) {
        SSD1306_DrawPixel(x0, y0, color);
        
        if (x0 == x1 && y0 == y1) {
            break;
        }
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

/* Draw rectangle */
void SSD1306_DrawRectangle(uint8_t x, uint8_t y, uint8_t w, uint8_t h, SSD1306_COLOR color) {
    SSD1306_DrawLine(x, y, x + w, y, color);
    SSD1306_DrawLine(x + w, y, x + w, y + h, color);
    SSD1306_DrawLine(x + w, y + h, x, y + h, color);
    SSD1306_DrawLine(x, y + h, x, y, color);
}

/* Draw filled rectangle */
void SSD1306_DrawFilledRectangle(uint8_t x, uint8_t y, uint8_t w, uint8_t h, SSD1306_COLOR color) {
    for (uint8_t i = 0; i <= h; i++) {
        SSD1306_DrawLine(x, y + i, x + w, y + i, color);
    }
}

/* Draw circle */
void SSD1306_DrawCircle(uint8_t x0, uint8_t y0, uint8_t radius, SSD1306_COLOR color) {
    int x = radius;
    int y = 0;
    int err = 0;
    
    while (x >= y) {
        SSD1306_DrawPixel(x0 + x, y0 + y, color);
        SSD1306_DrawPixel(x0 + y, y0 + x, color);
        SSD1306_DrawPixel(x0 - y, y0 + x, color);
        SSD1306_DrawPixel(x0 - x, y0 + y, color);
        SSD1306_DrawPixel(x0 - x, y0 - y, color);
        SSD1306_DrawPixel(x0 - y, y0 - x, color);
        SSD1306_DrawPixel(x0 + y, y0 - x, color);
        SSD1306_DrawPixel(x0 + x, y0 - y, color);
        
        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

/* Draw bitmap */
void SSD1306_DrawBitmap(uint8_t x, uint8_t y, const uint8_t *bitmap, uint8_t w, uint8_t h, SSD1306_COLOR color) {
    for (uint8_t i = 0; i < h; i++) {
        for (uint8_t j = 0; j < w; j++) {
            if (bitmap[i * ((w + 7) / 8) + j / 8] & (0x80 >> (j % 8))) {
                SSD1306_DrawPixel(x + j, y + i, color);
            }
        }
    }
}

/* Scroll right */
void SSD1306_ScrollRight(uint8_t start_row, uint8_t end_row) {
    SSD1306_WriteCommand(SSD1306_CMD_RIGHT_HORIZONTAL_SCROLL);
    SSD1306_WriteCommand(0x00);
    SSD1306_WriteCommand(start_row);
    SSD1306_WriteCommand(0x00);
    SSD1306_WriteCommand(end_row);
    SSD1306_WriteCommand(0x00);
    SSD1306_WriteCommand(0xFF);
    SSD1306_WriteCommand(SSD1306_CMD_ACTIVATE_SCROLL);
}

/* Scroll left */
void SSD1306_ScrollLeft(uint8_t start_row, uint8_t end_row) {
    SSD1306_WriteCommand(SSD1306_CMD_LEFT_HORIZONTAL_SCROLL);
    SSD1306_WriteCommand(0x00);
    SSD1306_WriteCommand(start_row);
    SSD1306_WriteCommand(0x00);
    SSD1306_WriteCommand(end_row);
    SSD1306_WriteCommand(0x00);
    SSD1306_WriteCommand(0xFF);
    SSD1306_WriteCommand(SSD1306_CMD_ACTIVATE_SCROLL);
}

/* Stop scroll */
void SSD1306_StopScroll(void) {
    SSD1306_WriteCommand(SSD1306_CMD_DEACTIVATE_SCROLL);
}

/* Invert display */
void SSD1306_InvertDisplay(bool invert) {
    SSD1306_WriteCommand(invert ? SSD1306_CMD_SET_INVERSE_DISPLAY : SSD1306_CMD_SET_NORMAL_DISPLAY);
}

/* Set contrast */
void SSD1306_SetContrast(uint8_t value) {
    SSD1306_WriteCommand(SSD1306_CMD_SET_CONTRAST);
    SSD1306_WriteCommand(value);
}
