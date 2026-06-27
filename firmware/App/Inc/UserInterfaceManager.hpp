/**
 * @file UserInterfaceManager.hpp
 * @brief Orchestrates system user inputs and visual output feedback.
 * @details This header defines the interface for the UserInterfaceManager,
 * which acts as the central controller between hardware inputs and display output.
 * @author s-t-e-f-a-n
 * @date 12.06.2026
 */

#ifndef INC_USERINTERFACEMANAGER_HPP_
#define INC_USERINTERFACEMANAGER_HPP_

#include "Types.hpp"
#include "LedDriver.hpp"
#include "DisplayAssets.hpp"
#include "DisplayManager.hpp"
#include "SensorManager.hpp"
#include "HardwareManager.hpp"
#include "UsbSystem.hpp"


namespace h2h3 {

/**
 * @class UserInterfaceManager
 * @brief Central controller for user interactions and visual feedback.
 * @details This class bridges raw hardware events and visual feedback by   
 * consuming telemetry from SensorManager and rendering it via DisplayManager.
 */
class UserInterfaceManager {
public:
    /**
     * @brief Constructs the UserInterfaceManager instance.
     * @param displayRef Reference to the persistent DisplayManager service.
     * @param sensorRef Reference to the persistent SensorManager for telemetry.
     * @param usbsytsemRef References to the persistent UsbSystem service.
     */
    UserInterfaceManager(DisplayManager& displayRef, SensorManager& sensorRef, 
                        UsbSystem& usbsystemRef,  LedDriver& ledRef)
        : display(displayRef), sensorManager(sensorRef), usbSystem(usbsystemRef), 
          statusLed(ledRef), lastInteractionTime(millis()) {
            statusLed.init(0, 100, 0);
          }

    /**
     * @brief Processes all system inputs.
     * @details Polls HardwareManager for pending events and routes them to
     * the appropriate system action handlers.
     */
    void processInputs();

/**
     * @brief Updates the visual output based on the current system state.
     * @details Clears the display buffer, dispatches rendering to specific state 
     * handlers (e.g., startup, operational, or fault), and pushes the final 
     * frame to the hardware display driver.
     */
    void updateOutputs();

    /**
     * @brief Evaluates system state and handles transitions.
     * @details Monitors activity (via sensors and input events) to manage 
     * transitions between operational modes and power-saving screensaver states.
     * Should be called in the main loop to ensure consistent responsiveness.
     */
    void updateState();

private:
    DisplayManager& display;       /**< Reference to the display service */
    SensorManager& sensorManager;  /**< Reference to the telemetry data source */
    UsbSystem& usbSystem;          /**< Reference to the usb system service */
    LedDriver& statusLed;          /**< Reference to the status led service */
    
    /** @brief Layout constants for 64x32 display partitioning. */
    static constexpr uint8_t HEADER_Y = 0;   ///< Y-origin for Host A (Header)
    static constexpr uint8_t CENTER_Y = 3;   ///< Y-origin for Telemetry/Faults
    static constexpr uint8_t FOOTER_Y = 29;  ///< Y-origin for Host B (Footer)

    /** @brief Timing constants of the UI interface. */
    static constexpr uint32_t STARTUP_DURATION_MS = 5000;
    static constexpr uint32_t SCREENSAVER_TIMEOUT_MS = 60000;
    static constexpr uint32_t TELEMETRY_PAGE_CYCLE_MS = 10000;

/** @name Activity Detection
     * @brief Sensors and flags to track system usage and connection changes.
     * @{
     */
    /** @brief Flag set by interrupt-driven input events to trigger UI wake-up. */
    bool eventOccurred = false;
    /** @brief Cache of the previous host state, used to detect logic-level upstream switching. */
    UpstreamType lastActiveUpstream = UpstreamType::NONE;
    /** @brief Cache of the physical VBUS presence, used to detect cable insertion/removal. */
    VbusPresenceStatus lastVbusStatus = VbusPresenceStatus::NONE;
    /** @} */

    /** * @brief Maps connection status to graphical icons.
     * @param status The current LinkStatus of a host.
     * @return Pointer to the corresponding asset icon.
     */
    const assets::Icon* getIconForStatus(LinkStatus status);
    /** * @name updateOutputs State Machine Control
     * @brief Variables managing the UI navigation, timing, and lifecycle transitions.
     * @{
     */

    /** @brief Tracks the current operational mode (e.g., STARTUP, FAULT, SCREENSAVER). */
    UIState currentState = UIState::STARTUP;

    /** @brief Tracks the active telemetry page during OPERATIONAL mode. */
    SubState telemetryPage = SubState::PAGE_A;

    /** @brief Timestamp (ms) of the last user or system interaction, used to trigger screensaver timeouts. */
    uint32_t lastInteractionTime = 0;

    /** @brief Timestamp (ms) of the last telemetry page rotation, used to throttle automatic cycling. */
    uint32_t lastPageSwitchTime = 0;

    /** * @brief Internal helper to update the RGB LED based on currentState.
     * @details Maps UI states to specific color profiles for the status LED.
     */
    void updateLedStatus();

    /** @} */

    /** * @name Rendering Helpers
     * @brief Specialized drawing routines invoked by updateOutputs().
     * @{
     */

    /** * @brief Renders system-wide status (Host A/B) at designated screen regions.
     * @param status The current usbSystems's connection state structure containing both host statuses.
     */
    void drawSystemStatus(const SystemStatus& status);

    /** @brief Renders the dynamic telemetry data or fault pages in the center area. */
    void drawCenterArea(const Telemetry& tel);

    /**
     * @brief Evaluates all system events to determine if the display should remain awake.
     * @details Probes input events, USB host status changes, and fault conditions.
     * @return true if activity is detected, false otherwise.
     */
    bool detectSystemActivity();

    /** @} */

    /**
     * @brief Internal helper to route switch events to system actions.
     * @param event The interaction event detected by the hardware layer.
     */
    void handleSwitchEvent(SwitchEvent event);
};

} // namespace h2h3

#endif /* INC_USERINTERFACEMANAGER_HPP_ */
