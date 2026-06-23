/**
 * @file Bootloader.hpp
 * @brief Bootloader entry request declarations.
 * @author s-t-e-f-a-n
 * @date 14.06.2026
 */
#ifndef INC_BOOTLOADER_HPP_
#define INC_BOOTLOADER_HPP_

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Evaluate the hardware bootloader request at reset.
 * @details This routine is executed before application initialization.
 *          If the bootloader switch is active, the MCU transfers
 *          execution to the internal system bootloader.
 */
void CheckBootloaderSwitch(void);

#ifdef __cplusplus
}
#endif

#endif /* INC_BOOTLOADER_HPP_ */
