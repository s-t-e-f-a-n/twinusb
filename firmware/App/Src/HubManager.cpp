/**
 * @file HubManager.cpp
 * @brief Implementation of GL852G register access and diagnostics.
 * @author s-t-e-f-a-n
 * @date 02.06.2026
 */

#include "HubManager.hpp"
#include "HardwareManager.hpp"
#include "ConsoleManager.hpp"
#include <cstdio>

extern "C" {
    #include "i2c.h" 
    #include "stm32g0xx_ll_i2c.h"
    #include "stm32g0xx_ll_bus.h" 
    extern I2C_HandleTypeDef hi2c1;
}

namespace h2h3 {

/**
 * @brief Forces a hardware reset and re-initialization of the I2C peripheral.
 */
void HubManager::I2C_ForceRecovery(I2C_HandleTypeDef *hi2c) {
    I2C_TypeDef* i2c_raw = hi2c->Instance;
    LL_I2C_Disable(i2c_raw);

    LL_APB1_GRP1_ForceReset(LL_APB1_GRP1_PERIPH_I2C1);
    HAL_Delay(1);
    LL_APB1_GRP1_ReleaseReset(LL_APB1_GRP1_PERIPH_I2C1);

    // Re-initialize peripheral registers via HAL
    MX_I2C1_Init();

    LL_I2C_Enable(i2c_raw);
}

/**
 * @brief Helper to verify the I2C bus is free before starting a transfer.
 */
bool HubManager::i2c_WaitWhileBusy(I2C_HandleTypeDef *hi2c, uint32_t timeout, bool log) {
    while (LL_I2C_IsActiveFlag_BUSY(hi2c->Instance)) {
        if (--timeout == 0) {
            if (log) LOG_DEBUG("DEBUG: Timeout: I2C Bus is stuck BUSY\r\n");
            return false;
        }
    }
    return true;
}

/**
 * @brief Helper to wait for specific I2C interrupt flags.
 */
bool HubManager::i2c_WaitForFlag(I2C_HandleTypeDef *hi2c, uint32_t flags, uint32_t timeout, bool log) {
    // Corrected to access the peripheral register instance directly
    while (!(hi2c->Instance->ISR & flags)) {
        if (--timeout == 0) {
            if (log) LOG_DEBUG("DEBUG: Timeout waiting for flags 0x%08X\r\n", flags);
            return false;
        }
    }
    return true;
}

/**
 * @brief Helper to check and clear the NACK flag.
 */
bool HubManager::i2c_CheckNack(I2C_HandleTypeDef *hi2c, const char* context, bool log) {
    I2C_TypeDef* i2c_raw = hi2c->Instance;
    if (LL_I2C_IsActiveFlag_NACK(i2c_raw)) {
        if (log) LOG_DEBUG("DEBUG: Slave NACK received at: %s\r\n", context);
        LL_I2C_ClearFlag_NACK(i2c_raw);
        LL_I2C_GenerateStopCondition(i2c_raw);
        return true;
    }
    return false;
}

/**
 * @brief Reads a single byte from a hub register.
 */
bool HubManager::readRegister(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t &val, bool log) {
    I2C_TypeDef* i2c_raw = hi2c->Instance;

    if (!i2c_WaitWhileBusy(hi2c)) return false;

    // --- PHASE 1: Write Address + Register Pointer ---
    LL_I2C_SetTransferRequest(i2c_raw, LL_I2C_REQUEST_WRITE);
    LL_I2C_SetSlaveAddr(i2c_raw, HUB_I2C_ADDR << 1);
    LL_I2C_SetTransferSize(i2c_raw, 1);
    LL_I2C_DisableAutoEndMode(i2c_raw);
    LL_I2C_GenerateStartCondition(i2c_raw);

    if (!i2c_WaitForFlag(hi2c, LL_I2C_ISR_TXIS | LL_I2C_ISR_NACKF)) return false;
    if (i2c_CheckNack(hi2c, "Phase 1 Addr")) return false;

    LL_I2C_TransmitData8(i2c_raw, reg);
    
    // Note: Consider reducing this delay if it blocks your bus for too long
    HAL_Delay(50); 

    if (!i2c_WaitForFlag(hi2c, LL_I2C_ISR_TC | LL_I2C_ISR_NACKF)) return false;
    if (i2c_CheckNack(hi2c, "Phase 1 Reg")) return false;

    // --- PHASE 2: Repeated Start + Read Data ---
    LL_I2C_SetTransferRequest(i2c_raw, LL_I2C_REQUEST_READ);
    LL_I2C_SetTransferSize(i2c_raw, 1);
    LL_I2C_GenerateStartCondition(i2c_raw);

    if (!i2c_WaitForFlag(hi2c, LL_I2C_ISR_RXNE | LL_I2C_ISR_NACKF)) return false;
    if (i2c_CheckNack(hi2c, "Phase 2 Addr")) return false;

    val = LL_I2C_ReceiveData8(i2c_raw);
    LL_I2C_GenerateStopCondition(i2c_raw);
    return i2c_WaitWhileBusy(hi2c);
}

/**
 * @brief Writes a single byte to a hub register.
 */
bool HubManager::writeRegister(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t val, bool log) {
    I2C_TypeDef* i2c_raw = hi2c->Instance;

    if (!i2c_WaitWhileBusy(hi2c)) return false;

    LL_I2C_SetTransferRequest(i2c_raw, LL_I2C_REQUEST_WRITE);
    LL_I2C_SetSlaveAddr(i2c_raw, HUB_I2C_ADDR << 1);
    LL_I2C_SetTransferSize(i2c_raw, 2);
    LL_I2C_DisableAutoEndMode(i2c_raw);
    LL_I2C_GenerateStartCondition(i2c_raw);

    if (!i2c_WaitForFlag(hi2c, LL_I2C_ISR_TXIS | LL_I2C_ISR_NACKF)) return false;
    if (i2c_CheckNack(hi2c, "Write Addr")) return false;

    LL_I2C_TransmitData8(i2c_raw, reg);
    if (!i2c_WaitForFlag(hi2c, LL_I2C_ISR_TXIS | LL_I2C_ISR_NACKF)) return false;

    LL_I2C_TransmitData8(i2c_raw, val);
    if (!i2c_WaitForFlag(hi2c, LL_I2C_ISR_TC | LL_I2C_ISR_NACKF)) return false;

    LL_I2C_GenerateStopCondition(i2c_raw);
    return i2c_WaitWhileBusy(hi2c);
}

/**
 * @brief Attempts to read a register from the I2C hub with a retry mechanism.
 */
bool HubManager::readRegisterWithRetry(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t &val, uint16_t maxRetries, bool log) {
    for (uint16_t i = 0; i < maxRetries; i++) {
        if (readRegister(hi2c, reg, val, log)) {
            return true;
        }

        if (log) LOG_DEBUG("DEBUG: Read failed, attempt %d/%d. Recovering...\r\n", i + 1, maxRetries);

        I2C_ForceRecovery(hi2c);
        HAL_Delay(20 * (i + 1));
    }
    return false;
}

/**
 * @brief Writes to a register and verifies the value, with automatic retry/recovery.
 */
bool HubManager::writeRegisterWithRetry(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t val, uint16_t maxRetries, bool log) {
    uint8_t readBack = 0;

    for (uint16_t i = 0; i < maxRetries; i++) {
        if (writeRegister(hi2c, reg, val, log)) {
            // Forward arguments explicitly to match function signature
            if (readRegisterWithRetry(hi2c, reg, readBack, maxRetries, log)) {
                if (readBack == val) {
                    return true;
                } else {
                    if (log) LOG_DEBUG("DEBUG: Verify failed! Wrote 0x%02X, read 0x%02X\r\n", val, readBack);
                }
            }
        }

        if (log) LOG_DEBUG("DEBUG: Write/Verify failed, attempt %d/%d. Recovering...\r\n", i + 1, maxRetries);

        I2C_ForceRecovery(hi2c);
        HAL_Delay(20 * (i + 1)); 
    }

    if (log) LOG_DEBUG("DEBUG: Failed to write and verify register 0x%02X after %d attempts.\r\n", reg, maxRetries);
    return false;
}

/**
 * @brief Performs a full dump of the GL852G internal registers to the system console.
 */
bool HubManager::dumpAllRegisters() {
    I2C_HandleTypeDef* hI2c = HardwareManager::getHubI2C();
    const uint8_t knownRegisters[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x07, 0x08, 0x2B, 0x2C };

    LOG_DEBUG("--- GL852G Known Register Dump ---\r\n");

    bool ok = true;
    for (uint8_t reg : knownRegisters) {
        uint8_t val = 0;
        // Explicitly passing a retry count (e.g., 3) and log toggle to avoid parameter mismatch
        if (readRegisterWithRetry(hI2c, reg, val, 3, true)) {
            LOG_DEBUG("SUCCESS | Reg 0x%02X: 0x%02X\r\n", reg, val);
        } else {
            LOG_DEBUG("FAILED  | Reg 0x%02X\r\n", reg);
            ok = false;
        }
    }
    return ok;
}

/**
 * @brief Performs a Read-Modify-Write operation on a register.
 */
bool HubManager::updateRegisterBits(I2C_HandleTypeDef *hi2c, HubReg reg, uint8_t data, uint8_t mask) {
    uint8_t currentVal = 0;
    // Standardizing on 3 retries and active logging for utility updates
    if (!readRegisterWithRetry(hi2c, (uint8_t)reg, currentVal, 3, true)) {
        return false;
    }

    uint8_t newVal = (currentVal & ~mask) | (data & mask);
    return writeRegisterWithRetry(hi2c, (uint8_t)reg, newVal, 3, true);
}

/**
 * @brief Sets the USB Vendor ID.
 */
bool HubManager::setVendorID(I2C_HandleTypeDef *hi2c, uint16_t vid) {
    bool ok = writeRegisterWithRetry(hi2c, (uint8_t)HubReg::VID_LSB, (uint8_t)(vid & 0xFF), 3, true);
    ok &= writeRegisterWithRetry(hi2c, (uint8_t)HubReg::VID_MSB, (uint8_t)((vid >> 8) & 0xFF), 3, true);
    return ok;
}

/**
 * @brief Sets the USB Product ID.
 */
bool HubManager::setProductID(I2C_HandleTypeDef *hi2c, uint16_t pid) {
    bool ok = writeRegisterWithRetry(hi2c, (uint8_t)HubReg::PID_LSB, (uint8_t)(pid & 0xFF), 3, true);
    ok &= writeRegisterWithRetry(hi2c, (uint8_t)HubReg::PID_MSB, (uint8_t)((pid >> 8) & 0xFF), 3, true);
    return ok;
}

/**
 * @brief Configures which downstream ports are non-removable.
 */
bool HubManager::setNonRemovablePorts(I2C_HandleTypeDef *hi2c, uint8_t portMask) {
    return updateRegisterBits(hi2c, HubReg::CONFIG, (portMask & 0x0F), 0x0F);
}

/**
 * @brief Tunes the electrical Slew Rate for ports.
 */
bool HubManager::setSlewRate(I2C_HandleTypeDef *hi2c, uint8_t portMask) {
    return updateRegisterBits(hi2c, HubReg::ELECTRICAL, (portMask & 0x1F), 0x1F);
}

/**
 * @brief Tunes Upstream Port JK level.
 */
bool HubManager::setUpstreamJKLevel(I2C_HandleTypeDef *hi2c, uint8_t level) {
    uint8_t safeLevel = (level & 0x07);
    return updateRegisterBits(hi2c, HubReg::ELECTRICAL, (safeLevel << 5), 0xE0);
}

/**
 * @brief Tunes Downstream Port JK level.
 */
bool HubManager::setDownstreamJKLevel(I2C_HandleTypeDef *hi2c, uint8_t portIndex, uint8_t level) {
    if (portIndex < 1 || portIndex > 4) return false;

    uint8_t safeLevel = (level & 0x07);
    HubReg reg = (portIndex <= 2) ? HubReg::ELEC_D12 : HubReg::ELEC_D34;

    uint8_t shift = ((portIndex == 1) || (portIndex == 3)) ? 1 : 5;
    uint8_t mask = (0x07 << shift);

    return updateRegisterBits(hi2c, reg, (safeLevel << shift), mask);
}

/**
 * @brief Sets the enable state for all ports at once using a bitmask.
 */
bool HubManager::setPortsEnabled(I2C_HandleTypeDef *hi2c, uint8_t portMask) {
    bool ok = true;
    uint8_t safeMask = (portMask & 0x0F);

    uint8_t val12 = ((safeMask & 0x01) << 3) | ((safeMask & 0x02) << 6);
    ok &= updateRegisterBits(hi2c, HubReg::PORT_EN_12, val12, 0x88);

    uint8_t val34 = ((safeMask & 0x04) << 1) | ((safeMask & 0x08) << 4);
    ok &= updateRegisterBits(hi2c, HubReg::PORT_EN_34, val34, 0x88);

    return ok;
}

/**
 * @brief Applies a complete set of default configurations to the hub.
 */
bool HubManager::setHubDefaults() {
    I2C_HandleTypeDef* hI2c = HardwareManager::getHubI2C();

    LOG_DEBUG("--- GL852G Set Default Configuration ---\r\n");

    bool ok = setNonRemovablePorts(hI2c, DEFAULT_NONREMOVABLE_PORTMASK);
    if (ok) {
        LOG_DEBUG("SUCCESS | Non-removable port mask set to 0x%02X\r\n", DEFAULT_NONREMOVABLE_PORTMASK);
    } else {
        LOG_DEBUG("FAILED  | Non-removable port mask\r\n");
    }

    return ok;
}

} // namespace h2h3