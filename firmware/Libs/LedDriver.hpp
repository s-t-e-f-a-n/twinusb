/**
 * @file LedDriver.hpp
 * @brief Hardware driver for RGB LED using STM32 TIM3 PWM.
 * @details Configured for active-low operation via TIM3 Channels 1, 3, and 4.
 */

#ifndef LEDDRIVER_HPP
#define LEDDRIVER_HPP


#include <cstdint>
#include "stm32g0xx_hal_tim.h"

namespace h2h3 {

// Forward declaration of the timer handle
typedef TIM_HandleTypeDef PwmController;

class LedDriver {
public:
    struct RGB { 
        uint8_t r, g, b; 
        // Constructor
        RGB(uint8_t _r = 0, uint8_t _g = 0, uint8_t _b = 0) : r(_r), g(_g), b(_b) {}

        // Copy constructor from volatile
        RGB(const volatile RGB& other) : r(other.r), g(other.g), b(other.b) {}

        // Assignment operator for volatile targets
        void operator=(const RGB& other) volatile {
            r = other.r; g = other.g; b = other.b;
        }
    };

    LedDriver(const LedDriver&) = delete;             // Disable copy constructor
    LedDriver& operator=(const LedDriver&) = delete;  // Disable copy assignment

private:
    PwmController* pwm;
    static constexpr uint32_t LEDRn = TIM_CHANNEL_1; 
    static constexpr uint32_t LEDBn = TIM_CHANNEL_3; 
    static constexpr uint32_t LEDGn = TIM_CHANNEL_4; 
    bool activeLow;

    // Animation state
    volatile RGB currentColor;
    volatile RGB targetColor;
    volatile RGB color1;
    volatile RGB color2;
    volatile bool transitionActive = false;
    volatile bool pulsingEnabled = false;

    // Static instance for ISR callback
    static LedDriver* instance;
    
public:
    /**
     * @brief Constructs the LedDriver and initializes animation state.
     * @details Sets up the PWM controller handle and configures the active-low logic.
     * Initializes the LED to an 'OFF' state with all animation buffers zeroed.
     * * @param controller Pointer to the configured STM32 TIM_HandleTypeDef (e.g., &htim3).
     * @param invert If true, logic is inverted (common for active-low common-anode LEDs). 
     * Defaults to true.
     */
    LedDriver(PwmController* controller, bool invert = true) 
        : pwm(controller), activeLow(invert), 
          currentColor{0,0,0}, targetColor{0,0,0}, color1{0,0,0}, color2{0,0,0} {
            instance = this; // register
          }
    
    /**
     * @brief Static callback to bridge C-linkage HAL interrupts to C++ instance.
     */
    static void timerCallback(TIM_HandleTypeDef* htim) {
        if (instance != nullptr && htim->Instance == TIM14) {
            instance->processLedAnimation();
        }
    }

    /** * @brief Starts PWM and sets an initial color. 
     * @param r, g, b Initial intensities (0-255).
     */
    void init(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0) {
        setColor(r, g, b);
        HAL_TIM_PWM_Start(pwm, LEDRn);
        HAL_TIM_PWM_Start(pwm, LEDGn);
        HAL_TIM_PWM_Start(pwm, LEDBn);
    }

    /**
     * @brief Sets RGB duty cycles.
     * @param r, g, b Red, Green, Blue intensity (0-255).
     */
    void setColor(uint8_t r, uint8_t g, uint8_t b) {
        currentColor = RGB(r, g, b);
        updateHardware(r, g, b);
    }
/**
     * @brief Sets primary color for constant mode.
     */
    void setPrimaryColor(uint8_t r, uint8_t g, uint8_t b) {
        pulsingEnabled = false;
        transitionActive = false;
        setColor(r, g, b);
    }

    /**
     * @brief Enables/disables breathing effect.
     * @param active True to start pulsing, false to stop.
     */
    void setPulse(bool active) {
        if (active) {
            // Copy the current (volatile) color to a local non-volatile struct
            RGB current = {currentColor.r, currentColor.g, currentColor.b};
            
            // Assign the local struct to the volatile targetColor
            targetColor = current;
        }
        transitionActive = false;
        pulsingEnabled = active;
    }

    /**
     * @brief Enables/disables Transition effect.
     */
    void setTransition(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2) {
        color1 = {r1, g1, b1};
        color2 = {r2, g2, b2};
        transitionActive = true;
        pulsingEnabled = true;
    }

/**
     * @brief Processes animation logic.
     * @details Call this from a high-frequency Timer ISR (e.g., 50Hz / 20ms).
     */
    void processLedAnimation() {
        if (!pulsingEnabled) return;

        static uint8_t pulseStep = 0;
        static int8_t direction = 1;

        pulseStep += direction;
        if (pulseStep >= 255 || (direction == -1 && pulseStep == 0)) direction *= -1;

        RGB out;
        if (transitionActive) {
            out.r = color1.r + ((int16_t)(color2.r - color1.r) * pulseStep / 255);
            out.g = color1.g + ((int16_t)(color2.g - color1.g) * pulseStep / 255);
            out.b = color1.b + ((int16_t)(color2.b - color1.b) * pulseStep / 255);
        } else {
            out.r = (targetColor.r * pulseStep) / 255;
            out.g = (targetColor.g * pulseStep) / 255;
            out.b = (targetColor.b * pulseStep) / 255;
        }
        updateHardware(out.r, out.g, out.b);
    }
private:
    void updateHardware(uint8_t r, uint8_t g, uint8_t b) {
        __HAL_TIM_SET_COMPARE(pwm, LEDRn, activeLow ? (255 - r) : r);
        __HAL_TIM_SET_COMPARE(pwm, LEDGn, activeLow ? (255 - g) : g);
        __HAL_TIM_SET_COMPARE(pwm, LEDBn, activeLow ? (255 - b) : b);
    }
};

} // namespace h2h3

#endif