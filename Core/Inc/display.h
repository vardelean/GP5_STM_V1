#ifndef DISPLAY_H
#define DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "ssd1306.h"

#define bankNumFirstDigitPosX 9
#define bankNumFirstDigitPosY 27
#define bankNumSecondDigitPosX 34
#define bankNumSecondDigitPosY 27
#define presetNumPosX 86
#define presetNumPosY 28
#define gp5SavedPresetPosX 106
#define gp5SavedPresetPosPosY 4

/* External variables */
extern uint8_t bankAreaCoords[]; // x, y, width, height
extern SSD1306_COLOR bankNumColor;

/* Function prototypes */
void Display_Init(void);
void Display_BankNumber(uint8_t bankNum, SSD1306_COLOR color);
void Display_PresetNumber(uint8_t presetNum);
void Display_GP5SavedPreset(uint8_t gp5Preset);
SSD1306_COLOR InvertBankBackground(SSD1306_COLOR currentColor, uint8_t bankNumber);

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_H */