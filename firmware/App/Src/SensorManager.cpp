/**
 * @file SensorManager.cpp
 * @brief Manages ADC data acquisition for three V and three I channels
 * @author s-t-e-f-a-n
 * @date 01.06.2026
 */

#include "SensorManager.hpp"
#include "ConsoleManager.hpp"
#include "main.h"

// Define the external handle at file scope so it's accessible to all methods in this file
extern ADC_HandleTypeDef hadc1;

// STM32G0 factory calibration address for internal voltage reference (typically 3.0V baseline)
#define STM32G0_VREFINT_CAL_ADDR ((uint16_t*)0x1FFF75AA)

namespace h2h3 {

    Telemetry SensorManager::currentData = {};

    // --- Persistent Filter States for EMA Smoothing ---
    namespace {
        uint32_t emaVbusA  = 0;
        uint32_t emaVbusB  = 0;
        uint32_t emaVbusDs = 0;
        
        uint32_t emaDs1    = 0;
        uint32_t emaDs2    = 0;
        uint32_t emaDs3    = 0;
    }

    /**
     * @brief Polls all ADC channels, averages noise out via 16x oversampling,
     * and passes raw data through an EMA low-pass filter before physical conversion.
     */
    void SensorManager::poll() {
        constexpr uint32_t kOversampleCount = 16u;
        
        // --- 1. Oversampled Read of VREFINT ---
        uint32_t sumVrefInt = 0;
        for (uint32_t i = 0; i < kOversampleCount; ++i) {
            sumVrefInt += Read_ADC(ADC_CHANNEL_VREFINT);
        }
        uint32_t rawVrefInt = (sumVrefInt + (kOversampleCount / 2u)) / kOversampleCount;
        
        if (rawVrefInt == 0) {
            rawVrefInt = 1485; // Fallback to nominal 12-bit value for 1.212V
        }
        
        const uint32_t factoryVRefMv = 3000u;
        const uint32_t vrefintCal = *STM32G0_VREFINT_CAL_ADDR;
        const uint32_t liveVRefMv = (factoryVRefMv * vrefintCal) / rawVrefInt;

        // --- 2. Oversampled Read of Voltage Channels ---
        uint32_t sumVbusA = 0, sumVbusB = 0, sumVbusDs = 0;
        for (uint32_t i = 0; i < kOversampleCount; ++i) {
            sumVbusA  += Read_ADC(ADC_CHANNEL_0);
            sumVbusB  += Read_ADC(ADC_CHANNEL_1);
            sumVbusDs += Read_ADC(ADC_CHANNEL_7);
        }
        uint32_t rawVbusA  = (sumVbusA  + (kOversampleCount / 2u)) / kOversampleCount;
        uint32_t rawVbusB  = (sumVbusB  + (kOversampleCount / 2u)) / kOversampleCount;
        uint32_t rawVbusDs = (sumVbusDs + (kOversampleCount / 2u)) / kOversampleCount;

        // Apply Heavier EMA Low-Pass Filter (Weight: 93.75% Old / 6.25% New)
        // Formula: (Old * 15 + New) / 16
        emaVbusA  = (emaVbusA  == 0) ? rawVbusA  : ((emaVbusA  * 63u) + rawVbusA)  >> 6;
        emaVbusB  = (emaVbusB  == 0) ? rawVbusB  : ((emaVbusB  * 63u) + rawVbusB)  >> 6;
        emaVbusDs = (emaVbusDs == 0) ? rawVbusDs : ((emaVbusDs * 63u) + rawVbusDs) >> 6;

        currentData.voltage.vbusa_mv  = static_cast<uint16_t>((emaVbusA  * liveVRefMv * kVoltageScale) / kAdcMaxTicks);
        currentData.voltage.vbusb_mv  = static_cast<uint16_t>((emaVbusB  * liveVRefMv * kVoltageScale) / kAdcMaxTicks);
        currentData.voltage.vbusds_mv = static_cast<uint16_t>((emaVbusDs * liveVRefMv * kVoltageScale) / kAdcMaxTicks);

        // --- 3. Oversampled Read of Current Channels ---
        uint32_t sumDs1 = 0, sumDs2 = 0, sumDs3 = 0;
        for (uint32_t i = 0; i < kOversampleCount; ++i) {
            sumDs1 += Read_ADC(ADC_CHANNEL_4);
            sumDs2 += Read_ADC(ADC_CHANNEL_5);
            sumDs3 += Read_ADC(ADC_CHANNEL_6);
        }
        uint32_t rawDs1 = (sumDs1 + (kOversampleCount / 2u)) / kOversampleCount;
        uint32_t rawDs2 = (sumDs2 + (kOversampleCount / 2u)) / kOversampleCount;
        uint32_t rawDs3 = (sumDs3 + (kOversampleCount / 2u)) / kOversampleCount;

        // Apply Heavier EMA Low-Pass Filter (Weight: 93.75% Old / 6.25% New)
        // Formula: (Old * 15 + New) / 16
        emaDs1    = (emaDs1    == 0) ? rawDs1    : ((emaDs1    * 63u) + rawDs1)    >> 6;
        emaDs2    = (emaDs2    == 0) ? rawDs2    : ((emaDs2    * 63u) + rawDs2)    >> 6;
        emaDs3    = (emaDs3    == 0) ? rawDs3    : ((emaDs3    * 63u) + rawDs3)    >> 6;

        const uint32_t liveCurrentMultiplier = (liveVRefMv * 1000000u) / (kAdcMaxTicks * kRShuntMOHm * kAmplifierAv);

        currentData.current.ds1_ma = static_cast<uint16_t>((emaDs1 * liveCurrentMultiplier) / (kCurrentPrecisionScale / 100u));
        currentData.current.ds2_ma = static_cast<uint16_t>((emaDs2 * liveCurrentMultiplier) / (kCurrentPrecisionScale / 100u));
        currentData.current.ds3_ma = static_cast<uint16_t>((emaDs3 * liveCurrentMultiplier) / (kCurrentPrecisionScale / 100u));
    }

    const Telemetry& SensorManager::getTelemetry() {
        return currentData;
    }

    /**
     * @brief Reads a single ADC channel.
     * * Configures the ADC for the specified channel, performs a single-shot
     * conversion, and returns the raw digital value.
     * * @param channel The ADC channel index to read (e.g., ADC_CHANNEL_0).
     * @return uint32_t The raw ADC conversion result, or 0 if conversion fails.
     */
    uint32_t SensorManager::Read_ADC(uint32_t channel) {
        // Force the linker to look in the global scope for the C-variable
        ADC_ChannelConfTypeDef sConfig = {0};
        sConfig.Channel = channel;
        sConfig.Rank = ADC_REGULAR_RANK_1;
        //sConfig.SamplingTime = ADC_SAMPLETIME_39CYCLES_5;
        sConfig.SamplingTime = ADC_SAMPLETIME_160CYCLES_5;

        if (HAL_ADC_ConfigChannel(&::hadc1, &sConfig) != HAL_OK) {
            return 0;
        }

        if (HAL_ADC_Start(&::hadc1) != HAL_OK) {
            return 0;
        }

        uint32_t result = 0;
        if (HAL_ADC_PollForConversion(&::hadc1, 10) == HAL_OK) {
            result = HAL_ADC_GetValue(&::hadc1);
        }
        HAL_ADC_Stop(&::hadc1);
        return result;
    }

    bool SensorManager::isSampleStable(uint32_t sample, uint32_t mean) {
        uint32_t diff = (sample > mean) ? (sample - mean) : (mean - sample);
        uint32_t tolerance = (mean * kStartupStabilityPercent + 50u) / 100u;
        if (tolerance == 0u) {
            tolerance = 1u;
        }
        return diff <= tolerance;
    }

    bool SensorManager::startupSelfTest() {
        constexpr uint32_t channels[] = {
            ADC_CHANNEL_0,
            ADC_CHANNEL_1,
            ADC_CHANNEL_7,
            ADC_CHANNEL_4,
            ADC_CHANNEL_5,
            ADC_CHANNEL_6
        };

        // Calibration
        HAL_ADCEx_Calibration_Start(&hadc1);

        for (uint32_t channel : channels) {
            uint32_t samples[kStartupSampleCount] = {0};
            uint32_t sum = 0;
            uint32_t minValue = UINT32_MAX;
            uint32_t maxValue = 0;

            for (uint8_t i = 0; i < kStartupSampleCount; ++i) {
                samples[i] = Read_ADC(channel);
                sum += samples[i];
                minValue = (samples[i] < minValue) ? samples[i] : minValue;
                maxValue = (samples[i] > maxValue) ? samples[i] : maxValue;
                HAL_Delay(5);
            }

            const uint32_t mean = (sum + (kStartupSampleCount / 2u)) / kStartupSampleCount;
            const uint32_t range = maxValue - minValue;
            uint32_t tolerance = (mean * kStartupStabilityPercent + 50u) / 100u;
            if (tolerance == 0u) {
                tolerance = 1u;
            }
            const uint32_t minAcceptableRange = (mean / 10000u) + 1u; // ~0.01% of mean, at least 1 count

            if (range < minAcceptableRange) {
                ConsoleManager::print("[DEBUG] Sensor startup self-test failed: ADC channel %lu too constant (range=%lu, mean=%lu)\r\n",
                                    static_cast<unsigned long>(channel),
                                    static_cast<unsigned long>(range),
                                    static_cast<unsigned long>(mean));
                return false;
            }

            if (range > tolerance) {
                ConsoleManager::print("[DEBUG] Sensor startup self-test failed: ADC channel %lu too unstable (range=%lu > tolerance=%lu, mean=%lu)\r\n",
                                    static_cast<unsigned long>(channel),
                                    static_cast<unsigned long>(range),
                                    static_cast<unsigned long>(tolerance),
                                    static_cast<unsigned long>(mean));
                return false;
            }

            for (uint8_t i = 0; i < kStartupSampleCount; ++i) {
                if (!SensorManager::isSampleStable(samples[i], mean)) {
                    ConsoleManager::print("[DEBUG] Sensor startup self-test failed: ADC channel %lu sample out of tolerance (sample=%lu, mean=%lu)\r\n",
                                        static_cast<unsigned long>(channel),
                                        static_cast<unsigned long>(samples[i]),
                                        static_cast<unsigned long>(mean));
                    return false;
                }
            }
        }

        return true;
    }

} // namespace h2h3
