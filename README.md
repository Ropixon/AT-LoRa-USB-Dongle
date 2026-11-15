# Ropixon AT LoRa USB Dongle

<table>
<tr>
<td>
<p><strong>Send, receive and inspect raw LoRa packets from your computer.</strong></p>
<p>Plug in the SX1262 dongle and open a serial terminal. Send text or hex with an AT command. On receive, see the packet data, RSSI and SNR together.</p>
<p><strong>Send</strong> <code>AT+RF_TX_TXT=Hello</code><br>
<strong>Receive</strong> <code>+RX:5,48656C6C6F,RSSI:-45,SNR:9</code></p>
<p><strong><a href="https://www.tindie.com/products/nn_6394/at-lora-usb-dongle/">Buy on Tindie</a></strong> · <a href="#quick-start">Quick start</a> · <a href="#build-with-ai">Build with AI</a> · <a href="AT_LORA_DONGLE_MANUAL_EN.md">AT command manual</a></p>
</td>
<td align="center">
<img src="HW_image.png" alt="Ropixon SX1262 LoRa dongle with USB-A connector and AUX pin headers" width="265">
</td>
</tr>
</table>

## What you can do with it

| Use case | What the dongle does | Try this |
| --- | --- | --- |
| **Debug a LoRa node** | View raw payloads with per-packet RSSI and SNR. Send your own ASCII or hex test packets. | `AT+RF_RX_FORMAT=HEX` · `AT+RF_TX_HEX=48656C6C6F` |
| **Build a point-to-point link** | Configure TX and RX separately, including frequency, spreading factor, bandwidth and sync word. | `AT+LR_TX_SET` · `AT+LR_RX_SET` |
| **Compare antennas or test range** | Repeat a saved packet from one end and compare packet reception, RSSI and SNR at the other end. | `AT+RF_TX_SAVED_REPEAT=ON` |
| **Check the channel before sending** | Use LoRa channel activity detection (CAD) on the selected TX profile, or let the dongle wait for the channel to clear before transmitting. | `AT+RF_CAD` · `AT+RF_TX_LBT=500,0,48656C6C6F` |
| **Run RF bench tests** | Generate a continuous carrier for lab measurements or calculate the airtime for a packet. | `AT+RF_TX_CW` · `AT+RF_GET_TOA=5` |
| **Control test signals** | Set eight AUX outputs or generate PWM. Pins 7 and 8 use hardware PWM. | `AT+AUX=1,1` · `AT+AUX_PULSE=7,1000,50` |
| **Automate a test or build an app** | Use USB serial from Python. Read packet events and wait for the radio to finish transmitting. | `AT+CAPS` · `+TX:DONE` · [Python example](examples/send_packet.py) |

Firmware 1.3.0 added SNR in received packet reports. Firmware 1.4.0 added CAD and listen before talk. Use `AT+IDENTIFY` to check the firmware and `AT+CAPS` to check supported features.

## Build with AI

Give [the AT command manual](https://github.com/Ropixon/AT-LoRa-USB-Dongle/blob/master/AT_LORA_DONGLE_MANUAL_EN.md) to ChatGPT, Claude or Copilot. Start with the prompt below to generate a Python tool for sending packets and logging their signal quality. You can also ask for a desktop serial monitor, a CSV logger or a range-test tool for two dongles.

> **Prompt to copy**
>
> Using the attached Ropixon AT LoRa USB Dongle manual, write a Python 3 tool with pyserial. Let me choose the serial port and baud rate. Use 230400 baud, 8N1, no flow control and CR+LF by default. Check the device with `AT+IDENTIFY` and read `AT+CAPS` before enabling any features that depend on firmware support. Keep the current RF settings. Enable serial packet output with `AT+RF_RX_TO_UART=1` and set `AT+RF_RX_FORMAT=HEX` in listen mode.
>
> Make the default mode listen for HEX packets and log timestamps, payload, RSSI and SNR when present. Add an option to send one hex packet with `AT+RF_TX_HEX`. Report transmit completion on `+TX:DONE` and show command errors and timeouts. Include the install and run commands for Windows and use only AT commands documented in the manual.

A working starting point for the transmit side is already in the repo. From the repository folder, use the following commands with an idle dongle and replace `COM3` with its serial port.

```powershell
python -m pip install pyserial
python examples/send_packet.py COM3
```

## Quick start

1. Plug the dongle into USB-A. If a serial port does not appear, install the [Silicon Labs CP210x VCP driver](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers).
2. Open the serial port at **230400 baud, 8N1, no flow control**, with **CR+LF** line endings. Send `AT+IDENTIFY` to see the firmware and board variant.
3. For a two-dongle test, replace `FREQUENCY_HZ` below with the same permitted frequency in Hz for both dongles and their hardware band. The examples set both profiles explicitly so they also work when the dongles had different settings before the test.

**On the receiving dongle**

```text
AT+LR_RX_SET=SF:7,BW:7,CR:45,Freq:FREQUENCY_HZ,IQInv:0,HeaderMode:0,CRC:1,Preamble:8,LDRO:2
AT+LR_RX_SYNCWORD=12
AT+RF_RX_TO_UART=1
AT+RF_RX_FORMAT=HEX
```

**On the transmitting dongle**

```text
AT+LR_TX_SET=SF:7,BW:7,CR:45,Freq:FREQUENCY_HZ,IQInv:0,HeaderMode:0,CRC:1,Preamble:8,LDRO:2,Power:0
AT+LR_TX_SYNCWORD=12
AT+RF_TX_TXT=Hello
```

**Example receiver output**

```text
+RX:5,48656C6C6F,RSSI:-45,SNR:9
```

`48656C6C6F` is `Hello` in hex. RSSI is in dBm and SNR is in dB. You can also use any compatible raw LoRa device with matching radio settings as the other end of the link.

## More examples

### Repeat a test packet

After setting the TX and RX profiles as above, save `Hello` and repeat it every five seconds. Watch the receiver while you move it or swap an antenna.

```text
AT+RF_SAVE_PACKET=48656C6C6F
AT+RF_TX_NVM_PERIOD=5000
AT+RF_TX_SAVED_REPEAT=ON
```

Stop with `AT+RF_TX_SAVED_REPEAT=OFF`.

### Check the channel and calculate packet airtime

```text
AT+RF_CAD
AT+RF_GET_TOA=5
```

Wait for `+CAD:FREE`, `+CAD:BUSY` or `+CAD:ERROR` before starting the next RF operation. CAD checks for LoRa activity using the configured TX spreading factor and bandwidth. The airtime command uses the TX profile and a payload length of five bytes.

### Generate a PWM signal

```text
AT+AUX_PULSE=7,1000,50
AT+AUX_PULSE_STOP=7
```

The first command starts 1 kHz at 50% duty cycle on AUX 7. The second stops it.

## Hardware at a glance

| | |
| --- | --- |
| **LoRa radio** | Semtech SX1262 · SF5 to SF12 · bandwidths from 7.81 to 500 kHz |
| **Hardware bands** | Firmware targets for 868 MHz and 915 MHz boards. See the listing for the version on sale. |
| **USB** | USB-A with CP2102 virtual serial port · USB powered |
| **AUX outputs** | 8 outputs · hardware PWM on AUX 7 and 8 up to 320 kHz · software PWM on AUX 1–6 up to 500 Hz |

The dongle handles the raw LoRa radio link. Your application defines the payload and protocol.

## Documentation and files

- [English AT command manual](AT_LORA_DONGLE_MANUAL_EN.md) · [Czech user manual](AT_LORA_DONGLE_MANUAL.md)
- [Python send example](examples/send_packet.py) · [RF test script](rf_range_test.py)
- [Hardware files](Hardware/) · [Build instructions](BUILD_INSTRUCTIONS.md) · [Firmware changelog](CHANGELOG.md)
