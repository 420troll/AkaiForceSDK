import importlib.util
import struct
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[1] / "tools" / "extract_es5510_program.py"
spec = importlib.util.spec_from_file_location("extract_es5510_program", SCRIPT)
extractor = importlib.util.module_from_spec(spec)
spec.loader.exec_module(extractor)


class CaptureExtractorTest(unittest.TestCase):
    def test_extracts_code_only_through_end(self):
        instructions = bytearray()
        words = [0x000000009000] * 160
        words[2] = 0x00000000f040
        for word in words:
            instructions.extend(word.to_bytes(6, "big"))
        gpr = bytes([0x11]) * (192 * 3)
        spr = bytes([0x22]) * (22 * 3)
        state = bytes([0x33]) * 74
        dram = bytes([0x44]) * (1 << 21)
        payload = bytes(instructions) + gpr + spr + state + dram
        header = b"ES5510C\0" + struct.pack("<9I", 1, 44, 160, 192, 22,
                                               1 << 20, 1, 74, len(payload))
        with tempfile.TemporaryDirectory() as tmp:
            src, dst = Path(tmp) / "fixture.esp", Path(tmp) / "program.esp"
            src.write_bytes(header + payload)
            count = extractor.extract(src, dst, 44100, 0x20)
            text = dst.read_text()
            self.assertEqual(count, 3)
            self.assertEqual(text.count("instruction_at "), 3)
            self.assertIn("host_serial 0x20", text)
            self.assertNotIn("gpr ", text)
            self.assertNotIn("spr ", text)
            self.assertNotIn("dram ", text)

    def test_rejects_malformed_capture(self):
        with tempfile.TemporaryDirectory() as tmp:
            src, dst = Path(tmp) / "bad.esp", Path(tmp) / "out.esp"
            src.write_bytes(b"not a capture")
            with self.assertRaises(ValueError):
                extractor.extract(src, dst, 44100, 0x20)


if __name__ == "__main__":
    unittest.main()
