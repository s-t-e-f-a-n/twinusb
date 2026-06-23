/**
 * @file AppMain.cpp
 * @brief Application entry point and main loop
 * @author s-t-e-f-a-n
 * @date 01.06.2026
 */

#include "AppMain.hpp"
#include "LedDriver.hpp"
#include "HardwareManager.hpp"
#include "HubManager.hpp"
#include "SensorManager.hpp"
#include "ConsoleManager.hpp"
#include "UserInterfaceManager.hpp"
#include "DisplayManager.hpp"
#include "UsbSystem.hpp"
#include "main.h"

extern "C" {
    #include "i2c.h"
    #include "tim.h"
}

// Global pointer definitions
namespace h2h3 {
    DisplayManager* display = nullptr;
    LedDriver* LedDriver::instance = nullptr;
}

extern "C" int _write(int file, char *ptr, int len) {
    h2h3::ConsoleManager::sendBinary(reinterpret_cast<uint8_t*>(ptr), len);
    return len;
}

extern "C" {
    void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
        h2h3::LedDriver::timerCallback(htim);
    }
}

/* -------------------------------------------------------------------------- */
/* APP MAIN										                              */
/* -------------------------------------------------------------------------- */

// Initialize the main controller components
extern "C" void AppMain(void) {
	using namespace h2h3;

	// Intialize console output
	ConsoleManager::init();
    LOG_DEBUG("\r\nSystem starting...\r\n");

	// Initialize hardware
	UpstreamType initialHost = HardwareManager::init();
	LOG_DEBUG("Host detected: %s\r\n", (initialHost == h2h3::UpstreamType::A) ? "A" : "B");

	// Instantiate raw drivers using low-level HAL handles
	static LedDriver statusLed(&htim3, true); 
    static DisplayManager displayInstance(&hi2c2, I2C_ADR);
    
    // Assign global pointer
    display = &displayInstance;
    
    // Ensure we only initialize if the pointer is valid
    if (display != nullptr) {
        display->init();
    }

	// Instantiate objects now, before the loop
	SensorManager sensors;
	static UsbSystem usbSystem(sensors); // Inject sensors into USB system

	// Instantiate Orchestrator (Dependency Injection)
	static UserInterfaceManager uiManager(*display, sensors, usbSystem, statusLed);

	// Start the USB subsystem
	usbSystem.init(initialHost);
	LOG_DEBUG("Host's USB data lines muxed to Hub, Hub started, Host's VBUS Muxed to Downstream Ports\r\n");

    LOG_DEBUG("System initialized. Starting main loop.\r\n");

    // Main control loop
    while (1) {

    	// UI Handling
    	uiManager.processInputs();

    	// Poll sensor data
        sensors.poll();

        // Run the state machine
    	usbSystem.runStateMachine();

		// Update state BEFORE rendering
        uiManager.updateState();

        // Visual Feedback
        uiManager.updateOutputs();

        // Add a small delay to avoid spamming the USB/UART bus
        HAL_Delay(20);
    }
}
