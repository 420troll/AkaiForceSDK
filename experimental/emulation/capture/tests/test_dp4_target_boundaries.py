import unittest

from tools.dp4_ghidra.validate_targets import validate
from tools.dp4_ghidra.dp4_bus_map import (
    u46_esp_output,
    u46_y_output,
    u54_cpu_file_offset,
    u54_file_offset,
    u56_file_offset,
)
from tools.dp4_ghidra.verify_u54_tag import U54_SIZE, U56_SIZE, verify_tag_pair


class CandidateBoundaryTests(unittest.TestCase):
    def test_accepts_only_reported_instruction_starts(self):
        report = """PROGRAM synthetic
INSTRUCTION_BOUNDARY 8000
INSTRUCTION_BOUNDARY 8003
INSTRUCTION_BOUNDARY 8005
"""
        self.assertEqual(validate(report, [0x8000, 0x8005]),
                         [(0x8000, True), (0x8005, True)])
        self.assertEqual(validate(report, [0x8001]), [(0x8001, False)])

    def test_rejects_report_without_boundaries(self):
        with self.assertRaisesRegex(ValueError, "no INSTRUCTION_BOUNDARY"):
            validate("PROGRAM empty\n", [0x8000])


class BaseDp4BusMapTests(unittest.TestCase):
    def test_u56_window_maps_cpu_reset_vector_to_file_offset(self):
        self.assertEqual(u56_file_offset(0x8000), 0)
        self.assertEqual(u56_file_offset(0xFFFE), 0x7FFE)
        with self.assertRaisesRegex(ValueError, "outside the U56 window"):
            u56_file_offset(0x7FFF)

    def test_u54_four_latched_bank_bits_cover_128k(self):
        self.assertEqual(u54_file_offset(0, 0), 0)
        self.assertEqual(u54_file_offset(1, 0), 0x2000)
        self.assertEqual(u54_file_offset(15, 0x1FFF), 0x1FFFF)
        with self.assertRaisesRegex(ValueError, "bank"):
            u54_file_offset(16, 0)
        with self.assertRaisesRegex(ValueError, "low address"):
            u54_file_offset(0, 0x2000)

    def test_firmware_correlated_u54_window_maps_revision_tag(self):
        self.assertEqual(u54_cpu_file_offset(9, 0x2000), 0x12000)
        self.assertEqual(u54_cpu_file_offset(9, 0x3FFF), 0x13FFF)
        with self.assertRaisesRegex(ValueError, "firmware-correlated U54 window"):
            u54_cpu_file_offset(9, 0x4000)

    def test_u46_low_address_bits_select_named_esp_outputs(self):
        # The helper models only U46 A9-A11.  It intentionally does not claim
        # that the decoder's other enable inputs are active at these addresses.
        self.assertEqual([u46_y_output(n * 0x200) for n in range(8)],
                         list(range(8)))
        self.assertEqual([u46_esp_output(n * 0x200) for n in range(8)],
                         [None, None, None, None, 1, 2, 3, 4])
        self.assertEqual(u46_y_output(0x1800), 4)


class U54RevisionTagTests(unittest.TestCase):
    @staticmethod
    def images(tag):
        u56 = bytearray([0xFF] * U56_SIZE)
        # Synthetic MC6801 LDX $2000; CPX #tag; BEQ rel sequence.
        check_offset = 0x572
        u56[check_offset:check_offset + 8] = bytes(
            [0xFE, 0x20, 0x00, 0x8C, tag >> 8, tag & 0xFF, 0x27, 0x05]
        )
        u54 = bytearray([0xFF] * U54_SIZE)
        u54[0x12000:0x12002] = tag.to_bytes(2, "big")
        return bytes(u56), bytes(u54)

    def test_matches_u56_revision_check_to_u54_bank9_tag(self):
        u56, u54 = self.images(0x0106)
        match = verify_tag_pair(u56, u54)
        self.assertEqual(match.u56_file_offset, 0x572)
        self.assertEqual(match.u56_cpu_address, 0x8572)
        self.assertEqual(match.u54_file_offset, 0x12000)
        self.assertEqual(match.tag, 0x0106)

    def test_rejects_mismatched_revision_tag(self):
        u56, u54 = self.images(0x010F)
        bad_u54 = bytearray(u54)
        bad_u54[0x12000:0x12002] = b"\x01\x06"
        with self.assertRaisesRegex(ValueError, "U56 expects UCODE tag"):
            verify_tag_pair(u56, bytes(bad_u54))

    def test_rejects_ambiguous_tag_checks(self):
        u56, u54 = self.images(0x0106)
        duplicate = bytearray(u56)
        duplicate[0x700:0x708] = duplicate[0x572:0x57A]
        with self.assertRaisesRegex(ValueError, "found 2"):
            verify_tag_pair(bytes(duplicate), u54)


if __name__ == "__main__":
    unittest.main()
