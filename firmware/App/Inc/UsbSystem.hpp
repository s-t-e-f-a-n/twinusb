/**
 * @file UsbSystem.hpp
 * @brief High-level orchestration for USB Upstream/Downstream port management.
 * @author s-t-e-f-a-n
 * @date 09.06.2026
 */

#ifndef INC_USBSYSTEM_HPP_
#define INC_USBSYSTEM_HPP_

#include "Types.hpp"
#include "SensorManager.hpp"
#include "HardwareManager.hpp"

namespace h2h3 {

/**
 * @class UsbSystem
 * @brief Orchestrates USB host switching and hub power management.
 */
class UsbSystem {
public:
    /** @brief Construct using Dependency Injection. */
	explicit UsbSystem(SensorManager& sensors);

    /** @brief Initializes the system with a starting host. */
    void init(UpstreamType initialHost);

    /** @brief Main non-blocking loop for the state machine. */
    void runStateMachine();

    /** @brief Requests a switch to the target host. */
    void requestSwitch(UpstreamType target);

    /** @brief Call this method when the user confirms they have resolved the issue. */
    void requestFaultRecovery();

    /** @brief Gets the currently active upstream host. */
    UpstreamType getActiveUpstream() const { return status.activeUpstream; }

    /** @brief Returns physical VBUS status. */
    VbusPresenceStatus getVbusPresence() const { return status.vbusPresence; }

    /** @brief Determines the other host based on the current state. */
    UpstreamType getToggleTarget() const;

    /** @brief Provides a snapshot of the current USB system status.*/
    const SystemStatus& getStatus() const { return status; }

private:
    SensorManager& sensorManager;     /**< Injected dependency */
    SystemStatus status;              /**< Uses global SystemStatus */
    ConnectionState currentState;     /**< Current state machine stage */
    uint32_t entryTick;               /**< Entry timestamp */
    bool bSwitchRequested;            /**< Flag for state transition */
    bool bRecoveryRequested = false;  /**< Tracks if the user gave the go-ahead to try a restart */
    UpstreamType targetUpstream;      /**< Requested host */

    /** @brief Hardware interface for VBUS detection. */
    UpstreamType probeUpstreamVbus();
};

} // namespace h2h3

#endif /* INC_USBSYSTEM_HPP_ */
