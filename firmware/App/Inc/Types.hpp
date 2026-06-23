/**
 * @file Types.hpp
 * @brief Global data structures and enumerations for the system.
 * @details Central source of truth for shared system types.
 * This file is dependency-free to prevent circular includes.
 * @author s-t-e-f-a-n
 * @date 12.06.2026
 */

#ifndef INC_TYPES_HPP_
#define INC_TYPES_HPP_

#include <cstdint>
#include "stm32g0xx_hal.h"

namespace h2h3 {

/**
 * @struct Telemetry
 * @brief Container for physical sensor readings.
 */
struct Telemetry {
    struct { uint32_t vbusa_mv; uint32_t vbusb_mv; uint32_t vbusds_mv; } voltage;
    struct { uint32_t ds1_ma; uint32_t ds2_ma; uint32_t ds3_ma; } current;
};

    /**
 * @brief Returns the system uptime in milliseconds.
 * @details Wrapper for HAL_GetTick() to provide a standard millis() interface.
 * @return Current system tick count in milliseconds.
 */
inline uint32_t millis() {
    return HAL_GetTick();
}

/**
 * @brief Represents the available Upstream ports.
 */
enum class UpstreamType : uint8_t {
    NONE = 0,   ///< No host selected
    A,          ///< Upstream Host A
    B,           ///< Upstream Host B
	BOTH
};

/**
 * @brief Represents the physical VBUS presence state.
 */
enum class VbusPresenceStatus : uint8_t {
    NONE   = 0x00,
    HOST_A = 0x01,
    HOST_B = 0x02,
    BOTH   = 0x03
};

/**
 * @brief Represents the logical connection status.
 */
enum class ConnectionState : uint8_t {
    IDLE,
    NEGOTIATING,
    CONFIGURED,
	SWITCH_TOGGLE,
	SWITCH_RECONNECT,
	FAULT
};

/**
 * @brief Represents the visual connection status of a host for the UI.
 * @details This maps the complex physical states (VBUS, Data, Negotiation)
 * into simple visual categories for the display.
 */
enum class LinkStatus : uint8_t {
    NOT_CONNECTED,           ///< No VBUS detected
    CONNECTED_IDLE,          ///< VBUS detected, not active
    CONNECTED_ACTIVE_UNKNOWN,///< Active, but not yet identified as Host/Charger
    CONNECTED_HOST,          ///< Active and identified as Host
    CONNECTED_CHARGER        ///< Active and identified as Charger
};

/**
 * @struct SystemStatus
 * @brief Snapshot of the system state for UI/Orchestration.
 */
struct SystemStatus {
    UpstreamType activeUpstream;
    ConnectionState usbState;
    VbusPresenceStatus vbusPresence;
    bool isDataPathActive;
    uint32_t activeAlerts;
    Telemetry telemetry;

    /**
     * @brief Constructor to ensure all status flags are initialized to safe defaults.
     */
    SystemStatus(
        UpstreamType upstream = UpstreamType::NONE,
        ConnectionState state = ConnectionState::IDLE,
        VbusPresenceStatus vbus = VbusPresenceStatus::NONE,
        bool dataActive = false,
        uint32_t alerts = 0,
        Telemetry tel = {}
    ) : activeUpstream(upstream),
        usbState(state),
        vbusPresence(vbus),
        isDataPathActive(dataActive),
        activeAlerts(alerts),
        telemetry(tel) {}
};

/**
 * @brief Represents the UI display modes.
 */
enum class UIState {
    STARTUP,
    OPERATIONAL,
    FAULT,
    SCREENSAVER
};

/**
 * @brief Represents the UI telemetry page states.
 */
enum class SubState {
    PAGE_A,
    PAGE_B,
    PAGE_C
};

} // namespace h2h3

#endif /* INC_TYPES_HPP_ */
