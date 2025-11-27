cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME beta)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    Core/BSP/Bsp_System/bsp_System.c
    Core/BSP/Bsp_IO/bsp_IO.c
    Core/BSP/Bsp_SPI/bsp_spi.c
    Core/BSP/Bsp_IIC/bsp_iic.c
    Core/BSP/Bsp_uart/bsp_uart.c
    
    # ICM42688P IMU Library
    Core/Lib/icm42688p/icm42688p.c
    Core/Lib/icm42688p/icm42688p_lib.c
    
    # BMP280 Barometer Library
    Core/Lib/bmp280/bmp280.c
    Core/Lib/bmp280/bmp280_lib.c
    
    # HMC5883L Magnetometer Library
    Core/Lib/hmc5883l/hmc5883l.c
    Core/Lib/hmc5883l/hmc5883l_lib.c
    
    # VL53L0X ToF Distance Sensor Library
    Core/Lib/tof/tof.c
    Core/Lib/tof/core/src/vl53l0x_api.c
    Core/Lib/tof/core/src/vl53l0x_api_calibration.c
    Core/Lib/tof/core/src/vl53l0x_api_core.c
    Core/Lib/tof/core/src/vl53l0x_api_ranging.c
    Core/Lib/tof/core/src/vl53l0x_api_strings.c
    
    # ELRS/CRSF UART helper
    Core/Lib/esrl/elrs_crsf_uart.c
    Core/Lib/esrl/elrs_crsf_port.c
    
    # PID Controller
    Core/Control/PID/pid.c

    # Attitude Estimator
    "Core/Control/Attitude Control/attitude.c"

    # Filters and maths utilities
    Core/Control/Filter/filter.c
    Core/Control/Tools/maths.c

    # Control tasks
    Core/Control/Tasks/task_register.c
    Core/Control/Tasks/scheduler.c
    Core/Control/Tasks/task_gyro.c
    Core/Control/Tasks/task_acc.c
    Core/Control/Tasks/task_mag.c
    Core/Control/Tasks/task_filter.c

    # On-board tests
    Core/Test/test_gyro.c
    Core/Test/test_attitude_full.c
    Core/Test/test_mag.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    Core/BSP/Bsp_System
    Core/BSP/Bsp_IO
    Core/BSP/Bsp_SPI
    Core/BSP/Bsp_IIC            # I2C BSP
    Core/BSP/Bsp_uart           # UART BSP
    Core/Src                    # bsp_pins.h
    Core/Lib/icm42688p          # ICM42688P library
    Core/Lib/bmp280             # BMP280 library
    Core/Lib/hmc5883l           # HMC5883L library
    Core/Lib/tof                # VL53L0X ToF library
    Core/Lib/tof/core/inc       # VL53L0X core API headers
    Core/Lib/tof/platform/inc   # VL53L0X platform headers
    Core/Lib/esrl               # ELRS/CRSF helper
    Core/Control/PID            # PID controller
    Core/Control/Filter         # Filters
    Core/Control/Tools          # Math helpers
    Core/Control/Tasks          # Control tasks
    "Core/Control/Attitude Control"  # Attitude module
    Core/Test                   # On-board tests
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
    USE_HAL_DRIVER
    ICM_USE_DMA             # Enable DMA for ICM42688P (启用后使用DMA模式)
    USE_UART1               # Enable UART1 BSP
    
    # 注意：如果遇到DMA问题，可以注释掉ICM_USE_DMA，回退到轮询模式
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)

# Enable float formatting for printf
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE -u _printf_float)

# Create .bin and .hex after linking and print size
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
)
