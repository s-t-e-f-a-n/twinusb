    /**
     * @file SensorManager.hpp
     * @brief Manages ADC data acquisition for three V and three I channels
     * @author s-t-e-f-a-n
     * @date 01.06.2026
     */
    #ifndef INC_SENSORMANAGER_HPP_
    #define INC_SENSORMANAGER_HPP_

    #include <cstdint>
    #include "Types.hpp"

    namespace h2h3 {

    /**
     * @class SensorManager
     * @brief Handles reading and scaling of raw ADC values from hardware.
     */
    class SensorManager {
    public:
        /**
         * @brief Performs one full acquisition cycle of all ADC channels.
         */
        static void poll();

        /**
         * @brief Performs a startup sensor health check.
         * @details Samples each ADC channel multiple times and verifies that
         * the raw values stay within a small tolerance around their mean.
         * @return true if all channels pass the stability check, false otherwise.
         */
        static bool startupSelfTest();

        /**
         * @brief Retrieves the latest telemetry data.
         * @return Telemetry struct containing voltage and current data.
         */
        static const Telemetry& getTelemetry();

    private:
        static Telemetry currentData;

        static constexpr uint8_t kStartupSampleCount = 8;
        static constexpr uint8_t kStartupStabilityPercent = 10;

        /**
         * @brief Reads a raw 12-bit value from a specified ADC channel.
         * @param channel The ADC channel index (e.g., ADC_CHANNEL_0).
         * @return The 12-bit raw conversion result.
         */
        static uint32_t Read_ADC(uint32_t channel);

        static bool isSampleStable(uint32_t sample, uint32_t mean);
    };

    } // namespace h2h3


    #endif /* INC_SENSORMANAGER_HPP_ */
