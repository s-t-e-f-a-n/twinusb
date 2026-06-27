/**
 * @file HardwareManager.cpp
 * @brief Manages base-level hardware initialization for the TWinUSB platform.
 * @author s-t-e-f-a-n
 * @date 01.06.2026
 */

#include "HardwareManager.hpp"
#include "ConsoleManager.hpp"
#include "HubManager.hpp"
#include "SensorManager.hpp"
#include "main.h"

extern "C" {
    #include "i2c.h"
    #include "tim.h"
    extern I2C_HandleTypeDef hi2c1;
    extern I2C_HandleTypeDef hi2c2;

    /**
     * @brief EXTI interrupt callback shim for STM32G0 HAL.
     * @details STM32G0 HAL fires edge-specific weak callbacks, so we forward both
     * rising and falling callbacks to the shared handler.
     * @param GPIO_Pin The pin that triggered the interrupt.
     */
    void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
        h2h3::HardwareManager::handleGpioInterrupt(GPIO_Pin);
    }

    void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin) {
        HAL_GPIO_EXTI_Callback(GPIO_Pin);
    }

    void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin) {
        HAL_GPIO_EXTI_Callback(GPIO_Pin);
    }
}

namespace h2h3 {


// Initialize static members
LedDriver HardwareManager::statusLed(&htim3, true); 
volatile SwitchEvent HardwareManager::pendingEvent = SwitchEvent::NONE;
volatile uint32_t HardwareManager::lastInterruptTick = 0;
volatile uint32_t HardwareManager::pressStartTime = 0;

LedDriver& HardwareManager::getStatusLed() {
    return statusLed;
}

/**
 * @brief Performs initial GPIO setup and MUX configuration.
 * * Sets the system to a safe "Reset Active" state, initializes MUX pins,
 * and determines the active host port from the physical VBUS sensing pins.
 */
UpstreamType HardwareManager::init() {
    /* Set all pins to safe default states */
    enableVbus(UpstreamType::NONE);												// VBUS A and B off
	HAL_GPIO_WritePin(HOSTMUX_OEn_GPIO_Port, HOSTMUX_OEn_Pin, GPIO_PIN_SET);	// Mux Disabled
    HAL_GPIO_WritePin(RESETn_GPIO_Port,      RESETn_Pin,      GPIO_PIN_RESET);	// Hub Reset Asserted

    /* Perform startup sensor health validation before enabling higher-level services. */
    if (!h2h3::SensorManager::startupSelfTest()) {
        LOG_DEBUG("Startup sensor health check failed.\r\n");
    }

    /* Validate I2C bus readiness before scanning */
	if (areBusesReady()) {
		/* Perform diagnostic I2C bus scans */
		scanBus(getHubI2C(), "Hub Bus (I2C1)");
		scanBus(getDisplayI2C(), "Display Bus (I2C2)");
	} else {
		LOG_DEBUG("Warning: One or more I2C buses not READY. Skipping scan.\r\n");
	}

	/* Check for Hub by dumping its registers and set Hub default config*/
	//h2h3::HubManager::dumpAllRegisters();
	//h2h3::HubManager::setHubDefaults();

    /* Retrieve and return current VBUS status */
    return (HAL_GPIO_ReadPin(ST_HUB_VBUS_GPIO_Port, ST_HUB_VBUS_Pin) == GPIO_PIN_SET)
            ? UpstreamType::A : UpstreamType::B;
}

/**
 * @brief Reads for active fault status on either VBUS Muxer status output.
  */
 UpstreamType HardwareManager::getActiveMuxFault() {
    bool faultA = (HAL_GPIO_ReadPin(FLT_HOSTA_VBUSn_GPIO_Port, FLT_HOSTA_VBUSn_Pin) == GPIO_PIN_RESET);
    bool faultB = (HAL_GPIO_ReadPin(FLT_HOSTB_VBUSn_GPIO_Port, FLT_HOSTB_VBUSn_Pin) == GPIO_PIN_RESET);
    
    if (faultA) return faultB ? UpstreamType::BOTH : UpstreamType::A;
    return faultB ? UpstreamType::B : UpstreamType::NONE;
}

void HardwareManager::clearMuxFault() {
    // Latched fault flags typically clear when the chip's enable line is cycled
    // or when the offending channel power is briefly forced off.
    powerDownHost();
    HAL_Delay(50); // Give bulk capacitance time to discharge
    
    // De-assert and re-assert the Mux routing lines to clear chip latch state
    resetMuxRoute();
    HAL_Delay(10);
}


/**
 * @brief Powers up the specified Upstream host and initializes the Hub.
 * * @details This method executes the full power-on sequence: it enables the VBUS
 * power rail for the target host, then de-asserts the Hub's reset line (RESETn).
 * * @note This method contains a blocking 100ms delay via startHub() to allow
 * the Hub's internal PLL and power-on-reset logic to stabilize.
 * * @param host The Upstream host to activate (UpstreamType::A or UpstreamType::B).
 */
void HardwareManager::powerUpHost(UpstreamType host) {
    enableVbus(host);
    startHub();
}

/**
 * @brief Safely shuts down the active Upstream host and silences the Hub.
 * * @details This method executes the power-down sequence: it asserts the Hub's
 * reset line (RESETn) to force it into a high-impedance state, then disables
 * all VBUS power rails to isolate the system from the host.
 * * @note After calling this, the Hub is held in a reset state and consumes
 * minimal power until powerUpHost() is called again.
 */
void HardwareManager::powerDownHost() {
    stopHub();
    enableVbus(UpstreamType::NONE);
}

/**
 * @brief Enables the VBUS power rail for a specific Upstream host.
 * @param target The Upstream port to enable (UpstreamType::A or UpstreamType::B).
 * @details This method ensures an atomic switch by disabling the inactive rail
 * before enabling the requested one.
 */
void HardwareManager::enableVbus(UpstreamType target) {
    if (target == UpstreamType::A) {
        HAL_GPIO_WritePin(EN_VBUSB_GPIO_Port, EN_VBUSB_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(EN_VBUSA_GPIO_Port, EN_VBUSA_Pin, GPIO_PIN_SET);
    } else if (target == UpstreamType::B) {
        HAL_GPIO_WritePin(EN_VBUSA_GPIO_Port, EN_VBUSA_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(EN_VBUSB_GPIO_Port, EN_VBUSB_Pin, GPIO_PIN_SET);
    } else { // Handle UpstreamType::NONE
        HAL_GPIO_WritePin(EN_VBUSA_GPIO_Port, EN_VBUSA_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(EN_VBUSB_GPIO_Port, EN_VBUSB_Pin, GPIO_PIN_RESET);
    }
}

/**
 * @brief Configures the physical MUX routing and enables the data path.
 * * @details This method sets the HOSTMUX_SEL pin to route the desired Upstream host (A or B)
 * and subsequently enables the data path by clearing the Output Enable (OEn) pin.
 * It ensures the physical MUX is configured before the data path is brought online.
 * * @param target The target Upstream host (UpstreamType::A or UpstreamType::B) to route to the hub.
 */
void HardwareManager::setMuxRoute(UpstreamType target) {
    // Set the physical routing
    HAL_GPIO_WritePin(HOSTMUX_SEL_GPIO_Port, HOSTMUX_SEL_Pin,
                     (target == UpstreamType::A) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // Enable the data path (Make)
    HAL_GPIO_WritePin(HOSTMUX_OEn_GPIO_Port, HOSTMUX_OEn_Pin, GPIO_PIN_RESET);
}

/**
 * @brief Safely disables the USB data path by setting the MUX to a high-impedance state.
 *
 * @details This method performs the "Break" portion of the break-before-make sequence.
 * By setting the Output Enable (OEn) pin to a HIGH state, it disconnects the
 * upstream USB data lines from the hub, preventing potential signal contention
 * or electrical transients during host switching operations.
 */
void HardwareManager::resetMuxRoute() {
    // Disable the data path (Break)
    HAL_GPIO_WritePin(HOSTMUX_OEn_GPIO_Port, HOSTMUX_OEn_Pin, GPIO_PIN_SET);
}

/**
 * @brief Triggers the hub reset release
 * * This method releases the RESETn line, allowing the GL852G to boot.
  */
void HardwareManager::startHub() {

	/* Hardware reset release sequence for the USB Hub IC */
    HAL_GPIO_WritePin(RESETn_GPIO_Port, RESETn_Pin, GPIO_PIN_SET);
    HAL_Delay(100);
}

/**
 * @brief Triggers the hub reset.
 * * This method asserts the RESETn line, resetting the GL852G.
 */
void HardwareManager::stopHub() {

	/* Hardware reset for the USB Hub IC */
	HAL_GPIO_WritePin(RESETn_GPIO_Port, RESETn_Pin, GPIO_PIN_RESET);

}


/**
 * @brief Provides access to the I2C1 (Hub) handle.
 * @return Pointer to I2C_HandleTypeDef for the Hub bus.
 */
I2C_HandleTypeDef* HardwareManager::getHubI2C() {
    return &::hi2c1;
}

/**
 * @brief Provides access to the I2C2 (Display) handle.
 * @return Pointer to I2C_HandleTypeDef for the Display bus.
 */
I2C_HandleTypeDef* HardwareManager::getDisplayI2C() {
    return &::hi2c2;
}

/**
 * @brief Validates hardware bus health.
 * * Performs a sanity check on both I2C peripheral states to ensure they are
 * initialized and ready for communication.
 * @return true if both I2C buses are in the READY state, false otherwise.
 */
bool HardwareManager::areBusesReady() {
    uint32_t state1 = HAL_I2C_GetState(&::hi2c1);
    uint32_t state2 = HAL_I2C_GetState(&::hi2c2);

    if (state1 != HAL_I2C_STATE_READY) {
        LOG_DEBUG("I2C1 State Error: 0x%08lX\r\n", state1);
    }
    if (state2 != HAL_I2C_STATE_READY) {
        LOG_DEBUG("I2C2 State Error: 0x%08lX\r\n", state2);
    }

    return (state1 == HAL_I2C_STATE_READY) && (state2 == HAL_I2C_STATE_READY);
}

/**
 * @brief Scans a specific I2C bus and reports active addresses via LOG_DEBUG.
 * * Iterates through all possible 7-bit I2C addresses (0x01-0x7F) and uses
 * HAL_I2C_IsDeviceReady to probe for connected peripherals.
 * @param i2c Pointer to the I2C_HandleTypeDef to scan.
 * @param busName Descriptive label for the log output.
 */
void HardwareManager::scanBus(I2C_HandleTypeDef* i2c, const char* busName) {
    LOG_DEBUG("--- Starting Scan: %s ---\r\n", busName);
    bool found = false;

    for (uint8_t addr = 0x01; addr < 0x80; addr++) {
        /* Ping the address (addr << 1 for HAL 8-bit addressing) */
        if (HAL_I2C_IsDeviceReady(i2c, (uint16_t)(addr << 1), 3, 10) == HAL_OK) {
            LOG_DEBUG("Device found at: 0x%02X\r\n", addr);
            found = true;
        }
    }

    if (!found) {
        LOG_DEBUG("No devices found on %s.\r\n", busName);
    }
}


/**
 * @brief Handles GPIO external interrupts for physical switch interactions.
 * @details This method acts as the ISR callback. It performs software debouncing
 * to filter out contact chatter, measures the press duration to
 * distinguish between short and long presses, and updates the
 * pending event state.
 * @param GPIO_Pin The numeric identifier of the pin that triggered the interrupt.
 * @note This method is designed to be called from the HAL_GPIO_EXTI_Callback.
 */
void HardwareManager::handleGpioInterrupt(uint16_t GPIO_Pin) {
    if (GPIO_Pin == SWITCH_Pin) {

        LOG_DEBUG("SWITCH interrupt fired, pin=%u\r\n", GPIO_Pin);

        uint32_t currentTick = millis();
        const uint32_t DEBOUNCE_MS = 50;

        // Debounce: Ignore if within lockout threshold
        if ((currentTick - lastInterruptTick) < DEBOUNCE_MS) return;
        lastInterruptTick = currentTick;

        bool isPressed = (HAL_GPIO_ReadPin(SWITCH_GPIO_Port, SWITCH_Pin) == GPIO_PIN_SET);

        if (isPressed) {
            pressStartTime = currentTick;
            pendingEvent = SwitchEvent::PRESSED;
        } else {
            uint32_t duration = currentTick - pressStartTime;
            pendingEvent = (duration > 1000) ? SwitchEvent::LONG_PRESS : SwitchEvent::SHORT_PRESS;
        }
    }
}

/**
 * @brief Retrieves and consumes the most recent switch interaction event.
 * @details This method implements a "get-and-clear" pattern. Once called, the
 * internal event flag is reset to SwitchEvent::NONE, ensuring that
 * each event is processed by the InterfaceManager exactly once.
 * @return The pending SwitchEvent enum value.
 */
SwitchEvent HardwareManager::getPendingEvent() {
    SwitchEvent event = pendingEvent;
    pendingEvent = SwitchEvent::NONE; // Reset flag after retrieval
    return event;
}
} // namespace h2h3
