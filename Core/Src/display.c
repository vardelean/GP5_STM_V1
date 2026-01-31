/**
 * @file display.c
 * @brief Display operations for SSD1306 OLED
 * 
 * This file contains functions to initialize and update the OLED display,
 * including bank and preset number display with background color inversion.
 */
#include "ssd1306.h"
#include "display.h"

/* Bank area coordinates: x, y, width, height */
uint8_t bankAreaCoords[] = {1, 19, 61, 43};

/* Current bank number display color */
SSD1306_COLOR bankNumColor = White;

void Display_Init(void)
{
  // Display initialization successful
  SSD1306_Fill(Black);     // Clear screen
  SSD1306_SetCursor(19,  4);  
  SSD1306_COLOR color = White; 

  SSD1306_WriteString("BANK", Font_7x10, color);
  SSD1306_SetCursor(75,  4); 
  SSD1306_WriteString("PRESET", Font_7x10, color);
 // SSD1306_DrawLine(0, 15, 127, 15, color); // Horizontal line below titles
  SSD1306_DrawRectangle(0, 0, 127, 15, color);  // Draw a border of one pixel around the title
 
  SSD1306_DrawFilledRectangle(63, 0, 0, 63, color);  // x=63, y=0, width=0 (one pixel), height=64; middle vertical line
  SSD1306_DrawRectangle(0, 16, 127, 47, color);  // Draw a border of one pixel around the bank/preset area
 
  SSD1306_UpdateScreen();  // Update display with all of the above graphics
  
}

void Display_BankNumber(uint8_t bankNum, SSD1306_COLOR color)
{
    // Display bank number on OLED
    uint8_t firstDigit = bankNum / 10;
    uint8_t secondDigit = bankNum % 10;

    SSD1306_SetCursor(bankNumFirstDigitPosX, bankNumFirstDigitPosY);
    SSD1306_WriteChar(0x30 + firstDigit, Font_19x26, color); 
    SSD1306_SetCursor(bankNumSecondDigitPosX, bankNumSecondDigitPosY);
    SSD1306_WriteChar(0x30 + secondDigit, Font_19x26, color); 
    SSD1306_UpdateScreen();  // Refresh display to show updated bank number
}

void Display_PresetNumber(uint8_t presetNum)
{
    // Display preset number on OLED (show button number 0-4)
    uint8_t buttonNum = presetNum % 5;
    SSD1306_SetCursor(presetNumPosX, presetNumPosY);
    SSD1306_WriteChar(0x30 + buttonNum, Font_19x26, White); 
    SSD1306_UpdateScreen();  // Refresh display to show updated preset number
}

SSD1306_COLOR InvertBankBackground(SSD1306_COLOR currentColor, uint8_t bankNumber)
{
    // Invert the bank background color
    SSD1306_COLOR newBackColor = (currentColor == White) ? White : Black;
    SSD1306_DrawFilledRectangle(bankAreaCoords[0], bankAreaCoords[1], bankAreaCoords[2], bankAreaCoords[3], newBackColor);
    SSD1306_UpdateScreen();  // Update display to show new background
    SSD1306_COLOR newBankColor = (currentColor == White) ? Black : White; 
    Display_BankNumber(bankNumber, newBankColor);  // Redraw bank number to show over new background
    SSD1306_UpdateScreen();  // Update display to show new background
    return newBankColor;
} 