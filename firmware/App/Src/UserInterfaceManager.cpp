/**
 * @file UserInterfaceManager.cpp
 * @brief Orchestration of system user inputs and visual output feedback.
 * @details Acts as the central bridge between HardwareManager events,
 * the USB state machine, and the DisplayManager service layer.
 * @author s-t-e-f-a-n
 * @date 12.06.2026
 */

#include "UserInterfaceManager.hpp"
#include "HardwareManager.hpp"
#include "ConsoleManager.hpp"
#include "UsbSystem.hpp"
#include "ConsoleManager.hpp"

namespace h2h3 {

/**
 * @brief Processes system input events.
 * @details Polls the HardwareManager for pending switch events. If an event
 * is detected, it routes the signal to handleSwitchEvent for state transitions.
 */
void UserInterfaceManager::processInputs() {
    SwitchEvent event = HardwareManager::getPendingEvent();

    if (event != SwitchEvent::NONE) {
        handleSwitchEvent(event);
    }
}

/**
 * @brief Updates the visual output based on the current system state.
 * @details Clears the display buffer, dispatches rendering to specific state 
 * handlers (e.g., startup, operational, or fault), and pushes the final 
 * frame to the hardware display driver.
 */
void UserInterfaceManager::updateOutputs() {
    // Prepare display buffer
    display.clear();

    // Dispatch rendering to appropriate state handler
    switch (currentState) {
        case UIState::STARTUP:
            display.drawBitmap(assets::img_logo, 0, 0);
            break;

        case UIState::OPERATIONAL: {
            const SystemStatus snapshot = usbSystem.getStatus();
            drawSystemStatus(snapshot);
            drawCenterArea(snapshot.telemetry);
            break;
        }

        case UIState::FAULT: {
            display.printf(0, CENTER_Y, "!!! FAULT !!!");
            display.printf(0, CENTER_Y + 12, "Code: 0x%04X", 
                           usbSystem.getStatus().activeAlerts);
            break;
        }

        case UIState::SCREENSAVER:
            // Frame remains cleared for power-saving mode
            break;
    }

    // Commit buffer to physical hardware
    display.update();
}

void UserInterfaceManager::drawSystemStatus(const SystemStatus& status) {
    auto getLinkStatus = [&](UpstreamType port) -> LinkStatus {
        if (status.activeUpstream == port) {
            // Future logic: Add identification check here (Host vs Charger)
            return LinkStatus::CONNECTED_ACTIVE_UNKNOWN; 
        }
        
        bool vbusPresent = (port == UpstreamType::A) ? 
                           (status.vbusPresence == VbusPresenceStatus::HOST_A || status.vbusPresence == VbusPresenceStatus::BOTH) :
                           (status.vbusPresence == VbusPresenceStatus::HOST_B || status.vbusPresence == VbusPresenceStatus::BOTH);
        return vbusPresent ? LinkStatus::CONNECTED_IDLE : LinkStatus::NOT_CONNECTED;
    };

    // Render Header (A) and Footer (B)
    const assets::Icon* iconA = getIconForStatus(getLinkStatus(UpstreamType::A));
    display.drawBitmap(iconA->data, 0, HEADER_Y, BitmapType::RAW, iconA->w, iconA->h);

    const assets::Icon* iconB = getIconForStatus(getLinkStatus(UpstreamType::B));
    display.drawBitmap(iconB->data, 0, FOOTER_Y, BitmapType::RAW, iconB->w, iconB->h);
}

const assets::Icon* UserInterfaceManager::getIconForStatus(LinkStatus status) {
    switch (status) {
        case LinkStatus::CONNECTED_HOST:            return &assets::icon_selected_host;
        case LinkStatus::CONNECTED_CHARGER:         return &assets::icon_selected_charger;
        case LinkStatus::NOT_CONNECTED:             return &assets::icon_not_connected;
        case LinkStatus::CONNECTED_ACTIVE_UNKNOWN:  return &assets::icon_selected_unknown;
        case LinkStatus::CONNECTED_IDLE:            
        default:                                    return &assets::icon_connected_not_selected;
    }
}

void UserInterfaceManager::drawCenterArea(const Telemetry& tel) {
    // Cycle Time
    uint8_t pageIndex = (millis() / TELEMETRY_PAGE_CYCLE_MS) % 3; 

    switch(pageIndex) {
        case 0: // Draw A/B Voltage
            display.printf(0, CENTER_Y, "A: %u.%03uV", tel.voltage.vbusa_mv / 1000, tel.voltage.vbusa_mv % 1000);
            display.printf(0, CENTER_Y + 12, "B: %u.%03uV", tel.voltage.vbusb_mv / 1000, tel.voltage.vbusb_mv % 1000);
            break;

        case 1: // Draw D/1 Current
            display.printf(0, CENTER_Y, "DS: %u.%03uV", tel.voltage.vbusds_mv / 1000, tel.voltage.vbusds_mv % 1000);
            display.printf(0, CENTER_Y + 12, "1: %umA", tel.current.ds1_ma);
            break;

        case 2: // Draw 2/3 Current
            display.printf(0, CENTER_Y, "2: %umA", tel.current.ds2_ma);
            display.printf(0, CENTER_Y + 12, "3: %umA", tel.current.ds3_ma);
            break;
    }
}

/**
 * @brief Evaluates system state and handles transitions.
 * @details Monitors activity (via sensors and input events) and system time to 
 * manage transitions between startup, operational, fault, and power-saving modes.
 */
void UserInterfaceManager::updateState() {
    const SystemStatus& status = usbSystem.getStatus();

    // Fault Handling (Highest Priority)
    // Overrides all other states if a critical fault is detected
    if (status.usbState == ConnectionState::FAULT) {
        currentState = UIState::FAULT;
        updateLedStatus();
        return;
    }
    
    // Recovery: Exit Fault mode if the issue is resolved
    if (currentState == UIState::FAULT && status.usbState != ConnectionState::FAULT) {
        currentState = UIState::OPERATIONAL;
        updateLedStatus();
    }

    // Startup Phase (Initial delay)
    // Keeps the logo visible until the system is ready
    if (currentState == UIState::STARTUP) {
       
        if (millis() - lastInteractionTime > STARTUP_DURATION_MS) {
            currentState = UIState::OPERATIONAL;
            updateLedStatus();
            lastInteractionTime = millis();
        }
        return; 
    }

    // Activity-Based State Management (Operational vs. Screensaver)
    bool activity = detectSystemActivity();

    if (currentState == UIState::SCREENSAVER) {
        // Wake up if activity is detected
        if (activity) {
            currentState = UIState::OPERATIONAL;
            lastVbusStatus = status.vbusPresence;
            updateLedStatus();
            lastInteractionTime = millis();
            LOG_DEBUG("Activity detected during Screensaver Mode!\r\n");
            updateOutputs();
        }
    } else {
        // Normal Operational Mode
        if (activity) {
            lastInteractionTime = millis(); // Reset idle timer on any activity
            LOG_DEBUG("Activity detected during Operational Mode!\r\n");
        } else if (millis() - lastInteractionTime > SCREENSAVER_TIMEOUT_MS) {
            // Enter power-save mode after 30 seconds of inactivity
            currentState = UIState::SCREENSAVER;
            updateLedStatus();

        }
        lastVbusStatus = status.vbusPresence;
    }
}

/**
 * @brief Updates the RGB LED color and pulse transitions using the LedDriver interface.
 * @details Evaluates the complete system status matrix against active upstreams
 * and VBUS connectivity, calling setPrimaryColor or setTransition to feed TIM14 ISR.
 */
void UserInterfaceManager::updateLedStatus() {
    const SystemStatus& status = usbSystem.getStatus();

    // FAULT State handling (Highest Priority)
    if (currentState == UIState::FAULT || status.usbState == ConnectionState::FAULT) {
        statusLed.setPrimaryColor(255, 0, 0); // Steady Red (Constants kill animation)
        LOG_DEBUG("UIState::FAULT\r\n");
        return;
    }

    // STARTUP State handling
    if (currentState == UIState::STARTUP) {
        // Soft blue breathing effect: maps to targetColor breathing from 0 to target
        // Note: setting pulse via setPulse requires an implicit base color
        statusLed.setColor(0, 0, 64);
        statusLed.setPulse(true); // Blue pulsing
        LOG_DEBUG("UIState::STARTUP\r\n");
        return;
    }

    // OPERATIONAL State Matrix evaluation
    if (currentState == UIState::OPERATIONAL) {
        bool hostA_connected = (status.vbusPresence == VbusPresenceStatus::HOST_A || 
                                status.vbusPresence == VbusPresenceStatus::BOTH);
        bool hostB_connected = (status.vbusPresence == VbusPresenceStatus::HOST_B || 
                                status.vbusPresence == VbusPresenceStatus::BOTH);
        
        bool hostA_selected = (status.activeUpstream == UpstreamType::A);
        bool hostB_selected = (status.activeUpstream == UpstreamType::B);

        // --- Case 1: Connected, Selected (Host A) | Not Connected (Host B) ---
        if (hostA_connected && hostA_selected && !hostB_connected) {
            statusLed.setPrimaryColor(0, 128, 128); // Cyan, constant
            LOG_DEBUG("UIState::OPERATIONAL CASE1\r\n");
        }
        
        // --- Case 2: Not Connected (Host A) | Connected, Selected (Host B) ---
        else if (!hostA_connected && hostB_connected && hostB_selected) {
            statusLed.setPrimaryColor(128, 0, 128); // Magenta, constant
            LOG_DEBUG("UIState::OPERATIONAL CASE2\r\n");
        }
        
        // --- Case 3: Connected, Selected (Host A) | Connected, Not Selected (Host B) ---
        else if (hostA_connected && hostA_selected && hostB_connected && !hostB_selected) {
            // Transitions color1 <-> color2 (Cyan <-> White)
            statusLed.setTransition(0, 128, 128, 128, 128, 128); 
            LOG_DEBUG("UIState::OPERATIONAL CASE3\r\n");
        }
        
        // --- Case 4: Connected, Not Selected (Host A) | Connected, Selected (Host B) ---
        else if (hostA_connected && !hostA_selected && hostB_connected && hostB_selected) {
            // Transitions color1 <-> color2 (Magenta <-> White)
            statusLed.setTransition(128, 0, 128, 128, 128, 128); 
            LOG_DEBUG("UIState::OPERATIONAL CASE4\r\n");
        }
        
        // --- Case 5: Connected, Not Selected (Host A) | Connected, Not Selected (Host B) ---
        else if (hostA_connected && !hostA_selected && hostB_connected && !hostB_selected) {
            statusLed.setPrimaryColor(128, 128, 128); // White constant (NOT DEFINED state)
            LOG_DEBUG("UIState::OPERATIONAL CASE5\r\n");
        }
    }
}

/**
 * @brief Probes all system-level activity conditions to determine if the 
 * interface should remain awake.
 * @details Evaluates user input, physical connection change, and fault events.
 * @return true if activity is detected, false otherwise.
 */
bool UserInterfaceManager::detectSystemActivity() {
    const SystemStatus& status = usbSystem.getStatus();

    // User Interaction
    bool keyPress = eventOccurred;
    eventOccurred = false;

    // Physical Connection Change (The "Connection taken place" trigger)
    bool vbusChanged = (lastVbusStatus != status.vbusPresence);
    if (vbusChanged) {
        lastVbusStatus = status.vbusPresence;
    }

    // System Fault
    bool faultDetected = (status.usbState == ConnectionState::FAULT);

    // If a cable was plugged in (vbusChanged), OR a button was pressed, 
    // OR a fault occurred, wake up the UI.
    return (keyPress || vbusChanged || faultDetected);
}

/**
 * @brief Routes hardware switch events to system actions.
 * @details Translates raw switch interactions (Short vs. Long press)
 * into system commands such as host toggling or system resets.
 * @param event The interaction event detected by the hardware layer.
 */
void UserInterfaceManager::handleSwitchEvent(SwitchEvent event) {
    eventOccurred = true; // Mark that user input happened

    switch (event) {
        case SwitchEvent::SHORT_PRESS:
            LOG_DEBUG("Switch: Short Press Detected -> toggle host target\r\n");
            usbSystem.requestSwitch(usbSystem.getToggleTarget());
            break;

        case SwitchEvent::LONG_PRESS:
            LOG_DEBUG("Switch: Long Press Detected - Triggering Reset\r\n");
            // Implement system reset logic here
            break;

        default:
            break;
    }
}

} // namespace h2h3
