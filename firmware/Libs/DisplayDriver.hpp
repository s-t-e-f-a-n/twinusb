#ifndef DISPLAYDRIVER_HPP
#define DISPLAYDRIVER_HPP

extern "C" {
	#include "OLED_0in49.h"
	#include "GUI_Paint.h"
}
#include <cstdio>
#include <cstdarg>
#include "DEV_Config.h"

// Define display dimensions at compile-time
#define DISP_WIDTH  64
#define DISP_HEIGHT 32

// Calculate buffer size at compile-time
// 64 * 32 / 8 = 256 bytes for a 1-bit monochrome display
#define BUFFER_SIZE ((DISP_WIDTH * DISP_HEIGHT) / 8)

// Use packed structure to ensure the header matches your 6-byte binary format exactly
// This is a header format output by Image2Lcd (v2.9) distributed by WaveShare https://www.waveshare.com/wiki/OLED_Draw
#pragma pack(push, 1)
struct BitmapHeader {
    uint8_t  scanMode;  // Byte 0: 0=Horiz, 1=Vert, 2=DataHoriz/ByteVert, 3=DataVert/ByteHoriz
    uint8_t  bitDepth;  // Byte 1: 1=Monochrome, 2=4-Color, etc.
    uint16_t width;     // Bytes 2-3: Image width
    uint16_t height;    // Bytes 4-5: Image height
};
#pragma pack(pop)

class DisplayDriver {
private:
    uint8_t buffer[BUFFER_SIZE];        // Display frame buffer
    const sFONT* activeFont = &Font8;   // Encapsulated font state

public:
    // Constructor with setting the initial font
    DisplayDriver() : activeFont(&Font12) {}

    // Initialize GUI with 64x32 dimensions
    // Note: 0 rotation, WHITE text/graphics on BLACK background
    void init() {
        OLED_0in49_Init();
        Paint_NewImage(buffer, DISP_HEIGHT, DISP_WIDTH, 270, BLACK);
        Paint_Clear(BLACK);
    }

    void clear() {
        Paint_Clear(BLACK);
    }

    // Transfers the buffer to the display RAM
    void update() {
        OLED_0in49_Display(buffer);
    }

    // Wrapper for drawing lines
    void draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
        Paint_DrawLine(x1, y1, x2, y2, color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    }

    /**
     * @brief Configures the active font for subsequent rendering.
     * @param font Pointer to the sFONT structure (e.g., &Font8, &Font12).
     */
    void setFont(const sFONT* font) { if (font) activeFont = font; }

    /**
     * @brief Performs formatted text rendering using an existing va_list.
     * @details This is the worker method that manages the string buffer and rendering.
     * @param x Horizontal coordinate.
     * @param y Vertical coordinate.
     * @param format C-style format string.
     * @param args Pre-initialized variadic argument list.
     */
    void printf_va(uint16_t x, uint16_t y, const char* format, va_list args) {
        char buf[64];
        vsnprintf(buf, sizeof(buf), format, args);
        Paint_DrawString_EN(x, y, buf, const_cast<sFONT*>(activeFont), WHITE, BLACK);
    }

    // Draw raw text at the given position
    void draw_text(uint16_t x, uint16_t y, const char* text) {
        Paint_DrawString_EN(x, y, text, const_cast<sFONT*>(activeFont), WHITE, BLACK);
    }

    // Draw a single pixel
    void draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
        Paint_SetPixel(x, y, color);
    }

	/**
     * @brief Renders raw pixel data (headerless) to the display buffer.
     * @details Interprets the provided data as either a horizontal or vertical-scan bitmap.
     * @param data      Pointer to the raw bitmap array.
     * @param x         Horizontal coordinate (in pixels).
     * @param y         Vertical coordinate (in pixels).
     * @param width     Width of the bitmap in pixels.
     * @param height    Height of the bitmap in pixels.
     * @param vertical  If true, uses vertical scan logic; if false, uses horizontal.
     */
    void draw_bitmap_raw(const uint8_t* data, uint16_t x, uint16_t y, 
                         uint16_t width, uint16_t height, bool vertical = false) {
        if (data == nullptr || width == 0 || height == 0) return;
        
        if (vertical) {
            draw_bitmap_vertical(data, x, y, width, height);
        } else {
            draw_bitmap_horizontal(data, x, y, width, height);
        }
    }

	/**
	 * @brief Parses the header and directs to the appropriate scanning algorithm.
	 * @param bitmap Pointer to the start of the bitmap array (header + data).
	 * @param x Target X-coordinate.
	 * @param y Target Y-coordinate.
	 */
	void draw_bitmap_with_header(const uint8_t* bitmap, uint16_t x, uint16_t y) {
		const BitmapHeader* header = reinterpret_cast<const BitmapHeader*>(bitmap);
		const uint8_t* pixelData = bitmap + sizeof(BitmapHeader);

		// Basic validation: Check bitDepth and ensure valid dimensions
		if (header->bitDepth != 1 || header->width == 0 || header->height == 0) return;

		// Route to the correct scanning logic based on the detected scanMode
		switch (header->scanMode) {
			case 0: // Horizontal Scan
				draw_bitmap_horizontal(pixelData, x, y, header->width, header->height);
				break;
			case 1: // Vertical Scan
				draw_bitmap_vertical(pixelData, x, y, header->width, header->height);
				break;
			default:
				// Other modes (2 and 3) could be implemented here as needed
				break;
		}
	}

private:
	/**
	 * @brief Renders a horizontal scan monochrome bitmap with clipping.
	 */
	void draw_bitmap_horizontal(const uint8_t* data, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
		uint16_t bytesPerRow = (w + 7) / 8;
		for (uint16_t j = 0; j < h; j++) {
			for (uint16_t i = 0; i < w; i++) {
				if ((x + i) < DISP_WIDTH && (y + j) < DISP_HEIGHT) {
					bool bitSet = (data[(j * bytesPerRow) + (i / 8)] & (1 << (7 - (i % 8))));
					Paint_SetPixel(x + i, y + j, bitSet ? WHITE : BLACK);
				}
			}
		}
	}

	/**
	 * @brief Renders a vertical scan monochrome bitmap with clipping.
	 */
	void draw_bitmap_vertical(const uint8_t* data, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
		uint16_t bytesPerCol = (h + 7) / 8;
		for (uint16_t j = 0; j < h; j++) {
			for (uint16_t i = 0; i < w; i++) {
				if ((x + i) < DISP_WIDTH && (y + j) < DISP_HEIGHT) {
					bool bitSet = (data[(i * bytesPerCol) + (j / 8)] & (1 << (7 - (j % 8))));
					Paint_SetPixel(x + i, y + j, bitSet ? WHITE : BLACK);
				}
			}
		}
	}

};

#endif
