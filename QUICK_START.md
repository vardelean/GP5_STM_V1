# Quick Start Guide - GP-5 MIDI Controller

## Hardware Setup

### Required Components
- **STM32G0B1KBU6** microcontroller (UFQFPN32 package) on custom PCB
- **Valeton GP-5** guitar effects pedal  
- USB-C cable to connect GP-5 to STM32 controller
- ST-Link V3 MINI debugger for programming and serial monitor
- **8MHz external oscillator** on PC14 (or use HSI48 internal oscillator)
- **3.3V power supply** for STM32

### Required Software
- CMake 3.22+
- ARM GCC Toolchain (arm-none-eabi-gcc 14.3.1 or later)
- STM32CubeProgrammer v2.21+
- VS Code with CMake Tools extension
- Serial terminal (115200 baud for debug output)

### Hardware Connections

#### Power
1. Apply 3.3V to STM32 VDD pin
2. Connect GND
3. USB VBUS (5V) powered via P-channel MOSFET controlled by PB9

#### USB Connection (GP-5 ↔ STM32)
1. Connect GP-5 USB-C port to STM32 USB HOST
   - GP-5 USB DP (B6 or A6) → STM32 PA12 (Pin 23)
   - GP-5 USB DN (B7 or A7) → STM32 PA11 (Pin 22)
   - **Note**: Type-C orientation matters if only one set of D+/D- is connected!

#### Debug (ST-Link ↔ STM32)
1. Connect SWDIO (ST-Link) → PA13 (STM32 Pin 26)
2. Connect SWCLK (ST-Link) → PA14 (STM32 Pin 27)
3. Connect GND
4. UART2 (PA2/PA3) automatically available as ST-Link VCP

#### Buttons (optional for initial test)
- PC4, PC5, PB3, PB4, PB5: Preset buttons (pull-up resistors)
- PC6: CTL button
- PC7: Tap Tempo button

---

## First-Time Setup

### Step 1: Clock Configuration

**Option A: External 8MHz Oscillator (Recommended)**
- Connect 8MHz oscillator to PC14 (Pin 2)
- Code uses HSE_BYPASS mode
- Provides stable 48MHz for USB via PLL

**Option B: Internal HSI48 (No external crystal needed)**
- Modify `SystemClock_Config()` in `main.c`
- Change `RCC_OSCILLATORTYPE_HSE` to `RCC_OSCILLATORTYPE_HSI48`
- Less accurate but works for USB

### Step 2: Verify USB Fixes

**CRITICAL**: Ensure these fixes are in your code (already present in repository):

1. **File: `USB_Host/Target/usbh_conf.c`** - `USBH_LL_Init()`:
   ```c
   hhcd_USB_DRD_FS.Init.Sof_enable = ENABLE;  // Must be ENABLE!
   ```

2. **File: `USB_Host/Target/usbh_conf.c`** - `USBH_LL_Start()`:
   ```c
   /* Manual USB initialization with USB_CNTR_SOFM bit */
   hhcd->Instance->CNTR = USB_CNTR_HOST | USB_CNTR_CTRM | 
                          USB_CNTR_WKUPM | USB_CNTR_SUSPM | 
                          USB_CNTR_SOFM;  // SOF enable is CRITICAL!
   ```

Without these, USB HOST will not enumerate devices!

---

## Firmware Installation

### Build and Flash

#### Method 1: VS Code CMake Tools
1. Open project in VS Code
2. Select "Debug" preset
3. Press F7 to build
4. Run task: "Flash and Run"

#### Method 2: Command Line
```powershell
# Build
cmake --build C:\path\to\GP5_STM_V1\build\Debug

# Flash
STM32_Programmer_CLI -c port=SWD -w build/Debug/GP5_STM_V1.elf -v -rst
```

### Verify Successful Flash

Open serial monitor (115200 baud) and look for:
```
USB Power Enabled
[MX_USB_Host_Init] USB HOST initialized and started
=== STM32 USB MIDI Controller ===
System Ready - Waiting for USB MIDI device...
```

---

## Testing USB Connection

### Connection Sequence

**Recommended Order**:
1. Power ON the GP-5
2. Power ON the STM32 controller
3. Connect USB cable between them
4. Wait 2-3 seconds for enumeration

**Success Messages** (via serial monitor):
```
[HCD] USB Port enabled
USB Device Detected - Starting Enumeration
USB: MIDI Class Selected
USB MIDI Device Connected - VID: 0x84EF, PID: 0x0184
MIDI Device Connected - Ready for operation
[GP-5] Received Preset: 24
```

### Troubleshooting Connection Issues

**Problem**: No enumeration, stuck at "Waiting for USB MIDI device"

**Solutions**:
1. **Check cable orientation**: Try flipping USB-C cable 180°
2. **Verify VBUS**: Measure 5V on GP-5 USB port
3. **Check HSE clock**: Serial should show system running at 48MHz
4. **Verify USB_CNTR_SOFM**: Check that SOFM bit is set in CNTR register
5. **Wait longer**: Enumeration can take up to 5 seconds

**Problem**: FNR register = 0 (no SOF packets)

**Solution**: This means USB_CNTR_SOFM bit is NOT set. Review USB fixes in code.

---

## Preset Management Quick Reference

### Save a Preset
1. Set GP-5 to desired preset (using GP-5 controls)
2. **Press a Bank button** to select which slot to save to
3. **Hold Tap Tempo** for 2-5 seconds
4. Release Tap Tempo
5. Serial shows: `[BTN] Tap Tempo SAVE (2s): Requesting current GP-5 preset...`
6. Serial shows: `[BTN] Preset saved successfully`

### Recall a Preset  
1. **Press a Bank button** briefly
2. GP-5 instantly switches to saved preset from that slot

### Clear a Preset
1. **Press the Bank button** of the slot you want to clear
2. **Hold Tap Tempo** for more than 5 seconds
3. Release Tap Tempo
4. Serial shows: `[BTN] Tap Tempo CLEAR (5s): Bank X, Button Y`
5. Serial shows: `[BTN] Preset cleared successfully`

### CTL Function
- **Press CTL button** → Sends CC#69 to GP-5
- GP-5 toggles tuner or designated function

---

## LED Indicators (if implemented)

### USB Connection LED
- **OFF**: No USB device
- **ON**: GP-5 connected and ready

### Button LEDs (if implemented)
- **Lit**: Preset saved to this button
- **Dim**: Empty slot

---

## Serial Monitor Debug

### Connect to Serial Port
1. Open serial terminal (PuTTY, Tera Term, Arduino Serial Monitor)
2. Settings: **115200 baud, 8N1, no flow control**
3. Find ST-Link VCP COM port (check Device Manager on Windows)

### Normal Operation Messages
```
USB MIDI Device Connected - VID: 0x84EF, PID: 0x0184
[GP-5] Received Preset: 24
[MIDI_Manager] Sending CC: Ch0 CC#0 = 24
[PresetButtons] Recalling stored preset: GP-5 #24
[BTN] Preset saved successfully
```

### Error Messages
```
[MIDI_Manager] ERROR: Cannot send CC - device not connected
[HCD] *** FILTERED DISCONNECT during boot
[GP-5] *** ERROR: Cannot request preset - no MIDI device connected ***
```

---

## Next Steps

- **User Guide**: See [USER_OPERATIONS_GUIDE.md](USER_OPERATIONS_GUIDE.md) for detailed button operations
- **USB Technical**: See [USB_MIDI_HOST_CONFIGURATION_GUIDE.md](USB_MIDI_HOST_CONFIGURATION_GUIDE.md) for USB HOST implementation details
- **Hardware**: See [HARDWARE_PORT_GUIDE.md](HARDWARE_PORT_GUIDE.md) for complete pinout

---

## Common Issues

### USB Enumeration Fails
- Verify USB_CNTR_SOFM bit is set (see USB fixes above)
- Check clock source: Should be PLLQ at 48MHz or HSI48
- Try different USB cable
- Power cycle both devices

### Preset Not Saving
- Check Flash write success in serial monitor
- Verify button debounce (50ms)
- Ensure Tap Tempo held 2-5 seconds (not <2 or >5)

### No Serial Output
- Check ST-Link connection
- Verify COM port and baud rate (115200)
- Install ST-Link VCP drivers if needed

---

*Last Updated: February 6, 2026*
*For STM32G0B1KBU6 with critical USB HOST fixes*
 
  - Short press: Recall preset defaults (CC#0)
  - Long/Extra-long press: No effect (Scene 1 cannot be programmed)
  
- **btnScene2** (PC2):
  - Short press: Recall Scene 2
  - Long press (2s): Save Scene 2
  - Extra-long press (5s): Delete Scene 2
  
- **btnScene3** (PC3):
  - Short press: Recall Scene 3
  - Long press (2s): Save Scene 3
  - Extra-long press (5s): Delete Scene 3

### Future
- **btnTap** (PC6): Reserved for tap tempo

---

## LED Indicators

- **Scene 1 LED (PA0) ON**: Preset defaults active or GP-5 connected
- **Scene 2 LED (PA1) ON**: Scene 2 active
- **Scene 3 LED (PA4) ON**: Scene 3 active
- **LED Blinking (slow)**: Saving scene
- **LED Blinking (fast)**: Deleting scene

Only one scene LED is ON at a time.

---

## Typical Workflow

### Setting Up Multiple Sounds Per Preset

**Example: Preset 33 - Three Variations**

1. Load Preset 33 on GP-5
2. **Scene 1**: Keep defaults (lead tone)
3. **Scene 2**: Turn off distortion, save as rhythm
   - Turn off DST patch in GP-5 app
   - Long press Scene 2 → Saved
4. **Scene 3**: Turn off everything except delay, save as ambient
   - Turn off all except DLY
   - Long press Scene 3 → Saved

**Result**: One preset, three instantly accessible sounds!

### Live Performance

1. **Navigate presets**: btnUp/btnDown
2. **Scene 1 always ready**: Default sound
3. **Switch to variation**: Short press Scene 2 or 3
4. **Quick reset**: Short press Scene 1 returns to defaults

---

## Technical Details

### What Gets Saved
- ✅ Patch ON/OFF states (10 effects)
- ❌ Effect parameters (controlled by GP-5 app)

### Storage
- **Flash**: 1000 bytes (100 presets × 10 bytes)
- **Per Scene**: 5 bytes (1 flag + 4 byte bitmap)
- **Persistence**: Survives power cycles

### Supported Patches
1. NR - Noise Reduction
2. PRE - Preamp
3. DST - Distortion
4. NS - Neural Amp
5. AMP - Amplifier
6. CAB - Cabinet
7. EQ - Equalizer
8. MOD - Modulation
9. DLY - Delay
10. RVB - Reverb

---

## Troubleshooting

**GP-5 won't connect**
- Try hot-plug method (connect after STM32 boots)
- System auto-resets after 3 disconnect attempts
- Check Scene 1 LED - should turn ON when connected

**Scene won't recall**
- Check if scene is programmed (LED should be OFF if empty)
- Try saving the scene again with long press

**Button doesn't respond**
- Ensure only one button pressed at a time
- Wait for debounce period (50ms)
- Check that rapid presses don't trigger long press

**Scene deleted accidentally**
- No undo - re-program the scene
- System returns to Scene 1 after delete

**LED stays off after preset change**
- This is normal - Scene 1 LED only indicates connection status
- Scene LEDs show active scene only

---

## Debug Output (Optional)

Connect serial terminal (115200 baud, ST-Link Virtual COM) to view:
- Connection: `[GP-5] Preset X`
- Scene operations: `[Scene] Saving/Deleting Scene X`
- Verbose logging: Set `DEBUG_VERBOSE 1` in gp5_midi.c

**Serial Output Examples**:
```
[GP-5] Preset 33
[Scene] Saving Scene 2 for preset 33
[Scene] Scene 2 saved
```

---

## Testing Sequence

```
1. Power on STM32 → Wait for boot
2. Connect GP-5 → See Scene 1 LED turn ON
3. Press btnUp → Preset increments, Scene 1 LED stays ON
4. Press Scene 1 → GP-5 reloads preset defaults
5. Modify patches in GP-5 app → Change some effects
6. Long press Scene 2 (2s) → LED blinks, then solid
7. Modify patches again
8. Short press Scene 2 → Patches return to saved state
9. Extra-long press Scene 2 (5s) → LED blinks fast, scene deleted
10. Scene 1 LED turns ON (system returns to defaults)
11. Power cycle → Scenes persist!
```

---

## Pin Configuration

| Function | Pin | GPIO Mode |
|----------|-----|-----------|
| btnUp | PC4 | INPUT_PULLUP + EXTI |
| btnDown | PC5 | INPUT_PULLUP + EXTI |
| btnScene1 | PC1 | INPUT_PULLUP + EXTI |
| btnScene2 | PC2 | INPUT_PULLUP + EXTI |
| btnScene3 | PC3 | INPUT_PULLUP + EXTI |
| btnTap | PC6 | INPUT_PULLUP + EXTI |
| ledScene1 | PA0 | OUTPUT |
| ledScene2 | PA1 | OUTPUT |
| ledScene3 | PA4 | OUTPUT |
| USART2_TX | PA2 | Debug output |
| USB_HOST | PA11/PA12 | USB DM/DP |
| USB_PWR | PC0 | USB power control |

---

## Success Criteria

You've successfully set up the controller when:
- ✅ Scene 1 LED turns ON when GP-5 connects
- ✅ btnUp/btnDown change presets
- ✅ Scene 1 short press reloads preset defaults
- ✅ Scene 2/3 can be saved with long press
- ✅ Scene 2/3 can be recalled with short press
- ✅ Scene 2/3 persist after power cycle
- ✅ Scene 2/3 can be deleted with extra-long press

---

## Next Steps

- Experiment with different scene combinations per preset
- Organize presets by song or playing style
- Use Scene 1 as instant "safety net" to return to defaults
- Program frequently-used variations to Scene 2 & 3

---

**System Ready for Live Performance!** 🎸

For detailed technical information, see:
- [GP5_OPERATIONAL_SPECIFICATION.md](GP5_OPERATIONAL_SPECIFICATION.md) - Complete system design
- [IMPLEMENTATION_NOTES.md](IMPLEMENTATION_NOTES.md) - Technical architecture
- [README.md](README.md) - Project overview
