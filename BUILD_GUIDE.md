# Build Configuration Guide

## Build Types

This project supports two build configurations:

### Debug Build (Development)
- **Optimization**: `-O0` (no optimization for easier debugging)
- **Size**: ~76KB Flash
- **Features**:
  - Full serial debug output via UART2
  - All `DEBUG_PRINTF()` statements active
  - Debug symbols included
  - Easier to step through with debugger

**Build Commands:**
```powershell
cmake --preset Debug
cmake --build build/Debug
```

**Flash:**
```powershell
& 'C:/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe' -c port=SWD -w build/Debug/GP5_STM_V1.elf -v -rst
```

### Release Build (Production)
- **Optimization**: `-O3` (maximum speed optimization)
- **Size**: ~49KB Flash (**36% smaller!**)
- **Features**:
  - No serial output (UART disabled)
  - All `DEBUG_PRINTF()` compiled out (zero overhead)
  - No debug symbols
  - Optimized for speed and size

**Build Commands:**
```powershell
cmake --preset Release
cmake --build build/Release
```

**Flash:**
```powershell
& 'C:/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe' -c port=SWD -w build/Release/GP5_STM_V1.elf -v -rst
```

## Code Differences

### Debug Output
Use `DEBUG_PRINTF()` instead of `printf()` for conditional compilation:

```c
#include "debug.h"

DEBUG_PRINTF("This only prints in Debug builds\r\n");
DEBUG_PRINTF("Value: %d\r\n", some_value);
```

### Conditional Compilation
Use `#ifdef DEBUG` for debug-only code:

```c
#ifdef DEBUG
  // Debug-only code here
  MX_USART2_UART_Init();
#endif
```

## Memory Usage Comparison

| Build Type | Flash Used | RAM Used | Optimization |
|------------|------------|----------|--------------|
| **Debug**  | 76,116 B (62%) | 6,480 B (4.4%) | -O0 |
| **Release** | 48,780 B (40%) | 6,192 B (4.2%) | -O3 |

## Recommendations

- **Development**: Use Debug build for testing, debugging, and serial monitoring
- **Production**: Use Release build for final deployment to save Flash and maximize performance
- **Testing**: Always test Release build before deploying to ensure no debug-dependent behavior

## VS Code Tasks

The project includes VS Code tasks for both builds. Use:
- `Ctrl+Shift+B` → Select "Build" for Debug
- Or manually run "Flash and Run" task for Debug builds

For Release builds, use the PowerShell commands above.
