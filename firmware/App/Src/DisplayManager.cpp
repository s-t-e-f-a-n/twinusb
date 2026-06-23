/**
 * @file DisplayManager.cpp
 * @brief Implementation of the display service layer acting as a Facade for the hardware driver.
 * @details This layer decouples application business logic from the low-level
 * DisplayDriver, providing a simplified, thread-agnostic API for screen operations.
 * @author s-t-e-f-a-n
 * @date 12.06.2026
 */

#include "DisplayManager.hpp"
#include <cstdarg>
#include "DEV_Config.h"

using namespace h2h3;

// File-local storage for the I2C adapter used by the legacy Waveshare lib
static I2C_HandleTypeDef* g_hi2c = nullptr;
static uint16_t g_i2c_addr = 0;

// Adapter bridging the DEV_Config expected callback to HAL I2C
static void Display_Adapter(UBYTE value, UBYTE Cmd) {
    if (g_hi2c == nullptr) return;
    uint8_t tx_buf[2] = { (uint8_t)Cmd, (uint8_t)value };
    HAL_I2C_Master_Transmit(g_hi2c, (uint16_t)(g_i2c_addr << 1), tx_buf, 2, 100);
}


/**
 * @brief Constructs the DisplayManager and initializes the driver instance.
 */
DisplayManager::DisplayManager(I2C_HandleTypeDef* hi2c, uint8_t addr)
    : driver()
{
    // Save I2C interface and address for the legacy adapter.
    hi2c_ = hi2c;
    addr_ = addr;
    g_hi2c = hi2c_;
    g_i2c_addr = addr_;
}

/**
 * @brief Initializes the display hardware.
 * @details Delegates the hardware-specific startup sequence to the underlying DisplayDriver.
 */
void DisplayManager::init() {
    // Register the Waveshare-style adapter so the C driver can send I2C bytes.
    DEV_I2C_RegisterCallback(Display_Adapter);
    driver.init();
}

/**
 * @brief Clears the internal framebuffer.
 * @details Resets the pixel buffer state to black.
 */
void DisplayManager::clear() {
    driver.clear();
}

/**
 * @brief Synchronizes the local framebuffer with the physical display.
 * @details Flushes the current contents of the RAM buffer to the OLED via I2C.
 */
void DisplayManager::update() {
    driver.update();
}

/**
 * @brief Renders formatted text to the framebuffer at specified coordinates.
 * @details Uses a variadic argument list to format strings, which are then
 * rendered using the driver's font engine.
 * @param x The horizontal starting coordinate in pixels.
 * @param y The vertical starting coordinate in pixels.
 * @param format The C-style format string.
 * @param ... Variable arguments for the format string.
 */
void DisplayManager::printf(uint16_t x, uint16_t y, const char* format, ...) {
    va_list args;
    va_start(args, format);

    driver.printf_va(x, y, format, args); 
    
    va_end(args);
}

/**
 * @brief Renders a bitmap image to the framebuffer based on the provided format type.
 *
 * This method acts as a facade, routing the rendering call to the appropriate 
 * low-level hardware driver function. It supports both standard header-prefixed 
 * bitmaps and raw pixel data buffers.
 *
 * @param data Pointer to the source pixel data array.
 * @param x    The horizontal coordinate (in pixels) for the top-left corner.
 * @param y    The vertical coordinate (in pixels) for the top-left corner.
 * @param type The format of the data. 
 * - If @ref BitmapType::WITH_HEADER, the hardware driver parses the first 6 bytes.
 * - If @ref BitmapType::RAW, the method uses the provided @p w and @p h.
 * @param w    The width of the bitmap in pixels (required only for @ref BitmapType::RAW).
 * @param h    The height of the bitmap in pixels (required only for @ref BitmapType::RAW).
 *
 * @see BitmapType
 * @see DisplayDriver::draw_bitmap_with_header
 * @see DisplayDriver::draw_bitmap_raw
 */
void DisplayManager::drawBitmap(const uint8_t* data, uint16_t x, uint16_t y, BitmapType type, uint16_t w, uint16_t h, bool vertical) {
    if (type == BitmapType::WITH_HEADER) {
        driver.draw_bitmap_with_header(data, x, y);
    } else {
        driver.draw_bitmap_raw(data, x, y, w, h, vertical);
    }
}

