/**
 * @file ConsoleManager.hpp
 * @brief Unified interface for console I/O (UART/USB)
 * @author s-t-e-f-a-n
 * @date 01.06.2026
 */
#ifndef INC_CONSOLEMANAGER_HPP_
#define INC_CONSOLEMANAGER_HPP_

#include <cstdint>
#include <cstdio>
#include <cstdarg>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callback bridge to allow the C-based USB stack
 * to pass received bytes into the C++ logic.
 * @param byte The received data byte.
 */
void Console_OnCharReceived(uint8_t byte);

#ifdef __cplusplus
}
#endif

namespace h2h3 {

enum class ConsoleState { NOT_INITIALIZED, WAITING, READY };

/**
 * @class ConsoleManager
 * @brief Provides a static interface for system-wide logging and console I/O.
 */
class ConsoleManager {
public:

	static ConsoleState getState();
	static void setReady(bool state);

    /**
     * @brief Initializes the underlying communication peripheral (USB or UART).
     */
    static void init();

    /**
	 * @brief Checks if the console channel is initialized and ready for transmission.
	 * @return true if ready, false otherwise.
	 */
    static bool isReady();

    /**
     * @brief Formatted string output for text-based logging.
     * @param format The format string (same as printf).
     * @param ... Variable arguments.
     */
    static void print(const char* format, ...);

    /**
     * @brief Sends raw binary data over the active communication channel.
     * @param data Pointer to the buffer.
     * @param len Number of bytes to send.
     */
    static void sendBinary(const uint8_t* data, uint16_t len);
};

#ifdef DEBUG
    #define LOG_DEBUG(...) h2h3::ConsoleManager::print("[DEBUG] " __VA_ARGS__)
#else
    #define LOG_DEBUG(...) ((void)0)
#endif

} // namespace h2h3

#endif /* INC_CONSOLEMANAGER_HPP_ */
