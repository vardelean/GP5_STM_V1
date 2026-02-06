# GP-5 STM32 MIDI Controller

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-STM32G0B1KBU6-blue.svg)](https://www.st.com/en/microcontrollers-microprocessors/STM32G0B1KBU6.html)
[![Status](https://img.shields.io/badge/Status-Production%20Ready-green.svg)](QUICK_START.md)

A production-ready USB HOST MIDI controller for controlling the Valeton GP-5 guitar effects pedal via STM32G0B1KBU6 microcontroller.

> **Repository**: [github.com/vardelean/GP-5_STM32_Eval](https://github.com/vardelean/GP-5_STM32_Eval) (Private)

---

## Project Status

✅ **PRODUCTION READY** - December 20, 2025

All core features implemented, tested, and optimized for live performance use.

### Implementation Complete (Phase 8)
- ✅ USB Host MIDI with hot-plug auto-reset
- ✅ Scene management (Scene 1: defaults, Scene 2/3: programmable)
- ✅ Flash persistence (1000 bytes optimized)
- ✅ Button handling with state reset
- ✅ GP-5 preset tracking from all sources
- ✅ Optimized logging (DEBUG_VERBOSE flag)
- ✅ Clean production code (CubeMX markers removed)

### Key Achievements
- **Scene 1**: Always available - recalls GP-5 preset defaults via CC#0
- **Scene 2 & 3**: User-programmable with flash persistence
- **USB Hot-Plug**: Auto-reset after 3 disconnects enables connection after boot
- **Flash Optimized**: 1000 bytes (10 bytes × 100 presets)
- **Production Ready**: 62KB flash, minimal logging, robust button handling

---

## Features

### USB HOST MIDI
- Full USB HOST support for MIDI class devices
- Automatic device detection with VID/PID identification
- Send and receive SysEx messages
- Send and receive Control Change (CC) messages
- Device connection/disconnection notifications
- Real-time MIDI data processing

### GPIO Button Control
- **6 GPIO Buttons** with interrupt-driven operation:
  - `btnUp` - PC4 (Preset Up)
  - `btnDown` - PC5 (Preset Down)
  - `btnScene1` - PC1 (Recall preset defaults)
  - `btnScene2` - PC2 (User scene save/recall/delete)
  - `btnScene3` - PC3 (User scene save/recall/delete)
  - `btnTap` - PC6 (Future: Tap Tempo)

- **Button Features**:
  - Interrupt-driven with 50ms debounce
  - One button active at a time (mutual exclusion)
  - **Short press** (< 2s): Recall/activate
  - **Long press** (2-5s): Save scene (Scene 2/3 only)
  - **Extra long press** (>5s): Delete scene (Scene 2/3 only)
  - State reset on release prevents false triggers

### LED Control
- **3 Scene LEDs** with exclusive operation:
  - `ledScene1` - PA0
  - `ledScene2` - PA1
  - `ledScene3` - PA4

- Only one LED active at a time
- Automatically controlled by scene button presses

### Flash Memory Storage
- **Scene Database**: 1000 bytes (Page 254: 0x0807F000)
  - 100 presets × 10 bytes each
  - Scene 2 & 3 only (Scene 1 virtual)
  - 5 bytes per scene (1 byte flag + 4 byte bitmap)
- Automatic flash writes on scene save/delete
- Full persistence across power cycles
- RAM cache for fast scene lookups

### Serial Communication
- UART2 for debugging via ST-Link Virtual COM Port
- `printf()` support for easy debugging
- Real-time event logging
- MIDI message monitoring

## Hardware Configuration

### Microcontroller
- **Device**: STM32G0B1KBU6
- **Board**: NUCLEO-G0B1RE
- **Clock**: HSI48 for USB (48MHz)
- **Debug**: ST-Link embedded debugger

### Pin Assignment

#### USB HOST
- USB_DM / USB_DP: USB data lines for HOST mode
- USB_PWR: PC0 (USB power control)

#### Buttons (All with GPIO_PULLUP, GPIO_MODE_IT_RISING_FALLING)
- btnUp: PC4
- btnDown: PC5
- btnScene1: PC1
- btnScene2: PC2
- btnScene3: PC3
- btnTap: PC6

#### LEDs (GPIO_MODE_OUTPUT_OD for scene LEDs)
- ledScene1: PA0
- ledScene2: PA1
- ledScene3: PA4
- LED_GREEN (Board LED): PA5

#### Serial Debug (USART2)
- TX: PA2
- RX: PA3

## Software Architecture

### Module Overview

#### 1. USB HOST MIDI Driver (`usbh_midi.c/h`)
Custom USB HOST class driver for MIDI devices:
- Device enumeration and initialization
- Bulk transfer handling (IN/OUT)
- MIDI packet formatting (USB-MIDI specification)
- SysEx message fragmentation/reassembly
- Control Change message handling

#### 2. Button Handler (`button_handler.c/h`)
Interrupt-driven button management:
- EXTI interrupt handling
- Software debouncing
- Long press timer (SysTick-based)
- Mutual exclusion logic
- Event callback system

#### 3. LED Controller (`led_controller.c/h`)
Scene LED management:
- Exclusive LED control
- Simple on/off/toggle API
- Scene selection

#### 4. MIDI Manager (`midi_manager.c/h`)
High-level MIDI interface:
- USB HOST abstraction
- Send/receive message queue
- Device connection management
- Callback registration for received data

#### 5. Flash Storage (`flash_storage.c/h`)
Non-volatile configuration storage:
- Page erase and program operations
- 4-byte data entries
- Index-based access
- Read/write/erase operations

### Main Application Flow

```
1. System Initialization
   ├── HAL_Init()
   ├── Clock Configuration
   ├── GPIO Configuration
   ├── UART Configuration
   └── USB HOST Init

2. Module Initialization
   ├── Button Handler Init
   ├── LED Controller Init
   ├── MIDI Manager Init
   └── Flash Storage Init

3. Main Loop
   ├── USB HOST Process (device enumeration)
   ├── Button Handler Process (check long press)
   └── MIDI Manager Process (RX/TX data)
```

### Interrupt Handling

- **SysTick**: Button timer updates (1ms)
- **EXTI2_3**: Scene button interrupts
- **EXTI4_15**: Other button interrupts
- **USB_UCPD1_2**: USB HOST interrupt

## Button Event Flow

### Short Press
```
Button Press → Debounce → Event Generated → Send MIDI Message → LED Update (if scene)
```

### Long Press
```
Button Press → Hold ≥1s → Long Press Event → Write to Flash → Release
```

### Mutual Exclusion
```
Button A Pressed → Button B Interrupt Ignored → Button A Released → Any Button Available
```

## MIDI Message Examples

### Control Change (Tap Button)
```c
MIDI_Manager_SendCC(0, 64, 127);  // Channel 0, CC#64, Value 127
```

### SysEx Message (Scene Buttons)
```c
uint8_t sysex[] = {0xF0, 0x43, 0x10, 0x01, 0xF7};
MIDI_Manager_SendSysEx(sysex, 5);
```

### Custom Button Configuration
Edit `InitializeSysExMessages()` in [main.c](Core/Src/main.c) to customize MIDI messages for each button.

## Building the Project

### Prerequisites
- CMake 3.22 or higher
- ARM GCC toolchain (arm-none-eabi-gcc)
- STM32CubeProgrammer (for flashing)
- OpenOCD or ST-Link utilities

### Build Steps

```powershell
# Configure CMake
cmake -B build -G Ninja --preset Debug

# Build
cmake --build build

# Flash (using STM32CubeProgrammer or OpenOCD)
STM32_Programmer_CLI -c port=SWD -w build/GP5_STM_V1.elf
```

## Debugging

### Serial Monitor
Connect to ST-Link Virtual COM Port (typically 115200 baud):
```
=== STM32 USB MIDI Controller ===
Initializing...
System Ready - Waiting for USB MIDI device...
USB Device Detected
USB MIDI Device Connected - VID: 0x0499, PID: 0x1234
Button: btnScene1, Event: Short Press
LED Scene 1 ON
Sending SysEx message for btnScene1
MIDI RX [4 bytes]: 0B B0 40 7F
```

### GDB Debugging
```powershell
# Start OpenOCD
openocd -f interface/stlink.cfg -f target/stm32g0x.cfg

# In another terminal, start GDB
arm-none-eabi-gdb build/GP5_STM_V1.elf
(gdb) target remote :3333
(gdb) monitor reset halt
(gdb) load
(gdb) continue
```

## Customization

### Modify SysEx Messages
Edit [main.c](Core/Src/main.c) `InitializeSysExMessages()` function to define custom MIDI messages for each button.

### Change Button Pins
Update pin definitions in STM32CubeMX .ioc file and regenerate, or manually edit [main.h](Core/Inc/main.h).

### Adjust Timing
- Debounce time: `BUTTON_DEBOUNCE_TIME_MS` in [button_handler.h](Core/Inc/button_handler.h)
- Long press time: `BUTTON_LONG_PRESS_TIME_MS` in [button_handler.h](Core/Inc/button_handler.h)

### Flash Storage
Modify `FLASH_STORAGE_BASE_ADDRESS` in [flash_storage.h](Core/Inc/flash_storage.h) to use different flash page.

## Testing

### Test Sequence
1. Connect NUCLEO board to PC via USB (ST-Link)
2. Open serial terminal (115200 baud)
3. Connect USB MIDI device to USB HOST port
4. Observe device detection in serial output
5. Press buttons and verify:
   - Serial messages
   - LED behavior
   - MIDI transmission
6. Long press any button
7. Verify flash write confirmation
8. Reset board
9. Check flash data readback on startup

### Expected Output
```
Button: btnScene1, Event: Short Press
LED Scene 1 ON
Sending SysEx message for btnScene1

Button: btnScene1, Event: Long Press - Storing to Flash
Flash write successful for button 2

Button: btnScene1, Event: Released
```

## Troubleshooting

### USB Device Not Detected
- Check USB power enable (PC0)
- Verify USB cable connection
- Check MIDI device class compatibility
- Monitor serial output for enumeration errors

### Buttons Not Responding
- Verify GPIO configuration (pull-up enabled)
- Check EXTI interrupt priorities
- Confirm button wiring (active low)

### MIDI Messages Not Sent
- Ensure USB device is connected (check `midi_device_connected`)
- Verify SysEx message format (must start with 0xF0, end with 0xF7)
- Check USB HOST process is running

### Flash Write Fails
- Verify flash page is not write-protected
- Check flash address is valid
- Ensure no code is executing from target page

## File Structure

```
GP5_STM_V1/
├── Core/
│   ├── Inc/
│   │   ├── main.h
│   │   ├── button_handler.h
│   │   ├── led_controller.h
│   │   ├── midi_manager.h
│   │   └── flash_storage.h
│   └── Src/
│       ├── main.c
│       ├── button_handler.c
│       ├── led_controller.c
│       ├── midi_manager.c
│       ├── flash_storage.c
│       ├── gpio.c
│       ├── usart.c
│       └── stm32g0xx_it.c
├── USB_Host/
│   ├── App/
│   │   └── usb_host.c
│   └── Target/
│       ├── usbh_conf.c
│       └── usbh_platform.c
├── Middlewares/
│   └── ST/
│       └── STM32_USB_Host_Library/
│           ├── Core/
│           └── Class/
│               └── MIDI/
│                   ├── Inc/usbh_midi.h
│                   └── Src/usbh_midi.c
├── Drivers/ (STM32 HAL)
├── CMakeLists.txt
└── README.md
```

## License

This project uses STMicroelectronics HAL library and USB HOST library which are licensed under ST's Ultimate Liberty license.

## References

- [USB MIDI Specification](https://www.usb.org/sites/default/files/midi10.pdf)
- [STM32G0B1KBU6 Datasheet](https://www.st.com/resource/en/datasheet/STM32G0B1KBU6.pdf)
- [NUCLEO-G0B1RE User Manual](https://www.st.com/resource/en/user_manual/um2324-stm32-nucleo64-boards-mb1360-stmicroelectronics.pdf)
- [STM32 USB HOST Library](https://www.st.com/en/embedded-software/stsw-stm32046.html)

## Author

Custom implementation for STM32 USB MIDI Controller project.

---

**Note**: This is a complete implementation ready for compilation and deployment. All modules are integrated and tested for the specified requirements.
