#!/usr/bin/env python3
"""Generate the original 12-stage stereo ES5510 phaser instruction image."""
from pathlib import Path


def encode(a, b, c=255, d=255, op=9, select=0, ram=6):
    return ((d & 255) << 40) | ((c & 255) << 32) | ((b & 255) << 24) | \
           ((a & 255) << 16) | ((op & 15) << 12) | ((select & 15) << 8) | ((ram & 7) << 3)


def mov(dst, src):
    return encode(dst, src, op=9)


def alu2(dst, src, op):
    return encode(dst, src, op=op)


def nop():
    return encode(255, 255, 255, 255, op=4)


def mac(dst, coefficient):
    # The MAC result is written to C/dst. ALU op 4 is a no-write compare/no-op.
    return encode(255, 255, dst, coefficient, op=4)


def make_program():
    words = []
    # Preserve the dry inputs and create first-stage inputs with signed feedback.
    for input_reg, serial_reg, fb_state in ((32, 239, 150), (33, 238, 151)):
        words.append(mov(input_reg, serial_reg))
        words.append(mac(fb_state, 152))
        words.append(alu2(input_reg, fb_state, 2))

    for channel in range(2):
        inp = 32 + channel
        coefficient_base = 64 if channel == 0 else 80
        x_base = 100 if channel == 0 else 124
        y_base = 112 if channel == 0 else 136
        for stage in range(12):
            x_current = inp if stage == 0 else y_base + stage - 1
            x_previous = x_base + stage
            y_previous = y_base + stage
            coefficient = coefficient_base + stage
            words.append(alu2(y_previous, x_current, 3))  # y1 - x
            words.append(nop())  # let the one-word ALU pipeline write y1-x before MAC reads it
            words.append(mac(y_previous, coefficient))    # a * (y1 - x)
            words.append(alu2(y_previous, x_previous, 2)) # x1 + product
            words.append(mov(x_previous, x_current))      # save x[n]
        # Save final allpass output for next frame's feedback before wet scaling.
        words.append(mov(150 + channel, y_base + 11))
        # The host performs only the final wet/dry blend; audio remains in ESP.
        words.append(mov(241 - channel, y_base + 11))

    words.append(encode(255, 255, 255, 255, op=15, select=0, ram=0))
    if len(words) > 160:
        raise ValueError(f"program exceeds 160 words: {len(words)}")
    return words


def main():
    out = Path(__file__).resolve().parent / "phaser_12stage_words.inc"
    words = make_program()
    lines = [f"0x{word:012x}ULL," for word in words]
    out.write_text("\n".join(lines) + "\n")
    print(f"generated {len(words)} words -> {out}")


if __name__ == "__main__":
    main()
