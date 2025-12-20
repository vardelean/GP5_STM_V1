/**
  ******************************************************************************
  * @file    scene_manager.h
  * @author  Custom Implementation
  * @brief   GP-5 Scene Management Interface
  * 
  * Manages 3 scenes per preset (300 total scenes for 100 presets)
  * Each scene stores which patches are ON/OFF for quick recall
  ******************************************************************************
  */

#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
  * @brief Patch information structure
  * Represents the ON/OFF state of all 10 patches in a preset
  */
typedef struct {
  bool patchCAB;  /* Cabinet */
  bool patchEQ;   /* Equalizer */
  bool patchMOD;  /* Modulation */
  bool patchDLY;  /* Delay */
  bool patchNR;   /* Noise Reduction */
  bool patchPRE;  /* Preamp */
  bool patchDST;  /* Distortion */
  bool patchAMP;  /* Amplifier */
  bool patchRVB;  /* Reverb */
  bool patchNS;   /* Neural Amp (N→S) */
} PatchInfo_t;

/**
  * @brief Scene data (5 bytes per scene)
  * Stores whether a scene is programmed and the patch ON/OFF bitmap
  */
typedef struct {
  uint8_t programmed;      /* 1 = Scene programmed, 0 = Empty */
  uint8_t patchStatus[4];  /* 32-bit bitmap of patch states */
} SceneData_t;

/**
  * @brief Per-preset scene storage (10 bytes per preset)
  * Contains Scene 2 and Scene 3 only (Scene 1 = preset defaults, not stored)
  */
typedef struct {
  SceneData_t scene2;  /* Scene 2 (user-defined) */
  SceneData_t scene3;  /* Scene 3 (user-defined) */
} PresetScenes_t;

/* Exported constants --------------------------------------------------------*/

#define SCENE_MANAGER_PRESET_COUNT    100  /* Total presets (0-99) */
#define SCENE_MANAGER_SCENES_PER_PRESET  2  /* User-programmable scenes per preset (Scene 2 & 3) */

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  Initialize scene manager
  * @note   Loads scene database from flash to RAM
  * @retval None
  */
void SceneManager_Init(void);

/**
  * @brief  Check if a scene is programmed
  * @param  preset: Preset number (0-99)
  * @param  sceneNum: Scene number (1-3)
  * @retval true if scene is programmed, false otherwise
  */
bool SceneManager_IsSceneProgrammed(uint8_t preset, uint8_t sceneNum);

/**
  * @brief  Get patch states for a scene
  * @param  preset: Preset number (0-99)
  * @param  sceneNum: Scene number (1-3)
  * @param  patches: Pointer to PatchInfo_t structure to fill
  * @retval None
  */
void SceneManager_GetScenePatches(uint8_t preset, uint8_t sceneNum, PatchInfo_t *patches);

/**
  * @brief  Save scene with current patch configuration
  * @param  preset: Preset number (0-99)
  * @param  sceneNum: Scene number (1-3)
  * @param  patchBitmap: 32-bit bitmap of patch states from GP-5
  * @retval None
  */
void SceneManager_SaveScene(uint8_t preset, uint8_t sceneNum, uint32_t patchBitmap);

/**
  * @brief  Delete a scene
  * @param  preset: Preset number (0-99)
  * @param  sceneNum: Scene number (1-3)
  * @retval None
  */
void SceneManager_DeleteScene(uint8_t preset, uint8_t sceneNum);

/**
  * @brief  Decode GP-5 patch bitmap into PatchInfo structure
  * @param  bitmap: 32-bit patch bitmap from GP-5
  * @param  patches: Pointer to PatchInfo_t structure to fill
  * @retval None
  */
void SceneManager_DecodePatchBitmap(uint32_t bitmap, PatchInfo_t *patches);

/**
  * @brief  Encode PatchInfo structure into GP-5 patch bitmap
  * @param  patches: Pointer to PatchInfo_t structure
  * @retval 32-bit patch bitmap
  */
uint32_t SceneManager_EncodePatchBitmap(const PatchInfo_t *patches);

/**
  * @brief  Get raw scene data (for advanced use)
  * @param  preset: Preset number (0-99)
  * @param  sceneNum: Scene number (1-3)
  * @param  sceneData: Pointer to SceneData_t structure to fill
  * @retval None
  */
void SceneManager_GetSceneData(uint8_t preset, uint8_t sceneNum, SceneData_t *sceneData);

/**
  * @brief  Test scene manager functionality (for development/debugging)
  * @retval None
  */
void SceneManager_Test(void);

#ifdef __cplusplus
}
#endif

#endif /* SCENE_MANAGER_H */
