/**
 * @file AppMain.hpp
* @brief Main application entry point and global definitions.
 * @author s-t-e-f-a-n
 * @date 01.06.2026
 */

#ifndef APPMAIN_HPP_
#define APPMAIN_HPP_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Main entry point for the C++ application layer.
 *        This function bridges the C HAL initialization with C++.
 */
void AppMain(void);

#ifdef __cplusplus
}

#include "DisplayManager.hpp"

namespace h2h3 {
    // Global display manager instance for system-wide access
    extern DisplayManager* display;
}
#endif

#endif /* APPMAIN_HPP_ */
