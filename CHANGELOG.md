# Changelog

All notable changes to this firmware are documented here.
Format based on [Keep a Changelog](https://keepachangelog.com/), versions follow [SemVer](https://semver.org/) and match the value returned by `AT+IDENTIFY` (`FW_VERSION_MAJOR.MINOR.PATCH` in `Modules/ATInterface/AT_cmd.h`).

## [1.1.0] - 2026-08-06
### Changed
- AUX pin PWM: `aux7` (PA3/TIM21_CH2) and `aux8` (PA2/TIM2_CH3) are now driven by true hardware PWM timers instead of FreeRTOS software timers; the remaining AUX pins keep using software timers.
- `AUX_StartPWM()` now takes a frequency in Hz (`freq_hz`) instead of a period in ms.
- Added `AUX_IsHwPwmPin()` to query whether a given AUX index is HW- or SW-driven.
- Added `AUX_SwPwm_RoundDuty()` to report the nearest achievable duty cycle on the software-timer engine (limited to whole RTOS ticks), since HW PWM pins have no such rounding.
- Added separate min/max frequency bounds for the HW and SW PWM engines (`AUX_PWM_{SW,HW}_{MIN,MAX}_FREQ_HZ`), used both for `AT+AUX_PULSE` argument validation and its help text.

## [1.0.1]
### Fixed
- `AT+RF_RX_TO_UART` query handling.
- HDR error now checked correctly.

## [1.0.0] - Release
- Initial tagged release (`ReleaseV_1_0_0`).
