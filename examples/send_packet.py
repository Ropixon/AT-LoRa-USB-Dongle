"""Send one raw LoRa packet and wait for the radio to finish transmitting."""

import argparse
import time

import serial


def wait_for_line(port, matches, expected, timeout):
    deadline = time.monotonic() + timeout
    pending = bytearray()

    while time.monotonic() < deadline:
        pending.extend(port.read_until())
        if not pending.endswith(b"\n"):
            continue

        line = pending.decode("ascii", errors="replace").rstrip("\r\n")
        pending.clear()
        if line.startswith("ERROR") or line in {"+RADIO:ERROR", "+TX:BUSY"}:
            raise RuntimeError(line)
        if matches(line):
            return line

    raise TimeoutError(f"Timed out waiting for {expected}.")


def send_command(port, command):
    port.write(f"{command}\r\n".encode("ascii"))


def send_packet(port, payload):
    """Use a configured, idle dongle with no periodic transmission running."""
    send_command(port, "AT+IDENTIFY")
    identity = wait_for_line(
        port,
        lambda line: line.startswith("Ropixon AT-USB LoRa Dongle,"),
        "the dongle identification",
        timeout=2,
    )
    print(identity)

    send_command(port, "AT+CAPS")
    capabilities = wait_for_line(
        port,
        lambda line: line.startswith("+CAPS:"),
        "the firmware capabilities",
        timeout=2,
    )
    if "TX_DONE" not in capabilities.removeprefix("+CAPS:").split(","):
        raise RuntimeError("This example requires firmware with the TX_DONE capability.")

    send_command(port, f"AT+RF_TX_HEX={payload.hex().upper()}")
    result = wait_for_line(
        port,
        lambda line: line == "+TX:DONE",
        "+TX:DONE",
        timeout=15,
    )
    print(result)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("port", help="USB serial port, for example COM3 or /dev/ttyUSB0")
    parser.add_argument("--baud", type=int, default=230400, help="baud rate (default: 230400)")
    args = parser.parse_args()

    try:
        with serial.Serial(args.port, args.baud, timeout=0.2, write_timeout=2) as port:
            port.reset_input_buffer()
            send_packet(port, b"Hello")
    except (serial.SerialException, RuntimeError, TimeoutError) as error:
        parser.exit(1, f"{error}\n")


if __name__ == "__main__":
    main()
