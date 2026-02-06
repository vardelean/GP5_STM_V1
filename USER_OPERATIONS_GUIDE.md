# GP-5 STM32 MIDI Controller - User Operations Guide

## Table of Contents
1. [Overview](#overview)
2. [Power-Up and Startup](#power-up-and-startup)
3. [Button Layout](#button-layout)
4. [Preset Navigation](#preset-navigation)
5. [Saving Presets](#saving-presets)
6. [Clearing Presets](#clearing-presets)
7. [CTL Button Operation](#ctl-button-operation)
8. [Tap Tempo Button](#tap-tempo-button)
9. [LED Indicators](#led-indicators)
10. [Troubleshooting](#troubleshooting)

---

## Overview

The GP-5 STM32 MIDI Controller provides an external control interface for the Valeton GP-5 guitar effects pedal. It connects via USB and allows you to:
- Navigate through GP-5 presets
- Save your favorite GP-5 presets to 80 quick-access buttons (Banks 1-16, Buttons 1-5)
- Recall saved presets instantly
- Send control change messages (CTL, Tap Tempo)
- Automatic recall of last-used preset on power-up

**Connection**: Connect the STM32 controller to the GP-5's USB HOST port using a USB-C cable.

---

## Power-Up and Startup

### Initial Power-On Sequence

1. **Connect GP-5 First**: Ensure the GP-5 is powered on
2. **Connect USB Cable**: Connect USB-C cable between STM32 controller and GP-5
3. **Power STM32**: Apply power to the STM32 controller

### Automatic Startup Behavior

When the controller powers up, it automatically:

1. **Initializes USB HOST** - Waits for GP-5 connection
2. **Detects GP-5** - Identifies the Valeton GP-5 (VID: 0x84EF, PID: 0x0184)
3. **Queries Current Preset** - Asks GP-5 what preset is currently active
4. **Recalls Last Preset** - If a preset was saved to a button, it sends that preset to the GP-5

**Example**: If you saved GP-5 preset #24 to Bank 2, Button 5, the controller will automatically send CC#0=24 to the GP-5 on startup, switching the GP-5 to preset #24.

### Status Messages (via Serial Monitor)

```
USB Power Enabled
[MX_USB_Host_Init] USB HOST initialized and started
=== STM32 USB MIDI Controller ===
[Flash] Storage system ready
[PresetButtons] Loaded saved preset: Bank 2, Button 5 (Index 9)
System Ready - Waiting for USB MIDI device...
USB Device Detected - Starting Enumeration
USB MIDI Device Connected - VID: 0x84EF, PID: 0x0184
[GP-5] Received Preset: 24
[PresetButtons] Recalling stored preset: GP-5 #24
[MIDI_Manager] Sending CC: Ch0 CC#0 = 24
```

---

## Button Layout

The controller has **5 preset buttons** with **16 banks** accessible via bank up/down buttons, plus CTL and Tap Tempo:

### Preset Buttons (1-5) - 16 Banks Available
- **Button 1** - Save/recall preset slot (Bank × 5 + 0)
- **Button 2** - Save/recall preset slot (Bank × 5 + 1)
- **Button 3** - Save/recall preset slot (Bank × 5 + 2)
- **Button 4** - Save/recall preset slot (Bank × 5 + 3)
- **Button 5** - Save/recall preset slot (Bank × 5 + 4)

**Examples**:
- Bank 1, Button 1 = Preset slot 0
- Bank 1, Button 5 = Preset slot 4
- Bank 2, Button 1 = Preset slot 5
- Bank 16, Button 5 = Preset slot 79 (last slot)

### Navigation Buttons
- **Bank Up** - Cycle to next bank (1→2→3...→16→1)
- **Bank Down** - Cycle to previous bank (1→16→15...→2→1)

### Control Buttons
- **CTL** - Send CC#69 (GP-5 CTL function toggle)
- **Tap Tempo** - Special functions (save/clear)

**GPIO Pin Mapping**:
- Preset Buttons 1-5: PC4, PC5, PB3, PB4, PB5
- Bank Up: PB6 (or alternate GPIO)
- Bank Down: PB7 (or alternate GPIO)
- CTL: PC6
- Tap Tempo: PC7

---

## Preset Navigation

### Quick Preset Recall

**Short Press (< 2 seconds)** on any Preset button:
- Instantly sends the saved GP-5 preset number to the GP-5
- If no preset is saved to that button, the controller queries the GP-5 for its current preset
- Use **Bank Up/Down** to navigate between banks 1-16

**Example**:
- Navigate to Bank 8 using Bank Up
- Saved GP-5 preset #42 to Bank 8, Button 3 (slot 38)
- Press Preset Button 3 briefly
- Controller sends CC#0=42 to GP-5
- GP-5 switches to preset #42

### First-Time Button Press

If you press a button that has **no saved preset**:
- Controller queries GP-5: "What preset are you currently on?"
- Controller receives the answer (e.g., preset #15)
- Controller displays: `[PresetButtons] No preset stored, requesting current GP-5 preset`
- You can now save this preset to the button (see [Saving Presets](#saving-presets))

---

## Saving Presets

### How to Save a Preset

1. **Set GP-5 to desired preset** - Use the GP-5's own controls to select the preset you want
2. **Press the Bank button** where you want to save (this selects the slot)
3. **Hold Tap Tempo button for 2-5 seconds** - Controller queries GP-5 and saves to selected slot
4. **Release Tap Tempo**
5. **Confirmation**: Serial monitor shows:
   ```
   [BTN] Tap Tempo SAVE (2s): Requesting current GP-5 preset...
   [GP-5] Received Preset: 24
   [BTN] Preset saved successfully
   ```

### Status Messages

```
[BTN] Tap Tempo SAVE (2s): Requesting current GP-5 preset...
[GP-5] Received Preset: 24
[BTN] SAVING GP-5 preset 24 to STM32 Bank 2, Button 5
[Flash] Saved: STM32[9] = GP-5[24]
[BTN] Preset saved successfully
```

### Saved Preset Storage

- Presets are stored in **Flash memory** (non-volatile)
- Survives power cycles
- 80 preset slots available (16 banks × 5 buttons)
- Each slot stores: GP-5 preset number (0-99)

---

## Clearing Presets

### How to Clear a Saved Preset

1. **Press the Bank button** of the slot you want to clear (this selects the slot)
2. **Hold Tap Tempo button for more than 5 seconds** - Clear mode activates
3. **Release Tap Tempo**
4. **Confirmation**: Serial monitor shows:
   ```
   [BTN] Tap Tempo CLEAR (5s): Bank X, Button Y
   [BTN] Preset cleared successfully
   ```

### Status Messages

```
[BTN] Tap Tempo CLEAR (>5s): Clear mode activated
[BTN] CLEARING preset from STM32 Bank 1, Button 2
[Flash] Cleared: STM32[1]
[BTN] Preset cleared successfully
```

### What Happens After Clearing

- Button slot is now empty
- Next press will query GP-5 for current preset
- Flash memory is updated immediately

---

## CTL Button Operation

The **CTL button** sends CC#69 to the GP-5, which toggles the GP-5's CTL function (typically an on-screen tuner or effects toggle).

### Usage

**Short Press (< 2 seconds)**:
- Sends CC#69 value=127 to GP-5
- GP-5 toggles its CTL function

**Hold (2+ seconds)**:
- Reserved for future tuner toggle feature
- Currently sends single CC#69

### Status Messages

```
[BTN] CTL pressed - hold for 2s to toggle tuner
[BTN] CTL (quick press): Sending CC#69 (CTL screen)
[MIDI_Manager] Sending CC: Ch0 CC#69 = 127
```

### GP-5 Response

The GP-5 typically:
- Shows/hides the tuner screen
- Or toggles a designated effect on/off (depending on GP-5 firmware)

---

## Tap Tempo Button

The **Tap Tempo button** has **three functions** depending on hold duration:

### Quick Press (< 2 seconds)
- **Reserved** for future tap tempo MIDI functionality
- Currently inactive for tapping

### Save Mode (2-5 seconds hold)
1. Hold Tap Tempo for 2-5 seconds
2. Controller queries GP-5 for current preset
3. Press a Bank button to save that preset
4. Release both buttons

**Use Case**: "Save the current GP-5 preset to button X"

### Clear Mode (>5 seconds hold)
1. Hold Tap Tempo for more than 5 seconds
2. Press a Bank button to clear its saved preset
3. Release both buttons

**Use Case**: "Remove the saved preset from button X"

### Status Messages

```
[BTN] Tap Tempo pressed - hold for save (2s) or clear (5s)
[BTN] Tap Tempo released after 3421ms
[BTN] Tap Tempo SAVE (2s): Requesting current GP-5 preset...
```

---

## LED Indicators

The controller has LEDs to indicate system status:

### USB Connection LED
- **OFF**: No USB device detected
- **ON**: GP-5 connected and ready

### Bank LEDs (if implemented)
- Indicate which bank is currently active
- Only one LED on at a time

### Button LEDs (if implemented)
- Light up when a button has a saved preset
- Dim when button slot is empty

**Note**: LED configuration depends on hardware implementation. Check your specific board schematic.

---

## Troubleshooting

### GP-5 Not Detected

**Symptoms**:
- Serial monitor shows: `System Ready - Waiting for USB MIDI device...`
- No enumeration messages

**Solutions**:
1. Check USB cable connection (both ends)
2. Ensure GP-5 is powered on BEFORE connecting USB
3. Try flipping USB-C cable 180° (orientation matters with some cables)
4. Verify VBUS power (should be ~5V on GP-5 USB port)
5. Wait 10-15 seconds for enumeration to complete

### Preset Not Recalling

**Symptoms**:
- Press button, but GP-5 stays on same preset
- Serial monitor shows: `[MIDI_Manager] ERROR: Cannot send CC - device not connected`

**Solutions**:
1. Check USB connection (see above)
2. Verify button has a saved preset: `[PresetButtons] Loaded saved preset: Bank X, Button Y`
3. If no preset saved, save one using Tap Tempo + Button

### Button Not Responding

**Symptoms**:
- Press button, nothing happens
- No serial monitor messages

**Solutions**:
1. Check button connection (GPIO pins)
2. Verify pull-up resistors are enabled (should be in code)
3. Test button with multimeter (should show continuity when pressed)
4. Check for button debounce issues (50ms debounce implemented)

### Wrong Preset Recalled

**Symptoms**:
- Press button, GP-5 switches to unexpected preset

**Solutions**:
1. Clear the button: Hold Tap Tempo >5s, press button
2. Re-save correct preset: Hold Tap Tempo 2-5s, press button
3. Verify Flash storage: Serial monitor shows `[Flash] Saved: STM32[X] = GP-5[Y]`

### Serial Monitor Not Working

**Symptoms**:
- No messages in serial monitor

**Solutions**:
1. Check ST-Link USB connection to PC
2. Verify COM port settings: 115200 baud, 8N1
3. Install ST-Link VCP drivers if needed
4. Open correct COM port (check Device Manager on Windows)

### USB Enumeration Fails

**Symptoms**:
- Serial shows: `[MIDI_Manager] USB gState: 13 (HOST_CLASS=11), device_connected: 0`
- Or: `[HCD] *** FILTERED DISCONNECT during boot`

**Solutions**:
1. This is normal during first 1 second after boot (disconnect filtering)
2. Wait for enumeration to complete (~2-3 seconds)
3. If persistent, check USB clock source: `[Clock] USB clock source: PLLQ` (should show PLLQ or HSI48)
4. Verify USB power: `USB Power Enabled` message should appear

### Flash Memory Corruption

**Symptoms**:
- All saved presets lost
- Random behavior on button presses

**Solutions**:
1. Clear all presets: Hold Tap Tempo >5s, press each button
2. Re-flash firmware via ST-Link
3. If persistent, may need to erase Flash Page 254 manually

---

## Advanced Features

### Serial Monitor Commands (Debug)

If DEBUG_VERBOSE is enabled in firmware, you can monitor:
- USB enumeration details
- MIDI message contents
- Flash read/write operations
- Button state changes

Connect via ST-Link VCP at 115200 baud.

### Preset Mapping

STM32 button indices map to Flash storage:
- Bank 1, Button 1 = Index 0
- Bank 1, Button 2 = Index 1
- ...
- Bank 2, Button 1 = Index 5
- ...
- Bank 16, Button 5 = Index 79 (last slot)

Each index stores 1 byte (GP-5 preset number 0-99).

### GP-5 Preset Range

The GP-5 has 100 user presets (0-99):
- Factory presets: Typically 0-49
- User presets: Typically 50-99 (varies by GP-5 firmware)

The controller can save and recall any of the 100 presets.

---

## Firmware Updates

To update the controller firmware:

1. **Connect ST-Link** to STM32 debug pins (SWDIO, SWCLK)
2. **Build firmware** in VS Code with CMake
3. **Flash via STM32CubeProgrammer** or ST-Link utility
4. **Reset board** - firmware automatically runs

**Repository**: Check GitHub for latest firmware releases

---

## Specifications

- **MCU**: STM32G0B1KBU6 (48MHz, 128KB Flash, 144KB RAM)
- **USB**: Full-Speed USB 2.0 HOST (12 Mbps)
- **MIDI**: USB MIDI Class 1.0 compliant
- **Storage**: 80 preset slots in Flash (non-volatile)
- **Latency**: < 5ms button-to-MIDI transmission
- **Power**: 5V via USB or external supply
- **Current Draw**: ~50mA typical, 150mA max (with GP-5 connected)

---

## Support and Documentation

For more information:
- **README.md** - Project overview and features
- **QUICK_START.md** - Hardware setup and first-time configuration
- **USB_MIDI_HOST_CONFIGURATION_GUIDE.md** - Technical USB HOST implementation
- **HARDWARE_PORT_GUIDE.md** - Pin assignments and connections

**Repository**: [github.com/vardelean/GP-5_STM32_Eval](https://github.com/vardelean/GP-5_STM32_Eval)

---

*Last Updated: February 6, 2026*
*Firmware Version: v1.0 - Production Ready*
