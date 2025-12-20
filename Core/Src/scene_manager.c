/**
  ******************************************************************************
  * @file    scene_manager.c
  * @author  Custom Implementation
  * @brief   GP-5 Scene Management Implementation
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "scene_manager.h"
#include "flash_storage.h"
#include <string.h>
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* RAM copy of entire scene database (1500 bytes) */
static PresetScenes_t allScenes[SCENE_MANAGER_PRESET_COUNT];

/* Initialization flag */
static bool initialized = false;

/* Private function prototypes -----------------------------------------------*/
static SceneData_t* GetScenePointer(uint8_t preset, uint8_t sceneNum);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize scene manager
  * @note   Loads scene database from flash to RAM
  * @retval None
  */
void SceneManager_Init(void)
{
  if (initialized)
  {
    printf("[SceneManager] Already initialized\r\n");
    return;
  }
  
  printf("[SceneManager] Initializing...\r\n");
  
  /* Clear RAM copy */
  memset(allScenes, 0, sizeof(allScenes));
  
  /* Load scene database from flash */
  FlashStorage_StatusTypeDef status = FlashStorage_ReadScenes((uint8_t*)allScenes, sizeof(allScenes));
  
  if (status == FLASH_STORAGE_OK)
  {
    printf("[SceneManager] Loaded %d bytes from flash\r\n", sizeof(allScenes));
    
    /* Count programmed scenes for diagnostics (Scene 2 and 3 only) */
    uint16_t programmedCount = 0;
    for (uint8_t i = 0; i < SCENE_MANAGER_PRESET_COUNT; i++)
    {
      if (allScenes[i].scene2.programmed) programmedCount++;
      if (allScenes[i].scene3.programmed) programmedCount++;
    }
    printf("[SceneManager] Found %d programmed scenes (Scene 2 & 3 only)\r\n", programmedCount);
  }
  else
  {
    printf("[SceneManager] Flash read failed, using empty database\r\n");
  }
  
  initialized = true;
}

/**
  * @brief  Test scene manager functionality
  * @note   Call this after initialization to verify basic operations
  * @retval None
  */
void SceneManager_Test(void)
{
  printf("\r\n[SceneManager] Running self-test...\r\n");
  
  /* Test 1: Save a scene */
  printf("Test 1: Save scene to Preset 0, Scene 1\r\n");
  uint32_t testBitmap = 0x03000F0F;  /* Some patches on */
  SceneManager_SaveScene(0, 1, testBitmap);
  
  /* Test 2: Check if programmed */
  printf("Test 2: Check if Scene 1 is programmed\r\n");
  bool isProgrammed = SceneManager_IsSceneProgrammed(0, 1);
  printf("  Result: %s\r\n", isProgrammed ? "PROGRAMMED" : "NOT PROGRAMMED");
  
  /* Test 3: Read back patches */
  printf("Test 3: Read back patches\r\n");
  PatchInfo_t patches;
  SceneManager_GetScenePatches(0, 1, &patches);
  printf("  CAB=%d EQ=%d MOD=%d DLY=%d NR=%d PRE=%d DST=%d AMP=%d RVB=%d NS=%d\r\n",
         patches.patchCAB, patches.patchEQ, patches.patchMOD, patches.patchDLY, patches.patchNR,
         patches.patchPRE, patches.patchDST, patches.patchAMP, patches.patchRVB, patches.patchNS);
  
  /* Test 4: Delete scene */
  printf("Test 4: Delete scene\r\n");
  SceneManager_DeleteScene(0, 1);
  isProgrammed = SceneManager_IsSceneProgrammed(0, 1);
  printf("  After delete: %s\r\n", isProgrammed ? "PROGRAMMED" : "NOT PROGRAMMED");
  
  printf("[SceneManager] Self-test complete\r\n\r\n");
}

/**
  * @brief  Check if a scene is programmed
  * @param  preset: Preset number (0-99)
  * @param  sceneNum: Scene number (1-3)
  * @retval true if scene is programmed, false otherwise
  */
bool SceneManager_IsSceneProgrammed(uint8_t preset, uint8_t sceneNum)
{
  if (!initialized)
  {
    printf("[SceneManager] ERROR: Not initialized\r\n");
    return false;
  }
  
  if (preset >= SCENE_MANAGER_PRESET_COUNT || sceneNum < 1 || sceneNum > 3)
  {
    printf("[SceneManager] ERROR: Invalid preset %d or scene %d\r\n", preset, sceneNum);
    return false;
  }
  
  /* Scene 1 is always "available" (preset defaults), not stored */
  if (sceneNum == 1)
  {
    return true;  /* Always programmed */
  }
  
  SceneData_t *scene = GetScenePointer(preset, sceneNum);
  return (scene != NULL && scene->programmed == 1);
}

/**
  * @brief  Get patch states for a scene
  * @param  preset: Preset number (0-99)
  * @param  sceneNum: Scene number (1-3)
  * @param  patches: Pointer to PatchInfo_t structure to fill
  * @retval None
  */
void SceneManager_GetScenePatches(uint8_t preset, uint8_t sceneNum, PatchInfo_t *patches)
{
  if (!initialized || patches == NULL)
  {
    printf("[SceneManager] ERROR: Not initialized or NULL pointer\r\n");
    return;
  }
  
  if (preset >= SCENE_MANAGER_PRESET_COUNT || sceneNum < 1 || sceneNum > 3)
  {
    printf("[SceneManager] ERROR: Invalid preset %d or scene %d\r\n", preset, sceneNum);
    return;
  }
  
  SceneData_t *scene = GetScenePointer(preset, sceneNum);
  if (scene == NULL || scene->programmed != 1)
  {
    printf("[SceneManager] Scene %d for preset %d not programmed\r\n", sceneNum, preset);
    memset(patches, 0, sizeof(PatchInfo_t));
    return;
  }
  
  /* Reconstruct 32-bit bitmap from 4 bytes */
  uint32_t bitmap = ((uint32_t)scene->patchStatus[3] << 24) |
                    ((uint32_t)scene->patchStatus[2] << 16) |
                    ((uint32_t)scene->patchStatus[1] << 8) |
                    ((uint32_t)scene->patchStatus[0]);
  
  /* Decode bitmap into patch structure */
  SceneManager_DecodePatchBitmap(bitmap, patches);
}

/**
  * @brief  Save scene with current patch configuration
  * @param  preset: Preset number (0-99)
  * @param  sceneNum: Scene number (1-3)
  * @param  patchBitmap: 32-bit bitmap of patch states from GP-5
  * @retval None
  */
void SceneManager_SaveScene(uint8_t preset, uint8_t sceneNum, uint32_t patchBitmap)
{
  if (!initialized)
  {
    printf("[SceneManager] ERROR: Not initialized\r\n");
    return;
  }
  
  if (preset >= SCENE_MANAGER_PRESET_COUNT || sceneNum < 2 || sceneNum > 3)
  {
    printf("[SceneManager] ERROR: Invalid preset %d or scene %d (only Scene 2 & 3 saveable)\r\n", preset, sceneNum);
    return;
  }
  
  SceneData_t *scene = GetScenePointer(preset, sceneNum);
  if (scene == NULL)
  {
    printf("[SceneManager] ERROR: Failed to get scene pointer\r\n");
    return;
  }
  
  /* Update RAM copy */
  scene->programmed = 1;
  scene->patchStatus[0] = (patchBitmap) & 0xFF;
  scene->patchStatus[1] = (patchBitmap >> 8) & 0xFF;
  scene->patchStatus[2] = (patchBitmap >> 16) & 0xFF;
  scene->patchStatus[3] = (patchBitmap >> 24) & 0xFF;
  
  printf("[SceneManager] Saving Scene %d for Preset %d (bitmap: 0x%08lX)\r\n", 
         sceneNum, preset, patchBitmap);
  
  /* Write entire preset scenes to flash (10 bytes: Scene 2 & 3 only) */
  FlashStorage_StatusTypeDef status = FlashStorage_WritePresetScenes(preset, &allScenes[preset]);
  
  if (status == FLASH_STORAGE_OK)
  {
    printf("[SceneManager] Scene saved successfully\r\n");
  }
  else
  {
    printf("[SceneManager] ERROR: Flash write failed\r\n");
  }
}

/**
  * @brief  Delete a scene
  * @param  preset: Preset number (0-99)
  * @param  sceneNum: Scene number (1-3)
  * @retval None
  */
void SceneManager_DeleteScene(uint8_t preset, uint8_t sceneNum)
{
  if (!initialized)
  {
    printf("[SceneManager] ERROR: Not initialized\r\n");
    return;
  }
  
  if (preset >= SCENE_MANAGER_PRESET_COUNT || sceneNum < 2 || sceneNum > 3)
  {
    printf("[SceneManager] ERROR: Invalid preset %d or scene %d (only Scene 2 & 3 deleteable)\r\n", preset, sceneNum);
    return;
  }
  
  SceneData_t *scene = GetScenePointer(preset, sceneNum);
  if (scene == NULL)
  {
    printf("[SceneManager] ERROR: Failed to get scene pointer\r\n");
    return;
  }
  
  printf("[SceneManager] Deleting Scene %d for Preset %d\r\n", sceneNum, preset);
  
  /* Clear RAM copy */
  scene->programmed = 0;
  memset(scene->patchStatus, 0, sizeof(scene->patchStatus));
  
  /* Write entire preset scenes to flash (15 bytes) */
  FlashStorage_StatusTypeDef status = FlashStorage_WritePresetScenes(preset, &allScenes[preset]);
  
  if (status == FLASH_STORAGE_OK)
  {
    printf("[SceneManager] Scene deleted successfully\r\n");
  }
  else
  {
    printf("[SceneManager] ERROR: Flash write failed\r\n");
  }
}

/**
  * @brief  Decode GP-5 patch bitmap into PatchInfo structure
  * @note   GP-5 patch bitmap format (from bytes 35-38 of 4th patch info message):
  *         Bit 0:  CAB (Cabinet)
  *         Bit 1:  EQ (Equalizer)
  *         Bit 2:  MOD (Modulation)
  *         Bit 3:  DLY (Delay)
  *         Bit 8:  NR (Noise Reduction)
  *         Bit 9:  PRE (Preamp)
  *         Bit 10: DST (Distortion)
  *         Bit 11: AMP (Amplifier)
  *         Bit 24: RVB (Reverb)
  *         Bit 25: NS (Neural Amp)
  * @param  bitmap: 32-bit patch bitmap from GP-5
  * @param  patches: Pointer to PatchInfo_t structure to fill
  * @retval None
  */
void SceneManager_DecodePatchBitmap(uint32_t bitmap, PatchInfo_t *patches)
{
  if (patches == NULL)
  {
    return;
  }
  
  patches->patchCAB = (bitmap      ) & 0x01;  /* Bit 0 */
  patches->patchEQ  = (bitmap >>  1) & 0x01;  /* Bit 1 */
  patches->patchMOD = (bitmap >>  2) & 0x01;  /* Bit 2 */
  patches->patchDLY = (bitmap >>  3) & 0x01;  /* Bit 3 */
  patches->patchNR  = (bitmap >>  8) & 0x01;  /* Bit 8 */
  patches->patchPRE = (bitmap >>  9) & 0x01;  /* Bit 9 */
  patches->patchDST = (bitmap >> 10) & 0x01;  /* Bit 10 */
  patches->patchAMP = (bitmap >> 11) & 0x01;  /* Bit 11 */
  patches->patchRVB = (bitmap >> 24) & 0x01;  /* Bit 24 */
  patches->patchNS  = (bitmap >> 25) & 0x01;  /* Bit 25 */
}

/**
  * @brief  Encode PatchInfo structure into GP-5 patch bitmap
  * @param  patches: Pointer to PatchInfo_t structure
  * @retval 32-bit patch bitmap
  */
uint32_t SceneManager_EncodePatchBitmap(const PatchInfo_t *patches)
{
  if (patches == NULL)
  {
    return 0;
  }
  
  uint32_t bitmap = 0;
  
  if (patches->patchCAB) bitmap |= (1 << 0);   /* Bit 0 */
  if (patches->patchEQ)  bitmap |= (1 << 1);   /* Bit 1 */
  if (patches->patchMOD) bitmap |= (1 << 2);   /* Bit 2 */
  if (patches->patchDLY) bitmap |= (1 << 3);   /* Bit 3 */
  if (patches->patchNR)  bitmap |= (1 << 8);   /* Bit 8 */
  if (patches->patchPRE) bitmap |= (1 << 9);   /* Bit 9 */
  if (patches->patchDST) bitmap |= (1 << 10);  /* Bit 10 */
  if (patches->patchAMP) bitmap |= (1 << 11);  /* Bit 11 */
  if (patches->patchRVB) bitmap |= (1 << 24);  /* Bit 24 */
  if (patches->patchNS)  bitmap |= (1 << 25);  /* Bit 25 */
  
  return bitmap;
}

/**
  * @brief  Get raw scene data (for advanced use)
  * @param  preset: Preset number (0-99)
  * @param  sceneNum: Scene number (1-3)
  * @param  sceneData: Pointer to SceneData_t structure to fill
  * @retval None
  */
void SceneManager_GetSceneData(uint8_t preset, uint8_t sceneNum, SceneData_t *sceneData)
{
  if (!initialized || sceneData == NULL)
  {
    return;
  }
  
  if (preset >= SCENE_MANAGER_PRESET_COUNT || sceneNum < 1 || sceneNum > 3)
  {
    return;
  }
  
  SceneData_t *scene = GetScenePointer(preset, sceneNum);
  if (scene != NULL)
  {
    memcpy(sceneData, scene, sizeof(SceneData_t));
  }
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Get pointer to specific scene in RAM
  * @param  preset: Preset number (0-99)
  * @param  sceneNum: Scene number (2-3 only, Scene 1 not stored)
  * @retval Pointer to SceneData_t or NULL if invalid
  */
static SceneData_t* GetScenePointer(uint8_t preset, uint8_t sceneNum)
{
  if (preset >= SCENE_MANAGER_PRESET_COUNT || sceneNum < 2 || sceneNum > 3)
  {
    return NULL;
  }
  
  switch (sceneNum)
  {
    case 2:
      return &allScenes[preset].scene2;
    case 3:
      return &allScenes[preset].scene3;
    default:
      return NULL;
  }
}
