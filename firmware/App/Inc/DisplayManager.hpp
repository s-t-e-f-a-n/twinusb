/**
 * @file DisplayManager.hpp
 * @brief High-level facade for display orchestration and UI rendering.
 * @details This class provides a clean, thread-agnostic API for the application
 * layer, abstracting the underlying hardware driver and font engine.
 * @author s-t-e-f-a-n
 * @date 13.06.2026
 */

#ifndef INC_DISPLAYMANAGER_HPP_
#define INC_DISPLAYMANAGER_HPP_

#include "stm32g0xx_hal.h"
#include "DisplayDriver.hpp"
#include <cstdint>

namespace h2h3 {

/**
 * @brief Defines the supported formats for bitmap rendering operations.
 */
enum class BitmapType { WITH_HEADER, RAW };

/**
 * @class DisplayManager
 * @brief Facade for the hardware driver to simplify display operations.
 */
class DisplayManager {
public:
    /**
     * @brief Constructs a new DisplayManager.
     * @details Initializes the internal DisplayDriver instance.
     */
	DisplayManager(I2C_HandleTypeDef* hi2c, uint8_t addr);

    /** @brief Initializes the display hardware and GUI environment. */
    void init();

    /** @brief Clears the internal framebuffer to black. */
    void clear();

    /** @brief Flushes the local framebuffer to the physical OLED display. */
    void update();

    /**
     * @brief Updates the active font globally via the driver.
     * @param font Pointer to the target sFONT.
     */
    void setFont(const sFONT* font) { driver.setFont(font); }
    
    /**
     * @brief Renders formatted text to the framebuffer.
     * @param x Horizontal starting coordinate in pixels.
     * @param y Vertical starting coordinate in pixels.
     * @param format C-style format string.
     * @param ... Variable arguments for formatting.
     */
    void printf(uint16_t x, uint16_t y, const char* format, ...);

    /**
     * @brief Renders a bitmap image to the display buffer.
     * * This method serves as a unified interface for rendering both pre-formatted 
     * bitmaps (with a 6-byte header) and raw pixel data.
     * * @param data Pointer to the start of the bitmap/icon byte array.
     * @param x    The horizontal coordinate (in pixels) for the top-left corner.
     * @param y    The vertical coordinate (in pixels) for the top-left corner.
     * @param type The format of the bitmap data (BitmapType::WITH_HEADER or BitmapType::RAW).
     * Defaults to BitmapType::WITH_HEADER.
     * @param w    The width of the bitmap in pixels. Only required if type is BitmapType::RAW.
     * Defaults to 0.
     * @param h    The height of the bitmap in pixels. Only required if type is BitmapType::RAW.
     * Defaults to 0.
     * * @note If @p type is set to @ref BitmapType::WITH_HEADER, the method expects the first 
     * 6 bytes of the @p data array to contain width, height, and format information 
     * as required by the hardware driver.
     * * @warning If @p type is set to @ref BitmapType::RAW, ensure that @p w and @p h 
     * are correctly provided, otherwise the rendering may be clipped or 
     * result in memory access errors.
     */
    void drawBitmap(const uint8_t* data, uint16_t x, uint16_t y, 
                    BitmapType type = BitmapType::WITH_HEADER, 
                    uint16_t w = 0, uint16_t h = 0,
                    bool vertical = false);

private:
    DisplayDriver driver; /**< Encapsulated hardware driver instance */
    I2C_HandleTypeDef* hi2c_; /**< I2C handle used for display transfers */
    uint8_t addr_; /**< I2C 7-bit address of the display */
};

} // namespace h2h3

#endif /* INC_DISPLAYMANAGER_HPP_ */
