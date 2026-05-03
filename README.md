# GP-5 STM32 MIDI Controller

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-STM32G0B1KBU6-blue.svg)](https://www.st.com/en/microcontrollers-microprocessors/STM32G0B1KBU6.html)
[![Status](https://img.shields.io/badge/Status-Production%20Ready-green.svg)](QUICK_START.md)

A production-ready USB HOST MIDI controller for controlling the Valeton GP-5 guitar effects pedal via STM32G0B1KBU6 microcontroller.

> **Repository**: [github.com/vardelean/GP-5_STM32_Eval](https://github.com/vardelean/GP-5_STM32_Eval) (Private)

**⚠️ IMPORTANT**: This project includes critical fixes for STM32G0's USB_DRD HOST mode (see [USB_MIDI_HOST_CONFIGURATION_GUIDE.md](USB_MIDI_HOST_CONFIGURATION_GUIDE.md))

---

## Quick Links

- **[User Operations Guide](USER_OPERATIONS_GUIDE.md)** - How to use the controller (buttons, presets, CTL, Tap Tempo)
- **[Quick Start Guide](QUICK_START.md)** - Hardware setup and first connection
- **[USB HOST Configuration](USB_MIDI_HOST_CONFIGURATION_GUIDE.md)** - Critical USB fixes and technical details
- **[Hardware Port Guide](HARDWARE_PORT_GUIDE.md)** - Pin assignments for STM32G0B1KBU6

---

## Project Status

✅ **PRODUCTION READY** - February 6, 2026

All core features implemented, tested, and optimized for live performance use on STM32G0B1KBU6 (UFQFPN32 package).

### Latest Updates (February 2026)
- ✅ **USB HOST working on STM32G0B1KBU6** - Critical USB_CNTR_SOFM fix discovered and implemented
- ✅ Migrated from STM32G0B1RET6 (LQFP64, 512KB) to STM32G0B1KBU6 (UFQFPN32, 128KB)
- ✅ Manual USB peripheral initialization to work around HAL driver limitations
- ✅ 8MHz HSE oscillator with PLLQ for USB clock (48MHz)
- ✅ Full preset management with 80 button slots (16 banks × 5 buttons)
- ✅ Flash persistence across power cycles
- ✅ Clean serial output with minimal debug messages

### Implementation Complete (Phase 8)
- ✅ USB Host MIDI with hot-plug detection
- ✅ Preset save/recall system (80 slots: 16 banks × 5 buttons)
- ✅ Flash persistence (non-volatile storage)
- ✅ Button handling with debounce
- ✅ GP-5 preset tracking and synchronization
- ✅ CTL and Tap Tempo functionality
- ✅ Automatic preset recall on power-up

### Key Achievements
- **USB HOST on STM32G0**: First working implementation with documented fixes for HAL driver bugs
- **Preset Management**: 80 quick-access slots (16 banks × 5 buttons) stored in Flash
- **Auto-Recall**: Last-used preset automatically loaded on startup
- **Compact**: 77KB flash usage, fits in 128KB STM32G0B1KBU6
- **Production Ready**: Minimal logging, robust error handling, stable operation

---

## Features

### USB HOST MIDI
- Full USB HOST support for MIDI class devices
- **CRITICAL FIX**: Manual USB_DRD initialization with SOF enable (see docs)
- Automatic device detection with VID/PID identification (Valeton GP-5: 0x84EF/0x0184)
- Send and receive SysEx messages
- Send and receive Control Change (CC) messages
- Device connection/disconnection notifications
- Real-time MIDI data processing
- Hot-plug support with disconnect filtering

### Preset Management
- **80 Preset Slots**: 16 banks × 5 buttons
- Save current GP-5 preset to any button
- Instant recall with button press
- Clear saved presets
- Flash storage (survives power cycles)
- Automatic startup recall
### GPIO Button Control
- **5 Preset buttons** with **16 banks** (bank up/down navigation):
  - **Preset Buttons 1-5**: Quick preset recall/save (80 total slots)
  - **Bank Up/Down**: Navigate between banks 1-16
  - **CTL**: Send CC#69 to GP-5 (toggle)
  - **Tap Tempo**: Preset save (2-5s hold) / clear (>5s hold)

- **Button Features**:
  - Interrupt-driven. Fast response on push, 50ms debounce on relesase
  - One button active at a time (mutual exclusion)
  - **Short press** (< 2s): Recall saved preset
  - **Tap Tempo + Button** (2-5s): Save current GP-5 preset
  - **Tap Tempo + Button** (>5s): Clear saved preset
  - State reset on release prevents false triggers

### Serial Communication
- UART2 for debugging via ST-Link Virtual COM Port
- `printf()` support for easy debugging
- Real-time event logging
- MIDI message monitoring
- 115200 baud, 8N1

---

## Hardware Configuration

### Microcontroller
- **Device**: STM32G0B1KBU6 (UFQFPN32 package)
- **Core**: ARM Cortex-M0+ @ 48MHz
- **Flash**: 128KB
- **RAM**: 144KB
- **USB**: USB_DRD_FS (Dual Role Device - Full Speed)
- **Package**: UFQFPN32 (5mm × 5mm, 0.5mm pitch)
- **Debug**: SWD via ST-Link

### Clock Configuration
- **HSE**: 8MHz external oscillator (PC14 - OSC32_IN with BYPASS mode)
- **PLL**: 8MHz × 12 / 1 = 96MHz VCO
- **PLLQ**: 96MHz / 2 = 48MHz (USB clock)
- **PLLR**: 96MHz / 2 = 48MHz (System clock)
- **Alternative**: HSI48 for USB (48MHz internal RC oscillator)

### Pin Assignment (UFQFPN32)

#### USB HOST
- **PA11** (Pin 22): USB_DM (USB Data Minus)
- **PA12** (Pin 23): USB_DP (USB Data Plus)
- **PB9**: USB_PWR (VBUS power control via P-channel MOSFET)

#### Buttons
- PC4, PC5, PB3, PB4, PB5: Bank buttons (with GPIO_PULLUP, EXTI)
- PC6: CTL button
- PC7: Tap Tempo button

#### Serial Debug (USART2)
- **PA2** (Pin 8): USART2_TX
- **PA3** (Pin 9): USART2_RX

#### Clock
- **PC14** (Pin 2): OSC32_IN (8MHz external oscillator)

#### Debug (SWD)
- **PA13** (Pin 26): SWDIO
- **PA14** (Pin 27): SWCLK

### Power Requirements
- **Supply Voltage**: 3.3V regulated
- **USB VBUS**: 5V (provided to GP-5 via USB HOST port)
- **Current Draw**: ~50mA typical, 150mA max (including GP-5)

---

## Software Architecture

### Module Overview

#### 1. USB HOST MIDI Driver (`usbh_midi.c/h`)
Custom USB HOST class driver for MIDI devices with critical fixes for STM32G0:
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
