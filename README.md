# Ropixon AT LoRa Dongle - User Manual

[![I sell on Tindie](https://d2ss6ovg47m0r5.cloudfront.net/badges/tindie-smalls.png)](https://www.tindie.com/products/nn_6394/at-lora-usb-dongle/)

**Ropixon AT-USB LoRa Dongle** enables sending and receiving all **LoRa** packets! Just configure it properly using intuitive **AT commands** via serial line. 

- No programming required.
- It's suitable for rapid development, testing, or creating your own wireless LoRa link.

## What is it for?

AT LoRa Dongle is suitable for:

- **Testing and development** of LoRa applications
- **Point-to-Point communication** over long distances
- **Range tests** - measuring capable of LoRa connection distance
- **Sensor networks** - collecting data from remote sensors
- **RF testing** - testing RF parameters, CW transmission
- **Scanning** - capturing random packets transmitted over the air

## Overview

When sending packets, no additional "wrapping" or encryption of data is used - you always send RAW (hex or ASCII) data. The same applies to reception.

Additional functions:
AT LoRa dongle has 8 aux pins - you can set its logical level or activate PWM on it - just through the AT commands.

![AT LoRa Dongle Hardware](HW_image.png)
*AT LoRa Dongle - hardware overview*

**For quick start:**

**Ready-to-use device available on [Tindie](https://www.tindie.com/products/nn_6394/at-lora-usb-dongle/).**

**1)** Install driver for serial interface CP2102 according to your system: [Download drivers](https://www.silabs.com/interface/usb-bridges/classic/device.cp2102?tab=softwareandtools)
 
**2)** Connect **AT dongle** to USB and set desired LoRa parameter configuration (or leave defaults)

**3)** Send your first LoRa packet via AT commands:

  - Send data in ASCII: ```AT+RF_TX_TXT=Hello, this is AT LoRa dongle Ropixon!```
  - Send data in HEX: ```AT+RF_TX_HEX=48656C6C6F2C2074686973206973204154204C6F526120646F6E676C6520526F7069786F6E21```

**4)** Reception is activated automatically - output format HEX/ASCII can be selected using `AT+RF_RX_FORMAT=HEX|ASCII, ?`

**Status LEDs**

The dongle has 3 LEDs serving these functions:

| LED | Function |
|-----|----------|
| 🔴 RED | Reception of new AT commands |
| 🟢 GREEN | System LED - indicates proper operation |
| 🔵 BLUE | Reception or transmission of LoRa data |


**Aux pins**

The **AT dongle** has 8 aux pins that can be "manually" controlled or run PWM modulation (maximum frequency 1 kHz): `AT+AUX_PULSE=<pin:1-8>,<period_ms>,<duty%:0-100>`

### Main Features

| Feature | Specification |
|---------|---------------|
| **RF LoRa Chip** | SX1262 LoRa transceiver from Semtech ([Datasheet](https://www.semtech.com/products/wireless-rf/lora-connect/sx1262)) |
| **Interface** | USB (Virtual COM port) |
| **Control** | AT commands via serial line |
| **Baud rates** | 9600, 19200, 38400, 57600, 115200, **230400** (default) |
| **Frequency** | 150 MHz - 960 MHz (optimized for 868 MHz band) |
| **TX power** | 0 to +22 dBm |
| **Spreading Factor** | SF5 to SF12 |
| **Bandwidths** | 7.81 kHz to 500 kHz |

> **Note:** RF path is optimized for 868 MHz band (EU: 863-870 MHz, USA: 915 MHz). The further from the operating band, the less power can be transmitted/received.


### AUX Pins

The dongle has 8 auxiliary pins that can be manually controlled or run PWM modulation (max frequency 1 kHz):

```
AT+AUX_PULSE=<pin:1-8>,<period_ms>,<duty%:0-100>
```

---

## Quick Start

**1.** Install driver according to your system: [Download drivers](https://www.silabs.com/interface/usb-bridges/classic/device.cp2102?tab=softwareandtools)

**2.** Connect **AT dongle** to USB and set desired LoRa parameter configuration (or leave defaults)

**3.** Send data via AT commands:
   - ASCII: `AT+RF_TX_TXT=Hello, this is AT LoRa dongle Ropixon!`
   - HEX: `AT+RF_TX_HEX=48656C6C6F2C2074686973206973204154204C6F526120646F6E676C6520526F7069786F6E21`

**4.** Reception is activated automatically - output format can be selected using `AT+RF_RX_FORMAT=HEX` or `AT+RF_RX_FORMAT=ASCII`

---

## Use Cases

- **Testing and development** of LoRa applications
- **Point-to-Point communication** over long distances
- **Range tests** - measuring LoRa connection range
- **Sensor networks** - collecting data from remote sensors
- **RF testing** - testing RF parameters, CW transmission
- **Scanning** - capturing packets transmitted over the air

---

## Serial Connection

| Parameter | Value |
|-----------|-------|
| Baud rate | 230400 (default) |
| Data bits | 8 |
| Parity | None |
| Stop bits | 1 |
| Flow control | None |
| Line ending | CR+LF (`\r\n`) |

### Communication Test

Send `AT` or `AT+HELP` - dongle will respond with a list of all available commands.

Send `AT+IDENTIFY` - dongle will respond with device identification:
```
Ropixon AT-USB LoRa_Dongle v1.0.0 UID:xxxxxxx.......
```

---

## AT Commands Reference

### System Commands

| Command | Description | Example |
|---------|-------------|---------|
| `AT` | Print help | `AT` |
| `AT+HELP` | Display all available commands | `AT+HELP` |
| `AT+IDENTIFY` | Return unique device ID | `AT+IDENTIFY` |
| `AT+FACTORY_RST` | Reset to factory defaults and restart | `AT+FACTORY_RST` |
| `AT+SYS_RESTART` | System restart | `AT+SYS_RESTART` |
| `AT+UART_BAUD` | Set/query baud rate (restarts on change) | `AT+UART_BAUD=230400` |

### LoRa TX Parameters

| Command | Description | Values | Example |
|---------|-------------|--------|---------|
| `AT+LR_TX_FREQ` | TX frequency in Hz | 150-960 MHz | `AT+LR_TX_FREQ=869525000` |
| `AT+LR_TX_POWER` | TX power | 0 to +22 dBm | `AT+LR_TX_POWER=14` |
| `AT+LR_TX_SF` | Spreading Factor | 5-12 | `AT+LR_TX_SF=7` |
| `AT+LR_TX_BW` | Bandwidth | 0-9 (see table below) | `AT+LR_TX_BW=7` |
| `AT+LR_TX_CR` | Coding Rate | 45, 46, 47, 48 | `AT+LR_TX_CR=45` |
| `AT+LR_TX_PREAMBLE` | Preamble length | symbols | `AT+LR_TX_PREAMBLE=8` |
| `AT+LR_TX_SYNCWORD` | Sync word | 0x0000-0xFFFF | `AT+LR_TX_SYNCWORD=0x3444` |
| `AT+LR_TX_CRC` | CRC enable/disable | 0=off, 1=on | `AT+LR_TX_CRC=1` |
| `AT+LR_TX_IQ` | IQ inversion | 0=off, 1=on | `AT+LR_TX_IQ=0` |
| `AT+LR_TX_HEADERMODE` | Header mode | 0=explicit, 1=implicit | `AT+LR_TX_HEADERMODE=0` |
| `AT+LR_TX_LDRO` | Low Data Rate Opt. | 0=off, 1=on, 2=auto | `AT+LR_TX_LDRO=2` |

### LoRa RX Parameters

Same parameters as TX, replace `TX` with `RX` in command names:
`AT+LR_RX_FREQ`, `AT+LR_RX_SF`, `AT+LR_RX_BW`, etc.

> **Important:** `AT+LR_RX_PLDLEN` - Payload length is required for implicit header mode.

### Bandwidth Reference

| Value | Bandwidth | | Value | Bandwidth |
|:-----:|----------:|-|:-----:|----------:|
| 0 | 7.81 kHz  | | 5 | 41.67 kHz |
| 1 | 10.42 kHz | | 6 | 62.5 kHz  |
| 2 | 15.63 kHz | | 7 | 125 kHz   |
| 3 | 20.83 kHz | | 8 | 250 kHz   |
| 4 | 31.25 kHz | | 9 | 500 kHz   |

### Quick Setup Commands

Configure all RF parameters at once:

**TX configuration:**
```
AT+LR_TX_SET=SF:7,BW:7,CR:45,Freq:869525000,IQInv:0,HeaderMode:0,CRC:1,Preamble:8,LDRO:2,Power:22
```

**RX configuration:**
```
AT+LR_RX_SET=SF:7,BW:7,CR:45,Freq:869525000,IQInv:0,HeaderMode:0,CRC:1,Preamble:8,LDRO:2
```

> **Tip:** Use [Semtech LoRa Calculator](https://www.semtech.com/design-support/lora-calculator) (select SX1262) to understand how different modulation settings affect your data/packet.

**Simple rule:** High BW + low SF = faster data rate but shorter range. Low BW + high SF = longer range but slower. Use `AT+RF_GET_TOA=<packet_size>` to check Time on Air.

### RF Transmission and Reception

| Command | Description | Example |
|---------|-------------|---------|
| `AT+RF_TX_HEX` | Transmit data in HEX format | `AT+RF_TX_HEX=48656C6C6F` |
| `AT+RF_TX_TXT` | Transmit data as text | `AT+RF_TX_TXT=Hello` |
| `AT+RF_RX_TO_UART` | Enable/disable RX output | `AT+RF_RX_TO_UART=1` |
| `AT+RF_RX_FORMAT` | RX data output format | `AT+RF_RX_FORMAT=HEX` |

**Received data format:**
```
+RX:<length>,<data>,RSSI:<rssi>
```
Example: `+RX:5,48656C6C6F,RSSI:-45` (5 bytes, data "Hello" in hex, RSSI -45 dBm)

### Saved Packets and Periodic Transmission

Store one packet in NVM memory for later or periodic transmission:

| Command | Description | Example |
|---------|-------------|---------|
| `AT+RF_SAVE_PACKET` | Save packet to memory | `AT+RF_SAVE_PACKET=010203` |
| `AT+RF_TX_SAVED` | Transmit saved packet once | `AT+RF_TX_SAVED` |
| `AT+RF_TX_SAVED_REPEAT` | Start/stop periodic TX | `AT+RF_TX_SAVED_REPEAT=ON` |
| `AT+RF_TX_NVM_PERIOD` | Set TX period in ms | `AT+RF_TX_NVM_PERIOD=1000` |
| `AT+RF_TX_PERIOD_STATUS` | Query periodic TX status | `AT+RF_TX_PERIOD_STATUS?` |

### RF Test Commands

| Command | Description | Example |
|---------|-------------|---------|
| `AT+RF_TX_CW` | Continuous wave (carrier only) | `AT+RF_TX_CW=1` |
| `AT+RF_GET_TOA` | Calculate Time on Air (ms) | `AT+RF_GET_TOA=10` |
| `AT+RF_GET_TSYM` | Get symbol time | `AT+RF_GET_TSYM` |

### AUX GPIO Pins (1-8)

| Command | Description | Example |
|---------|-------------|---------|
| `AT+AUX` | Set pin state | `AT+AUX=1,1` (pin 1 HIGH) |
| `AT+AUX_PULSE` | Start PWM | `AT+AUX_PULSE=1,1000,50` (1 Hz, 50% duty) |
| `AT+AUX_PULSE_STOP` | Stop PWM | `AT+AUX_PULSE_STOP=1` |

---

## Practical Examples

### Example 1: Point-to-Point Communication

**Transmitter:**
```
AT+LR_TX_SET=SF:9,BW:7,CR:45,Freq:869525000,IQInv:0,HeaderMode:0,CRC:1,Preamble:8,Power:22,LDRO:2
AT+RF_TX_HEX=AABBCCDD
```

**Receiver:**
```
AT+LR_RX_SET=SF:9,BW:7,CR:45,Freq:869525000,IQInv:0,HeaderMode:0,CRC:1,Preamble:8,LDRO:2
```

Receiver output: `+RX:4,AABBCCDD,RSSI:-47`

### Example 2: Range Test with Periodic TX

**TX Dongle** (sends packet every 2 seconds):
```
AT+LR_TX_SET=SF:12,BW:7,CR:45,Freq:869525000,IQInv:0,HeaderMode:0,CRC:1,Preamble:8,LDRO:2,Power:22
AT+RF_SAVE_PACKET=31313232333334343535
AT+RF_TX_NVM_PERIOD=2000
AT+RF_TX_SAVED_REPEAT=ON
```

**RX Dongle:**
```
AT+LR_RX_SET=SF:12,BW:7,CR:45,Freq:869525000,IQInv:0,HeaderMode:0,CRC:1,Preamble:8,LDRO:2
AT+RF_RX_TO_UART=1
AT+RF_RX_FORMAT=ASCII
```

Output: `+RX:10,1122334455,RSSI:-68`

### Example 3: CW Test

Transmit carrier wave at 868 MHz with 10 dBm:
```
AT+LR_TX_FREQ=868000000
AT+LR_TX_POWER=10
AT+RF_TX_CW=1
```

Stop: `AT+RF_TX_CW=0`

### Example 4: Change Baud Rate

```
AT+UART_BAUD?        → 115200
AT+UART_BAUD=230400  → OK (dongle restarts)
```

> **Note:** Update your terminal to the new baud rate after restart.

---

## Important Notes

### TX/RX Parameter Compatibility

For successful communication, these RF settings **must match** on both sides:

| Parameter | TX = RX |
|-----------|---------|
| Frequency | `TX_FREQ` = `RX_FREQ` |
| Spreading Factor | `TX_SF` = `RX_SF` |
| Bandwidth | `TX_BW` = `RX_BW` |
| Coding Rate | `TX_CR` = `RX_CR` |
| Sync Word | `TX_SYNCWORD` = `RX_SYNCWORD` |
| Header Mode | `TX_HEADERMODE` = `RX_HEADERMODE` |
| CRC | `TX_CRC` = `RX_CRC` |
| IQ Inversion | `TX_IQ` = `RX_IQ` |
| LDRO | Use `2` (auto) on both sides |

### Range vs. Speed Optimization

| Setting | Maximum Range | Maximum Speed |
|---------|---------------|---------------|
| SF | 12 | 5 |
| BW | 0 (7.81 kHz) | 9 (500 kHz) |
| CR | 48 (4/8) | 45 (4/5) |
| Power | 22 dBm | 22 dBm |
| Result | > 10 km (open space) | Fastest data rate |

### Header Modes

| Mode | Value | Description |
|------|-------|-------------|
| Explicit | 0 | Header contains packet length (recommended) |
| Implicit | 1 | No header - `RX_PLDLEN` must be set on receiver! |

### LDRO (Low Data Rate Optimization)

| Value | Description |
|-------|-------------|
| 0 | Off (manual) |
| 1 | On (manual) |
| 2 | **Auto (recommended)** - firmware selects based on SF/BW |

---

## Troubleshooting

### Dongle Not Responding

1. Check baud rate (try 115200 or 230400)
2. Verify **MCU-USB** jumpers are connected on both RX and TX headers
3. Check line ending (must be CR+LF)
4. Reconnect USB cable
5. Try `AT+SYS_RESTART`

### RX Not Receiving Data

1. Verify all RF parameters match between TX and RX
2. Enable RX output: `AT+RF_RX_TO_UART=1`
3. Increase TX power or reduce distance

### Low RSSI / Packet Loss

1. Increase TX power (max 22 dBm)
2. Reduce distance
3. Increase SF (7 → 9 → 12)
4. Use external antenna (requires resoldering 0402 jumper from chip antenna to U.FL connector)

### CRC Errors

1. Change frequency (RF interference)
2. Verify TX/RX parameter match
3. Set LDRO to `2` (auto) on both sides

---

## Technical Specifications

| Component | Specification |
|-----------|---------------|
| **MCU** | STM32L071 (ARM Cortex-M0+) |
| **RF chip** | Semtech SX1262 |
| **Frequency** | 150 MHz - 960 MHz |
| **TX power** | 0 to +22 dBm |
| **RX sensitivity** | -149.1 dBm (SF12, BW 7.81 kHz) |
| **USB** | Virtual COM port (CP2102) |
| **GPIO** | 8x AUX pins (PWM capable) |
| **Power supply** | 5V from USB |
| **Operating temp.** | -20°C to +70°C |

---

## Appendix: Python Automation

Example script using `pyserial` for automated testing:

```python
#!/usr/bin/env python3
import serial
import time

# Configuration
PORT = "COM3"           # Windows: COM3, Linux: /dev/ttyUSB0
BAUD = 230400
FREQ = 869525000
DATA_HEX = "48656C6C6F" # "Hello"

def send_AT(ser, cmd, timeout=2.0):
    """Send AT command and wait for response."""
    ser.reset_input_buffer()
    ser.write((cmd + "\r\n").encode())

    t0, resp = time.time(), ""
    while time.time() - t0 < timeout:
        if ser.in_waiting:
            resp += ser.read(ser.in_waiting).decode(errors="ignore")
            if "OK" in resp or "ERROR" in resp:
                break
        time.sleep(0.01)
    return resp.strip()

# Main
ser = serial.Serial(PORT, BAUD, timeout=1)
time.sleep(0.5)

# Verify device
resp = send_AT(ser, "AT+IDENTIFY")
if "Ropixon AT-USB LoRa_Dongle" not in resp:
    print(f"Unknown device: {resp}")
    ser.close()
    exit(1)

print(resp)

# Configure and send
send_AT(ser, f"AT+LR_TX_SET=SF:7,BW:7,CR:45,Freq:{FREQ},IQInv:0,HeaderMode:0,CRC:1,Preamble:8,LDRO:2,Power:22")

for i in range(5):
    resp = send_AT(ser, f"AT+RF_TX_HEX={DATA_HEX}", timeout=3.0)
    print(f"Packet {i+1}: {'OK' if 'OK' in resp else 'FAIL'}")
    time.sleep(1.0)

ser.close()
```

---

**© 2024-2026 Ropixon - AT LoRa Dongle**
