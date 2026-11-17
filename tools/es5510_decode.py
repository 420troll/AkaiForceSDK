#!/usr/bin/env python3
"""Field decoder for 48-bit ES5510 words, using the pinned MAME layout."""

from __future__ import annotations

ALU_NAMES = (
    "ADD", "SUB", "ADDU", "SUBU", "CMP", "AND", "OR", "XOR",
    "ABS", "MOV", "ASL2", "ASL8", "LS15", "DIFF", "ASR", "END",
)
RAM_NAMES = (
    "delay-read", "delay-write", "table-a-read", "table-a-write",
    "table-b-read", "delay-read-dump-fifo", "io-read", "io-write",
)
SPECIAL_REGISTERS = {
    234: "SER0R", 235: "SER0L", 236: "SER1R", 237: "SER1L",
    238: "SER2R", 239: "SER2L", 240: "SER3R", 241: "SER3L",
    242: "MACL", 243: "MACH", 244: "DIL_MEMSIZ", 245: "DLENGTH",
    246: "ABASE", 247: "BBASE", 248: "DBASE", 249: "SIGREG",
    250: "CCR", 251: "CMR", 252: "MINUS1", 253: "MIN",
    254: "MAX", 255: "ZERO",
}


def decode_word(word: int) -> dict[str, int | str | bool]:
    if not 0 <= word < (1 << 48):
        raise ValueError("ES5510 instruction must fit 48 bits")
    a = (word >> 16) & 0xFF
    b = (word >> 24) & 0xFF
    c = (word >> 32) & 0xFF
    d = (word >> 40) & 0xFF
    op = (word >> 12) & 0x0F
    select = (word >> 8) & 0x0F
    ram = (word >> 3) & 0x07
    return {
        "a": a, "b": b, "c": c, "d": d,
        "alu_op": ALU_NAMES[op], "operand_select": select,
        "skip": bool(word & (1 << 7)),
        "mac_accumulate": bool(word & (1 << 6)),
        "ram_control": RAM_NAMES[ram],
        "reserved_low_bits": word & 0x07,
    }

