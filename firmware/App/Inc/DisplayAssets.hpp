/**
 * @file DisplayAssets.hpp
 * @brief Graphical assets for the H2H3 OLED interface.
 * @details Contains 3x8 byte raw bitmap arrays for status icons.
 * @author s-t-e-f-a-n
 * @date 16.06.2026
 */

#ifndef INC_DISPLAYASSETS_HPP_
#define INC_DISPLAYASSETS_HPP_

#include <cstdint>

namespace h2h3 {
namespace assets {

// Logo Asset: H2H3
// Header: 0x01,0x01,0x40,0x00,0x1A,0x00 (6 bytes)
// Data: 256 bytes
static constexpr uint8_t img_logo[262] = { 
    0X01,0X01,0X40,0X00,0X1A,0X00,
    0X1F,0XFF,0XFC,0X00,0X1F,0XFF,0XFC,0X00,0X1F,0XFF,0XFC,0X00,0X1F,0XFF,0XFC,0X00,
    0X1F,0XFF,0XFC,0X00,0X1F,0XFF,0XFC,0X00,0X00,0X3E,0X00,0X00,0X00,0X3E,0X00,0X00,
    0X00,0X3E,0X00,0X00,0X00,0X3E,0X00,0X00,0X00,0X3E,0X00,0X00,0X00,0X3E,0X00,0X00,
    0X00,0X3E,0X00,0X00,0X1F,0XFF,0XFC,0X00,0X1F,0XFF,0XFC,0X00,0X1F,0XFF,0XFC,0X00,
    0X1F,0XFF,0XFC,0X00,0X1F,0XFF,0XFC,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
    0X00,0X00,0X00,0X00,0X00,0X00,0X40,0X40,0X00,0X01,0XC1,0XC0,0X00,0X01,0XC3,0XC0,
    0X00,0X03,0X87,0XC0,0X00,0X03,0X0F,0XC0,0X00,0X03,0X9D,0XC0,0X00,0X03,0XFD,0XC0,
    0X00,0X01,0XF9,0XC0,0X00,0X01,0XF1,0XC0,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
    0X00,0X00,0X00,0X00,0X1F,0XFF,0XFC,0X00,0X1F,0XFF,0XFC,0X00,0X1F,0XFF,0XFC,0X00,
    0X1F,0XFF,0XFC,0X00,0X1F,0XFF,0XFC,0X00,0X1F,0XFF,0XFC,0X00,0X00,0X3E,0X00,0X00,
    0X00,0X3E,0X00,0X00,0X00,0X3E,0X00,0X00,0X00,0X3E,0X00,0X00,0X00,0X3E,0X00,0X00,
    0X00,0X3E,0X00,0X00,0X00,0X3E,0X00,0X00,0X1F,0XFF,0XFC,0X00,0X1F,0XFF,0XFC,0X00,
    0X1F,0XFF,0XFC,0X00,0X1F,0XFF,0XFC,0X00,0X1F,0XFF,0XFC,0X00,0X1F,0XFF,0XFC,0X00,
    0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X60,0XE0,0X00,0X00,
    0X60,0XE0,0X00,0X00,0XE0,0XF0,0X00,0X00,0XCC,0X30,0X00,0X00,0XCC,0X30,0X00,0X00,
    0XFE,0X70,0X00,0X00,0X7F,0XE0,0X00,0X00,0X73,0XE0,0X00,0X00,0X01,0X80,0X00,0X00
};

// Each icon is 3 rows x 8 bytes (64x3 bits effective area)
// Represented as uint8_t[3][8]

static constexpr uint8_t img_not_connected[3][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

static constexpr uint8_t img_connected_not_selected[3][8] = {
    {0xE1, 0x86, 0x18, 0x61, 0x86, 0x18, 0x61, 0x87},
    {0xE1, 0x86, 0x18, 0x61, 0x86, 0x18, 0x61, 0x87},
    {0xE1, 0x86, 0x18, 0x61, 0x86, 0x18, 0x61, 0x87}
};

static constexpr uint8_t img_selected_charger[3][8] = {
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
};

static constexpr uint8_t img_selected_host[3][8] = {
    {0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07},
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07}
};

static constexpr uint8_t img_selected_unknown[3][8] = {
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
};

// 
struct Icon {
    const uint8_t* data;
    uint16_t w;
    uint16_t h;
};

// Asset Catalog
static constexpr Icon icon_not_connected          = { &img_not_connected[0][0], 64, 3 };
static constexpr Icon icon_connected_not_selected = { &img_connected_not_selected[0][0], 64, 3 };
static constexpr Icon icon_selected_charger       = { &img_selected_charger[0][0], 64, 3 };
static constexpr Icon icon_selected_host          = { &img_selected_host[0][0], 64, 3 };
static constexpr Icon icon_selected_unknown          = { &img_selected_unknown[0][0], 64, 3 };

} // namespace assets
} // namespace h2h3

#endif /* INC_DISPLAYASSETS_HPP_ */