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
#include "ConsoleManager.hpp"

namespace h2h3 {

/* * Constructor initializes the state machine variables.
 */
UsbSystem::UsbSystem(SensorManager& sensors)
    : sensorManager(sensors),
	  status(),
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
 * @brief Requests the latched vbus muxer fault to recover.
  */
void UsbSystem::requestFaultRecovery() {
    if (currentState == ConnectionState::FAULT) {
        bRecoveryRequested = true;
    } else {
        LOG_DEBUG("Recovery requested, but system is not in a FAULT state.\r\n");
    }
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
    status.telemetry = sensorManager.getTelemetry();

    // Poll the physical hardware fault pins
    status.activeFault = HardwareManager::getActiveMuxFault();

    // Check for latched vbus muxer fault
    if (status.activeFault != UpstreamType::NONE) {
        if (currentState != ConnectionState::FAULT) {
            LOG_DEBUG("[CRITICAL] Hardware Mux Fault line pulled low! Switching to FAULT state.\r\n");
            currentState = ConnectionState::FAULT;
            bRecoveryRequested = false;
        } 
        else {
            // Diagnostic trace: Ensures we see if the fault is constantly hammering the system
            LOG_DEBUG("[FAULT ACTIVE] System is currently locked in FAULT state.\r\n");
        }
    }

    status.usbState = currentState;

	switch (currentState) {
        case ConnectionState::IDLE:
            if (bSwitchRequested) {
                // BREAK: Disconnect data lines
                HardwareManager::resetMuxRoute();
                status.isDataPathActive = false;
                entryTick = millis();
                currentState = ConnectionState::SWITCH_TOGGLE;
            }
            else if (status.config.autoFailover) { 
                // Check if our currently active host has lost its physical VBUS link
                bool activeHostPresent = false;
                if (status.activeUpstream == UpstreamType::A) {
                    activeHostPresent = (static_cast<uint8_t>(status.vbusPresence) & static_cast<uint8_t>(VbusPresenceStatus::HOST_A));
                } else if (status.activeUpstream == UpstreamType::B) {
                    activeHostPresent = (static_cast<uint8_t>(status.vbusPresence) & static_cast<uint8_t>(VbusPresenceStatus::HOST_B));
                }

                // If the active host was pulled out, look for the backup connection
                if (!activeHostPresent) {
                    UpstreamType backupHost = getToggleTarget();
                    
                    bool backupPresent = false;
                    if (backupHost == UpstreamType::A) {
                        backupPresent = (static_cast<uint8_t>(status.vbusPresence) & static_cast<uint8_t>(VbusPresenceStatus::HOST_A));
                    } else if (backupHost == UpstreamType::B) {
                        backupPresent = (static_cast<uint8_t>(status.vbusPresence) & static_cast<uint8_t>(VbusPresenceStatus::HOST_B));
                    }

                    if (backupPresent) {
                        LOG_DEBUG("[AUTO] Active host disconnected. Failover routing to remaining host...\r\n");
                        
                        targetUpstream = backupHost;
                        bSwitchRequested = true; 
                    }
                }
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
            // Wait for user confirmation to attempt clearing the hardware latch
            if (bRecoveryRequested) {
                LOG_DEBUG("[INFO] Attempting to clear hardware fault latch...\r\n");
                
                // Reset the power switches/muxer lines to unlatch the fault flags
                HardwareManager::clearMuxFault();
                HAL_Delay(100); // Give the physical lines time to settle

                // Verify if the physical condition has been resolved
                if (HardwareManager::getActiveMuxFault() != UpstreamType::NONE) {
                    LOG_DEBUG("[WARNING] Clear rejected: Hardware fault line is still indicating an overload.\r\n");
                    bRecoveryRequested = false; // Reset request flag, stay in FAULT
                } else {
                    LOG_DEBUG("[SUCCESS] Fault cleared. Re-aligning state machine with current host.\r\n");
                    
                    bRecoveryRequested = false;
                    bSwitchRequested = false;
                    
                    // Re-sync with whatever upstream connection is currently live/saved
                    init(status.activeUpstream); 
                }
            }
            break;
    }
}

} // namespace h2h3
