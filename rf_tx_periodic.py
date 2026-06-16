#!/usr/bin/env python3
"""
Periodic RF TX - sends a 29-byte HEX packet every 1 second on COM7.
Does NOT change any RF settings - uses whatever is stored in the dongle.
Packet content: 4-byte sequence counter (big-endian) + 25 random bytes.
"""

import serial
import time
import os
import struct
import sys

PORT       = "COM7"
BAUD_LIST  = [115200, 230400]   # tried in order during auto-detection
PACKET_LEN = 29                 # bytes
PERIOD_S   = 10.0                # seconds between packets
TX_TIMEOUT = 3.0                # max seconds to wait for OK after TX command


# ---------------------------------------------------------------------------
# Serial helpers
# ---------------------------------------------------------------------------

def _read_response(ser: serial.Serial, timeout: float) -> tuple[bool, str]:
    """Read lines until 'OK' or 'ERROR' is found or timeout expires.
    Uses in_waiting polling so it never blocks longer than needed."""
    buf = b""
    lines = []
    deadline = time.time() + timeout

    while time.time() < deadline:
        if ser.in_waiting:
            buf += ser.read(ser.in_waiting)
            while b"\n" in buf:
                raw, buf = buf.split(b"\n", 1)
                line = raw.decode("utf-8", errors="ignore").strip()
                if not line:
                    continue
                lines.append(line)
                if line == "OK":
                    return True, "\n".join(lines)
                if "ERROR" in line:
                    return False, "\n".join(lines)
        else:
            time.sleep(0.005)

    return False, "\n".join(lines)


def send_at(ser: serial.Serial, cmd: str, timeout: float = TX_TIMEOUT) -> tuple[bool, str]:
    ser.reset_input_buffer()
    ser.write((cmd + "\r\n").encode())
    return _read_response(ser, timeout)


def detect_baud(port: str) -> tuple[serial.Serial, int]:
    """Try each baud rate; return open Serial + working baud or raise."""
    for baud in BAUD_LIST:
        try:
            ser = serial.Serial(port, baudrate=baud, timeout=0.5, write_timeout=1.0)
            ser.reset_input_buffer()
            ser.write(b"AT\r\n")
            time.sleep(0.3)
            if ser.in_waiting:
                data = ser.read(ser.in_waiting).decode("utf-8", errors="ignore")
                if "AT+" in data or "OK" in data:
                    ser.timeout = TX_TIMEOUT
                    ser.reset_input_buffer()
                    print(f"  Detected {baud} baud on {port}")
                    return ser, baud
            ser.close()
        except serial.SerialException:
            pass
    raise RuntimeError(f"Cannot detect baud rate on {port}. Tried: {BAUD_LIST}")


# ---------------------------------------------------------------------------
# Packet generation
# ---------------------------------------------------------------------------

def make_packet(seq: int) -> str:
    """4-byte big-endian counter + 25 random bytes → uppercase hex string."""
    header = struct.pack(">I", seq & 0xFFFFFFFF)
    payload = os.urandom(PACKET_LEN - len(header))
    return (header + payload).hex().upper()


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    print(f"Auto-detecting baud rate on {PORT}...")
    try:
        ser, baud = detect_baud(PORT)
    except (RuntimeError, serial.SerialException) as e:
        print(f"ERROR: {e}")
        sys.exit(1)

    print(f"Connected at {baud} baud. Sending {PACKET_LEN}-byte packets every {PERIOD_S}s. Ctrl+C to stop.\n")

    seq = 0
    try:
        while True:
            loop_start = time.time()

            hex_data = make_packet(seq)
            ok, response = send_at(ser, f"AT+RF_TX_HEX={hex_data}")

            status = "OK " if ok else "ERR"
            resp_short = response.replace("\n", " | ")
            print(f"[{seq:>6}] {status}  {hex_data}  {resp_short!r}")

            seq += 1

            elapsed = time.time() - loop_start
            remaining = PERIOD_S - elapsed
            if remaining > 0:
                time.sleep(remaining)

    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        ser.close()


if __name__ == "__main__":
    main()
