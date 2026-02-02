# Flash Storage and Preset Management Implementation

## Overview

This implementation provides a complete preset management system for the STM32 MIDI controller that interfaces with the Boss GP-5 effects unit. The system stores 80 presets (16 banks × 5 presets per bank) in Flash memory with wear leveling for the current preset index.

## Key Features

### 1. Flash Storage System
- **80 Preset Storage**: Stores GP-5 preset numbers (0-99) for each STM32 preset location
- **Dual Array Redundancy**: Presets are stored in two arrays (A and B) for data integrity
- **Wear Leveling**: Current preset index uses wear leveling to extend Flash lifetime
- **Persistent Initialization**: Initialization flag prevents data loss on firmware updates
- **Non-volatile**: All data survives power cycles

### 2. Preset Management
- **Bank/Preset Selection**: 16 banks (displayed 1-16) with 5 presets each (displayed 1-5)
- **Visual Feedback**: Display shows:
  - Current bank number (1-16)
  - Current preset button (1-5)
  - Stored GP-5 preset number (00-99 or "--" if empty)
- **Bank Inversion**: Bank background inverts when selecting different bank

### 3. Button Operations

#### Quick Press (Preset Recall)
- Press preset button (1-5)
- If stored GP-5 preset is valid (0-99), sends CC command to GP-5
- If empty (0xFF), no command sent, position updated
- Automatically switches to selected bank if different from current

#### Hold 2-5 Seconds (Save)
- Hold preset button for 2+ seconds
- Requests current GP-5 preset number
- Saves current GP-5 preset to the STM32 preset location
- Display updates to show saved preset number

#### Hold 5+ Seconds (Clear)
- Hold preset button for 5+ seconds
- Clears the preset (sets to 0xFF)
- Display shows "--" for cleared preset

### 4. Startup Behavior
- Loads last used STM32 preset index from Flash
- Retrieves stored GP-5 preset for that location
- After GP-5 connection, recalls the stored preset
- Retry mechanism (3 attempts, 50ms delay) for reliability
- Updates display with current position and stored preset

### 5. Bank Navigation
- **Bank Up/Down Buttons**: Navigate banks 1-16
- **Visual Feedback**: Bank number background inverts when selecting different bank
- **Preset Button**: Confirms bank selection and recalls preset

## Flash Memory Layout

The system uses the last 4 pages of the 512KB Flash (pages 252-255):

```
Page 252-253 (4KB): Preset Array Storage
  - Offset 0-79:   Array A (80 preset values)
  - Offset 80-159: Array B (80 preset values, duplicate)
  
Page 254 (2KB): Current Preset Index with Wear Leveling
  - Up to 256 entries (8 bytes each)
  - Format: [index, checksum, 0xFF, 0xFF, marker (0x5A5A5A5A)]
  
Page 255 (2KB): Initialization Flag
  - Value: 0xA5A5A5A5 if initialized
  - Prevents re-initialization on firmware updates
```

## Preset Index Calculation

```c
// STM32 preset index = bank * 5 + button
// Example: Bank 6 (display), Button 4 (display)
// Real bank = 6-1 = 5, Real button = 4-1 = 3
// Index = 5 * 5 + 3 = 28
```

## File Structure

### Core Files
- `Core/Inc/flash_storage.h` - Flash storage API
- `Core/Src/flash_storage.c` - Flash storage implementation
- `Core/Inc/preset_buttons.h` - Button handler API
- `Core/Src/preset_buttons.c` - Button handler with hold detection
- `Core/Inc/display.h` - Display API
- `Core/Src/display.c` - Display implementation with GP-5 preset display

### Modified Files
- `Core/Src/main.c` - Added startup preset recall call
- `cmake/stm32cubemx/CMakeLists.txt` - Added flash_storage.c

## API Functions

### Flash Storage

```c
int FlashStorage_Init(void);
uint8_t* FlashStorage_GetPresetArray(void);
uint8_t FlashStorage_GetPreset(uint8_t presetIndex);
int FlashStorage_SavePreset(uint8_t presetIndex, uint8_t gp5Preset);
uint8_t FlashStorage_GetCurrentPresetIndex(void);
int FlashStorage_SaveCurrentPresetIndex(uint8_t presetIndex);
```

### Preset Buttons

```c
void PresetButtons_Init(void);
void PresetButtons_RequestStartupPresetRecall(void);
void PresetButtons_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
void PresetButtons_Process(void);
uint8_t PresetButtons_GetCurrentBank(void);
uint8_t PresetButtons_GetCurrentPreset(void);
void PresetButtons_SetCurrentPreset(uint8_t preset);
void PresetButtons_OnPresetChangeACK(bool sentByUs);
```

### Display

```c
void Display_Init(void);
void Display_BankNumber(uint8_t bankNum, SSD1306_COLOR color);
void Display_PresetNumber(uint8_t presetNum);
void Display_GP5SavedPreset(uint8_t gp5Preset);
SSD1306_COLOR InvertBankBackground(SSD1306_COLOR currentColor, uint8_t bankNumber);
```

## Operation Flow

### Startup
1. `FlashStorage_Init()` - Initialize Flash, load presets to RAM
2. `PresetButtons_Init()` - Load saved preset index, update display
3. Wait for GP-5 connection (1 second delay)
4. `PresetButtons_RequestStartupPresetRecall()` - Recall saved preset
5. Retry up to 3 times if no ACK received

### Preset Recall
1. User presses preset button
2. System checks if stored preset is valid (0-99)
3. If valid, sends CC#0 with preset number to GP-5
4. If empty (0xFF), updates position but sends nothing
5. Saves current preset index to Flash (with wear leveling)

### Preset Save
1. User holds preset button for 2+ seconds
2. System requests current GP-5 preset
3. When preset received, saves to Flash at button location
4. Display updates to show saved preset number

### Preset Clear
1. User holds preset button for 5+ seconds
2. System writes 0xFF to preset location
3. Display shows "--"

## Display Format

```
┌─────────────────────────────┬─────────────────────────────┐
│       BANK                  │  GP5: 56                    │
├─────────────────────────────┼─────────────────────────────┤
│                             │                             │
│         12                  │          3                  │
│      (Bank 1-16)            │      (Button 1-5)           │
│                             │                             │
└─────────────────────────────┴─────────────────────────────┘
```

- Bank: 1-16 (inverted background when temporary bank selected)
- Button: 1-5 (current preset button)
- GP5: Stored GP-5 preset (00-99 or "--" if empty)

## Important Notes

1. **Display Numbers**: Banks and buttons are 1-indexed for users (1-16, 1-5) but 0-indexed in code (0-15, 0-4)
2. **Preset Values**: Only 0-99 are valid GP-5 presets. 0xFF means empty/cleared.
3. **Wear Leveling**: Only the current preset index uses wear leveling (up to 256 writes before page erase)
4. **Flash Updates**: The initialization flag protects against data loss during firmware updates
5. **Retry Mechanism**: Preset recalls retry 3 times with 50ms delay if ACK not received

## Build Information

- Added `flash_storage.c` to CMake build system
- Increased Flash usage by ~2KB for new functionality
- Memory usage: ~73KB Flash, ~6KB RAM

## Testing Recommendations

1. Test first initialization (Flash cleared)
2. Test preset save/recall cycle
3. Test preset clear operation
4. Test bank switching with preset recall
5. Test power cycle persistence
6. Test firmware update (data should persist)
7. Test wear leveling (multiple preset index saves)
8. Test empty preset handling (no CC sent)
9. Test retry mechanism (disconnect during preset recall)
