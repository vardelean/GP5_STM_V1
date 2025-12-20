# GP-5 MIDI Controller - Operational Specification

## Document Purpose

This document defines the complete operational behavior of the STM32 USB MIDI Controller for the Valeton GP-5 guitar effects pedal. This specification must be reviewed and approved before implementation begins.

---

## Table of Contents

1. [System Overview](#system-overview)
2. [GP-5 Pedal Architecture](#gp-5-pedal-architecture)
3. [MIDI Communication Protocol](#midi-communication-protocol)
4. [Scene Management System](#scene-management-system)
5. [Button Behavior Specification](#button-behavior-specification)
6. [Flash Memory Organization](#flash-memory-organization)
7. [State Machine Design](#state-machine-design)
8. [User Interface Rules](#user-interface-rules)
9. [Implementation Phases](#implementation-phases)

---

## System Overview

### Purpose
The MIDI Controller provides tactile, physical control over the GP-5 pedal, allowing musicians to:
- Change presets using btnUp/btnDown
- Save and recall up to 3 custom patch configurations (Scenes) per preset
- Switch between Scenes during live performance
- Tap tempo control (future implementation)

### Key Design Principles
1. **Always synchronized**: Controller must track GP-5 state regardless of how it was changed
2. **Non-destructive**: Scene changes only affect patch on/off states, not parameters
3. **Fail-safe**: Unprogrammed scenes do nothing (no LED, no commands sent)
4. **Immediate feedback**: LED indicators show current scene at all times
5. **Flash-backed**: Scene configurations persist across power cycles

---

## GP-5 Pedal Architecture

### Preset Structure

**Total Capacity**: 100 Presets (PST 00 through PST 99)

**Each Preset Contains**:
- 10 effect patches (blocks)
- Each patch can be ON or OFF
- Each patch has parameters (not controlled by this application)

### The 10 Patches

| Patch Code | Name | Display Label | Description |
|------------|------|---------------|-------------|
| `patchCAB` | Cabinet | CAB | Cabinet simulator |
| `patchEQ`  | Equalizer | EQ | Tone equalization |
| `patchMOD` | Modulation | MOD | Chorus/Flanger/Phaser |
| `patchDLY` | Delay | DLY | Echo/Delay effect |
| `patchNR`  | Noise Reduction | NR | Noise gate |
| `patchPRE` | Preamp | PRE | Preamp stage |
| `patchDST` | Distortion | DST | Distortion/Overdrive |
| `patchAMP` | Amplifier | AMP | Amplifier simulator |
| `patchRVB` | Reverb | RVB | Reverb effect |
| `patchNS`  | Neural Amp | N→S | Neural amplifier |

### User Interaction Methods

The GP-5 can be controlled via three independent interfaces:
1. **Bluetooth App** (smartphone)
2. **Hardware Preset Selector** (rotary switch on GP-5)
3. **USB MIDI** (our controller)

**Critical Constraint**: Since all three can change the GP-5 state at any time, the controller must:
- Monitor for preset changes from ANY source
- Always retrieve current state before making changes
- Never assume state based on local history

---

## MIDI Communication Protocol

### Message Types Summary

| Purpose | Direction | Length | Identifier |
|---------|-----------|--------|------------|
| Preset Change ACK | GP-5 → Controller | 22 bytes | SysEx |
| Request Preset Number | Controller → GP-5 | 14 bytes | SysEx |
| Preset Number Response | GP-5 → Controller | 18 bytes | SysEx |
| Request Patch Info | Controller → GP-5 | 14 bytes | SysEx |
| Patch Info Response (25 msgs) | GP-5 → Controller | 24×48B + 1×24B | SysEx |
| Preset Up | Controller → GP-5 | 3 bytes | CC#25 |
| Preset Down | Controller → GP-5 | 3 bytes | CC#24 |
| Patch On/Off | Controller → GP-5 | 3 bytes | CC#48-57 |

### Detailed Message Specifications

#### 1. Preset Change ACK (GP-5 → Controller)

**Length**: 22 bytes  
**Format**: `F0 0E 02 00 01 00 00 00 06 01 02 01 0B 00 01 00 00 00 00 00 00 F7`

**When Sent**: 
- After any preset change (from Bluetooth, hardware, or USB)
- Confirms preset has changed, but does NOT indicate which preset

**Recognition Logic**:
```c
if (len == 22 && 
    data[3] == 0x00 && data[4] == 0x01 && 
    data[8] == 0x06 && data[9] == 0x01 && 
    data[10] == 0x02 && data[11] == 0x01 && 
    data[12] == 0x0B)
{
  // Preset change acknowledged
  // Must now request preset number
}
```

**Controller Response**: Immediately send "Request Preset Number"

---

#### 2. Request Preset Number (Controller → GP-5)

**Length**: 14 bytes  
**Format**: `F0 00 07 00 01 00 00 00 02 01 02 04 03 F7`

**Array Definition**:
```c
uint8_t reqPstNum[] = {0xF0, 0x00, 0x07, 0x00, 0x01, 0x00, 0x00, 0x00, 
                       0x02, 0x01, 0x02, 0x04, 0x03, 0xF7};
```

**When Sent**:
- After receiving Preset Change ACK
- On system startup
- Before any scene operation

---

#### 3. Preset Number Response (GP-5 → Controller)

**Length**: 18 bytes  
**Format**: `F0 0E 0A 00 01 00 00 00 04 01 02 04 03 [PST_H] [PST_L] 00 00 F7`

**Preset Number Extraction**:
```c
if (len == 18 && 
    data[3] == 0x00 && data[4] == 0x01 && 
    data[8] == 0x04 && data[9] == 0x01 && 
    data[10] == 0x02 && data[11] == 0x04 && 
    data[15] == 0x00)
{
  *pst = (data[13] << 4) | data[14];
  // pst now contains preset number (0-99)
}
```

**Preset Encoding**: Preset number split into high nibble (data[13]) and low nibble (data[14])
- Example: PST 25 → data[13]=0x02, data[14]=0x05

---

#### 4. Request Patch Info (Controller → GP-5)

**Length**: 14 bytes  
**Format**: `F0 00 0E 00 01 00 00 00 02 01 02 04 00 F7`

**Array Definition**:
```c
uint8_t reqPatchInfo[] = {0xF0, 0x00, 0x09, 0x00, 0x01, 0x00, 0x00, 0x00, 
                          0x02, 0x01, 0x02, 0x04, 0x01, 0xF7};
```

**When Sent**:
- Before saving a scene (to capture current patch states)
- Before applying a scene (to compare with saved states)

---

#### 5. Patch Info Response (GP-5 → Controller)

**Response Structure**: 25 SysEx messages
- First 24 messages: 48 bytes each
- Last message: 24 bytes

**Message of Interest**: 4th message (index 3, counting from 0)

**Recognition Logic**:
```c
if (len == 48 && 
    data[3] == 0x01 && data[4] == 0x09 && 
    data[5] == 0x00 && data[6] == 0x03)
{
  // This is the 4th message containing patch status
  // Bytes 35-38 contain the patch on/off bitmap
  *effStatus = ((uint32_t)data[38] << 24) | 
               ((uint32_t)data[37] << 16) | 
               ((uint32_t)data[36] << 8) | 
               ((uint32_t)data[35]);
}
```

**Patch Status Bitmap** (32-bit):
```
Byte 35 (LSB): [7:4]=unused [3]=DLY [2]=MOD [1]=EQ [0]=CAB
Byte 36:       [3]=AMP [2]=DST [1]=PRE [0]=NR
Byte 37:       [7:2]=unused [1]=NS [0]=RVB
Byte 38 (MSB): unused
```

**Decoding Function**:
```c
void decodePatchEffects(uint32_t effStatus, PatchInfo *pInfo) {
  pInfo->patchCAB = (effStatus      ) & 0x01;  // Bit 0
  pInfo->patchEQ  = (effStatus >>  1) & 0x01;  // Bit 1
  pInfo->patchMOD = (effStatus >>  2) & 0x01;  // Bit 2
  pInfo->patchDLY = (effStatus >>  3) & 0x01;  // Bit 3
  pInfo->patchNR  = (effStatus >>  8) & 0x01;  // Bit 8
  pInfo->patchPRE = (effStatus >>  9) & 0x01;  // Bit 9
  pInfo->patchDST = (effStatus >> 10) & 0x01;  // Bit 10
  pInfo->patchAMP = (effStatus >> 11) & 0x01;  // Bit 11
  pInfo->patchRVB = (effStatus >> 24) & 0x01;  // Bit 24
  pInfo->patchNS  = (effStatus >> 25) & 0x01;  // Bit 25
}
```

---

#### 6. Patch Control via Control Change (Controller → GP-5)

**Format**: Standard MIDI CC message (3 bytes)
- Byte 0: `0xB0` (CC on channel 1)
- Byte 1: CC number (48-57)
- Byte 2: Value (0-63=OFF, 64-127=ON)

**CC Assignments**:

| CC# | Patch | OFF Value | ON Value |
|-----|-------|-----------|----------|
| 48  | NR    | 0-63      | 64-127   |
| 49  | PRE   | 0-63      | 64-127   |
| 50  | DST   | 0-63      | 64-127   |
| 51  | NS    | 0-63      | 64-127   |
| 52  | AMP   | 0-63      | 64-127   |
| 53  | CAB   | 0-63      | 64-127   |
| 54  | EQ    | 0-63      | 64-127   |
| 55  | MOD   | 0-63      | 64-127   |
| 56  | DLY   | 0-63      | 64-127   |
| 57  | RVB   | 0-63      | 64-127   |

**Typical Values**:
- OFF: Use 0
- ON: Use 127

**Important Note**: GP-5 does NOT send ACK for patch on/off commands!

---

#### 7. Preset Up/Down via Control Change (Controller → GP-5)

**Preset Up (CC#25)**:
```c
MIDI_Manager_SendCC(0, 25, 127);  // Channel 0, CC#25, Value 127
```

**Preset Down (CC#24)**:
```c
MIDI_Manager_SendCC(0, 24, 127);  // Channel 0, CC#24, Value 127
```

**GP-5 Response**: Preset Change ACK (22 bytes)

---

## Scene Management System

### Scene Concept

**Definition**: A Scene is a saved snapshot of which patches are ON or OFF for a specific preset.

**Scope**: 
- 3 Scenes per preset
- 100 presets total
- 300 total scenes

**Scene Numbering**:
- Scene 1: **Always available** - Recalls preset defaults (not user-programmable, sends CC#0)
- Scene 2: User-programmable patch combination
- Scene 3: User-programmable patch combination

### Scene Storage Structure

**Per-Scene Data** (5 bytes):
```c
typedef struct {
  uint8_t programmed;      // 1=Scene is programmed, 0=Empty
  uint8_t patchStatus[4];  // 32-bit bitmap of patch on/off states
} SceneData_t;
```

**Per-Preset Data** (10 bytes):
```c
typedef struct {
  SceneData_t scene2;  // 5 bytes (user-programmable)
  SceneData_t scene3;  // 5 bytes (user-programmable)
  // Scene 1 not stored - always represents preset defaults
} PresetScenes_t;
```

**Total Storage** (1000 bytes):
```c
PresetScenes_t allScenes[100];  // 100 presets × 10 bytes = 1000 bytes
```

### RAM Copy Management

**Startup Behavior**:
```c
// Load entire scene database from flash to RAM
void SceneManager_Init(void) {
  FlashStorage_ReadScenes(allScenes, sizeof(allScenes));
}
```

**Runtime Behavior**:
- All scene lookups use RAM copy (fast)
- Flash writes only when programming/deleting scenes

**Scene Programming**:
```c
// Save scene to RAM and flash
void SceneManager_SaveScene(uint8_t preset, uint8_t sceneNum, uint32_t patchStatus) {
  allScenes[preset].sceneX.patchStatus = patchStatus;
  allScenes[preset].sceneX.programmed = 1;
  
  // Write entire preset to flash
  FlashStorage_WritePresetScenes(preset, &allScenes[preset]);
}
```

---

## Button Behavior Specification

### btnUp and btnDown

**Trigger**: Button press (falling edge interrupt)  
**Action**: Immediate - DO NOT wait for button release

**btnUp Behavior**:
```
1. Send CC#25 (value 127) to GP-5
2. Wait for Preset Change ACK (22 bytes)
3. Send reqPstNum (request preset number)
4. Receive preset number response (18 bytes)
5. Update current preset number
6. Check if Scene1 is programmed for this preset
   - If programmed: Turn ON ledScene1
   - If not programmed: Turn OFF all scene LEDs
7. Turn OFF ledScene2 and ledScene3
8. Done
```

**btnDown Behavior**:
```
1. Send CC#24 (value 127) to GP-5
2. Wait for Preset Change ACK (22 bytes)
3. Send reqPstNum (request preset number)
4. Receive preset number response (18 bytes)
5. Update current preset number
6. Check if Scene1 is programmed for this preset
   - If programmed: Turn ON ledScene1
   - If not programmed: Turn OFF all scene LEDs
7. Turn OFF ledScene2 and ledScene3
8. Done
```

**Critical Notes**:
- Send CC immediately on button press
- Do NOT debounce btnUp/btnDown (user expects instant response)
- Preset number retrieval happens asynchronously after CC is sent

---

### btnScene1, btnScene2, btnScene3

**Three Distinct Behaviors**:
1. **Short Press** (< 1 second): Recall scene
2. **Long Press** (1-5 seconds): Save scene
3. **Extra Long Press** (> 5 seconds): Delete scene

---

#### Short Press: Recall Scene

**Trigger**: Button released before 1 second elapsed  
**Precondition**: Scene must be programmed (check `scene.programmed == 1`)

**Behavior Flow**:
```
1. Check if scene is programmed
   - If NOT programmed: Do nothing, return
   
2. Get current patch status from GP-5:
   - Send reqPatchInfo
   - Wait for 4th response message (48 bytes)
   - Extract current patch bitmap from bytes 35-38
   
3. Compare current patch status with saved scene:
   - For each patch that differs:
     - Send CC command to turn patch ON or OFF
     
4. Update LED indicators:
   - Turn ON LED for selected scene
   - Turn OFF other two scene LEDs
   
5. Done
```

**Example Pseudocode**:
```c
void RecallScene(uint8_t preset, uint8_t sceneNum) {
  // Check if programmed
  if (!allScenes[preset].sceneX.programmed) {
    return;  // Do nothing
  }
  
  // Get current patches from GP-5
  uint32_t currentPatches = GetCurrentPatchStatus();
  
  // Get saved patches from RAM
  uint32_t savedPatches = GetScenePatchStatus(preset, sceneNum);
  
  // Apply changes
  ApplyPatchChanges(currentPatches, savedPatches);
  
  // Update LEDs
  LED_SetScene(sceneNum);
}
```

---

#### Long Press: Save Scene

**Trigger**: Button held for 1+ seconds  
**Visual Feedback**: LED blinks at 250ms intervals while button is held

**Behavior Flow**:
```
1. Detect 1-second threshold crossed
2. Start LED blinking (250ms ON, 250ms OFF)
3. While button still pressed:
   - Continue blinking LED
   
4. On button release:
   - Stop LED blinking
   - Get current patch status from GP-5:
     - Send reqPatchInfo
     - Wait for 4th response message
     - Extract patch bitmap
   
   - Save to RAM and flash:
     - allScenes[currentPreset].sceneX.patchStatus = bitmap (RAM)
     - allScenes[currentPreset].sceneX.programmed = 1 (RAM)
     - FlashStorage_WritePresetScenes(currentPreset, ...) (Flash)
     - CRITICAL: Keep RAM and Flash synchronized
   
   - Turn ON scene LED (solid, not blinking)
   
5. Done
```

**Special Case: Scene 1**
- Scene 1 should always capture the preset's default configuration
- User should select preset, then long-press btnScene1 to capture defaults

---

#### Extra Long Press: Delete Scene

**Trigger**: Button held for 5+ seconds  
**Visual Feedback**: LED blinks faster (100ms intervals) when threshold crossed

**Behavior Flow**:
```
1. Detect 1-second threshold: Start blinking at 250ms
2. Detect 5-second threshold: Change to fast blink (100ms)
3. While button still pressed:
   - Continue fast blinking
   
4. On button release:
   - Stop LED blinking
   - Delete scene from RAM and flash:
     - allScenes[currentPreset].sceneX.programmed = 0 (RAM)
     - allScenes[currentPreset].sceneX.patchStatus = {0} (RAM)
     - FlashStorage_WritePresetScenes(currentPreset, ...) (Flash)
     - CRITICAL: Keep RAM and Flash synchronized
   
   - Turn OFF scene LED
   - If this was the active scene, turn ON ledScene1
   
5. Done
```

**Notes**:
- Deleted scenes have `programmed = 0`
- Short press on deleted scene does nothing

---

### btnTapTempo

**Status**: Future implementation (TO DO)  
**Placeholder**: Currently sends CC#64

**Future Behavior**:
- Detect tap tempo rhythm
- Send appropriate SysEx to GP-5
- Details to be determined

---

## User Interface Rules

### LED Behavior Rules

**Rule 1: Always Indicate Active Scene**
- One and only one scene LED must be ON at all times
- If no scene is active, ledScene1 is ON by default

**Rule 2: Preset Change Resets to Scene 1**
- Any preset change (from any source) → ledScene1 ON
- ledScene2 and ledScene3 turn OFF

**Rule 3: Scene Button Press Updates LED**
- Successful scene recall → Turn ON scene LED
- Failed scene recall (not programmed) → No LED change

**Rule 4: Blink Patterns Indicate Mode**
- Slow blink (250ms): Saving scene
- Fast blink (100ms): Deleting scene
- Solid: Scene active

### System Startup Behavior

**Power-On Sequence**:
```
1. Initialize all hardware
2. Load scene database from flash to RAM
3. Enable USB power, wait for GP-5 enumeration
   - If enumeration fails: Turn OFF all LEDs, disable scene buttons
4. Send reqPstNum to get current preset
5. Receive preset number
6. Check if Scene1 is programmed for this preset
   - If programmed: Turn ON ledScene1
   - If not programmed: All scene LEDs stay OFF
7. Ready for user input

**Disconnect Event**:
- Monitor for USB disconnect
- On disconnect: Turn OFF all LEDs, disable scene buttons
- Reset to default/startup state

**USB Hot-Plug Recovery** (Auto-Reset Feature):
- Problem: USB host cannot enumerate devices connected after boot
- Solution: Auto-reset after detecting connection failure
- If 3 consecutive disconnect events occur after boot:
  1. System detects stuck USB state
  2. Stops USB restart attempts
  3. Triggers automatic system reset after 1 second
  4. On reboot with GP-5 connected, enumeration succeeds
- This mimics pressing the reset button manually
- Enables hot-plug workflow: Boot → Plug GP-5 → Auto-reset → Connect
```

### Preset Change Detection (Any Source)

**When Preset Change ACK Received**:
```
1. Recognize 22-byte ACK pattern
2. Send reqPstNum
3. Receive preset number (18 bytes)
4. Update currentPresetNumber
5. Check if Scene1 is programmed for this preset
   - If programmed: Turn ON ledScene1
   - If not programmed: Turn OFF all scene LEDs
6. Turn OFF ledScene2 and ledScene3
7. Done
```

**Critical**: This happens whether preset was changed by:
- Bluetooth app
- GP-5 hardware knob
- btnUp/btnDown on controller

---

## Flash Memory Organization

### Memory Map

**Base Address**: 0x0807F000 (Page 254)  
**Total Size**: 1000 bytes (100 presets × 10 bytes)

**Structure**:
```
Offset     Size    Content
---------- ------- -----------------------------------------
0x0000     10      Preset 0 (Scene2, Scene3 only)
0x000A     10      Preset 1
0x0014     10      Preset 2
...
0x03DE     10      Preset 99
```

**Per-Preset Layout** (10 bytes):
```
Offset  Size  Content
------- ----- ----------------------------------
0       1     Scene2.programmed
1-4     4     Scene2.patchStatus[4]
5       1     Scene3.programmed
6-9     4     Scene3.patchStatus[4]
```

**Note**: Scene 1 not stored (always virtual, represents preset defaults via CC#0)

### Flash Operations

**Read All Scenes** (on startup):
```c
void FlashStorage_ReadScenes(PresetScenes_t *scenes, uint16_t size);
// Read 1500 bytes from flash to RAM
```

**Write Preset Scenes** (when saving/deleting scene):
```c
void FlashStorage_WritePresetScenes(uint8_t preset, PresetScenes_t *data);
// Write 15 bytes for specific preset
```

**Erase Considerations**:
- Flash erase granularity: Typically page-aligned (e.g., 2KB pages)
- May need to read-modify-write entire page
- Minimize erase cycles (10,000 cycle limit)

---

## State Machine Design

### System States

```
IDLE
  ├─ WAITING_FOR_PRESET_ACK
  ├─ WAITING_FOR_PRESET_NUMBER
  ├─ WAITING_FOR_PATCH_INFO
  ├─ APPLYING_SCENE
  ├─ SAVING_SCENE
  └─ DELETING_SCENE
```

### State Transitions

**IDLE**:
- **Trigger**: btnUp/btnDown pressed
- **Next State**: WAITING_FOR_PRESET_ACK
- **Action**: Send CC#25 or CC#24

**WAITING_FOR_PRESET_ACK**:
- **Trigger**: Receive 22-byte ACK
- **Next State**: WAITING_FOR_PRESET_NUMBER
- **Action**: Send reqPstNum

**WAITING_FOR_PRESET_NUMBER**:
- **Trigger**: Receive 18-byte response
- **Next State**: IDLE
- **Action**: Update currentPreset, turn ON ledScene1

**Scene Button Short Press**:
- **From**: IDLE
- **Next State**: WAITING_FOR_PATCH_INFO
- **Action**: Send reqPatchInfo

**WAITING_FOR_PATCH_INFO**:
- **Trigger**: Receive 4th message (48 bytes)
- **Next State**: APPLYING_SCENE
- **Action**: Extract patch bitmap

**APPLYING_SCENE**:
- **Action**: Send CC commands for patch changes
- **Next State**: IDLE
- **Final Action**: Update scene LED

**Scene Button Long Press**:
- **From**: IDLE
- **Next State**: SAVING_SCENE
- **Action**: Start LED blinking

**SAVING_SCENE**:
- **Trigger**: Button released (after 1s)
- **Next State**: WAITING_FOR_PATCH_INFO
- **Action**: Send reqPatchInfo, prepare to save

**After WAITING_FOR_PATCH_INFO** (when saving):
- **Action**: Save patch bitmap to flash
- **Next State**: IDLE
- **Final Action**: Turn ON scene LED (solid)

**Scene Button Extra Long Press**:
- **From**: SAVING_SCENE
- **Trigger**: 5-second threshold
- **Next State**: DELETING_SCENE
- **Action**: Change to fast LED blink

**DELETING_SCENE**:
- **Trigger**: Button released
- **Action**: Clear scene from flash, turn OFF LED
- **Next State**: IDLE

---

## Implementation Phases

### Phase 1: Data Structures and Flash Management
- [ ] Define SceneData_t and PresetScenes_t structures
- [ ] Implement flash read/write for scene database
- [ ] Create RAM copy initialization
- [ ] Test flash persistence across power cycles

### Phase 2: GP-5 Communication
- [ ] Implement reqPstNum and response parsing
- [ ] Implement reqPatchInfo and response parsing
- [ ] Create patch bitmap decode function
- [ ] Test preset number retrieval
- [ ] Test patch info retrieval

### Phase 3: Preset Change Handling
- [ ] Detect 22-byte ACK pattern
- [ ] Implement preset change callback
- [ ] Update currentPresetNumber on change
- [ ] Reset to Scene 1 on preset change
- [ ] Test with btnUp/btnDown
- [ ] Test with GP-5 hardware knob
- [ ] Test with Bluetooth app

### Phase 4: Scene Recall (Short Press)
- [ ] Implement scene programmed check
- [ ] Get current patches from GP-5
- [ ] Compare current vs. saved patches
- [ ] Generate CC commands for differences
- [ ] Send CC commands to GP-5
- [ ] Update scene LEDs
- [ ] Test with all 3 scene buttons

### Phase 5: Scene Save (Long Press)
- [ ] Detect 1-second threshold
- [ ] Implement LED blink timer (250ms)
- [ ] Get current patches on button release
- [ ] Save to RAM copy
- [ ] Save to flash
- [ ] Verify RAM and flash are synchronized
- [ ] Update scene LED
- [ ] Test save persistence

### Phase 6: Scene Delete (Extra Long Press)
- [ ] Detect 5-second threshold
- [ ] Implement fast LED blink (100ms)
- [ ] Clear scene from RAM copy
- [ ] Clear scene from flash
- [ ] Verify RAM and flash are synchronized
- [ ] Update LED state
- [ ] Test deletion

### Phase 7: Integration Testing
- [x] Test all buttons in sequence
- [x] Test multiple preset changes
- [x] Test scene save/recall/delete cycle
- [x] Test power cycle persistence
- [x] Test edge cases (rapid button presses, etc.)
- [x] Button state reset fix (prevent false triggers)

### Phase 8: Optimization and Polish ✅ **COMPLETED**
- [x] Reduce debug logging (DEBUG_VERBOSE flag)
- [x] Optimize flash storage (1500→1000 bytes)
- [x] USB hot-plug auto-reset feature
- [x] Performance tuning (62KB flash)
- [x] Remove STM32CubeMX USER CODE markers
- [x] Production-ready code quality

---

## Critical Implementation Notes

### Timing Considerations

**Button Timing**:
- Debounce: 50ms (already implemented)
- Long press threshold: 2000ms
- Extra long press threshold: 5000ms
- LED blink: 250ms (slow), 100ms (fast)

**MIDI Response Timeouts**:
- Preset Change ACK: Expect within 100ms
- Preset Number Response: Expect within 200ms
- Patch Info Response: 25 messages, may take 500ms+

### Synchronization Challenges

**Problem**: Multiple entities can change GP-5 state
**Solution**: Always query before assuming

**Never Assume**:
- Current preset number (always query after ACK)
- Current patch states (always query before scene operations)

**Always Verify**:
- Scene is programmed before recalling
- Current preset matches expected preset

### Error Handling

**Timeout Scenarios**:
- No ACK received after CC command
- No response to reqPstNum
- Incomplete patch info response

**Recovery Actions**:
- Retry request (up to 3 times)
- Return to IDLE state
- Log error for debugging

**Flash Failures**:
- Verify write operation
- Maintain last-known-good state in RAM
- Alert user via LED pattern (future)

---

## User Experience Goals

### Performance Targets
- **Preset change**: < 500ms from button press to LED update
- **Scene recall**: < 1 second from button press to patches applied
- **Scene save**: < 2 seconds from button release to flash write complete

### Reliability Targets
- **Flash endurance**: 10,000 writes per scene minimum
- **State sync**: 100% accuracy tracking GP-5 changes
- **Scene recall**: 100% accuracy applying saved patches

### Usability Targets
- **Intuitive**: Musician can learn operation without manual
- **Immediate feedback**: LEDs always show current state
- **Fail-safe**: Invalid operations (e.g., recalling unprogrammed scene) do nothing

---

## Validation Checklist

Before implementation begins, verify understanding of:

- [ ] GP-5 preset and patch architecture
- [ ] All MIDI message formats and byte positions
- [ ] Scene storage structure (5 bytes per scene)
- [ ] Flash memory layout (1500 bytes total)
- [ ] Button timing thresholds (1s, 5s)
- [ ] LED feedback patterns (solid, slow blink, fast blink)
- [ ] State machine transitions
- [ ] Preset change synchronization from all sources
- [ ] CC assignments for patch control (48-57)
- [ ] No ACK from GP-5 for patch on/off

---

## Design Decisions (Answered)

1. **Scene 1 Default Behavior**: User CAN overwrite Scene 1. While Scene 1 typically captures preset defaults, it can be reprogrammed like Scene 2 and Scene 3. This may be revised based on user feedback.

2. **Partial Scene Application**: If CC commands fail during scene recall, ABORT and REVERT. Do not leave GP-5 in a partially-applied state. Future enhancement: retry logic.

3. **Flash Wear Leveling**: NOT IMPLEMENTED. Keep simple for now. With 10,000 write cycles, even frequent scene changes (100/day) would last 100 days per scene slot. Monitor in production if needed.

4. **Scene Switching Speed**: Send all CCs IMMEDIATELY for fast switching. Key optimization: Only send CC commands for patches that NEED to change (compare current state vs. saved state). Example: If Scene has NR=OFF, MOD=OFF, DST=ON, only send CCs for those 3 patches, not all 10.

5. **Button Priority**: IGNORE preset change (btnUp/btnDown) if scene operation is in progress. Prevent race conditions and ensure scene application completes atomically.

6. **LED Blink During Recall**: NO BLINKING during patch application. Patch changes should be fast enough (<1 second) that blinking would be imperceptible and distracting.

---

## Document Status

**Version**: 1.0 Production  
**Date**: December 20, 2025  
**Status**: ✅ Implemented and Tested  
**Implementation**: Complete (All 8 Phases)

**Key Achievements**:
- Scene 1: Always available (preset defaults via CC#0)
- Scene 2 & 3: User-programmable with flash persistence
- USB hot-plug support via auto-reset
- Optimized flash storage (1000 bytes)
- Production-ready code (Phase 8 complete)

---

**End of Operational Specification**
