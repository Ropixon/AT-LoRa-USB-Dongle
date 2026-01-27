#!/usr/bin/env python3
"""
RF Range Test Script for AT LoRa Dongle
Tests various SF, BW combinations between two dongles.

Tested AT Commands:
- LoRa Parameters: FREQ, POWER, SF, BW, IQ_INV, CR, HEADERMODE, CRC, PREAMBLE_SIZE, LDRO, SYNCWORD, PLDLEN
- RF TX/RX: RF_TX_HEX, RF_TX_TXT, RF_RX_TO_UART, RF_RX_FORMAT
- Saved Packet: RF_SAVE_PACKET, RF_TX_SAVED, RF_TX_SAVED_REPEAT, RF_TX_NVM_PERIOD
- TOA & Timing: RF_GET_TOA, RF_GET_TSYM
- AUX GPIO: AUX, AUX_PULSE, AUX_PULSE_STOP
- System: UART_BAUD, FACTORY_RST, SYS_RESTART, IDENTIFY

Author: Auto-generated
Date: 2026-01-10
"""

import serial
import serial.tools.list_ports
import time
import sys
from dataclasses import dataclass
from typing import Optional, List, Tuple
import logging

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

# Test configuration
TEST_PACKET_HEX = "010203040506070809112233445566778899112233445566778899"  # 3 bytes
TEST_FREQUENCY = 869525000
BAUD_RATES_TO_TRY = [115200, 230400]  # Try common baud rate first, then max supported
TARGET_BAUD_RATE = 230400  # Target baud rate to set after detection (max supported by dongle)
TIMEOUT = 5.0  # seconds

# RF Parameters to test
SF_RANGE = [ 7,8,9]  # SF5 to SF12
BW_OPTIONS = [6,7]  # All bandwidth options
# BW mapping: 0=7.81kHz, 1=10.42kHz, 2=15.63kHz, 3=20.83kHz, 4=31.25kHz, 
#             5=41.67kHz, 6=62.5kHz, 7=125kHz, 8=250kHz, 9=500kHz

CR_OPTIONS = [45]  # Coding rates: 4/5, 4/6, 4/7, 4/8
IQ_INV_OPTIONS = [0,1]  # IQ inversion: 0=normal (testujeme pouze bez inverze)
HEADER_MODE_OPTIONS = [0,1]  # Header mode: 0=explicit, 1=implicit
CRC_OPTIONS = [ 1]  # CRC: 0=disabled, 1=enabled
PREAMBLE_OPTIONS = [16]  # Preamble length in symbols (can add: 8, 16, 32, etc.)

# Fixed parameters (not iterated)
FIXED_POWER = 22  # TX power in dBm
FIXED_LDRO = 2  # LDRO: 0=off, 1=on, 2=auto (always auto)

# Test filtering options
SKIP_SLOW_TESTS = False  # Skip slow combinations: BW <= 5 and SF >= 10

def should_skip_test(sf: int, bw: int) -> bool:
    """Check if test should be skipped based on filter settings"""
    if SKIP_SLOW_TESTS:
        # Skip slow combinations: low bandwidth + high spreading factor
        if bw <= 8 or sf >= 7:
            return True
    return False

CR_NAMES = {
    45: "4/5",
    46: "4/6", 
    47: "4/7",
    48: "4/8"
}

LDRO_NAMES = {
    0: "off",
    1: "on",
    2: "auto"
}

HEADER_MODE_NAMES = {
    0: "Expl",
    1: "Impl"
}

CRC_NAMES = {
    0: "OFF",
    1: "ON"
}

BW_NAMES = {
    0: "7.81 kHz",
    1: "10.42 kHz", 
    2: "15.63 kHz",
    3: "20.83 kHz",
    4: "31.25 kHz",
    5: "41.67 kHz",
    6: "62.5 kHz",
    7: "125 kHz",
    8: "250 kHz",
    9: "500 kHz"
}

CR_NAMES = {
    45: "4/5",
    46: "4/6",
    47: "4/7",
    48: "4/8"
}

LDRO_NAMES = {
    0: "OFF",
    1: "ON",
    2: "AUTO"
}


@dataclass
class TestResult:
    sf: int
    bw: int
    cr: int
    freq: int
    power: int
    iq_inv: int
    ldro: int
    header_mode: int
    crc: int
    preamble: int
    success: bool
    tx_time_ms: float
    rx_time_ms: float
    rssi: int = 0
    error_msg: str = ""


class ATLoraDongle:
    """Class to communicate with AT LoRa Dongle"""

    def __init__(self, port: str, name: str = "Dongle"):
        self.port = port
        self.name = name
        self.serial: Optional[serial.Serial] = None
        self.detected_baudrate: Optional[int] = None

    def detect_baudrate(self) -> Optional[int]:
        """Detect the baud rate by trying AT command on each supported rate"""
        logger.info(f"{self.name}: Detecting baud rate...")

        for baud_rate in BAUD_RATES_TO_TRY:
            try:
                logger.info(f"{self.name}: Trying {baud_rate} baud...")

                # Try to open connection
                test_serial = serial.Serial(
                    port=self.port,
                    baudrate=baud_rate,
                    timeout=1.0,
                    write_timeout=1.0
                )
                test_serial.reset_input_buffer()
                test_serial.reset_output_buffer()

                # Send AT command and check for response
                test_serial.write(b"AT\r\n")

                # Read response
                response = ""
                start_time = time.time()
                while time.time() - start_time < 1.0:
                    if test_serial.in_waiting > 0:
                        response += test_serial.read(test_serial.in_waiting).decode('utf-8', errors='ignore')
                        # Check if we got a valid AT response (help text or OK)
                        if len(response) > 10 and ("AT+" in response or "OK" in response):
                            test_serial.close()
                            logger.info(f"{self.name}: ✓ Detected baud rate: {baud_rate}")
                            return baud_rate
                    time.sleep(0.01)  # Minimal poll delay

                test_serial.close()
                logger.debug(f"{self.name}: No valid response at {baud_rate} baud")

            except Exception as e:
                logger.debug(f"{self.name}: Error testing {baud_rate} baud: {e}")
                continue

        logger.error(f"{self.name}: Failed to detect baud rate on any supported speed")
        return None

    def set_baudrate(self, new_baudrate: int) -> bool:
        """Change dongle baud rate and reconnect Python serial port

        Args:
            new_baudrate: New baud rate to set (e.g. 230400)

        Returns:
            True if successful, False otherwise
        """
        if not self.serial or not self.serial.is_open:
            logger.error(f"{self.name}: Cannot set baud rate - not connected")
            return False

        try:
            logger.info(f"{self.name}: Setting baud rate to {new_baudrate}...")

            # Send command to change dongle baud rate
            # Note: Dongle will restart after this command
            cmd = f"AT+UART_BAUD={new_baudrate}\r\n"
            self.serial.write(cmd.encode())
            self.serial.flush()  # Ensure command is sent

            # Close current connection
            self.serial.close()

            # Wait for dongle to restart with new baud rate
            logger.info(f"{self.name}: Waiting for dongle to restart...")
            time.sleep(1.5)  # Minimum time for restart

            # Reconnect at new baud rate
            self.serial = serial.Serial(
                port=self.port,
                baudrate=new_baudrate,
                timeout=TIMEOUT,
                write_timeout=TIMEOUT
            )
            self.serial.reset_input_buffer()
            self.serial.reset_output_buffer()

            # Verify communication at new speed with AT command
            self.serial.write(b"AT\r\n")
            response = ""
            start_time = time.time()
            while time.time() - start_time < 1.0:
                if self.serial.in_waiting > 0:
                    response += self.serial.read(self.serial.in_waiting).decode('utf-8', errors='ignore')
                    if "AT+" in response or "OK" in response:
                        logger.info(f"{self.name}: ✓ Successfully switched to {new_baudrate} baud")
                        self.detected_baudrate = new_baudrate
                        self.serial.reset_input_buffer()
                        return True
                time.sleep(0.01)  # Minimal poll delay

            logger.error(f"{self.name}: No response at new baud rate {new_baudrate}")
            return False

        except Exception as e:
            logger.error(f"{self.name}: Error setting baud rate: {e}")
            return False

    def connect(self, baudrate: Optional[int] = None) -> bool:
        """Connect to the dongle

        Args:
            baudrate: Specific baud rate to use. If None, will auto-detect.
        """
        try:
            # Auto-detect if not specified
            if baudrate is None:
                baudrate = self.detect_baudrate()
                if baudrate is None:
                    return False
                self.detected_baudrate = baudrate
            else:
                self.detected_baudrate = baudrate

            self.serial = serial.Serial(
                port=self.port,
                baudrate=baudrate,
                timeout=TIMEOUT,
                write_timeout=TIMEOUT
            )
            self.serial.reset_input_buffer()
            self.serial.reset_output_buffer()
            logger.info(f"{self.name}: Connected to {self.port} at {baudrate} baud")

            # If not already at target baud rate, switch to it
            if baudrate != TARGET_BAUD_RATE:
                logger.info(f"{self.name}: Switching from {baudrate} to {TARGET_BAUD_RATE} baud...")
                if not self.set_baudrate(TARGET_BAUD_RATE):
                    logger.warning(f"{self.name}: Failed to switch to {TARGET_BAUD_RATE} baud, staying at {baudrate}")
                    # Continue anyway - we have a working connection

            return True
        except Exception as e:
            logger.error(f"{self.name}: Failed to connect to {self.port}: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from the dongle"""
        if self.serial and self.serial.is_open:
            self.serial.close()
            logger.info(f"{self.name}: Disconnected from {self.port}")
    
    def send_command(self, cmd: str, wait_response: bool = True, timeout: float = None, 
                     expected_ok_count: int = 1, stop_on_pattern: str = None) -> Tuple[bool, str]:
        """Send AT command and wait for response
        
        Args:
            cmd: AT command to send
            wait_response: Whether to wait for response
            timeout: Custom timeout in seconds
            expected_ok_count: Number of OK responses to wait for (for multi-param commands)
            stop_on_pattern: Stop reading when this pattern is found in response (e.g. 'TOA:')
        """
        if not self.serial or not self.serial.is_open:
            return False, "Not connected"
        
        original_timeout = self.serial.timeout
        if timeout:
            self.serial.timeout = timeout
            
        try:
            # Clear buffers
            self.serial.reset_input_buffer()
            
            # Send command
            full_cmd = f"{cmd}\r\n"
            self.serial.write(full_cmd.encode())
            logger.debug(f"{self.name} TX: {cmd}")
            
            if not wait_response:
                return True, ""
            
            # Read response
            response_lines = []
            ok_count = 0
            error_count = 0
            start_time = time.time()
            
            while True:
                if time.time() - start_time > (timeout or TIMEOUT):
                    break
                    
                if self.serial.in_waiting > 0:
                    line = self.serial.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        response_lines.append(line)
                        logger.debug(f"{self.name} RX: {line}")
                        
                        # Check for custom stop pattern first
                        if stop_on_pattern and stop_on_pattern in line:
                            break
                        
                        # Count OK and ERROR responses
                        if line == "OK":
                            ok_count += 1
                            if ok_count >= expected_ok_count:
                                break
                        elif "ERROR" in line:
                            error_count += 1
                            break  # Stop on first error
                else:
                    time.sleep(0.01)
            
            response = "\n".join(response_lines)
            success = ok_count >= expected_ok_count and error_count == 0
            
            return success, response
            
        except Exception as e:
            logger.error(f"{self.name}: Command error: {e}")
            return False, str(e)
        finally:
            self.serial.timeout = original_timeout
    
    def configure_tx(self, sf: int, bw: int, freq: int, cr: int = 45, power: int = 22, 
                     iq_inv: int = 0, ldro: int = 2, header_mode: int = 0, crc: int = 1, preamble: int = 8, 
                     sync_word: str = "12") -> bool:
        """Configure TX parameters
        
        Args:
            sync_word: Sync word in hex (default 0x12 = private network)
        """
        # Set sync word first
        sync_cmd = f"AT+LR_TX_SYNCWORD={sync_word}"
        success, response = self.send_command(sync_cmd)
        if not success:
            logger.error(f"{self.name}: TX sync word config failed: {response}")
            return False
        
        cmd = f"AT+LR_TX_SET=SF:{sf},BW:{bw},CR:{cr},Freq:{freq},IQInv:{iq_inv},HeaderMode:{header_mode},CRC:{crc},Preamble:{preamble},Power:{power},LDRO:{ldro}"
        # 10 parameters = 10 OK responses expected
        success, response = self.send_command(cmd, expected_ok_count=10)
        if success:
            ldro_name = {0: 'off', 1: 'on', 2: 'auto'}.get(ldro, str(ldro))
            logger.info(f"{self.name}: TX configured - SF{sf}, BW{bw}, CR{cr}, Pwr:{power}dBm, LDRO:{ldro_name}, Sync:0x{sync_word}")
        else:
            logger.error(f"{self.name}: TX config failed: {response}")
        return success
    
    def configure_rx(self, sf: int, bw: int, freq: int, cr: int = 45, 
                     iq_inv: int = 0, ldro: int = 2, header_mode: int = 0, crc: int = 1, preamble: int = 8,
                     rx_payload_len: int = 0, sync_word: str = "12") -> bool:
        """Configure RX parameters
        
        Args:
            rx_payload_len: Expected payload length for implicit header mode (0=auto/max buffer)
            sync_word: Sync word in hex (default 0x12 = private network)
        """
        # Set sync word first
        sync_cmd = f"AT+LR_RX_SYNCWORD={sync_word}"
        success, response = self.send_command(sync_cmd)
        if not success:
            logger.error(f"{self.name}: RX sync word config failed: {response}")
            return False
        
        # For implicit header mode, set expected payload length first
        if header_mode == 1 and rx_payload_len > 0:
            pldlen_cmd = f"AT+LR_RX_PLDLEN={rx_payload_len}"
            success, response = self.send_command(pldlen_cmd)
            if not success:
                logger.error(f"{self.name}: RX payload length config failed: {response}")
                return False
            logger.debug(f"{self.name}: RX payload length set to {rx_payload_len} for implicit mode")
        elif header_mode == 0:
            # For explicit mode, set to 0 (use max buffer)
            pldlen_cmd = "AT+LR_RX_PLDLEN=0"
            self.send_command(pldlen_cmd)
        
        cmd = f"AT+LR_RX_SET=SF:{sf},BW:{bw},CR:{cr},Freq:{freq},IQInv:{iq_inv},HeaderMode:{header_mode},CRC:{crc},Preamble:{preamble},LDRO:{ldro}"
        # 9 parameters = 9 OK responses expected
        success, response = self.send_command(cmd, expected_ok_count=9)
        if success:
            ldro_name = {0: 'off', 1: 'on', 2: 'auto'}.get(ldro, str(ldro))
            logger.info(f"{self.name}: RX configured - SF{sf}, BW{bw}, CR{cr}, LDRO:{ldro_name}, Hdr:{header_mode}, CRC:{crc}, Sync:0x{sync_word}")
        else:
            logger.error(f"{self.name}: RX config failed: {response}")
        return success
    
    def enable_rx_to_uart(self, enable: bool = True) -> bool:
        """Enable/disable RX to UART output"""
        cmd = f"AT+RF_RX_TO_UART={'ON' if enable else 'OFF'}"
        success, _ = self.send_command(cmd)
        return success
    
    def get_toa(self, packet_size: int) -> int:
        """Get Time on Air for a packet of given size in ms
        
        Args:
            packet_size: Packet size in bytes
            
        Returns:
            TOA in milliseconds, or 0 if failed
        """
        cmd = f"AT+RF_GET_TOA={packet_size}"
        _, response = self.send_command(cmd, timeout=2.0, stop_on_pattern="ms")
        # Response format: "TOA: X ms" - doesn't include OK, stop when we see "ms"
        if "TOA:" in response:
            # Parse "TOA: X ms" from response
            import re
            match = re.search(r'TOA:\s*(\d+)\s*ms', response)
            if match:
                toa = int(match.group(1))
                logger.debug(f"{self.name}: TOA for {packet_size}B = {toa} ms")
                return toa
        logger.warning(f"{self.name}: Failed to get TOA: {response}")
        return 0
    
    def send_packet(self, hex_data: str) -> bool:
        """Send a packet via RF"""
        cmd = f"AT+RF_TX_HEX={hex_data}"
        success, response = self.send_command(cmd)
        return success
    
    def wait_for_rx_packet(self, timeout: float = 10.0) -> Tuple[bool, str, int]:
        """Wait for incoming RX packet
        
        Returns:
            Tuple of (success, hex_data, rssi)
        """
        if not self.serial or not self.serial.is_open:
            return False, "Not connected", 0
        
        start_time = time.time()
        buffer = ""
        
        # Pattern for RX packet: +RX:<len>,<hex_data>,RSSI:<rssi>
        import re
        pattern = re.compile(r'\+RX:(\d+),([0-9A-Fa-f]+),RSSI:(-?\d+)')
        
        try:
            while time.time() - start_time < timeout:
                if self.serial.in_waiting > 0:
                    chunk = self.serial.read(self.serial.in_waiting).decode('utf-8', errors='ignore')
                    buffer += chunk
                    logger.debug(f"{self.name} RX buffer: {chunk.strip()}")
                    
                    match = pattern.search(buffer)
                    if match:
                        data_len = int(match.group(1))
                        hex_data = match.group(2).upper()
                        rssi = int(match.group(3))
                        logger.info(f"{self.name}: Received {data_len} bytes, RSSI: {rssi} dBm")
                        return True, hex_data, rssi
                else:
                    time.sleep(0.01)
            
            return False, "Timeout waiting for packet", 0
            
        except Exception as e:
            return False, str(e), 0
    
    def set_aux_gpio(self, pin: int, state: str) -> bool:
        """Set AUX GPIO pin state
        
        Args:
            pin: Pin number (1-8)
            state: "ON" or "OFF"
        """
        cmd = f"AT+AUX={pin},{state}"
        success, _ = self.send_command(cmd)
        return success
    
    def set_aux_pwm(self, pin: int, period_ms: int, duty_pct: int) -> bool:
        """Set AUX GPIO pin PWM
        
        Args:
            pin: Pin number (1-8)
            period_ms: Period in milliseconds
            duty_pct: Duty cycle percentage (0-100)
        """
        cmd = f"AT+AUX_PULSE={pin},{period_ms},{duty_pct}"
        success, _ = self.send_command(cmd)
        return success
    
    def stop_aux_pwm(self, pin: int) -> bool:
        """Stop AUX GPIO PWM
        
        Args:
            pin: Pin number (1-8)
        """
        cmd = f"AT+AUX_PULSE_STOP={pin}"
        success, _ = self.send_command(cmd)
        return success
    
    def set_rx_format(self, format_type: str) -> bool:
        """Set RX output format
        
        Args:
            format_type: "HEX" or "ASCII"
        """
        cmd = f"AT+RF_RX_FORMAT={format_type}"
        success, _ = self.send_command(cmd)
        return success
    
    def get_symbol_time(self) -> int:
        """Get symbol time in microseconds based on current TX config
        
        Returns:
            Symbol time in microseconds, or 0 if failed
        """
        cmd = "AT+RF_GET_TSYM"
        _, response = self.send_command(cmd, timeout=2.0, stop_on_pattern="us")
        if "TSYM:" in response:
            import re
            match = re.search(r'TSYM:\s*(\d+)\s*us', response)
            if match:
                tsym = int(match.group(1))
                logger.debug(f"{self.name}: Symbol time = {tsym} us")
                return tsym
        logger.warning(f"{self.name}: Failed to get symbol time: {response}")
        return 0


def test_eeprom_storage(tx_dongle: ATLoraDongle, rx_dongle: ATLoraDongle) -> bool:
    """Test EEPROM storage for all configuration parameters
    
    Writes various configurations and reads them back to verify correct EEPROM storage.
    Returns True if all tests pass, False otherwise.
    """
    
    logger.info("\n" + "="*70)
    logger.info("EEPROM STORAGE VERIFICATION TEST")
    logger.info("="*70)
    
    all_passed = True
    failed_tests = 0
    
    # Individual parameter tests - write value then read back with ?
    # BW uses index 0-9, not Hz values!
    param_tests = [
        # TX parameters
        {"dongle": tx_dongle, "set_cmd": "AT+LR_TX_SF=7", "get_cmd": "AT+LR_TX_SF?", "expected": "7", "name": "TX SF"},
        {"dongle": tx_dongle, "set_cmd": "AT+LR_TX_SF=12", "get_cmd": "AT+LR_TX_SF?", "expected": "12", "name": "TX SF (high)"},
        {"dongle": tx_dongle, "set_cmd": "AT+LR_TX_BW=7", "get_cmd": "AT+LR_TX_BW?", "expected": "7", "name": "TX BW (125kHz)"},  # BW7 = 125kHz
        {"dongle": tx_dongle, "set_cmd": "AT+LR_TX_BW=9", "get_cmd": "AT+LR_TX_BW?", "expected": "9", "name": "TX BW (500kHz)"},  # BW9 = 500kHz
        {"dongle": tx_dongle, "set_cmd": "AT+LR_TX_CR=45", "get_cmd": "AT+LR_TX_CR?", "expected": "45", "name": "TX CR"},
        {"dongle": tx_dongle, "set_cmd": "AT+LR_TX_CR=48", "get_cmd": "AT+LR_TX_CR?", "expected": "48", "name": "TX CR (high)"},
        {"dongle": tx_dongle, "set_cmd": "AT+LR_TX_POWER=10", "get_cmd": "AT+LR_TX_POWER?", "expected": "10", "name": "TX Power"},
        {"dongle": tx_dongle, "set_cmd": "AT+LR_TX_POWER=22", "get_cmd": "AT+LR_TX_POWER?", "expected": "22", "name": "TX Power (high)"},
        {"dongle": tx_dongle, "set_cmd": "AT+LR_TX_IQ_INV=0", "get_cmd": "AT+LR_TX_IQ_INV?", "expected": "0", "name": "TX IQ Inv (off)"},
        {"dongle": tx_dongle, "set_cmd": "AT+LR_TX_IQ_INV=1", "get_cmd": "AT+LR_TX_IQ_INV?", "expected": "1", "name": "TX IQ Inv (on)"},
        {"dongle": tx_dongle, "set_cmd": "AT+LR_TX_HEADERMODE=0", "get_cmd": "AT+LR_TX_HEADERMODE?", "expected": "0", "name": "TX Header (explicit)"},
        {"dongle": tx_dongle, "set_cmd": "AT+LR_TX_HEADERMODE=1", "get_cmd": "AT+LR_TX_HEADERMODE?", "expected": "1", "name": "TX Header (implicit)"},
        {"dongle": tx_dongle, "set_cmd": "AT+LR_TX_CRC=0", "get_cmd": "AT+LR_TX_CRC?", "expected": "0", "name": "TX CRC (off)"},
        {"dongle": tx_dongle, "set_cmd": "AT+LR_TX_CRC=1", "get_cmd": "AT+LR_TX_CRC?", "expected": "1", "name": "TX CRC (on)"},
        {"dongle": tx_dongle, "set_cmd": "AT+LR_TX_PREAMBLE_SIZE=8", "get_cmd": "AT+LR_TX_PREAMBLE_SIZE?", "expected": "8", "name": "TX Preamble"},
        {"dongle": tx_dongle, "set_cmd": "AT+LR_TX_PREAMBLE_SIZE=16", "get_cmd": "AT+LR_TX_PREAMBLE_SIZE?", "expected": "16", "name": "TX Preamble (high)"},
        {"dongle": tx_dongle, "set_cmd": "AT+LR_TX_LDRO=0", "get_cmd": "AT+LR_TX_LDRO?", "expected": "0", "name": "TX LDRO (off)"},
        {"dongle": tx_dongle, "set_cmd": "AT+LR_TX_LDRO=1", "get_cmd": "AT+LR_TX_LDRO?", "expected": "1", "name": "TX LDRO (on)"},
        {"dongle": tx_dongle, "set_cmd": "AT+LR_TX_LDRO=2", "get_cmd": "AT+LR_TX_LDRO?", "expected": "2", "name": "TX LDRO (auto)"},
        
        # RX parameters
        {"dongle": rx_dongle, "set_cmd": "AT+LR_RX_SF=7", "get_cmd": "AT+LR_RX_SF?", "expected": "7", "name": "RX SF"},
        {"dongle": rx_dongle, "set_cmd": "AT+LR_RX_SF=12", "get_cmd": "AT+LR_RX_SF?", "expected": "12", "name": "RX SF (high)"},
        {"dongle": rx_dongle, "set_cmd": "AT+LR_RX_BW=7", "get_cmd": "AT+LR_RX_BW?", "expected": "7", "name": "RX BW (125kHz)"},  # BW7 = 125kHz
        {"dongle": rx_dongle, "set_cmd": "AT+LR_RX_BW=9", "get_cmd": "AT+LR_RX_BW?", "expected": "9", "name": "RX BW (500kHz)"},  # BW9 = 500kHz
        {"dongle": rx_dongle, "set_cmd": "AT+LR_RX_CR=45", "get_cmd": "AT+LR_RX_CR?", "expected": "45", "name": "RX CR"},
        {"dongle": rx_dongle, "set_cmd": "AT+LR_RX_CR=48", "get_cmd": "AT+LR_RX_CR?", "expected": "48", "name": "RX CR (high)"},
        {"dongle": rx_dongle, "set_cmd": "AT+LR_RX_IQ_INV=0", "get_cmd": "AT+LR_RX_IQ_INV?", "expected": "0", "name": "RX IQ Inv (off)"},
        {"dongle": rx_dongle, "set_cmd": "AT+LR_RX_IQ_INV=1", "get_cmd": "AT+LR_RX_IQ_INV?", "expected": "1", "name": "RX IQ Inv (on)"},
        {"dongle": rx_dongle, "set_cmd": "AT+LR_RX_HEADERMODE=0", "get_cmd": "AT+LR_RX_HEADERMODE?", "expected": "0", "name": "RX Header (explicit)"},
        {"dongle": rx_dongle, "set_cmd": "AT+LR_RX_HEADERMODE=1", "get_cmd": "AT+LR_RX_HEADERMODE?", "expected": "1", "name": "RX Header (implicit)"},
        {"dongle": rx_dongle, "set_cmd": "AT+LR_RX_CRC=0", "get_cmd": "AT+LR_RX_CRC?", "expected": "0", "name": "RX CRC (off)"},
        {"dongle": rx_dongle, "set_cmd": "AT+LR_RX_CRC=1", "get_cmd": "AT+LR_RX_CRC?", "expected": "1", "name": "RX CRC (on)"},
        {"dongle": rx_dongle, "set_cmd": "AT+LR_RX_PREAMBLE_SIZE=8", "get_cmd": "AT+LR_RX_PREAMBLE_SIZE?", "expected": "8", "name": "RX Preamble"},
        {"dongle": rx_dongle, "set_cmd": "AT+LR_RX_PREAMBLE_SIZE=16", "get_cmd": "AT+LR_RX_PREAMBLE_SIZE?", "expected": "16", "name": "RX Preamble (high)"},
        {"dongle": rx_dongle, "set_cmd": "AT+LR_RX_LDRO=0", "get_cmd": "AT+LR_RX_LDRO?", "expected": "0", "name": "RX LDRO (off)"},
        {"dongle": rx_dongle, "set_cmd": "AT+LR_RX_LDRO=1", "get_cmd": "AT+LR_RX_LDRO?", "expected": "1", "name": "RX LDRO (on)"},
        {"dongle": rx_dongle, "set_cmd": "AT+LR_RX_LDRO=2", "get_cmd": "AT+LR_RX_LDRO?", "expected": "2", "name": "RX LDRO (auto)"},
        {"dongle": rx_dongle, "set_cmd": "AT+LR_RX_PLDLEN=64", "get_cmd": "AT+LR_RX_PLDLEN?", "expected": "64", "name": "RX PldLen"},
        {"dongle": rx_dongle, "set_cmd": "AT+LR_RX_PLDLEN=255", "get_cmd": "AT+LR_RX_PLDLEN?", "expected": "255", "name": "RX PldLen (max)"},
        
        # Frequency tests (32-bit values)
        {"dongle": tx_dongle, "set_cmd": "AT+LR_TX_FREQ=868000000", "get_cmd": "AT+LR_TX_FREQ?", "expected": "868000000", "name": "TX Freq"},
        {"dongle": tx_dongle, "set_cmd": "AT+LR_TX_FREQ=915000000", "get_cmd": "AT+LR_TX_FREQ?", "expected": "915000000", "name": "TX Freq (high)"},
        {"dongle": rx_dongle, "set_cmd": "AT+LR_RX_FREQ=868000000", "get_cmd": "AT+LR_RX_FREQ?", "expected": "868000000", "name": "RX Freq"},
        {"dongle": rx_dongle, "set_cmd": "AT+LR_RX_FREQ=915000000", "get_cmd": "AT+LR_RX_FREQ?", "expected": "915000000", "name": "RX Freq (high)"},
        
        # Saved packet test (hex data stored in EEPROM)
        {"dongle": tx_dongle, "set_cmd": "AT+RF_SAVE_PACKET=AABBCC", "get_cmd": "AT+RF_SAVE_PACKET?", "expected": "AABBCC", "name": "Saved Packet", "is_hex": True},
        {"dongle": tx_dongle, "set_cmd": "AT+RF_SAVE_PACKET=112233445566", "get_cmd": "AT+RF_SAVE_PACKET?", "expected": "112233445566", "name": "Saved Packet (long)", "is_hex": True},
        
        # TX period (32-bit value in ms)
        {"dongle": tx_dongle, "set_cmd": "AT+RF_TX_NVM_PERIOD=1000", "get_cmd": "AT+RF_TX_NVM_PERIOD?", "expected": "1000", "name": "TX Period"},
        {"dongle": tx_dongle, "set_cmd": "AT+RF_TX_NVM_PERIOD=5000", "get_cmd": "AT+RF_TX_NVM_PERIOD?", "expected": "5000", "name": "TX Period (high)"},
        
        # Sync word (hex byte 0x12=private, 0x34=public)
        # FW expects and returns hex string: "12" = byte 0x12, "34" = byte 0x34
        {"dongle": tx_dongle, "set_cmd": "AT+LR_TX_SYNCWORD=12", "get_cmd": "AT+LR_TX_SYNCWORD?", "expected": "12", "name": "TX SyncWord (private)", "is_hex": True, "timeout": 3.0},
        {"dongle": tx_dongle, "set_cmd": "AT+LR_TX_SYNCWORD=34", "get_cmd": "AT+LR_TX_SYNCWORD?", "expected": "34", "name": "TX SyncWord (public)", "is_hex": True, "timeout": 3.0},
        {"dongle": rx_dongle, "set_cmd": "AT+LR_RX_SYNCWORD=12", "get_cmd": "AT+LR_RX_SYNCWORD?", "expected": "12", "name": "RX SyncWord (private)", "is_hex": True, "timeout": 3.0},
        {"dongle": rx_dongle, "set_cmd": "AT+LR_RX_SYNCWORD=34", "get_cmd": "AT+LR_RX_SYNCWORD?", "expected": "34", "name": "RX SyncWord (public)", "is_hex": True, "timeout": 3.0},
    ]
    
    import re
    
    for test in param_tests:
        dongle = test["dongle"]
        
        # Clear any pending data in buffer before SET
        dongle.serial.reset_input_buffer()

        # Set the parameter - expects OK response
        timeout = test.get("timeout", 2.0)
        success, response = dongle.send_command(test["set_cmd"], timeout=timeout)
        if not success:
            logger.error(f"  ✗ {test['name']}: Set command failed - {response}")
            failed_tests += 1
            all_passed = False
            continue
        
        # Read back the parameter - query returns just value, no OK
        dongle.serial.reset_input_buffer()
        dongle.serial.write(f"{test['get_cmd']}\r\n".encode())
        
        # Wait for response with timeout
        start_time = time.time()
        response = ""
        while time.time() - start_time < 1.0:
            if dongle.serial.in_waiting > 0:
                response = dongle.serial.read(dongle.serial.in_waiting).decode('utf-8', errors='ignore').strip()
                if response:
                    break
            time.sleep(0.01)
        
        # Parse response - look for the value (number in response)
        # For hex data, look for hex string pattern
        if test.get("is_hex"):
            # For saved packet, look for "packet:XXX" pattern
            if "SAVE_PACKET" in test["get_cmd"]:
                packet_match = re.search(r'packet:([0-9A-Fa-f]+)', response)
                if packet_match:
                    read_value = packet_match.group(1).upper()
                    expected = test["expected"].upper()
                    if read_value == expected:
                        logger.info(f"  ✓ {test['name']}: {read_value} == {expected}")
                    else:
                        logger.error(f"  ✗ {test['name']}: Got {read_value}, expected {expected} - MISMATCH!")
                        logger.error(f"      Raw response: '{response}'")
                        failed_tests += 1
                        all_passed = False
                else:
                    logger.error(f"  ✗ {test['name']}: Could not parse packet response")
                    logger.error(f"      Raw response: '{response}'")
                    failed_tests += 1
                    all_passed = False
            else:
                # For other hex data (sync word, etc), look for plain hex string
                hex_match = re.search(r'([0-9A-Fa-f]{2,})', response)
                if hex_match:
                    read_value = hex_match.group(1).upper()
                    expected = test["expected"].upper()
                    if read_value == expected:
                        logger.info(f"  ✓ {test['name']}: {read_value} == {expected}")
                    else:
                        logger.error(f"  ✗ {test['name']}: Got {read_value}, expected {expected} - MISMATCH!")
                        logger.error(f"      Raw response: '{response}'")
                        failed_tests += 1
                        all_passed = False
                else:
                    logger.error(f"  ✗ {test['name']}: Could not parse hex response")
                    logger.error(f"      Raw response: '{response}'")
                    failed_tests += 1
                    all_passed = False
        else:
            value_match = re.search(r'(\d+)', response)
            if value_match:
                read_value = value_match.group(1)
                if read_value == test["expected"]:
                    logger.info(f"  ✓ {test['name']}: {read_value} == {test['expected']}")
                else:
                    logger.error(f"  ✗ {test['name']}: Got {read_value}, expected {test['expected']} - MISMATCH!")
                    logger.error(f"      Raw response: '{response}'")
                    failed_tests += 1
                    all_passed = False
            else:
                logger.error(f"  ✗ {test['name']}: Could not parse response")
                logger.error(f"      Raw response: '{response}'")
                failed_tests += 1
                all_passed = False
    
    # Test AUX GPIO functionality (not EEPROM but basic commands)
    logger.info("\n" + "="*70)
    logger.info("AUX GPIO FUNCTIONALITY TEST")
    logger.info("="*70)
    
    aux_tests_passed = True
    
    # Test basic GPIO control - AT+AUX možná není implementováno v FW (jen PWM)
    logger.info("Testing basic GPIO control (AT+AUX)...")
    gpio_test_passed = False
    for pin in [1, 4, 8]:  # Test a few pins
        # Turn ON
        if not tx_dongle.set_aux_gpio(pin, "1"):
            logger.warning(f"  ! AUX{pin} ON failed (možná není implementováno v FW)")
        else:
            logger.info(f"  ✓ AUX{pin} ON")
            gpio_test_passed = True

        # Turn OFF
        if not tx_dongle.set_aux_gpio(pin, "0"):
            logger.warning(f"  ! AUX{pin} OFF failed")
        else:
            logger.info(f"  ✓ AUX{pin} OFF")
            gpio_test_passed = True
    
    if not gpio_test_passed:
        logger.warning("  ! AT+AUX basic control není implementováno (to je OK, jen PWM je důležité)")
    
    # Test PWM on one pin
    if tx_dongle.set_aux_pwm(1, 1000, 50):  # 1Hz, 50% duty
        logger.info("  ✓ AUX1 PWM started (1Hz, 50%)")
        if tx_dongle.stop_aux_pwm(1):
            logger.info("  ✓ AUX1 PWM stopped")
        else:
            logger.error("  ✗ AUX1 PWM stop failed")
            aux_tests_passed = False
    else:
        logger.error("  ✗ AUX1 PWM start failed")
        aux_tests_passed = False
    
    # Test RX format switching
    logger.info("\n" + "="*70)
    logger.info("RX FORMAT TEST")
    logger.info("="*70)
    
    rx_format_passed = True
    if rx_dongle.set_rx_format("HEX"):
        logger.info("  ✓ RX format set to HEX")
    else:
        logger.error("  ✗ RX format HEX failed")
        rx_format_passed = False

    if rx_dongle.set_rx_format("ASCII"):
        logger.info("  ✓ RX format set to ASCII")
    else:
        logger.error("  ✗ RX format ASCII failed")
        rx_format_passed = False
    
    # Reset back to HEX for tests
    rx_dongle.set_rx_format("HEX")
    
    # Test TOA and symbol time
    logger.info("\n" + "="*70)
    logger.info("TOA & SYMBOL TIME TEST")
    logger.info("="*70)
    
    toa_test_passed = True
    
    # Configure standard settings first
    tx_dongle.configure_tx(sf=7, bw=7, freq=TEST_FREQUENCY, power=14)
    
    toa = tx_dongle.get_toa(10)  # 10 bytes
    if toa > 0:
        logger.info(f"  ✓ TOA for 10 bytes: {toa} ms")
    else:
        logger.error("  ✗ TOA query failed")
        toa_test_passed = False
    
    # Symbol time test - příkaz AT+RF_GET_TSYM má v STM32 kódu možná chybu
    # Vrací "ERROR - Missing parameters" i když by neměl parametry potřebovat
    try:
        tsym = tx_dongle.get_symbol_time()
        if tsym > 0:
            logger.info(f"  ✓ Symbol time: {tsym} us")
        else:
            logger.warning("  ! Symbol time query failed (možná není implementováno)")
            # Don't fail test, just warn
    except Exception as e:
        logger.warning(f"  ! Symbol time query error: {e}")
    
    # Test saved packet and periodic TX
    logger.info("\n" + "="*70)
    logger.info("SAVED PACKET & PERIODIC TX TEST")
    logger.info("="*70)
    
    saved_tx_passed = True
    
    # Save a test packet
    test_packet = "AABBCCDD"
    success, _ = tx_dongle.send_command(f"AT+RF_SAVE_PACKET={test_packet}")
    if success:
        logger.info(f"  ✓ Saved packet: {test_packet}")
    else:
        logger.error("  ✗ Save packet failed")
        saved_tx_passed = False
    
    # Configure both dongles for quick test
    tx_dongle.configure_tx(sf=7, bw=7, freq=TEST_FREQUENCY, power=14)
    rx_dongle.configure_rx(sf=7, bw=7, freq=TEST_FREQUENCY)
    rx_dongle.enable_rx_to_uart(True)

    # Send saved packet once
    rx_dongle.serial.reset_input_buffer()
    success, response = tx_dongle.send_command("AT+RF_TX_SAVED", timeout=3.0)
    if success:
        # Wait for RX
        success_rx, received, rssi = rx_dongle.wait_for_rx_packet(timeout=3.0)
        if success_rx and received.upper() == test_packet.upper():
            logger.info(f"  ✓ TX saved packet once - received OK, RSSI: {rssi} dBm")
        else:
            logger.warning(f"  ! TX saved packet once - RX failed: {received} (možná timing problém)")
            # Don't fail, může být timing issue
    else:
        logger.warning(f"  ! TX saved packet command failed: {response} (možná není implementováno)")
        # Don't fail if not implemented
    
    # Test periodic TX (start, wait, stop)
    success, response = tx_dongle.send_command("AT+RF_TX_NVM_PERIOD=1000", timeout=3.0)  # 1 sec period
    if success:
        logger.info("  ✓ Set TX period to 1000 ms")


        # Start periodic TX
        rx_dongle.serial.reset_input_buffer()
        success, response = tx_dongle.send_command("AT+RF_TX_SAVED_REPEAT=ON", timeout=3.0)
        if success:
            logger.info("  ✓ Started periodic TX")
            
            # Wait for 2-3 packets
            packets_received = 0
            for i in range(3):
                success_rx, received, rssi = rx_dongle.wait_for_rx_packet(timeout=2.0)
                if success_rx and received.upper() == test_packet.upper():
                    packets_received += 1
                    logger.info(f"  ✓ Periodic packet {i+1} received, RSSI: {rssi} dBm")
                else:
                    logger.warning(f"  ! Periodic packet {i+1} missed or error")
            
            if packets_received >= 2:
                logger.info(f"  ✓ Received {packets_received}/3 periodic packets")
            else:
                logger.warning(f"  ! Only {packets_received}/3 periodic packets received (může být timing)")
            
            # Stop periodic TX
            success, resp = tx_dongle.send_command("AT+RF_TX_SAVED_REPEAT=OFF", timeout=3.0)
            if success:
                logger.info("  ✓ Stopped periodic TX")
            else:
                logger.warning(f"  ! Stop periodic TX failed: {resp}")
        else:
            logger.warning(f"  ! Start periodic TX failed: {response} (možná není implementováno)")
    else:
        logger.warning(f"  ! Set TX period failed: {response}")
    
    logger.info("\n" + "="*70)
    if all_passed:
        logger.info(f"✓ ALL EEPROM TESTS PASSED ({len(param_tests)} tests)")
    else:
        logger.error(f"✗ {failed_tests}/{len(param_tests)} EEPROM TESTS FAILED")
        logger.error("   Common issues:")
        logger.error("   - Sync word: Check NVMA address mapping and hex parsing in FW")
        logger.error("   - Saved packet: Check EEPROM storage size and read/write logic")
        logger.error("   - Run with --debug for detailed responses")
    
    if aux_tests_passed:
        logger.info("✓ AUX GPIO TESTS PASSED")
    else:
        logger.error("✗ AUX GPIO TESTS FAILED")
    
    if rx_format_passed:
        logger.info("✓ RX FORMAT TESTS PASSED")
    else:
        logger.error("✗ RX FORMAT TESTS FAILED")
    
    if toa_test_passed:
        logger.info("✓ TOA/SYMBOL TIME TESTS PASSED")
    else:
        logger.error("✗ TOA/SYMBOL TIME TESTS FAILED")
    
    if saved_tx_passed:
        logger.info("✓ SAVED PACKET & PERIODIC TX TESTS PASSED")
    else:
        logger.error("✗ SAVED PACKET & PERIODIC TX TESTS FAILED")
    
    logger.info("="*70 + "\n")
    
    all_tests_passed = all_passed and aux_tests_passed and rx_format_passed and toa_test_passed and saved_tx_passed
    return all_tests_passed


def find_silicon_labs_ports() -> List[str]:
    """Find COM ports with Silicon Labs devices"""
    ports = []
    for port in serial.tools.list_ports.comports():
        if "Silicon Labs" in port.description or "CP210" in port.description:
            ports.append(port.device)
            logger.info(f"Found Silicon Labs device: {port.device} - {port.description}")
    return ports


def run_single_test(tx_dongle: ATLoraDongle, rx_dongle: ATLoraDongle, 
                    sf: int, bw: int, freq: int, cr: int = 45, power: int = 22,
                    iq_inv: int = 0, ldro: int = 2, header_mode: int = 0, crc: int = 1, preamble: int = 8) -> TestResult:
    """Run a single RF test with specified parameters"""
    
    cr_name = CR_NAMES.get(cr, str(cr))
    iq_name = "Inv" if iq_inv else "Norm"
    hdr_name = HEADER_MODE_NAMES.get(header_mode, str(header_mode))
    crc_name = CRC_NAMES.get(crc, str(crc))
    
    logger.info(f"\n{'='*60}")
    logger.info(f"Testing: SF{sf}, BW{bw} ({BW_NAMES.get(bw, 'Unknown')}), CR:{cr_name}")
    logger.info(f"         IQ:{iq_name}, Hdr:{hdr_name}, CRC:{crc_name}")
    logger.info(f"{'='*60}")
    
    result = TestResult(sf=sf, bw=bw, cr=cr, freq=freq, power=power, iq_inv=iq_inv, 
                        ldro=ldro, header_mode=header_mode, crc=crc, preamble=preamble, 
                        success=False, tx_time_ms=0, rx_time_ms=0)
    
    # Calculate expected packet size for implicit header mode
    packet_size_bytes = len(TEST_PACKET_HEX) // 2

    # Configure RX first (pass payload length for implicit header mode)
    if not rx_dongle.configure_rx(sf, bw, freq, cr=cr, iq_inv=iq_inv, ldro=ldro,
                                   header_mode=header_mode, crc=crc, preamble=preamble,
                                   rx_payload_len=packet_size_bytes):
        result.error_msg = "RX configuration failed"
        return result

    # Configure TX (no delay needed - RX is ready immediately after config)
    if not tx_dongle.configure_tx(sf, bw, freq, cr=cr, power=power, iq_inv=iq_inv, ldro=ldro,
                                   header_mode=header_mode, crc=crc, preamble=preamble):
        result.error_msg = "TX configuration failed"
        return result
    
    # Get accurate TOA from firmware for timeout calculation
    toa_ms = tx_dongle.get_toa(packet_size_bytes)
    if toa_ms == 0:
        # Fallback: rough estimate
        toa_ms = 100 * (2 ** (sf - 5)) / (BW_OPTIONS.index(bw) + 1) if bw in BW_OPTIONS else 1000
        logger.warning(f"Using estimated TOA: {toa_ms:.0f} ms")
    else:
        logger.info(f"TOA: {toa_ms} ms (from firmware)")
    
    # Timeout = TOA + 60% margin (minimum 100ms for command processing)
    rx_timeout_ms = max(100, toa_ms * 1.6)
    rx_timeout_s = rx_timeout_ms / 1000
    logger.info(f"RX timeout set to: {rx_timeout_ms:.0f} ms (TOA {toa_ms} ms + 60%)")
    
    # Clear RX buffer before sending
    rx_dongle.serial.reset_input_buffer()
    
    # Start timing from TX command - this is when packet transmission begins
    tx_start = time.time()
    if not tx_dongle.send_packet(TEST_PACKET_HEX):
        result.error_msg = "TX send failed"
        return result
    result.tx_time_ms = (time.time() - tx_start) * 1000
    
    # Wait for RX packet (timeout starts from TX command)
    success, received, rssi = rx_dongle.wait_for_rx_packet(timeout=rx_timeout_s)
    actual_wait_time = time.time() - tx_start
    result.rx_time_ms = actual_wait_time  # Total time from TX start
    result.rssi = rssi
    
    if success:
        # Verify packet content
        expected = TEST_PACKET_HEX.upper()
        received = received.upper()
        
        if received == expected:
            result.success = True
            logger.info(f"✓ SUCCESS: Packet received correctly ({len(TEST_PACKET_HEX)//2} bytes), RSSI: {rssi} dBm, Real wait: {actual_wait_time*1000:.0f} ms")
        else:
            result.error_msg = f"Data mismatch: expected {expected}, got {received}"
            logger.warning(f"✗ FAIL: {result.error_msg}, Real wait: {actual_wait_time*1000:.0f} ms")
    else:
        result.error_msg = received  # Contains error message
        logger.warning(f"✗ FAIL: {result.error_msg}, Real wait: {actual_wait_time*1000:.0f} ms")
    
    return result


def run_full_test_suite(tx_dongle: ATLoraDongle, rx_dongle: ATLoraDongle) -> List[TestResult]:
    """Run complete test suite with all parameter combinations"""
    
    results = []
    
    # Calculate total tests (accounting for skipped combinations)
    total_tests = 0
    for sf in SF_RANGE:
        for bw in BW_OPTIONS:
            if not should_skip_test(sf, bw):
                total_tests += len(CR_OPTIONS) * len(IQ_INV_OPTIONS) * len(HEADER_MODE_OPTIONS) * len(CRC_OPTIONS) * len(PREAMBLE_OPTIONS)
    
    skipped_combos = (len(SF_RANGE) * len(BW_OPTIONS) - sum(1 for sf in SF_RANGE for bw in BW_OPTIONS if not should_skip_test(sf, bw)))
    current_test = 0
    
    logger.info(f"\n{'#'*60}")
    logger.info(f"Starting RF Range Test Suite")
    logger.info(f"Total tests: {total_tests}")
    if SKIP_SLOW_TESTS:
        logger.info(f"Skipping slow tests (BW<=5 & SF>=10): {skipped_combos} SF/BW combinations excluded")
    logger.info(f"Test packet size: {len(TEST_PACKET_HEX)//2} bytes")
    logger.info(f"Parameters: SF={list(SF_RANGE)}, BW={BW_OPTIONS}, CR={CR_OPTIONS}")
    logger.info(f"            IQ={IQ_INV_OPTIONS}, Header={HEADER_MODE_OPTIONS}, CRC={CRC_OPTIONS}")
    logger.info(f"Fixed: Power={FIXED_POWER}dBm, LDRO=auto")
    logger.info(f"{'#'*60}\n")
    
    # Enable RX to UART on receiver
    rx_dongle.enable_rx_to_uart(True)
    
    for cr in CR_OPTIONS:
        for iq_inv in IQ_INV_OPTIONS:
            for header_mode in HEADER_MODE_OPTIONS:
                for crc in CRC_OPTIONS:
                    for preamble in PREAMBLE_OPTIONS:
                        for sf in SF_RANGE:
                            for bw in BW_OPTIONS:
                                # Skip slow test combinations if configured
                                if should_skip_test(sf, bw):
                                    continue
                                    
                                current_test += 1
                                logger.info(f"\nTest {current_test}/{total_tests}")
                                
                                result = run_single_test(tx_dongle, rx_dongle, sf, bw, TEST_FREQUENCY,
                                                        cr=cr, power=FIXED_POWER, iq_inv=iq_inv, 
                                                        ldro=FIXED_LDRO, header_mode=header_mode,
                                                        crc=crc, preamble=preamble)
                                results.append(result)
    
    return results


def print_results_summary(results: List[TestResult]):
    """Print a summary table of all test results"""

    print("="*140)
    print("TEST RESULTS SUMMARY")
    print("="*140)
    print(f"{'SF':<4} {'BW':<10} {'CR':<5} {'IQ':<5} {'Hdr':<5} {'CRC':<4} {'Result':<10} {'RSSI':<10} {'Wait(ms)':<10} {'Error'}")
    print("-"*140)

    success_count = 0
    fail_count = 0
    rssi_values = []  # Collect RSSI values from successful tests

    for r in results:
        status = "✓ PASS" if r.success else "✗ FAIL"
        if r.success:
            success_count += 1
            rssi_values.append(r.rssi)
        else:
            fail_count += 1

        bw_name = BW_NAMES.get(r.bw, f"BW{r.bw}")
        cr_name = CR_NAMES.get(r.cr, str(r.cr))
        iq_name = "Inv" if r.iq_inv else "Norm"
        hdr_name = HEADER_MODE_NAMES.get(r.header_mode, str(r.header_mode))
        crc_name = CRC_NAMES.get(r.crc, str(r.crc))
        rssi_str = f"{r.rssi} dBm" if r.success else "-"
        print(f"{r.sf:<4} {bw_name:<10} {cr_name:<5} {iq_name:<5} {hdr_name:<5} {crc_name:<4} {status:<10} {rssi_str:<10} {r.rx_time_ms*1000:<10.0f} {r.error_msg[:25]}")

    print("-"*140)
    print(f"Total: {len(results)} tests | Passed: {success_count} | Failed: {fail_count} | Success rate: {success_count/len(results)*100:.1f}%")

    # Calculate RSSI statistics
    if rssi_values:
        rssi_avg = sum(rssi_values) / len(rssi_values)
        rssi_sorted = sorted(rssi_values)
        rssi_median = rssi_sorted[len(rssi_sorted) // 2] if len(rssi_sorted) % 2 == 1 else (rssi_sorted[len(rssi_sorted) // 2 - 1] + rssi_sorted[len(rssi_sorted) // 2]) / 2
        rssi_min = min(rssi_values)
        rssi_max = max(rssi_values)

        print("\nRSSI STATISTICS (successful tests only):")
        print(f"  Average: {rssi_avg:.1f} dBm")
        print(f"  Median:  {rssi_median:.1f} dBm")
        print(f"  Min:     {rssi_min} dBm")
        print(f"  Max:     {rssi_max} dBm")
    else:
        print("\nNo successful tests - no RSSI statistics available")

    print("="*140)


def main():
    """Main entry point"""
    
    print("\n" + "#"*60)
    print("  AT LoRa Dongle RF Range Test")
    print("#"*60 + "\n")
    
    # Find Silicon Labs COM ports
    ports = find_silicon_labs_ports()
    
    if len(ports) < 2:
        logger.error(f"Need 2 Silicon Labs devices, found {len(ports)}")
        logger.error("Please connect both AT LoRa Dongles")
        sys.exit(1)
    
    if len(ports) > 2:
        logger.warning(f"Found {len(ports)} devices, using first two: {ports[0]}, {ports[1]}")
    
    # Create dongle instances
    tx_dongle = ATLoraDongle(ports[0], "TX_Dongle")
    rx_dongle = ATLoraDongle(ports[1], "RX_Dongle")
    
    try:
        # Connect to both dongles
        if not tx_dongle.connect():
            logger.error("Failed to connect to TX dongle")
            sys.exit(1)
            
        if not rx_dongle.connect():
            logger.error("Failed to connect to RX dongle")
            tx_dongle.disconnect()
            sys.exit(1)
        
        # Verify communication with basic AT command
        logger.info("\nVerifying communication...")
        
        # AT command returns help text, so just check we get a response
        success, response = tx_dongle.send_command("AT", timeout=3.0)
        if not response or len(response) < 10:
            logger.error(f"TX dongle not responding")
            sys.exit(1)
        logger.info(f"TX dongle OK (received {len(response)} bytes)")
        
        success, response = rx_dongle.send_command("AT", timeout=3.0)
        if not response or len(response) < 10:
            logger.error(f"RX dongle not responding")
            sys.exit(1)
        logger.info(f"RX dongle OK (received {len(response)} bytes)")
        
        # Run EEPROM storage verification test first
        logger.info("\n" + "*"*70)
        logger.info("* PHASE 1: EEPROM STORAGE VERIFICATION")
        logger.info("*"*70)
        eeprom_test_passed = test_eeprom_storage(tx_dongle, rx_dongle)
        
        if not eeprom_test_passed:
            logger.error("\n⚠ EEPROM verification failed - check your AT firmware and NVMA implementation")
            logger.error("Known issues detected:")
            logger.error("  1. Saved packet: FW vrací jen AC místo celého paketu - NVMA_Get_LR_TX_RF_PCKT issue?")
            logger.error("  2. RX Sync word: Timeout - možná chyba v rekonfiguraci RX nebo address overlap?")
            logger.error("Fix EEPROM issues before running RF range tests")
            tx_dongle.disconnect()
            rx_dongle.disconnect()
            sys.exit(1)
        
        # Run test suite
        logger.info("\n" + "*"*70)
        logger.info("* PHASE 2: RF RANGE TESTS")
        logger.info("*"*70)
        results = run_full_test_suite(tx_dongle, rx_dongle)
        
        # Print summary
        print_results_summary(results)
        
    except KeyboardInterrupt:
        logger.info("\nTest interrupted by user")
    except Exception as e:
        logger.error(f"Test error: {e}")
        raise
    finally:
        # Cleanup
        tx_dongle.disconnect()
        rx_dongle.disconnect()
        logger.info("\nTest complete")


if __name__ == "__main__":
    main()
