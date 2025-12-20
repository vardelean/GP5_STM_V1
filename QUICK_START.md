# Quick Start Guide - GP-5 MIDI Controller

## Hardware Setup

### Required Components
- **NUCLEO-G0B1RE** evaluation board
- **Valeton GP-5** guitar effects pedal  
- USB cable to connect GP-5 to STM32
- ST-Link USB cable for programming

### Required Software
- CMake 3.22+
- ARM GCC Toolchain (arm-none-eabi-gcc)
- STM32CubeProgrammer
- Serial terminal (optional for debug)

### Connections
1. Connect ST-Link cable to PC for programming
2. Connect GP-5 to STM32 USB HOST port
3. Power on the system

---

## Firmware Installation

### Build and Flash

```powershell
# One-command build and flash
cmake --build build --preset Debug
STM32_Programmer_CLI -c port=SWD -w build/Debug/GP5_STM_V1.elf -rst
```

### Connection Methods

**Method 1: GP-5 Already Connected**
- Connect GP-5 → Power STM32 → Instant connection

**Method 2: Hot-Plug**  
- Power STM32 → Connect GP-5 → Auto-reset after ~3s → Connected

**Success Indicator**: Scene 1 LED turns ON

---

## Scene Management Guide

### Scene 1 - Preset Defaults (Built-In)

**What It Does**: Instantly reloads GP-5 preset defaults

**How To Use**:
1. Select any preset (using GP-5 app, knob, or btnUp/btnDown)
2. Modify effects (turn patches on/off)
3. **Short press Scene 1** button
4. GP-5 returns to preset defaults

**Technical**: Sends MIDI CC#0 with current preset number  
**LED**: Always ON (cannot be saved or deleted)

---

### Scene 2 & 3 - User Programmable

#### Save a Scene

1. **Select** preset on GP-5
2. **Configure** patches (effects on/off) as desired
3. **Long press** (2 seconds) Scene 2 or 3
4. **LED blinks** to confirm detection
5. **Release** → Scene saved to flash
6. **LED solid** to confirm save

**Example**: Save a "clean" version of Preset 33 to Scene 2

#### Recall a Scene

1. **Short press** Scene 2 or 3
2. If programmed → Patches switch instantly
3. LED shows active scene

**Fast Switching**: Only changes patches that differ from current state

#### Delete a Scene

1. **Extra long press** (5 seconds) Scene 2 or 3
2. **LED blinks fast** when threshold reached
3. **Release** → Scene deleted
4. System returns to Scene 1 (defaults)

---

## Button Reference

### Preset Navigation
- **btnUp** (PC4): Next preset (CC#25)
- **btnDown** (PC5): Previous preset (CC#24)

### Scene Control  
- **btnScene1** (PC1): 
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
