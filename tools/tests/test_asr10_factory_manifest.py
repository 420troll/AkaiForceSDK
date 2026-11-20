import tempfile
import unittest
from pathlib import Path

from tools.asr10_factory_manifest import build


def effect(name: str, code: bytes) -> bytes:
    data = bytearray(96 + len(code))
    data[8:10] = (5).to_bytes(2, "big")
    for i, c in enumerate(name.encode("ascii")[:12]):
        data[10 + i * 2] = c
    data[34:38] = len(data).to_bytes(4, "big")
    data[38:40] = (96).to_bytes(2, "big")
    data[60:72] = b"A\0B\0C\0\0\x82\x83\x01\0\0"
    data[96:] = code
    return bytes(data)


class FactoryManifestTests(unittest.TestCase):
    def test_cross_version_metadata_and_end_positions(self):
        # Ninety 6-byte words, END at indices 3 and 89, plus one unresolved byte.
        code = bytearray(90 * 6 + 1)
        code[3 * 6 + 4] = 0xF0
        code[89 * 6 + 4] = 0xF0
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            a, b = root / "161", root / "353"
            a.mkdir(); b.mkdir()
            for i in range(12):
                name = f"FX{i:02d}"
                payload = effect(name, bytes(code))
                (a / f"{name}.payload.bin").write_bytes(payload)
                (b / f"{name}.payload.bin").write_bytes(payload)
            manifest = build(a, b)
            self.assertEqual(len(manifest["effects"]), 12)
            row = next(x for x in manifest["effects"] if x["name"] == "FX00")
            self.assertTrue(row["payloads_byte_identical"])
            self.assertEqual(row["os_1_61"]["candidate_byte_start"], 96)
            self.assertEqual(row["os_1_61"]["fx_output_gpr_left"], 0x82)
            self.assertEqual(row["os_1_61"]["fx_output_gpr_right"], 0x83)
            self.assertEqual(row["os_1_61"]["candidate_words"], 90)
            self.assertEqual(row["os_1_61"]["section_remainder_bytes"], 1)
            self.assertEqual(row["os_1_61"]["end_opcode_indices"], [3, 89])
            serialized = str(manifest)
            self.assertNotIn("instruction_bytes", serialized)

    def test_requires_full_effect_inventory(self):
        with tempfile.TemporaryDirectory() as tmp:
            a, b = Path(tmp) / "a", Path(tmp) / "b"
            a.mkdir(); b.mkdir()
            (a / "x.payload.bin").write_bytes(effect("ONLYONE", b"\0" * 6))
            (b / "x.payload.bin").write_bytes(effect("ONLYONE", b"\0" * 6))
            with self.assertRaisesRegex(ValueError, "expected 12"):
                build(a, b)


if __name__ == "__main__":
    unittest.main()
