import unittest

from tools.dp4_ghidra.validate_targets import validate


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


if __name__ == "__main__":
    unittest.main()
