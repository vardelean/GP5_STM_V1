cmake_minimum_required(VERSION 3.22)
# Enable CMake support for ASM and C languages
enable_language(C ASM)
# STM32CubeMX generated symbols (macros)
set(MX_Defines_Syms 
	USE_HAL_DRIVER 
	STM32G0B1xx
    $<$<CONFIG:Debug>:DEBUG>
)

# STM32CubeMX generated include paths
set(MX_Include_Dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../USB_Host/App
    ${CMAKE_CURRENT_SOURCE_DIR}/../../USB_Host/Target
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G0xx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G0xx_HAL_Driver/Inc/Legacy
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/STM32_USB_Host_Library/Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/STM32_USB_Host_Library/Class/HID/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/STM32_USB_Host_Library/Class/MIDI/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/CMSIS/Device/ST/STM32G0xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/CMSIS/Include
)

# STM32CubeMX generated application sources
set(MX_Application_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../USB_Host/Target/usbh_conf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../USB_Host/Target/usbh_platform.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../USB_Host/App/usb_host.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/gpio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/usart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/stm32g0xx_it.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/stm32g0xx_hal_msp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/sysmem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/syscalls.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/button_handler.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/led_controller.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/midi_manager.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/flash_storage.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/gp5_midi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/scene_manager.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../startup_stm32g0b1xx.s
)

# STM32 HAL/LL Drivers
set(STM32_Drivers_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/system_stm32g0xx.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_hcd.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_ll_usb.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_rcc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_rcc_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_ll_rcc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_flash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_flash_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_gpio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_dma.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_dma_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_pwr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_pwr_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_cortex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_exti.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_uart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_uart_ex.c
)

# Drivers Midllewares


set(USB_Host_Library_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_core.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_ctlreq.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_ioreq.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_pipes.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/STM32_USB_Host_Library/Class/MIDI/Src/usbh_midi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/STM32_USB_Host_Library/Class/HID/Src/usbh_hid.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/STM32_USB_Host_Library/Class/HID/Src/usbh_hid_keybd.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/STM32_USB_Host_Library/Class/HID/Src/usbh_hid_mouse.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/ST/STM32_USB_Host_Library/Class/HID/Src/usbh_hid_parser.c
)

# Link directories setup
set(MX_LINK_DIRS

)
# Project static libraries
set(MX_LINK_LIBS 
    STM32_Drivers
    ${TOOLCHAIN_LINK_LIBRARIES}
    USB_Host_Library
	
)
# Interface library for includes and symbols
add_library(stm32cubemx INTERFACE)
target_include_directories(stm32cubemx INTERFACE ${MX_Include_Dirs})
target_compile_definitions(stm32cubemx INTERFACE ${MX_Defines_Syms})

# Create STM32_Drivers static library
add_library(STM32_Drivers OBJECT)
target_sources(STM32_Drivers PRIVATE ${STM32_Drivers_Src})
target_link_libraries(STM32_Drivers PUBLIC stm32cubemx)


# Create USB_Host_Library static library
add_library(USB_Host_Library OBJECT)
target_sources(USB_Host_Library PRIVATE ${USB_Host_Library_Src})
target_link_libraries(USB_Host_Library PUBLIC stm32cubemx)

# Add STM32CubeMX generated application sources to the project
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${MX_Application_Src})

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE ${MX_LINK_DIRS})

# Add libraries to the project
target_link_libraries(${CMAKE_PROJECT_NAME} ${MX_LINK_LIBS})

# Add the map file to the list of files to be removed with 'clean' target
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES ADDITIONAL_CLEAN_FILES ${CMAKE_PROJECT_NAME}.map)

# Validate that STM32CubeMX code is compatible with C standard
if((CMAKE_C_STANDARD EQUAL 90) OR (CMAKE_C_STANDARD EQUAL 99))
    message(ERROR "Generated code requires C11 or higher")
endif()
