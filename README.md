# Ropixon AT LoRa USB Dongle

**Send and receive raw LoRa packets from your computer.**

Plug the SX1262 dongle into USB, open a serial terminal, and send a packet with one AT command. See the payload and signal quality (RSSI and SNR) for each received packet. Use it to debug a LoRa device, test a radio link, or automate RF tests with Python.

**[Buy on Tindie](https://www.tindie.com/products/nn_6394/at-lora-usb-dongle/) · [Quick start](#quick-start) · [AT command manual](AT_LORA_DONGLE_MANUAL_EN.md)**

<img src="HW_image.png" alt="Ropixon LoRa USB dongle with its USB-A connector and AUX pin headers" width="400">

| Send from the first dongle | See on the receiving dongle |
| --- | --- |
| `AT+RF_TX_TXT=Hello` | `+RX:5,48656C6C6F,RSSI:-45,SNR:9` |

Example with matching radio settings and HEX output. `48656C6C6F` is the five-byte payload `Hello`. RSSI is in dBm and SNR is in dB.

## What you can do with it

- **Debug a LoRa device.** Match its radio settings and inspect the packets it sends. Transmit your own text or hex payloads to test how it responds.
- **Compare antennas and test range.** Set one dongle to repeat a saved packet. Use the other to compare RSSI, SNR and packet reception at different locations or with different settings.
- **Automate RF tests.** Use a USB serial port from Python, configure TX and RX separately, and wait for `+TX:DONE` before moving on to the next test.

## Quick start

1. Plug the dongle into USB. If no serial port appears, install the [Silicon Labs CP210x VCP driver](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers).
2. Open the port in a serial terminal at **230400 baud, 8N1, no flow control**. Set the terminal to send **CR+LF**. Use the configured baud rate if it was changed.
3. Send `AT+IDENTIFY` to check the connection and see the firmware version and hardware variant.
4. To see the packet on a second dongle, configure its RX profile to match the sender, then run `AT+RF_RX_TO_UART=1` and `AT+RF_RX_FORMAT=HEX` on the receiver. See [which settings to match](AT_LORA_DONGLE_MANUAL_EN.md#txrx-parameter-compatibility).
5. Send `AT+RF_TX_TXT=Hello` on the transmitter. On firmware with the `TX_DONE` capability, `+TX:DONE` confirms that the radio finished transmitting. Check the receiver for its `+RX:` line.

A second dongle makes the demo easy. A compatible LoRa device with matching radio settings can also act as the other end of the link.

## Radio and hardware

- **Radio** — Semtech SX1262, with independently configurable TX and RX profiles.
- **Hardware variants** — Firmware build targets cover 868 MHz and 915 MHz hardware. Check the Tindie options for the version on sale.
- **RF tools** — Periodic packets, channel activity detection (CAD), listen before talk, continuous wave (CW) and a time-on-air calculator. Feature support depends on firmware version.
- **USB and I/O** — CP2102 USB serial interface and eight AUX outputs with PWM support. AUX 7 and 8 use hardware PWM.

## Documentation and examples

| I want to… | Where to go |
| --- | --- |
| Look up an AT command or troubleshoot a link | [English user manual](AT_LORA_DONGLE_MANUAL_EN.md) · [Czech user manual](AT_LORA_DONGLE_MANUAL.md) |
| Send a packet from Python | [Python quick start](examples/send_packet.py) · [Setup and usage](AT_LORA_DONGLE_MANUAL_EN.md#python-automation) |
| Run the developer RF tests | [RF test script](rf_range_test.py) |
| Build firmware for my board | [Build instructions](BUILD_INSTRUCTIONS.md) · [Firmware changelog](CHANGELOG.md) |
| See the schematic or PCB files | [Hardware files](Hardware/) |
