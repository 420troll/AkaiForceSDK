"""Pure address calculations from the base DP/4 Rev J schematic.

This module deliberately does not guess the U54 CPU window or the high-address
enable terms for U46.  It only models wiring directly visible on the drawing.
"""

U56_BASE = 0x8000
U56_SIZE = 0x8000
U54_BANK_SIZE = 0x2000


def u56_file_offset(cpu_address: int) -> int:
    """Map an address in the schematic-confirmed U56 window to its file offset."""
    if not U56_BASE <= cpu_address < U56_BASE + U56_SIZE:
        raise ValueError("CPU address is outside the U56 window")
    return cpu_address - U56_BASE


def u54_file_offset(bank: int, low_address: int) -> int:
    """Map U54's four U57 bank bits and CPU A0-A12 to a physical ROM offset."""
    if not 0 <= bank < 16:
        raise ValueError("U54 bank must fit U57 B0-B3")
    if not 0 <= low_address < U54_BANK_SIZE:
        raise ValueError("U54 low address must fit A0-A12")
    return bank * U54_BANK_SIZE + low_address


def u46_y_output(cpu_address: int) -> int:
    """Return the raw U46 74HC138 output index from address inputs A9-A11.

    The returned index does not mean the decoder is enabled.  Its global
    address-enable terms are not folded into this helper.
    """
    if not 0 <= cpu_address <= 0xFFFF:
        raise ValueError("6803 CPU address must be 16-bit")
    return (cpu_address >> 9) & 0b111


def u46_esp_output(cpu_address: int) -> int | None:
    """Return ESP number for U46 Y4-Y7, or None for Y0-Y3.

    This is a decoder-output candidate only; callers must independently
    establish that U46's other enable inputs are active at the CPU address.
    """
    output = u46_y_output(cpu_address)
    return output - 3 if output >= 4 else None
