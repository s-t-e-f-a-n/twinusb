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

namespace h2h3 {

    Telemetry SensorManager::currentData = {};

    /**
     * @brief Polls all ADC channels and updates the internal currentData object.
     * * This method iterates through voltage and current sensing channels,
     * populating the SensorData structure with the latest readings.
     */
    void SensorManager::poll() {
        currentData.voltage.vbusa_mv  = static_cast<uint16_t>(Read_ADC(ADC_CHANNEL_0));
        currentData.voltage.vbusb_mv  = static_cast<uint16_t>(Read_ADC(ADC_CHANNEL_1));
        currentData.voltage.vbusds_mv = static_cast<uint16_t>(Read_ADC(ADC_CHANNEL_7));

        currentData.current.ds1_ma    = static_cast<uint16_t>(Read_ADC(ADC_CHANNEL_4));
        currentData.current.ds2_ma    = static_cast<uint16_t>(Read_ADC(ADC_CHANNEL_5));
        currentData.current.ds3_ma    = static_cast<uint16_t>(Read_ADC(ADC_CHANNEL_6));
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
        sConfig.SamplingTime = ADC_SAMPLETIME_39CYCLES_5;

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
