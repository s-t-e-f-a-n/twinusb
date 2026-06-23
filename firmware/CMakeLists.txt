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
set(CMAKE_PROJECT_NAME twinusb.stm32)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME} C CXX ASM)
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

# ====================================================================
# USER SOURCES SETUP (DYNAMIC GLOBBING)
# ====================================================================

# Automatically find application sources across your custom structure
file(GLOB_RECURSE ALL_SOURCES 
    "Core/Src/*.c"
    "Core/Src/*.cpp"
    "Core/Src/*.s"
    "App/Src/*.c"
    "App/Src/*.cpp"
    "Libs/oled_waveshare/*.c"
    "Libs/oled_waveshare/*.cpp"
    "USB_Device/App/*.c"
)

# Crucial: Exclude system configuration files managed by the stm32cubemx subdirectory
list(FILTER ALL_SOURCES EXCLUDE REGEX ".*system_stm32g0xx.*")
list(FILTER ALL_SOURCES EXCLUDE REGEX ".*Templates.*")

# EXCLUDE UNUSED SOFTWARE I2C TO PREVENT COMPILER ERRORS
list(FILTER ALL_SOURCES EXCLUDE REGEX ".*Soft_IIC.*")

# Add discovered application sources to executable target
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${ALL_SOURCES}
)

# ====================================================================
# USER INCLUDES SETUP (DYNAMIC GLOBBING)
# ====================================================================

# Automatically find all directories containing header files (.h, .hpp)
file(GLOB_RECURSE HEADER_FILES "*.h" "*.hpp")
set(HEADER_DIRS "")

foreach(FILE_PATH ${HEADER_FILES})
    # Ignore build artifacts and old IDE output folders completely
    if(NOT FILE_PATH MATCHES ".*/build/.*" AND 
       NOT FILE_PATH MATCHES ".*/Debug/.*" AND 
       NOT FILE_PATH MATCHES ".*/Release/.*" AND
       NOT FILE_PATH MATCHES ".*/.bak/.*")
        
        get_filename_component(DIR_PATH ${FILE_PATH} DIRECTORY)
        list(APPEND HEADER_DIRS ${DIR_PATH})
    endif()
endforeach()

# Remove duplicate folder paths
list(REMOVE_DUPLICATES HEADER_DIRS)

# Add these folders to your project dynamically
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${HEADER_DIRS}
)


# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Disable specific warnings for the legacy Waveshare font library
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
    -Wno-missing-braces
    -Wno-write-strings
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
