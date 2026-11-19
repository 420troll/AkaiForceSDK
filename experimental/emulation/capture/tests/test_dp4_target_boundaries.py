import unittest

from tools.dp4_ghidra.validate_targets import validate
from tools.dp4_ghidra.dp4_bus_map import (
    u46_esp_output,
    u46_y_output,
    u54_file_offset,
    u56_file_offset,
)


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

    def test_u46_low_address_bits_select_named_esp_outputs(self):
        # The helper models only U46 A9-A11.  It intentionally does not claim
        # that the decoder's other enable inputs are active at these addresses.
        self.assertEqual([u46_y_output(n * 0x200) for n in range(8)],
                         list(range(8)))
        self.assertEqual([u46_esp_output(n * 0x200) for n in range(8)],
                         [None, None, None, None, 1, 2, 3, 4])
        self.assertEqual(u46_y_output(0x1800), 4)


if __name__ == "__main__":
    unittest.main()
