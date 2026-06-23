/**
 * @file Bootloader.cpp
 * @brief Bootloader request handling for STM32G0 application startup.
 * @author s-t-e-f-a-n
 * @date 14.06.2026
 *
 * This module implements a hardwired bootloader entry sequence that is
 * evaluated early in reset before full application initialization. When the
 * designated switch pin is asserted at startup, control is transferred to the
 * STM32 system bootloader in system flash.
 */

#include "Bootloader.hpp"
#include "main.h"

namespace {

/**
 * @brief Internal bootloader entry address for STM32G0 series.
 * @note  The system bootloader ROM is mapped at 0x1FFF0000.
 */
static constexpr uint32_t kSystemBootloaderAddress = 0x1FFF0000u;

/**
 * @brief Configure the physical bootloader switch pin as input.
 * @details Uses the HAL GPIO configuration helpers for clarity and
 *          to keep hardware pin definitions centralized in main.h.
 */
static void ConfigureBootloaderPin(void) {
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef gpioInit = {0};
    gpioInit.Pin = SWITCH_Pin;
    gpioInit.Mode = GPIO_MODE_INPUT;
    gpioInit.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(SWITCH_GPIO_Port, &gpioInit);
}

/**
 * @brief Read the raw state of the bootloader switch.
 * @return true when the switch is active-high and pressed.
 */
static bool IsBootloaderSwitchActive(void) {
    return (HAL_GPIO_ReadPin(SWITCH_GPIO_Port, SWITCH_Pin) == GPIO_PIN_SET);
}

} // namespace

/**
 * @brief Jump to the STM32 system bootloader.
 * @details
 *  - Disables interrupts.
 *  - Stops the SysTick timer.
 *  - Clears pending NVIC interrupts.
 *  - Remaps system flash into memory space.
 *  - Sets the MSP from the bootloader vector table.
 *  - Transfers control to the system bootloader reset handler.
 */
void JumpToSystemBootloader(void) {
    using BootloaderEntry = void(*)(void);

    __disable_irq();

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    for (uint8_t i = 0; i < 8; ++i) {
        NVIC->ICER[i] = 0xFFFFFFFFu;
        NVIC->ICPR[i] = 0xFFFFFFFFu;
    }

    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH();

    __set_MSP(*reinterpret_cast<volatile uint32_t*>(kSystemBootloaderAddress));
    BootloaderEntry bootEntry = reinterpret_cast<BootloaderEntry>(
        *reinterpret_cast<volatile uint32_t*>(kSystemBootloaderAddress + 4));
    bootEntry();

    while (true) {
        __NOP();
    }
}

/**
 * @brief Check the bootloader switch at reset and branch if requested.
 * @details The switch pin is initialized as a simple input before the
 *          application begins. If the request is detected, this function
 *          performs a safe transfer into the internal system bootloader.
 */
void CheckBootloaderSwitch(void) {
    ConfigureBootloaderPin();

    // Short debounce delay while the pin stabilizes after reset.
    for (volatile int i = 0; i < 2000; ++i) {
        __NOP();
    }

    if (IsBootloaderSwitchActive()) {
        __disable_irq();
        JumpToSystemBootloader();
    }
}
