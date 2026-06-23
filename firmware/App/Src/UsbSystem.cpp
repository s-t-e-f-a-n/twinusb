/**
 * @file UsbSystem.cpp
* @brief Implementation of the USB Upstream/Downstream port switching state machine.
 * * This module manages the physical layer switching between two upstream hosts (A and B)
 * and controls the data path to the triple downstream hub. It maintains synchronization
 * with the HardwareManager to ensure electrical safety (break-before-make) during
 * host transitions.
 * @author s-t-e-f-a-n
 * @date 09.06.2026
 */

#include "UsbSystem.hpp"


#ifndef LOG_DEBUG
#define LOG_DEBUG(...) ((void)0)
#endif

namespace h2h3 {


/* * Constructor initializes the state machine variables.
 */
UsbSystem::UsbSystem(SensorManager& sensors)
    : sensorManager(sensors),
	  // Change this line in UsbSystem.cpp:
	  status(UpstreamType::NONE, ConnectionState::IDLE, VbusPresenceStatus::NONE, false, 0),
	  currentState(ConnectionState::IDLE),
      entryTick(0),
      bSwitchRequested(false),
      targetUpstream(UpstreamType::NONE) {}

/**
 * @brief Initializes the USB system state and synchronizes with hardware.
 */
void UsbSystem::init(UpstreamType detectedHost) {
    // Synchronous Power-Up (Blocking)
    HardwareManager::powerUpHost(detectedHost);
    HardwareManager::setMuxRoute(detectedHost);

    status.activeUpstream = detectedHost;
    status.isDataPathActive = true;
    targetUpstream = detectedHost;
    currentState = ConnectionState::IDLE;
}

/**
 * @brief Determines the other host based on the current state.
 * @return UpstreamType::B if A is active, UpstreamType::A if B is active.
 */
UpstreamType UsbSystem::getToggleTarget() const {
    if (status.activeUpstream == UpstreamType::A) {
        return UpstreamType::B;
    } 
    if (status.activeUpstream == UpstreamType::B) {
        return UpstreamType::A;
    }
    
    // If NONE or BOTH is currently active, we define a safe default fallback
    return UpstreamType::A; 
}

/**
 * @brief Requests a transition to a different Upstream host.
 * @param target The desired Upstream port (A or B).
 */
void UsbSystem::requestSwitch(UpstreamType target) {
    // Safety check: Is the target physically connected?
    bool isTargetPresent = false;

    if (target == UpstreamType::A) {
        isTargetPresent = (static_cast<uint8_t>(status.vbusPresence) & static_cast<uint8_t>(VbusPresenceStatus::HOST_A));
    } else if (target == UpstreamType::B) {
        isTargetPresent = (static_cast<uint8_t>(status.vbusPresence) & static_cast<uint8_t>(VbusPresenceStatus::HOST_B));
    }

    // Only accept request if target is valid AND different from current
    if (isTargetPresent && (target != status.activeUpstream)) {
        targetUpstream = target;
        bSwitchRequested = true;
        LOG_DEBUG("Switch request accepted for Host %s\r\n", (target == UpstreamType::A ? "A" : "B"));
    } else {
        LOG_DEBUG("Switch request rejected: Host %s not detected or already active.\r\n",
                  (target == UpstreamType::A ? "A" : "B"));
    }
}

/**
 * @brief Probes the VBUS voltage levels via SensorManager to update VbusPresenceStatus.
 * @details This method polls the ADC channels, determines the physical connection
 * status of Host A and Host B based on a 4.5V threshold, and updates the system's
 * internal status.
 */
h2h3::UpstreamType UsbSystem::probeUpstreamVbus() {
    sensorManager.poll();

    const uint16_t VBUS_THRESHOLD_MV = 2500;

    bool hostA = (sensorManager.getTelemetry().voltage.vbusa_mv > VBUS_THRESHOLD_MV);
    bool hostB = (sensorManager.getTelemetry().voltage.vbusb_mv > VBUS_THRESHOLD_MV);

    // Update status based on all possible physical combinations
    status.vbusPresence = static_cast<VbusPresenceStatus>(
        (hostA ? (uint8_t)VbusPresenceStatus::HOST_A : 0) |
        (hostB ? (uint8_t)VbusPresenceStatus::HOST_B : 0)
    );

    // Explicitly return the detected state
        if (hostA && hostB) return UpstreamType::BOTH;
        if (hostA)          return UpstreamType::A;
        if (hostB)          return UpstreamType::B;

        return UpstreamType::NONE;
}

/**
 * @brief Main non-blocking state machine loop.
 * @details Executes the host-switching sequence. Call this function repeatedly
 * within the main super-loop or a periodic RTOS task.
 */
void UsbSystem::runStateMachine() {

	// Probe upstream VBUS voltage levels to update VbusPresenceStatus
	probeUpstreamVbus();

    // Refresh the integrated telemetry snapshot in SystemStatus
    // Since probeUpstreamVbus() calls sensorManager.poll(), 
    // the SensorManager is now up-to-date.
    status.telemetry = sensorManager.getTelemetry();

	switch (currentState) {
        case ConnectionState::IDLE:
            if (bSwitchRequested) {
                // BREAK: Disconnect data lines
                HardwareManager::resetMuxRoute();
                status.isDataPathActive = false;
                entryTick = millis();
                currentState = ConnectionState::SWITCH_TOGGLE;
            }
            break;
        case ConnectionState::NEGOTIATING:
        case ConnectionState::CONFIGURED:
        	break;
        case ConnectionState::SWITCH_TOGGLE:
            // Wait for data line stabilization after break
            if ((millis() - entryTick) > 100) {
                // SWITCH POWER: Cleanly toggle VBUS and cycle Hub Reset
                HardwareManager::powerDownHost();
                HardwareManager::powerUpHost(targetUpstream);

                status.activeUpstream = targetUpstream;
                entryTick = millis();
                currentState = ConnectionState::SWITCH_RECONNECT;
            }
            break;

        case ConnectionState::SWITCH_RECONNECT:
            // MAKE: Re-enable the data path
            if ((millis() - entryTick) > 100) {
                HardwareManager::setMuxRoute(status.activeUpstream);
                status.isDataPathActive = true;
                bSwitchRequested = false;
                currentState = ConnectionState::IDLE;
            }
            break;

        case ConnectionState::FAULT:
            HardwareManager::resetMuxRoute();
            HardwareManager::stopHub();
            status.isDataPathActive = false;
            break;
    }
}

} // namespace h2h3
