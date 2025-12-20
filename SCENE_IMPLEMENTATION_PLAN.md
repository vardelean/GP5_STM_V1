# Scene Management Implementation Plan

## Document Status
**Version**: 1.0  
**Date**: December 19, 2025  
**Status**: Ready for Implementation  
**Specification**: GP5_OPERATIONAL_SPECIFICATION.md (Approved)

---

## Implementation Strategy

### Approach
- **Incremental**: Implement one feature at a time
- **Test-driven**: Test each feature before moving to next
- **Non-breaking**: Existing btnUp/btnDown/btnTap functionality remains operational
- **Modular**: Scene logic isolated in dedicated module

### Order of Implementation
1. Data structures and flash layout
2. GP-5 communication primitives  
3. Basic preset tracking
4. Scene recall (short press)
5. Scene save (long press)
6. Scene delete (extra long press)
7. Integration and testing

---

## Phase 1: Data Structures and Flash Storage

### Tasks
1. **Define scene data structures** in new file `scene_manager.h`
2. **Implement flash read/write** functions
3. **Initialize RAM copy** on startup
4. **Test persistence** across power cycles

### Files to Create
- `Core/Inc/scene_manager.h` - Scene management interface
- `Core/Src/scene_manager.c` - Scene management implementation

### Data Structures

```c
/* scene_manager.h */

#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

/* Patch information structure */
typedef struct {
  bool patchCAB;  // Cabinet
  bool patchEQ;   // Equalizer
  bool patchMOD;  // Modulation
  bool patchDLY;  // Delay
  bool patchNR;   // Noise Reduction
  bool patchPRE;  // Preamp
  bool patchDST;  // Distortion
  bool patchAMP;  // Amplifier
  bool patchRVB;  // Reverb
  bool patchNS;   // Neural Amp
} PatchInfo_t;

/* Scene data (5 bytes) */
typedef struct {
  uint8_t programmed;      // 1 = Scene programmed, 0 = Empty
  uint8_t patchStatus[4];  // 32-bit bitmap of patch states
} SceneData_t;

/* Per-preset scenes (15 bytes) */
typedef struct {
  SceneData_t scene1;
  SceneData_t scene2;
  SceneData_t scene3;
} PresetScenes_t;

/* Public Functions */
void SceneManager_Init(void);
bool SceneManager_IsSceneProgrammed(uint8_t preset, uint8_t sceneNum);
void SceneManager_GetScenePatches(uint8_t preset, uint8_t sceneNum, PatchInfo_t *patches);
void SceneManager_SaveScene(uint8_t preset, uint8_t sceneNum, uint32_t patchBitmap);
void SceneManager_DeleteScene(uint8_t preset, uint8_t sceneNum);
void SceneManager_DecodePatchBitmap(uint32_t bitmap, PatchInfo_t *patches);
uint32_t SceneManager_EncodePatchBitmap(PatchInfo_t *patches);

#endif /* SCENE_MANAGER_H */
```

### Flash Storage Extension

**Add to `flash_storage.h`**:
```c
/* Scene database storage */
#define FLASH_SCENE_BASE_ADDRESS  (FLASH_STORAGE_BASE_ADDRESS + 0x1000)  // Offset 4KB
#define FLASH_SCENE_SIZE          1500  // 100 presets × 15 bytes

FlashStorage_StatusTypeDef FlashStorage_ReadScenes(uint8_t *data, uint16_t size);
FlashStorage_StatusTypeDef FlashStorage_WritePresetScenes(uint8_t preset, PresetScenes_t *data);
```

### Test Plan
- [ ] Create scene, save to flash, power cycle, verify loaded
- [ ] Save scene to preset 0, verify at correct flash address
- [ ] Save scene to preset 99, verify at correct flash address
- [ ] Modify scene, verify RAM and flash sync
- [ ] Delete scene, verify cleared from both RAM and flash

---

## Phase 2: GP-5 Communication Primitives

### Tasks
1. **Implement SysEx message builders**
2. **Implement response parsers**
3. **Create patch control functions**
4. **Test with real GP-5**

### New Functions in `gp5_midi.c`

```c
/* Request preset number from GP-5 */
void GP5_MIDI_RequestPresetNumber(void);

/* Parse preset number response (18 bytes) */
bool GP5_MIDI_ParsePresetNumber(uint8_t *data, uint16_t len, uint8_t *preset);

/* Detect preset change ACK (22 bytes) */
bool GP5_MIDI_IsPresetChangeACK(uint8_t *data, uint16_t len);

/* Request patch info from GP-5 */
void GP5_MIDI_RequestPatchInfo(void);

/* Parse patch info response (4th message, 48 bytes) */
bool GP5_MIDI_ParsePatchInfo(uint8_t *data, uint16_t len, uint32_t *patchBitmap);

/* Send patch on/off command */
void GP5_MIDI_SetPatchState(uint8_t patchIndex, bool turnOn);
```

### CC Number Mapping

```c
/* In gp5_midi.h */
#define GP5_CC_NR   48  // Noise Reduction
#define GP5_CC_PRE  49  // Preamp
#define GP5_CC_DST  50  // Distortion
#define GP5_CC_NS   51  // Neural Amp
#define GP5_CC_AMP  52  // Amplifier
#define GP5_CC_CAB  53  // Cabinet
#define GP5_CC_EQ   54  // Equalizer
#define GP5_CC_MOD  55  // Modulation
#define GP5_CC_DLY  56  // Delay
#define GP5_CC_RVB  57  // Reverb
```

### Test Plan
- [ ] Request preset number, verify 18-byte response parsed correctly
- [ ] Detect ACK after btnUp/btnDown
- [ ] Request patch info, capture all 25 messages, verify 4th message parsed
- [ ] Send patch ON command (CC=127), verify GP-5 turns patch on
- [ ] Send patch OFF command (CC=0), verify GP-5 turns patch off

---

## Phase 3: Preset Tracking and ACK Handling

### Tasks
1. **Add global current preset tracking**
2. **Implement ACK detection**
3. **Auto-request preset number on ACK**
4. **Update scene LEDs based on preset**

### Global State Variables

```c
/* In gp5_midi.c */
static uint8_t currentPresetNumber = 0;
static bool presetNumberValid = false;
static bool awaitingPresetNumber = false;
static bool awaitingPatchInfo = false;
static uint32_t lastPatchBitmap = 0;
```

### Modified `GP5_MIDI_ProcessReceivedData()`

```c
void GP5_MIDI_ProcessReceivedData(uint8_t *data, uint16_t length)
{
  // ... existing USB packet extraction ...
  
  /* Check for Preset Change ACK */
  if (GP5_MIDI_IsPresetChangeACK(clean_midi, clean_length))
  {
    printf("[GP-5] Preset change ACK received\r\n");
    GP5_MIDI_RequestPresetNumber();
    awaitingPresetNumber = true;
    return;
  }
  
  /* Check for Preset Number Response */
  uint8_t preset;
  if (GP5_MIDI_ParsePresetNumber(clean_midi, clean_length, &preset))
  {
    printf("[GP-5] Preset number: %d\r\n", preset);
    currentPresetNumber = preset;
    presetNumberValid = true;
    awaitingPresetNumber = false;
    
    // Update LEDs: Scene1 ON if programmed, otherwise all OFF
    if (SceneManager_IsSceneProgrammed(preset, 1))
    {
      LED_SetScene(LED_SCENE1);
    }
    else
    {
      LED_AllOff();
    }
    return;
  }
  
  /* Check for Patch Info Response (4th message) */
  uint32_t patchBitmap;
  if (GP5_MIDI_ParsePatchInfo(clean_midi, clean_length, &patchBitmap))
  {
    printf("[GP-5] Patch info received: 0x%08lX\r\n", patchBitmap);
    lastPatchBitmap = patchBitmap;
    awaitingPatchInfo = false;
    // Trigger scene application or save based on context
    return;
  }
  
  // ... existing message decoding ...
}
```

### Test Plan
- [ ] Press btnUp, verify ACK triggers preset number request
- [ ] Change preset on GP-5 hardware, verify ACK detected
- [ ] Verify currentPresetNumber updates correctly
- [ ] Verify LED turns ON only if Scene1 is programmed

---

## Phase 4: Scene Recall (Short Press)

### Tasks
1. **Detect short press** on scene buttons
2. **Check if scene is programmed**
3. **Request current patches from GP-5**
4. **Compare and send CC commands**
5. **Update scene LED**

### Modified Button Handler

```c
/* In button_handler.c - add to ButtonEventHandler callback */

void ButtonEventHandler(ButtonID_t button, ButtonEvent_t event)
{
  if (event == BTN_EVENT_SHORT_PRESS)
  {
    if (button == BTN_SCENE1 || button == BTN_SCENE2 || button == BTN_SCENE3)
    {
      uint8_t sceneNum = (button - BTN_SCENE1) + 1;
      SceneManager_RecallScene(currentPresetNumber, sceneNum);
    }
  }
}
```

### New Function: `SceneManager_RecallScene()`

```c
void SceneManager_RecallScene(uint8_t preset, uint8_t sceneNum)
{
  /* Check if scene is programmed */
  if (!SceneManager_IsSceneProgrammed(preset, sceneNum))
  {
    printf("Scene %d not programmed for preset %d\r\n", sceneNum, preset);
    return;  // Do nothing
  }
  
  /* Request current patch status from GP-5 */
  printf("Recalling Scene %d...\r\n", sceneNum);
  GP5_MIDI_RequestPatchInfo();
  awaitingPatchInfo = true;
  pendingSceneRecall = sceneNum;  // Store for callback
}
```

### Callback When Patch Info Received

```c
void GP5_MIDI_OnPatchInfoReceived(uint32_t currentPatchBitmap)
{
  if (pendingSceneRecall > 0)
  {
    /* Get saved scene patches */
    PatchInfo_t savedPatches, currentPatches;
    SceneManager_GetScenePatches(currentPresetNumber, pendingSceneRecall, &savedPatches);
    SceneManager_DecodePatchBitmap(currentPatchBitmap, &currentPatches);
    
    /* Apply changes */
    SceneManager_ApplyPatchChanges(&currentPatches, &savedPatches);
    
    /* Update LED */
    LED_SetScene(pendingSceneRecall);
    
    pendingSceneRecall = 0;
  }
}
```

### Apply Patch Changes

```c
void SceneManager_ApplyPatchChanges(PatchInfo_t *current, PatchInfo_t *target)
{
  /* Compare and send CC only for patches that need to change */
  
  if (current->patchNR != target->patchNR)
    GP5_MIDI_SetPatchState(PATCH_NR, target->patchNR);
    
  if (current->patchPRE != target->patchPRE)
    GP5_MIDI_SetPatchState(PATCH_PRE, target->patchPRE);
    
  // ... repeat for all 10 patches ...
  
  if (current->patchRVB != target->patchRVB)
    GP5_MIDI_SetPatchState(PATCH_RVB, target->patchRVB);
    
  printf("Scene applied: %d patches changed\r\n", changesCount);
}
```

### Test Plan
- [ ] Program Scene1, short press btnScene1, verify patches change
- [ ] Short press unprogrammed scene, verify no action, no LED
- [ ] Program Scene2 with different patches, verify correct recall
- [ ] Verify only changed patches get CC commands (not all 10)

---

## Phase 5: Scene Save (Long Press)

### Tasks
1. **Detect 1-second threshold**
2. **Start LED blinking at 250ms**
3. **On release, request patch info**
4. **Save to RAM and flash**
5. **Turn ON scene LED (solid)**

### Button Handler Extension

```c
void ButtonHandler_Process(void)
{
  for (uint8_t i = 0; i < BUTTON_COUNT; i++)
  {
    // ... existing press detection ...
    
    /* Detect long press threshold (1000ms) */
    if (button_states[i].state == BTN_STATE_PRESSED &&
        (current_time - button_states[i].press_time) >= BUTTON_LONG_PRESS_TIME_MS &&
        !button_states[i].long_press_triggered)
    {
      button_states[i].long_press_triggered = true;
      
      /* Start LED blinking if scene button */
      if (i >= BTN_SCENE1 && i <= BTN_SCENE3)
      {
        uint8_t sceneNum = (i - BTN_SCENE1) + 1;
        LED_StartBlink(sceneNum, 250);  // 250ms blink
        pendingSceneSave = sceneNum;
      }
    }
    
    /* Detect release after long press */
    if (button_states[i].state == BTN_STATE_RELEASED &&
        button_states[i].long_press_triggered &&
        pendingSceneSave > 0)
    {
      /* Request patch info to save */
      GP5_MIDI_RequestPatchInfo();
      awaitingPatchInfoForSave = true;
    }
  }
}
```

### Callback When Patch Info Received (Save Context)

```c
void GP5_MIDI_OnPatchInfoReceived(uint32_t patchBitmap)
{
  if (awaitingPatchInfoForSave && pendingSceneSave > 0)
  {
    /* Save to scene manager */
    SceneManager_SaveScene(currentPresetNumber, pendingSceneSave, patchBitmap);
    
    /* Stop blinking, turn LED solid */
    LED_StopBlink();
    LED_SetScene(pendingSceneSave);
    
    printf("Scene %d saved for preset %d\r\n", pendingSceneSave, currentPresetNumber);
    
    pendingSceneSave = 0;
    awaitingPatchInfoForSave = false;
  }
}
```

### Test Plan
- [ ] Long press btnScene1 for 1 second, verify LED starts blinking
- [ ] Release button, verify patch info requested
- [ ] Verify scene saved to flash
- [ ] Power cycle, verify scene persists
- [ ] Recall saved scene, verify patches match

---

## Phase 6: Scene Delete (Extra Long Press)

### Tasks
1. **Detect 5-second threshold**
2. **Change to fast blink (100ms)**
3. **On release, clear scene**
4. **Turn OFF scene LED**

### Button Handler Extension

```c
/* Detect extra long press threshold (5000ms) */
if (button_states[i].state == BTN_STATE_PRESSED &&
    (current_time - button_states[i].press_time) >= 5000 &&
    !button_states[i].extra_long_press_triggered &&
    pendingSceneSave > 0)
{
  button_states[i].extra_long_press_triggered = true;
  
  /* Change to fast blink */
  LED_StartBlink(pendingSceneSave, 100);  // 100ms fast blink
  pendingSceneDelete = pendingSceneSave;
  pendingSceneSave = 0;  // Cancel save, switch to delete mode
}

/* On release after extra long press */
if (button_states[i].state == BTN_STATE_RELEASED &&
    button_states[i].extra_long_press_triggered &&
    pendingSceneDelete > 0)
{
  /* Delete scene */
  SceneManager_DeleteScene(currentPresetNumber, pendingSceneDelete);
  
  /* Stop blinking, turn OFF LED */
  LED_StopBlink();
  LED_TurnOff(pendingSceneDelete);
  
  printf("Scene %d deleted for preset %d\r\n", pendingSceneDelete, currentPresetNumber);
  
  /* If this was active scene, switch to Scene1 */
  if (activeScene == pendingSceneDelete)
  {
    if (SceneManager_IsSceneProgrammed(currentPresetNumber, 1))
    {
      LED_SetScene(LED_SCENE1);
    }
  }
  
  pendingSceneDelete = 0;
}
```

### Test Plan
- [ ] Long press btnScene2 for 5+ seconds, verify LED switches to fast blink
- [ ] Release, verify scene deleted from flash
- [ ] Short press deleted scene, verify no action
- [ ] Power cycle, verify scene remains deleted

---

## Phase 7: Integration and Testing

### Comprehensive Test Scenarios

#### Test 1: Basic Scene Save/Recall
1. Power on, connect GP-5
2. Preset 00 loads, verify ledScene1 OFF (not programmed)
3. Long press btnScene1, LED blinks
4. Release, verify Scene1 saved
5. Short press btnScene1, verify patches recalled

#### Test 2: Multiple Scenes Per Preset
1. Set preset to 05
2. Save Scene1 with NR=ON, AMP=ON
3. Save Scene2 with DST=ON, CAB=ON
4. Save Scene3 with MOD=ON, RVB=ON
5. Recall Scene1, verify NR and AMP on
6. Recall Scene2, verify DST and CAB on
7. Recall Scene3, verify MOD and RVB on

#### Test 3: Preset Change Resets to Scene1
1. Program Scene2 for preset 10
2. Switch to preset 10
3. Recall Scene2 (LED2 on)
4. Press btnDown to change preset
5. Verify ledScene1 turns on, ledScene2 off

#### Test 4: GP-5 Hardware Preset Change
1. Program Scene2 for preset 20
2. Recall Scene2 on controller
3. Change preset using GP-5 rotary knob
4. Verify controller detects ACK
5. Verify ledScene1 turns on

#### Test 5: Unprogrammed Scene
1. Select preset 99 (never programmed)
2. Short press btnScene2
3. Verify no LED turns on, no CC sent

#### Test 6: Scene Delete
1. Program Scene3 for preset 15
2. Recall Scene3 (verify working)
3. Extra long press btnScene3 (5+ seconds)
4. Verify fast blink, release
5. Short press btnScene3, verify no action
6. Power cycle, verify Scene3 still deleted

#### Test 7: Flash Persistence
1. Program all 3 scenes for preset 50
2. Power cycle STM32
3. Select preset 50
4. Verify all 3 scenes recall correctly

#### Test 8: Race Condition - Button During Scene Recall
1. Recall Scene1
2. While patches are being applied, press btnUp
3. Verify preset change is ignored
4. Verify scene recall completes

---

## File Modifications Summary

### New Files
- `Core/Inc/scene_manager.h` - Scene management interface
- `Core/Src/scene_manager.c` - Scene management implementation

### Modified Files
- `Core/Inc/gp5_midi.h` - Add GP-5 communication functions
- `Core/Src/gp5_midi.c` - Implement ACK handling, preset/patch requests
- `Core/Inc/flash_storage.h` - Add scene database storage functions
- `Core/Src/flash_storage.c` - Implement scene flash read/write
- `Core/Src/button_handler.c` - Add long press and extra long press detection
- `Core/Inc/led_controller.h` - Add LED blink functions
- `Core/Src/led_controller.c` - Implement LED blinking
- `cmake/stm32cubemx/CMakeLists.txt` - Add scene_manager.c

---

## Implementation Order

**Week 1**: Phases 1-2 (Data structures, GP-5 communication)  
**Week 2**: Phase 3 (Preset tracking and ACK handling)  
**Week 3**: Phase 4 (Scene recall - short press)  
**Week 4**: Phases 5-6 (Scene save and delete - long press)  
**Week 5**: Phase 7 (Integration testing)

---

## Next Steps

**Ready to begin?** 

Start with **Phase 1, Task 1**: Create `scene_manager.h` and `scene_manager.c` with data structure definitions.

**Command to verify**: No code changes yet - document approval complete.

---

**Document Version**: 1.0  
**Date**: December 19, 2025  
**Status**: Ready for Implementation  
**Estimated Completion**: 5 weeks (incremental testing)
