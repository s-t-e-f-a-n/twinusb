/**
 * @file HardwareManager.hpp
 * @brief Manages base-level hardware initialization for the Twin USB Host Triple Hub platform.
 * @author s-t-e-f-a-n
 * @date 01.06.2026
 */
#ifndef INC_HARDWAREMANAGER_HPP_
#define INC_HARDWAREMANAGER_HPP_

#include "main.h"
#include "Types.hpp"
#include "LedDriver.hpp"

namespace h2h3 {

/** * @brief Represents detected physical button interaction events.
 */
enum class SwitchEvent : uint8_t {
    NONE = 0,       /**< No interaction */
    PRESSED,        /**< Button transitioned to active state */
    RELEASED,       /**< Button transitioned to idle state */
    SHORT_PRESS,    /**< Button held for a short duration */
    LONG_PRESS      /**< Button held for a prolonged duration */
};

/**
 * @class HardwareManager
 * @brief Handles core board power, mux, and hub reset sequencing.
 */
class HardwareManager {
public:
    /**
     * @brief Performs full power-on hardware initialization.
     * Configures MUX pins, disables VBUS/Hub, and performs initial I2C diagnostic scans.
     * @return The currently detected UpstreamType based on VBUS sensing.
     */
    static UpstreamType init();

    /**
     * @brief Provides access to the system status LED driver.
     * @return Reference to the system LedDriver instance.
     */
    static LedDriver& getStatusLed();

    /**
     * @brief Powers up the specified Upstream host and initializes the Hub.
     * Executes the sequence: Enable VBUS -> Start Hub reset release.
     * @param host The Upstream host to activate.
     */
    static void powerUpHost(UpstreamType host);

    /**
     * @brief Safely shuts down the active Upstream host and silences the Hub.
     * Asserts hub reset and disables all VBUS rails.
     */
    static void powerDownHost();

    /**
     * @brief Enables the VBUS power rail for a specific Upstream host.
     * Ensures an atomic switch by disabling the inactive rail before enabling the requested one.
     * @param target The target Upstream port to enable.
     */
    static void enableVbus(UpstreamType target);

    /**
     * @brief Configures the physical MUX routing and enables the data path.
     * @param target The target Upstream host (A or B) to route to the hub.
     */
    static void setMuxRoute(UpstreamType target);

    /**
     * @brief Safely disables the USB data path (Break).
     * Sets the MUX output-enable pin to high-impedance.
     */
    static void resetMuxRoute();

    /**
     * @brief Probes the active-low physical fault lines from the VBUS muxer.
     * @return UpstreamType indicating A, B, BOTH, or NONE if no fault is active.
     */
    static UpstreamType getActiveMuxFault();

    /**
     * @brief Cycle or toggle lines required to clear a latched hardware fault.
     * @details Resets the power distribution switches/muxer to clear the latched flag
     * after an over-current condition has been resolved.
     */
    static void clearMuxFault();

    /**
     * @brief Releases the Hub reset line (RESETn).
     */
    static void startHub();

    /**
     * @brief Asserts the Hub reset line (RESETn).
     */
    static void stopHub();

    /**
     * @return Pointer to I2C_HandleTypeDef for the Hub bus (I2C1).
     */
    static I2C_HandleTypeDef* getHubI2C();

    /**
     * @return Pointer to I2C_HandleTypeDef for the Display bus (I2C2).
     */
    static I2C_HandleTypeDef* getDisplayI2C();

    /**
     * @brief Validates hardware bus health.
     * @return true if both I2C buses are in the READY state.
     */
    static bool areBusesReady();

    /**
     * @brief Scans a specific I2C bus and reports active addresses.
     * @param i2c Pointer to the I2C_HandleTypeDef to scan.
     * @param busName Descriptive label for logs.
     */
    static void scanBus(I2C_HandleTypeDef* i2c, const char* busName);

    /**
     * @brief Captures GPIO transitions for the switch button.
     * @param GPIO_Pin The pin that triggered the interrupt.
     */
    static void handleGpioInterrupt(uint16_t GPIO_Pin);

    /**
     * @brief Retrieves and consumes the most recent switch interaction event.
     * @return The pending SwitchEvent enum.
     */
    static SwitchEvent getPendingEvent();

private:
    static LedDriver statusLed;                    /**< LED instance  */
    static volatile SwitchEvent pendingEvent;      /**< Stores event pending processing */
    static volatile uint32_t lastInterruptTick;    /**< Timestamp for debounce logic */
    static volatile uint32_t pressStartTime;       /**< Timestamp for duration measurement */
};

} // namespace h2h3


#endif /* INC_HARDWAREMANAGER_HPP_ */
