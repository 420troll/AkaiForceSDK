"""Opt-in Ghidra smoke test using synthetic bytes, never proprietary firmware."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest


GHIDRA = os.environ.get("GHIDRA_INSTALL_DIR")
JAVA_HOME = os.environ.get("JAVA_HOME")


@unittest.skipUnless(GHIDRA and JAVA_HOME, "set GHIDRA_INSTALL_DIR and JAVA_HOME")
class GhidraSmokeTest(unittest.TestCase):
    def test_binary_import_reset_vector_and_report(self):
        repo = Path(__file__).resolve().parents[4]
        runner = repo / "tools/dp4_ghidra/run_u56.sh"
        with tempfile.TemporaryDirectory(prefix="dp4-ghidra-smoke-") as temp:
            root = Path(temp)
            rom = bytearray([0x01] * 0x8000)
            rom[0x0000:0x0006] = bytes([0xCE, 0x51, 0x00, 0x6E, 0x00, 0x39])
            rom[0x7FFE:0x8000] = bytes([0x80, 0x00])
            image = root / "synthetic-u56.bin"
            image.write_bytes(rom)
            (root / "ghidra-home").mkdir()
            result = subprocess.run(
                [str(runner), str(image), str(root / "work"), "smoke"],
                text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                env={**os.environ, "GHIDRA_INSTALL_DIR": GHIDRA,
                     "JAVA_HOME": JAVA_HOME,
                     "JAVA_TOOL_OPTIONS": f"-Duser.home={root / 'ghidra-home'}"}, timeout=180,
            )
            self.assertEqual(result.returncode, 0, result.stdout[-6000:])
            report = (root / "work/smoke-report.txt").read_text()
            self.assertIn("LANGUAGE MC6801:BE:16:default", report)
            self.assertIn("entry=8000", result.stdout.lower())
            self.assertIn("INSTRUCTION_COUNT", report)


if __name__ == "__main__":
    unittest.main()
