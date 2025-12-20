# Implementation Notes - GP-5 MIDI Controller

## Project Status

**Status**: ✅ PRODUCTION READY (Phase 8 Complete)  
**Date**: December 20, 2025

All features implemented, tested, and optimized for live performance.

---

## Implementation Summary

### ✅ Core Features (Phase 1-7)

1. **USB HOST MIDI with Hot-Plug**
   - GP-5 device enumeration and communication
   - Auto-reset after 3 disconnects enables hot-plug
   - 1-second preset sync delay after connection
   - VID/PID detection, SysEx and CC message handling

2. **Scene Management System**
   - **Scene 1**: Always available (preset defaults via CC#0)
   - **Scene 2 & 3**: User-programmable save/recall/delete
   - Flash persistence (1000 bytes, 100 presets × 10 bytes)
   - RAM cache for fast lookups

3. **Button System with State Reset**
   - 6 buttons: Up/Down (presets), Scene 1/2/3, Tap (future)
   - Interrupt-driven with 50ms debounce
   - Long press (2s) and extra-long (5s) detection
   - **Critical fix**: Flags reset on release prevents false triggers
   - Mutual exclusion (one button at a time)

4. **LED Indicators**
   - 3 scene LEDs (exclusive operation)
   - Scene 1 always ON after preset changes
   - Scene 2/3 ON when those scenes active
   - Blink patterns for save/delete operations

5. **GP-5 Communication**
   - Preset tracking from all sources (app, hardware, USB)
   - Patch info parsing (10 effects on/off bitmap)
   - Only sends CC commands for changed patches
   - CTL screen flicker for app refresh

### ✅ Phase 8 Optimization

6. **Performance Tuning**
   - Flash reduced from 63.2KB to 62.0KB
   - DEBUG_VERBOSE flag (0=production, 1=verbose)
   - Removed 200+ STM32CubeMX USER CODE markers
   - Clean, professional codebase

---

### MIDI Protocol
- **USB-MIDI Format**: All MIDI messages are wrapped in USB-MIDI packets (4 bytes per event)
- **SysEx Handling**: Large SysEx messages are fragmented into USB packets with proper CIN codes
- **Bulk Transfers**: Used for reliable, non-real-time MIDI communication

### Flash Storage
- **Page-based**: Uses entire page erase/program cycle
- **Read-Modify-Write**: Preserves other data in page during write operations
- **64-bit Alignment**: Flash programming uses DOUBLEWORD (64-bit) operations for G0 family

## Code Organization

### Module Responsibilities

| Module | Responsibility |
|--------|----------------|
| `usbh_midi.c` | USB HOST MIDI class driver, low-level USB operations |
| `midi_manager.c` | High-level MIDI API, message formatting |
| `button_handler.c` | Button interrupt handling, debounce, long-press |
| `led_controller.c` | LED control, exclusive scene selection |
| `flash_storage.c` | Flash read/write/erase operations |
| `main.c` | Application logic, module integration, callbacks |

### Callback Chain

```
Button Press (Hardware)
    ↓
EXTI Interrupt (HAL)
    ↓
HAL_GPIO_EXTI_Callback
    ↓
ButtonHandler_GPIO_EXTI_Callback
    ↓
ButtonEventHandler (main.c)
    ↓
LED_SetScene / MIDI_Manager_SendSysEx / FlashStorage_WriteData
```

## Customization Guide

### GP-5 Pedal MIDI Configuration

The GP-5 specific MIDI communication is handled in dedicated files:
- [gp5_midi.h](Core/Inc/gp5_midi.h) - GP-5 MIDI interface definitions
- [gp5_midi.c](Core/Src/gp5_midi.c) - GP-5 MIDI message handling

#### Button to MIDI Mapping

Current button configuration:
- **btnUp**: Sends CC#25 (Patch Up) with value 127
- **btnDown**: Sends CC#24 (Patch Down) with value 127
- **btnScene1-3**: Sends SysEx messages for scene changes
- **btnTap**: Sends CC#64 (Tap Tempo) with value 127

#### Modifying GP-5 MIDI Messages

Edit `GP5_MIDI_Init()` in [gp5_midi.c](Core/Src/gp5_midi.c) to customize SysEx messages:

```c
void GP5_MIDI_Init(void)
{
  // Customize scene SysEx messages
  button_sysex_config[BTN_SCENE1].sysex_data[0] = 0xF0;  // SysEx Start
  button_sysex_config[BTN_SCENE1].sysex_data[1] = 0x00;  // Your manufacturer ID
  button_sysex_config[BTN_SCENE1].sysex_data[2] = 0x01;  // Command byte
  button_sysex_config[BTN_SCENE1].sysex_data[3] = 0x01;  // Scene 1
  button_sysex_config[BTN_SCENE1].sysex_data[4] = 0xF7;  // SysEx End
  button_sysex_config[BTN_SCENE1].sysex_length = 5;
}
```

Edit `GP5_MIDI_HandleButtonEvent()` in [gp5_midi.c](Core/Src/gp5_midi.c) to change CC numbers or add custom actions:

```c
if (button == BTN_UP)
{
  // Change CC number or value
  MIDI_Manager_SendCC(GP5_MIDI_CHANNEL, 25, 127);  // CC#25, Value 127
}
```

#### Processing Received MIDI Messages from GP-5

Edit `GP5_MIDI_ProcessReceivedData()` in [gp5_midi.c](Core/Src/gp5_midi.c):

```c
void GP5_MIDI_ProcessReceivedData(uint8_t *data, uint16_t length)
{
  // Custom GP-5 response handling
  // See function implementation for packet decoding
}
```

### Changing Flash Storage Location

Edit [flash_storage.h](Core/Inc/flash_storage.h):

```c
// Use a different page (e.g., page 254)
#define FLASH_STORAGE_BASE_ADDRESS    0x0807F000  // Page 254
```

**Warning**: Ensure the selected page is not used by your program code!

## Performance Characteristics

### Timing
- **Button Debounce**: 50ms
- **Long Press Threshold**: 2000ms
- **SysTick Interval**: 1ms
- **USB Polling**: Every main loop iteration (~1ms typical)

### Memory Usage
- **Flash**: ~30KB program code + MIDI driver
- **RAM**: ~2KB for buffers and state
- **Stack**: ~1KB (default)

### Flash Endurance
- **Write Cycles**: 10,000 cycles typical for STM32G0
- **Long Press Operations**: Each long press = 1 erase + 1 write cycle
- **Expected Lifetime**: 10,000 long presses per button position

## Known Limitations

1. **Single MIDI Device**: Only one USB MIDI device can be connected at a time
2. **Flash Write Time**: Flash operations may take 20-100ms, blocking operation
3. **SysEx Size**: Maximum SysEx message size is 256 bytes
4. **Button Bounce**: Very short button presses (<50ms) may be ignored
5. **USB Speed**: Full-speed USB only (12 Mbps)

## Debugging Tips

### Enable More Verbose Logging

Add to `usbh_conf.h`:
```c
#define USBH_DEBUG_LEVEL    2  // 0=None, 1=User, 2=All
```

### Monitor USB Enumeration

Check serial output during device connection:
```
USB Device Detected
USB MIDI Device Connected - VID: 0x0499, PID: 0x1234
```

### Test Flash Operations

Add to main initialization:
```c
// Test flash write
uint8_t test_data[4] = {0xAA, 0xBB, 0xCC, 0xDD};
FlashStorage_WriteData(0, test_data);

// Test flash read
uint8_t read_data[4];
FlashStorage_ReadData(0, read_data);
printf("Flash test: %02X %02X %02X %02X\r\n", 
       read_data[0], read_data[1], read_data[2], read_data[3]);
```

### Verify Button Interrupts

Add printf in `ButtonHandler_GPIO_EXTI_Callback()`:
```c
printf("IRQ: Pin %d\r\n", GPIO_Pin);
```

## Future Enhancements

Potential improvements for future versions:

1. **Multiple MIDI Devices**: USB hub support
2. **MIDI Clock**: Tap tempo with MIDI clock generation
3. **Configuration Storage**: Store SysEx messages in flash
4. **USB Device Mode**: Act as USB MIDI device (instead of HOST)
5. **Display Support**: Add LCD/OLED for visual feedback
6. **MIDI Learn**: Record received MIDI messages to buttons
7. **Button Combinations**: Support multi-button shortcuts
8. **Preset Management**: Multiple button configuration presets

## Testing Checklist

### Basic Functionality
- [ ] Power on and serial output appears
- [ ] USB MIDI device detected and VID/PID shown
- [ ] All 6 buttons respond to short press
- [ ] Scene LEDs work exclusively (only one on)
- [ ] Long press detected after 1 second
- [ ] Flash write confirmation on long press
- [ ] Flash data read back on next power-on

### MIDI Communication
- [ ] SysEx messages transmitted correctly
- [ ] CC messages transmitted correctly
- [ ] Received MIDI data appears in serial log
- [ ] No USB errors during enumeration

### Edge Cases
- [ ] Rapid button presses handled correctly
- [ ] Button release during long press handled
- [ ] USB device disconnect/reconnect handled
- [ ] Multiple flash writes to same location work

## Support and Contribution

For issues or questions about this implementation:
- Review the [README.md](README.md) for basic usage
- Check serial output for error messages
- Verify hardware connections match pin assignments
- Test with a known-working USB MIDI device

## Version History

- **v1.0** (2025-12-15): Initial implementation
  - USB HOST MIDI class driver
  - Button interrupt handling with mutual exclusion
  - LED control system
  - Flash storage operations
  - Complete integration and testing

---

**Development Platform**: STM32CubeMX + CMake + ARM GCC  
**Target**: STM32G0B1RE on NUCLEO-G0B1RE  
**Last Updated**: December 15, 2025
