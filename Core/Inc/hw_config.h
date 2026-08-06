/**
 * @file hw_config.h
 * @brief Hardware configuration definitions for different AT-USB LoRa Dongle variants
 * @date 2026-01-13
 *
 * This file defines hardware variants based on RF frequency band and oscillator type.
 * Set one of the HW_RF_* defines below or via build system (CMake/Makefile) to select
 * the hardware configuration.
 *
 * Available variants:
 * - HW_RF_868_XTAL: 868 MHz band with XTAL oscillator
 * - HW_RF_868_TCXO: 868 MHz band with TCXO oscillator
 * - HW_RF_915_XTAL: 915 MHz band with XTAL oscillator
 * - HW_RF_915_TCXO: 915 MHz band with TCXO oscillator
 */

#ifndef HW_CONFIG_H
#define HW_CONFIG_H

/* ============================================================================
 * Hardware Variant Selection
 * ============================================================================
 * Select hardware variant by:
 * 1. CMake: Use presets (cmake --preset 868-xtal-debug) or set HW_VARIANT variable
 * 2. Manual: Uncomment ONE of the defines below
 * 3. Build flags: Pass -DHW_RF_868_XTAL (or other variant) to compiler
 */

#ifndef HW_RF_868_XTAL
#ifndef HW_RF_868_TCXO
#ifndef HW_RF_915_XTAL
#ifndef HW_RF_915_TCXO
    // No variant defined via build system, use default
    #define HW_RF_915_TCXO    // Default configuration
    #pragma message "No HW variant defined via build system, using default: HW_RF_915_TCXO"
#endif
#endif
#endif
#endif

/* ============================================================================
 * Derived Configuration - DO NOT MODIFY BELOW
 * ============================================================================ */

/* Frequency Band Configuration */
#if defined(HW_RF_868_XTAL) || defined(HW_RF_868_TCXO)
    #define HW_RF_FREQ_BAND     "868 MHz"
    #define HW_RF_BASE_FREQ     869525000UL
#elif defined(HW_RF_915_XTAL) || defined(HW_RF_915_TCXO)
    #define HW_RF_FREQ_BAND     "915 MHz"
    #define HW_RF_BASE_FREQ     923300000UL
#else
    #error "No hardware variant defined! Please define one of: HW_RF_868_XTAL, HW_RF_868_TCXO, HW_RF_915_XTAL, HW_RF_915_TCXO"
#endif

/* Oscillator Type Configuration */
#if defined(HW_RF_868_TCXO) || defined(HW_RF_915_TCXO)
    #define HW_RF_OSC_TYPE      "TCXO"
    #define HW_USE_TCXO         true
#elif defined(HW_RF_868_XTAL) || defined(HW_RF_915_XTAL)
    #define HW_RF_OSC_TYPE      "XTAL"
    #define HW_USE_TCXO         false
#endif

/* Validation - ensure only one variant is defined */
#if (defined(HW_RF_868_XTAL) + defined(HW_RF_868_TCXO) + \
     defined(HW_RF_915_XTAL) + defined(HW_RF_915_TCXO)) != 1
    #error "Exactly ONE hardware variant must be defined!"
#endif

/* ============================================================================
 * Board Revision Selection
 * ============================================================================ */

#ifndef HW_BOARD_REV_0_0
#ifndef HW_BOARD_REV_1_0
    // No board revision defined, use default (current production board, not the obsolete v0.0 proto)
    #define HW_BOARD_REV_1_0
    #pragma message "No board revision defined, using default: v1.0"
#endif
#endif

/* Include board-specific pin definitions */
#if defined(HW_BOARD_REV_0_0)
    #include "hw_boards/hw_board_v0_0.h"
#elif defined(HW_BOARD_REV_1_0)
    #include "hw_boards/hw_board_v1_0.h"
#else
    #error "No board revision defined!"
#endif

#endif /* HW_CONFIG_H */
