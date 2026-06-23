/**
 * @file HubManager.hpp
 * @brief Manages communication and diagnostics for the GL852G USB Hub
 * @author s-t-e-f-a-n
 * @date 02.06.2026
 */
#ifndef INC_HUBMANAGER_HPP_
#define INC_HUBMANAGER_HPP_

#include "main.h"

namespace h2h3 {

/**
 * @brief Register map for the Genesys Logic GL852G hub.
 * * Defines the internal I2C accessible registers used for configuration
 * of VID/PID, electrical tuning, and port status management.
 */
enum class HubReg : uint8_t {
        VID_LSB    = 0x00, // Vendor ID LSB
        VID_MSB    = 0x01, // Vendor ID MSB
        PID_LSB    = 0x02, // Product ID LSB
        PID_MSB    = 0x03, // Product ID MSB
        ELECTRICAL = 0x04, // Upstream and Logical Downstream port electrical tuning option
        CONFIG     = 0x05, // Hub non-removable configuration
        ELEC_D12   = 0x07, // Logical Downstream port 1 & 2 electrical tuning option
        ELEC_D34   = 0x08, // Logical Downstream port 3 & 4 electrical tuning option
        PORT_EN_12 = 0x2B, // Physical Downstream port 1 & 2 enable/disable option
        PORT_EN_34 = 0x2C  // Physical Downstream port 3 & 4 enable/disable option
    };

/**
 * @class HubManager
 * @brief Handles I2C interaction with the GenesysLogic GL852G hub controller.
 */
class HubManager {
public:
	/** @brief Default Hub Configs */
	static constexpr uint16_t DEFAULT_VENDOR_ID = 0x05E3;			// h/w default: 0x05E3
	static constexpr uint16_t DEFAULT_PRODUCT_ID = 0x0610;			// h/w default: 0x0610
	static constexpr uint8_t DEFAULT_NONREMOVABLE_PORTMASK = 0x08;	// Port#4 is STM32; hardware default: 0x00
	static constexpr uint8_t DEFAULT_SLEWRATE_PORTMASK = 0x00;		// hardware default: 0x00
	static constexpr uint8_t DEFAULT_PORTENABLED_PORTMASK = 0x0F;	// hardware default: 0x0F
	static constexpr uint8_t DEFAULT_JKUPSTREAM_LEVEL = 0x04;		// hardware default: 0x04
	static constexpr uint8_t DEFAULT_JKDOWNSTREAM_LEVEL = 0x04;		// hardware default: 0x04
	static constexpr bool DEFAULT_LOG_DEBUG = false;				// true for verbose log outputs
	static constexpr uint16_t DEFAULT_MAX_RETRIES = 50;				// GL852G I2C communication needs many retries
	static constexpr uint32_t DEFAULT_TIMEOUT     = 1000;			// ensures non-blocking at locking states
	static constexpr uint8_t HUB_I2C_ADDR = 0x2C;					// GL852G default I2C address
	/** @brief Initializes the hub communication interface. */
    static bool init();
    /** @brief Sets the USB Vendor ID. */
    static bool setVendorID(I2C_HandleTypeDef *hi2c, uint16_t vid);
    /** @brief Sets the USB Product ID. */
    static bool setProductID(I2C_HandleTypeDef *hi2c, uint16_t pid);
    /** @brief Configures which downstream ports are non-removable. */
    static bool setNonRemovablePorts(I2C_HandleTypeDef *hi2c, uint8_t portMask);
    /** @brief Tunes the electrical Slew Rate for ports. */
    static bool setSlewRate(I2C_HandleTypeDef *hi2c, uint8_t portMask);
    /** @brief Tunes Upstream Port JK level. */
    static bool setUpstreamJKLevel(I2C_HandleTypeDef *hi2c, uint8_t level);
    /** @brief Tunes Downstream Port JK level. */
    static bool setDownstreamJKLevel(I2C_HandleTypeDef *hi2c, uint8_t portIndex, uint8_t level);
    /** @brief Sets the enable state for all downstream ports at once using a bitmask. */
    static bool setPortsEnabled(I2C_HandleTypeDef *hi2c, uint8_t portMask);
    /** @brief Applies a complete set of default configurations to the hub. */
    static bool setHubDefaults();
    /** @brief Dumps all hub registers to the debug console. */
    static bool dumpAllRegisters();
    /** @brief Checks if an upstream host is detected. */
    static bool isUpstreamConnected();

private:
    /** @brief Forces a hardware reset and re-initialization of the I2C peripheral.  */
    static void I2C_ForceRecovery(I2C_HandleTypeDef *hi2c);
    /** @brief Helper to verify the I2C bus is free before starting a transfer. */
    static bool i2c_WaitWhileBusy(I2C_HandleTypeDef *hi2c, uint32_t timeout = DEFAULT_TIMEOUT, bool log = DEFAULT_LOG_DEBUG);
    /** @brief Helper to wait for specific I2C interrupt flags. */
    static bool i2c_WaitForFlag(I2C_HandleTypeDef *hi2c, uint32_t flags, uint32_t timeout = DEFAULT_TIMEOUT, bool log = DEFAULT_LOG_DEBUG);
    /** @brief Helper to check and clear the NACK flag. */
    static bool i2c_CheckNack(I2C_HandleTypeDef *hi2c, const char* context, bool log = DEFAULT_LOG_DEBUG);
    /** @brief Attempts to read a register from the I2C hub with a retry mechanism. */
    static bool readRegisterWithRetry(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t &val, uint16_t maxRetries = DEFAULT_MAX_RETRIES, bool log = DEFAULT_LOG_DEBUG);
    // @brief Writes a single byte to a hub register. */
    static bool writeRegister(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t val, bool log = DEFAULT_LOG_DEBUG);
    /** @brief Writes to a register and verifies the value, with automatic retry/recovery. */
    static bool writeRegisterWithRetry(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t val, uint16_t maxRetries = DEFAULT_MAX_RETRIES, bool log = DEFAULT_LOG_DEBUG);
    /** @brief Reads a single byte from a hub register. */
    static bool readRegister(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t &val, bool log = DEFAULT_LOG_DEBUG);
    /** @brief Performs a Read-Modify-Write operation on a register. */
    static bool updateRegisterBits(I2C_HandleTypeDef *hi2c, HubReg reg, uint8_t data, uint8_t mask);

};

} // namespace h2h3

#endif /* INC_HUBMANAGER_HPP_ */
