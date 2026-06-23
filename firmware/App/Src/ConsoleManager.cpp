/**
 * @file ConsoleManager.cpp
 * @brief Unified interface for console I/O (UART/USB)
 * @author s-t-e-f-a-n
 * @date 01.06.2026
 */

#include "ConsoleManager.hpp"
#include "main.h"
#include <cstdarg>

/* Externs for USB middleware to ensure C-linkage */
extern "C" {
    #include "usbd_core.h"
    #include "usbd_cdc_if.h"

	/* Device handle and configuration objects */
    extern USBD_HandleTypeDef hUsbDeviceFS;
    extern USBD_DescriptorsTypeDef CDC_Desc;
    extern USBD_CDC_ItfTypeDef USBD_Interface_fops_FS;
    extern PCD_HandleTypeDef hpcd_USB_DRD_FS;

    /* Bridge for CDC transmission */
    uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len);
}

/* External dependencies */
extern UART_HandleTypeDef huart2;
extern uint8_t debugRxByte;

/* Internal state tracking and circular buffer for asynchronous RX handling */
static volatile h2h3::ConsoleState g_console_state = h2h3::ConsoleState::NOT_INITIALIZED;
static volatile uint8_t rx_buffer[128];
static volatile uint16_t head_ptr = 0;
static volatile uint16_t tail_ptr = 0;

namespace h2h3 {

// Define the state variable
static volatile ConsoleState g_console_state = ConsoleState::NOT_INITIALIZED;

/**
 * @brief Initializes communication peripherals based on the build configuration.
 * Configures either USB (CDC) or UART depending on the CONSOLE_TYPE macro.
 */
void ConsoleManager::init() {
#if CONSOLE_TYPE == CONSOLE_USB
	g_console_state = ConsoleState::WAITING;
	/* Safe shutdown interrupts to prevent bus contention */
	__HAL_RCC_USB_CLK_DISABLE();
	HAL_NVIC_DisableIRQ(USB_UCPD1_2_IRQn);
	__HAL_RCC_USB_CLK_ENABLE();

	memset(&hUsbDeviceFS, 0, sizeof(USBD_HandleTypeDef));
	hUsbDeviceFS.pData = &hpcd_USB_DRD_FS;
	hpcd_USB_DRD_FS.pData = &hUsbDeviceFS;
	hpcd_USB_DRD_FS.Init.vbus_sensing_enable = DISABLE;

	if (HAL_PCD_Init(&hpcd_USB_DRD_FS) != HAL_OK) return;

	/* Initialize USB Middleware */
    USBD_Init(&hUsbDeviceFS, &CDC_Desc, DEVICE_FS);
    USBD_RegisterClass(&hUsbDeviceFS, &USBD_CDC);
    USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_Interface_fops_FS);
    USBD_Start(&hUsbDeviceFS);

    /* Configure Interrupts */
    HAL_NVIC_SetPriority(USB_UCPD1_2_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(USB_UCPD1_2_IRQn);
#elif CONSOLE_TYPE == CONSOLE_UART
    g_console_state = ConsoleState::READY;
    __HAL_UART_CLEAR_FLAG(&huart2, UART_CLEAR_OREF | UART_CLEAR_NEF);
    HAL_UART_Receive_IT(&huart2, &::debugRxByte, 1);
#endif
}

/**
 * @brief Checks if the communication interface is fully enumerated and ready.
 * @return true if the console is ready to transmit, false otherwise.
 */
bool ConsoleManager::isReady() {
    return (g_console_state == ConsoleState::READY);
}

/**
 * @brief Retrieves the current state of the console.
 * @return The current ConsoleState.
 */
ConsoleState ConsoleManager::getState() {
    return g_console_state;
}

/**
 * @brief Updates the console readiness state.
 * This should be called by the USB CDC callback (e.g., SET_CONTROL_LINE_STATE).
 * @param state Set to true when the host opens the terminal.
 */
void ConsoleManager::setReady(bool state) {
    g_console_state = state ? ConsoleState::READY : ConsoleState::WAITING;
}

/**
 * @brief Formatted string output using standard printf-style arguments.
 * Silently drops output if the console is not yet ready.
 * @param format Printf-style format string.
 * @param ... Arguments for the format string.
 */
void ConsoleManager::print(const char* format, ...) {
	if (!isReady()) return;

	char buffer[128];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (len > 0) {
    	__disable_irq();
        sendBinary(reinterpret_cast<uint8_t*>(buffer), static_cast<uint16_t>(len));
        __enable_irq();
    }
}

/**
 * @brief Sends binary data to the active interface.
 * @param data Pointer to the binary buffer.
 * @param len Number of bytes to send.
 */
void ConsoleManager::sendBinary(const uint8_t* data, uint16_t len) {
#if CONSOLE_TYPE == CONSOLE_UART
    HAL_UART_Transmit(&huart2, const_cast<uint8_t*>(data), len, HAL_MAX_DELAY);
#elif CONSOLE_TYPE == CONSOLE_USB
    if (!isReady()) return;

	uint32_t timeout = 0;
	const uint32_t MAX_TIMEOUT = 100000;

	// Attempt to send the entire block at once
	uint8_t result = CDC_Transmit_FS(const_cast<uint8_t*>(data), len);

	// Only block if the buffer is currently busy
	while (result == USBD_BUSY) {
		if (++timeout > MAX_TIMEOUT) {
			// Log failure or handle broken connection
			setReady(false);
			break;
		}
		// Yield/Retry
		result = CDC_Transmit_FS(const_cast<uint8_t*>(data), len);
	}
#endif
}

} // namespace h2h3

/**
 * @brief Bridge function for the C-based USB stack to feed data into the C++ RX buffer.
 * @param byte The received character byte.
 */
extern "C" void Console_OnCharReceived(uint8_t byte) {
    uint16_t next_head = (head_ptr + 1) % 128;
    if (next_head != tail_ptr) {
        rx_buffer[head_ptr] = byte;
        head_ptr = next_head;
    }
}
