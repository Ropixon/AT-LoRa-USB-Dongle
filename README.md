# Ropixon AT LoRa Dongle - User Manual

## What is AT LoRa Dongle?

**Ropixon AT-USB LoRa Dongle** enables sending and receiving all **LoRa** packets! Just configure it properly using intuitive **AT commands** via serial line.
- No programming required.
- It's suitable for rapid development, testing, or creating your own wireless LoRa link for greater distances than Wi-Fi, BLE, or other wireless modulations like FSK or OOK.
- Maximum Link Budget is 155.1 dB!

When sending packets, no additional "wrapping" or encryption of data is used - you always send RAW (hex or ASCII) data. The same applies to reception.

**For quick start:**

**1)** Install driver according to your system: [Download drivers](https://www.silabs.com/interface/usb-bridges/classic/device.cp2102?tab=softwareandtools)

**2)** Connect **AT dongle** to USB and set desired LoRa parameter configuration (or leave defaults)

**3)** Send data via AT commands:
 - Send data in ASCII: ```AT+RF_TX_TXT=Hello, this is AT LoRa dongle Ropixon!```
 - Send data in HEX: ```AT+RF_TX_HEX=48656C6C6F2C2074686973206973204154204C6F526120646F6E676C6520526F7069786F6E21```
   
**4)** Reception is activated automatically - output format HEX/ASCII can be selected using `AT+RF_RX_FORMAT=HEX|ASCII, ?`



**Aux pins**

The **AT dongle** has 8 aux pins that can be "manually" controlled or run PWM modulation (maximum frequency 1 kHz): `AT+AUX_PULSE=<pin:1-8>,<period_ms>,<duty%:0-100>`

**Status LEDs**
The dongle has 3 LEDs serving these functions:
- RED = Reception of new AT commands
- GREEN = System LED - indicates proper operation of the entire device
- BLUE = Reception or transmission of LoRa data

### Main Features:
- **RF LoRa Chip**: SX1262 LoRa transceiver from Semtech - Datasheet here: [SX1262](https://www.semtech.com/products/wireless-rf/lora-connect/sx1262)
- **Interface**: USB (Virtual COM port) - Drivers can be found here: [Download drivers](https://www.silabs.com/interface/usb-bridges/classic/device.cp2102?tab=softwareandtools)

- **Control**: AT commands via serial line
- **Supported baud rates**: 9600, 19200, 38400, 57600, 115200, 230400 (default/recommended)
- **Frequency**: 150 MHz - 960 MHz (typically **863-870 MHz for EU, 915 MHz for USA)**, RF path is optimized for 868 MHz band, but 915 MHz operation is also fine. The further from the operating band, the less power can be transmitted/received from the dongle.
- **TX power**: up to **22 dBm**
- **Spreading Factor**: SF5 to SF12
- **Bandwidths**: 7.81 kHz to 500 kHz

### 1. What is it for?

AT LoRa Dongle is suitable for:
- **Testing and development** of LoRa applications
- **Point-to-Point communication** over long distances
- **Range tests** - measuring LoRa connection range
- **Sensor networks** - collecting data from remote sensors
- **RF testing** - testing RF parameters, CW transmission
- **Scanning** - scanning and capturing "random" packets transmitted over the air

### 2. Serial terminal connection configuration:
- **Baud rate**: 230400 (default)
- **Data bits**: 8
- **Parity**: None
- **Stop bits**: 1
- **Flow control**: None
- **Line ending**: CR+LF (\r\n)

### 3. Communication test
Send command:
```
AT (or AT+HELP)
```
Dongle will respond with a list of all available commands.
```
AT+IDENTIFY
```
Dongle will respond "Ropixon AT-USB LoRa_Dongle v1.0.0 UID:xxxxxxx......."

---

## AT Commands - Complete Overview

### Basic System Commands

| Command | Description | Example |
|---------|-------------|---------|
| `AT` | Print help | `AT` |
| `AT+HELP` | Display all available commands | `AT+HELP` |
| `AT+IDENTIFY` | Return unique device ID | `AT+IDENTIFY` |
| `AT+FACTORY_RST` | Reset all settings to factory defaults and restart | `AT+FACTORY_RST` |
| `AT+SYS_RESTART` | System restart | `AT+SYS_RESTART` |
| `AT+UART_BAUD` | Set/query baud rate, restart on change | `AT+UART_BAUD=230400` |

### LoRa TX Parameters (transmission)

| Command | Description | Value Range | Example |
|---------|-------------|-------------|---------|
| `AT+LR_TX_FREQ` | TX frequency in Hz | - | `AT+LR_TX_FREQ=869525000` |
| `AT+LR_TX_POWER` | TX power | -17 to 22 dBm | `AT+LR_TX_POWER=14` |
| `AT+LR_TX_SF` | Spreading Factor | 5 to 12 | `AT+LR_TX_SF=7` |
| `AT+LR_TX_BW` | Bandwidth | 0-9 (see table) | `AT+LR_TX_BW=7` |
| `AT+LR_TX_CR` | Coding Rate | 45, 46, 47, 48 | `AT+LR_TX_CR=45` |
| `AT+LR_TX_PREAMBLE` | Preamble length | symbols | `AT+LR_TX_PREAMBLE=8` |
| `AT+LR_TX_SYNCWORD` | Sync word | 0x0000-0xFFFF | `AT+LR_TX_SYNCWORD=0x3444` |
| `AT+LR_TX_CRC` | CRC enable/disable | 0, 1 | `AT+LR_TX_CRC=1` |
| `AT+LR_TX_IQ` | IQ inversion | 0, 1 | `AT+LR_TX_IQ=0` |
| `AT+LR_TX_HEADERMODE` | Header mode | 0=explicit, 1=implicit | `AT+LR_TX_HEADERMODE=0` |
| `AT+LR_TX_LDRO` | Low Data Rate Optimization | 0=off, 1=on, 2=auto | `AT+LR_TX_LDRO=2` |

### LoRa RX Parameters (reception)

Same parameters as TX, just replace `TX` with `RX`:
- `AT+LR_RX_FREQ`, `AT+LR_RX_SF`, `AT+LR_RX_BW`, etc.

**Important**: `AT+LR_RX_PLDLEN` - Payload length (required for implicit header mode)

### Bandwidth Values

| Value | Bandwidth |
|-------|-----------|
| 0 | 7.81 kHz |
| 1 | 10.42 kHz |
| 2 | 15.63 kHz |
| 3 | 20.83 kHz |
| 4 | 31.25 kHz |
| 5 | 41.67 kHz |
| 6 | 62.5 kHz |
| 7 | 125 kHz |
| 8 | 250 kHz |
| 9 | 500 kHz |

### Quick Setup (all RF parameters at once)

**TX configuration:**
```
AT+LR_TX_SET=SF:7,BW:7,CR:45,Freq:869525000,IQInv:0,HeaderMode:0,CRC:1,Preamble:8,LDRO:2,Power:22
```

**RX configuration:**
```
AT+LR_RX_SET=SF:7,BW:7,CR:45,Freq:869525000,IQInv:0,HeaderMode:0,CRC:1,Preamble:8,LDRO:2
```


**How to navigate all parameter configurations?** 
Here is calculator showing what happens to data/packet with different modulation settings
(select SX1262 as device):

- Online calculator: [Semtech LoRa Calculator](https://www.semtech.com/design-support/lora-calculator)
- Offline calculator (PDF): [Semtech SX1262 Calculator](https://semtech.my.salesforce.com/sfc/p/E0000000JelG/a/2R000000Q2OT/GhbZe2lGVNO6sNDUlo6lcHVaKMQvcVCdaYfFeSjyitk)

A simple rule applies:
For high data rates, choose high BW and low SF - however, this reduces the receiver's ability to obtain data at greater distances. Conversely, for poor transmission conditions, SF needs to be increased and BW decreased. All this affects packet Time on Air (TOA). TOA can also be easily verified using the command: `AT+RF_GET_TOA=<packet_size_bytes>`, the result is calculated from the current LoRa TX settings.
### RF Transmission and Reception

| Command | Description | Example |
|---------|-------------|---------|
| `AT+RF_TX_HEX` | Transmit data in HEX format | `AT+RF_TX_HEX=48656C6C6F` |
| `AT+RF_TX_TXT` | Transmit data as text | `AT+RF_TX_TXT=Hello` |
| `AT+RF_RX_TO_UART` | Enable/disable RX to UART | `AT+RF_RX_TO_UART=1` |
| `AT+RF_RX_FORMAT` | RX data output format | `AT+RF_RX_FORMAT=HEX` |

**Received data is displayed automatically** (if `RX_TO_UART=1 (ON), =0 (OFF), ?`
Received data output format:
```
+RX:5,48656C6C6F,RSSI:-45
```
Where **5** = number of received bytes, **48656C6C6F** = packet itself, **-45** = packet RSSI

### Saved Packets and Periodic Transmission
One packet can be stored in the **AT dongle** (NVM memory), which can then be sent at any time or set for periodic transmission
| Command | Description | Example |
|---------|-------------|---------|
| `AT+RF_SAVE_PACKET` | Save packet to memory | `AT+RF_SAVE_PACKET=010203` |
| `AT+RF_TX_SAVED` | Transmit saved packet once | `AT+RF_TX_SAVED` |
| `AT+RF_TX_SAVED_REPEAT` | Start/stop periodic TX | `AT+RF_TX_SAVED_REPEAT=ON` |
| `AT+RF_TX_NVM_PERIOD` | Set saved packet TX period | `AT+RF_TX_NVM_PERIOD=1000` |
| `AT+RF_TX_PERIOD_STATUS` | Current periodic TX status | `AT+RF_TX_PERIOD_STATUS?` |

### RF Test Commands
For various testing purposes, these 3 commands may be useful, especially `AT+RF_TX_CW=1 (ON), =0 (OFF), ?`, which enables carrier wave transmission (no modulated data).

| Command | Description | Example |
|---------|-------------|---------|
| `AT+RF_TX_CW` | Enable/disable CW (continuous wave) transmission | `AT+RF_TX_CW=1` |
| `AT+RF_GET_TOA` | Calculate Time on Air from current TX settings, parameter is packet length in bytes | `AT+RF_GET_TOA=10` |
| `AT+RF_GET_TSYM` | Get symbol time from current TX settings | `AT+RF_GET_TSYM` |

### AUX GPIO Pins (1-8)

| Command | Description | Example |
|---------|-------------|---------|
| `AT+AUX` | Set pin state | `AT+AUX=1,1` (pin 1 HIGH) |
| `AT+AUX_PULSE` | Start PWM | `AT+AUX_PULSE=1,1000,50` (pin 1, 1Hz, duty 50%) |
| `AT+AUX_PULSE_STOP` | Stop PWM | `AT+AUX_PULSE_STOP=1` (stop PWM on pin 1) |

---

## Practical Usage Examples

### Example 1: Simple Point-to-Point Communication

**Dongle 1 (transmitter):**
```
AT+LR_TX_SET=SF:9,BW:7,CR:45,Freq:869525000,IQInv:0,HeaderMode:0,CRC:1,Preamble:8,Power:22,LDRO:2
AT+RF_TX_HEX=AABBCCDD
```

**Dongle 2 (receiver):**
```
AT+LR_RX_SET=SF:9,BW:7,CR:45,Freq:869525000,IQInv:0,HeaderMode:0,CRC:1,Preamble:8,LDRO:2
```
→ Receiver will output: `+RX:4,AABBCCDD,RSSI:-47`

### Example 2: Range Test with Periodic Transmission

**TX Dongle:**
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
Subsequent RX reception:
`+RX:5,1122334455,RSSI:-68`

### Example 3: CW Test on Frequency

```
AT+LR_TX_FREQ=868000000
AT+LR_TX_POWER=10
AT+RF_TX_CW=1
```
Transmits carrier wave at 868 MHz with 10 dBm power

Stop TX:
```
AT+RF_TX_CW=0
```

### Example 4: Baud Rate Change

**Query current speed:**
```
AT+UART_BAUD?
```
115200

**Change to faster:**
```
AT+UART_BAUD=230400
OK
```
Dongle restarts and switches to **230400** baud. Terminal must also be switched.

---

## Important Notes

### TX/RX Parameter Compatibility
**For successful communication, RF settings must match:**
- Frequency **(TX_FREQ = RX_FREQ)**
- Spreading Factor **(TX_SF = RX_SF)**
- Bandwidth **(TX_BW = RX_BW)**
- Coding Rate **(TX_CR = RX_CR)**
- Sync Word **(TX_SYNCWORD = RX_SYNCWORD)**
- Header Mode **(TX_HEADERMODE = RX_HEADERMODE)**
- CRC **(TX_CRC = RX_CRC)**
- LDRO **(LDRO 2 = automatic mode)**
- IQInv **(TX_IQInv = RX_IQInv)**


### Range vs. Speed Optimization

**Maximum range (lowest speed):**
High SF and low BW → maximum range → slowest data rate
- **SF:** 12
- **BW:** 0 (7.81 kHz)
- **CR:** 48 (4/8)
- **Power:** 22 dBm
- → Range: > 10 km (open space)

**High speed (shorter range):**
- **SF:** 5
- **BW:** 9 (500 kHz)
- **CR:** 45 (4/5)
- **Power:** 22 dBm


### Time on Air (ToA)
The higher the SF and lower the BW, the longer the transmission time. Use `AT+RF_GET_TOA` to get packet time on air.

### Implicit vs. Explicit Header
- **Explicit (0)**: Header contains packet length
- **Implicit (1)**: No header - RX_PLDLEN must be set on receiver side before starting reception!

### LDRO (Low Data Rate Optimization)
- **Auto (2)**: Firmware automatically selects based on SF and BW - **recommended**
- **On (1)**: Manually enabled - you can experiment and select suitable option for your settings
- **Off (0)**: Manually disabled

---

## Troubleshooting





### Dongle not responding to AT commands
**1.** Check baud rate (try 115200 or 230400)
**2.** Check connection jumpers on serial line **MCU-USB** must be connected on both RX and TX headers
**3.** Check line ending (must be CR+LF)
**4.** Restart dongle (disconnect and reconnect USB)
**5.** Try `AT+SYS_RESTART`

### RX not seeing TX data
**1.** Verify all RF parameters for TX and RX are identical
**2.** Verify RX has enabled `AT+RF_RX_TO_UART=1`
**3.** Increase TX power or shorten distance

### Low RSSI or Packet Loss
**1.** Increase TX power (**max 22 dBm**)
**2.** Reduce distance
**3.** Try higher SF (7→9→12)
**4.** Try connecting external antenna - need to resolder jumper (zero ohm resistor in 0402 package) from chip antenna to UFL connector!!

### CRC Errors
**1.** RF interference - change frequency
**2.** TX/RX parameter mismatch
**3.** Set LDRO to "auto" on both TX and RX sides

---

## Technical Specifications

### Hardware
- **MCU**: STM32L071 (ARM Cortex-M0+)
- **RF chip**: Semtech SX1262
- **Frequency**: 150 MHz - 960 MHz
- **TX power**: 0 to +22 dBm
- **RX sensitivity**: up to -149.1 dBm (SF12, BW 7.81 kHz)
- **USB**: Virtual COM port
- **GPIO**: 8x AUX pins (PWM capable)

### Operating Conditions
- **Power supply**: 5V from USB
- **Temperature**: -20°C to +70°C (operating)

---

## Appendix: Python Automation

For automated tests and applications, you can use Python with `pyserial`:

```python
#!/usr/bin/env python3

import serial
import time

PORT = "COM3"          # Windows: COM3, Linux: /dev/ttyUSB0
BAUD = 230400

FREQ = 869525000
DATA_HEX = "48656C6C6F"   # Hello
PACKET_COUNT = 5
PACKET_DELAY = 1.0

def send_AT(cmd, timeout=2.0):
    ser.reset_input_buffer()
    ser.write((cmd + "\r\n").encode())

    t0 = time.time()
    resp = ""

    while time.time() - t0 < timeout:
        if ser.in_waiting:
            resp += ser.read(ser.in_waiting).decode(errors="ignore")
            if "OK" in resp or "ERROR" in resp:
                break
        time.sleep(0.01)

    return resp.strip()


print("Open serial port")
ser = serial.Serial(PORT, BAUD, timeout=1)
time.sleep(0.5)

print("Identify dongle")
resp = send_AT("AT+IDENTIFY")

if "Ropixon AT-USB LoRa_Dongle" not in resp:
    print("Unknown device")
    print(resp)
    ser.close()
    exit(1)

print(resp)

print("Configure TX")
send_AT(
    f"AT+LR_TX_SET=SF:7,BW:7,CR:45,Freq:{FREQ},"
    f"IQInv:0,HeaderMode:0,CRC:1,Preamble:8,LDRO:2,Power:22"
)

print("Configure RX")
send_AT(
    f"AT+LR_RX_SET=SF:7,BW:7,CR:45,Freq:{FREQ},"
    f"IQInv:0,HeaderMode:0,CRC:1,Preamble:8,LDRO:2"
)

print("Send packets")
for i in range(PACKET_COUNT):
    resp = send_AT(f"AT+RF_TX_HEX={DATA_HEX}", timeout=3.0)
    print("Packet", i + 1, "OK" if "OK" in resp else "FAIL")
    time.sleep(PACKET_DELAY)

ser.close()
print("Done")

```


**© 2024-2026 Ropixon - AT LoRa Dongle**
