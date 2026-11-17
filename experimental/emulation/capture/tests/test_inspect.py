import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[4]))
from tools.es5510_capture_inspect import CaptureError, inspect_capture


class CaptureInspectorTests(unittest.TestCase):
    def capture(self, text: str):
        temp = tempfile.NamedTemporaryFile("w", encoding="ascii", delete=False)
        try:
            temp.write(text)
            temp.close()
            return inspect_capture(Path(temp.name))
        finally:
            Path(temp.name).unlink(missing_ok=True)

    def test_neutral_hex_word_list(self):
        result = self.capture("# synthetic\n000: fffffffff000\n0x123456789abc\n")
        self.assertEqual(result["source_format"], "neutral-hex-48-bit-word-list")
        self.assertEqual(result["instruction_count"], 2)
        self.assertEqual(result["end_opcode_indices_candidate"], [0])
        self.assertEqual(result["instructions"][0]["decoded_mame_fields"]["alu_op"], "END")
        self.assertEqual(result["instructions"][1]["decoded_mame_fields"]["alu_op"], "MOV")

    def test_eps16_trace_records_do_not_claim_full_gpr_image(self):
        result = self.capture(
            "es5510_program[000]=fffffffff000 gpr=000000\n"
            "es5510_program[001]=123456789abc gpr=654321\n"
        )
        self.assertEqual(result["source_format"], "eps16-trace-es5510-program-lines")
        self.assertEqual(result["instructions"][1]["trace_gpr_at_same_index_hex"], "654321")
        self.assertIn("not a complete GPR", result["limitations"][1])

    def test_non_contiguous_trace_rejected(self):
        with self.assertRaisesRegex(CaptureError, "not contiguous"):
            self.capture("es5510_program[001]=fffffffff000 gpr=000000\n")

    def test_malformed_record_rejected(self):
        with self.assertRaisesRegex(CaptureError, "expected up to twelve hex"):
            self.capture("0x123456789abcdef\n")

    def test_more_than_program_capacity_rejected(self):
        with self.assertRaisesRegex(CaptureError, "at most 160"):
            self.capture("\n".join(f"{i:03x}" for i in range(161)))


if __name__ == "__main__":
    unittest.main()
