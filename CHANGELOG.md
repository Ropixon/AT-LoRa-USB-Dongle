# Changelog

All notable changes to this firmware are documented here.
Format based on [Keep a Changelog](https://keepachangelog.com/), versions follow [SemVer](https://semver.org/) and match the value returned by `AT+IDENTIFY` (`FW_VERSION_MAJOR.MINOR.PATCH` in `Modules/ATInterface/AT_cmd.h`).

## [1.4.0] - 2026-11-07
### Added
- `AT+RF_TX_LBT=<max_ms>,<force>,<HEX data>` - transmit after a CAD channel check. While the channel is busy the dongle stays in RX and retries after a random backoff, for at most `max_ms`. After that the packet is sent anyway (`force=1`) or discarded (`force=0`). New `AT+CAPS` token `TX_LBT`.
- `+TX:BUSY` asynchronous notification - an `AT+RF_TX_LBT` packet was discarded because the channel stayed busy, or the pending transmission was cancelled by another radio command.

## [1.3.0] - 2026-10-22
### Added
- `+RX:` notification now includes packet SNR: `+RX:<len>,<data>,RSSI:<rssi>,SNR:<snr>` (SNR in dB). New `AT+CAPS` token `RX_SNR`.
- `+RADIO:ERROR` asynchronous notification - LoRa radio failed to initialize, or TX was requested while the radio is not working. New `AT+CAPS` token `RADIO_ERR`.
### Changed
- Radio init failure no longer resets the dongle - it is reported as `+RADIO:ERROR`, the AT interface keeps running.
- SX1262 BUSY pin wait now has a 100 ms timeout, so an unresponsive radio can no longer hang the RF task.

## [1.2.0] - 2026-10-06
### Added
- `AT+CAPS` command - returns firmware capabilities as one line: `+CAPS:HOST_V1,TX_DONE,...` (`HOST_V1` is required by the Python host driver).
- `+TX:DONE` asynchronous notification after every completed RF transmission.
- `AT+HELP` now lists asynchronous notifications (`+RX:`, `+TX:DONE`).
### Fixed
- Numeric query responses (e.g. `AT+LR_TX_FREQ?`, `AT+LR_TX_SF?`, `AT+RF_TX_NVM_PERIOD?`) are now terminated with `\r\n` like all other responses.

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
