/******************************************************************************
**************************Hardware interface layer*****************************
* | file           : DEV_Config.h
* | version        : V1.0
* | date           : 2020-06-17
* | function       : Provide the hardware underlying interface
******************************************************************************/
#ifndef _DEV_CONFIG_H_
#define _DEV_CONFIG_H_

#include <stdint.h>
#include <stdlib.h>

/**
 * Data types
**/
#define UBYTE   uint8_t
#define UWORD   uint16_t
#define UDOUBLE uint32_t

/**
 * Configuration: Hardware I2C only
**/
#define USE_SPI_4W      0
#define USE_IIC         1
#define USE_IIC_SOFT    0

/* I2C Address: Usually 0x3C or 0x3D */
#define I2C_ADR         0X3C

/* Control bytes for SSD1315 */
#define IIC_CMD         0X00
#define IIC_RAM         0X40

/**
 * GPIO Macros: Neutralized
**/
#define OLED_CS_0       ((void)0)
#define OLED_CS_1       ((void)0)
#define OLED_DC_0       ((void)0)
#define OLED_DC_1       ((void)0)
#define OLED_RST_0      ((void)0)
#define OLED_RST_1      ((void)0)

/* Callback definition for architecture-independent I2C transfer */
typedef void (*I2C_Transfer_Callback)(UBYTE value, UBYTE Cmd);

#ifdef __cplusplus
extern "C" {
#endif

/* Function prototypes */
UBYTE   System_Init(void);
void    System_Exit(void);

/* Registration and I2C Interface */
void    DEV_I2C_RegisterCallback(I2C_Transfer_Callback cb);
void    I2C_Write_Byte(UBYTE value, UBYTE Cmd);

/* Timing functions */
void    Driver_Delay_ms(uint32_t xms);
void    Driver_Delay_us(uint32_t xus);

#ifdef __cplusplus
}
#endif

#endif
