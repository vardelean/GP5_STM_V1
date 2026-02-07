/**
  ******************************************************************************
  * @file    debug.h
  * @brief   Debug utilities - conditionally compiled based on DEBUG macro
  ******************************************************************************
  */

#ifndef __DEBUG_H
#define __DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Debug printf - only active in Debug builds */
#ifdef DEBUG
  #include <stdio.h>
  #define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
  #define DEBUG_PRINTF(...) ((void)0)  /* No-op in Release */
#endif

#ifdef __cplusplus
}
#endif

#endif /* __DEBUG_H */
