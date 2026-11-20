import importlib.util
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location("inventory",ROOT/"tools/asr10_firmware_inventory.py")
inventory=importlib.util.module_from_spec(spec); spec.loader.exec_module(inventory)

class InventoryTest(unittest.TestCase):
    def test_ne_is_identified_without_execution(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/"installer.exe"; data=bytearray(0x200)
            data[:2]=b"MZ"; data[0x3c:0x40]=(0x40).to_bytes(4,"little")
            data[0x40:0x42]=b"NE"; data[0x40+0x1c:0x40+0x1e]=(4).to_bytes(2,"little")
            data[0x40+0x22:0x40+0x24]=(0x40).to_bytes(2,"little")
            data[0x40+0x32:0x40+0x34]=(8).to_bytes(2,"little")
            data.extend(b"SETUPMN.ARV asr161os.eda")
            p.write_bytes(data)
            result=inventory.inspect(p)
            self.assertIn("NE self-extracting installer",result["format"])
            self.assertEqual(result["segment_count"],4)
            self.assertEqual(result["referenced_filename_strings"],["SETUPMN.ARV","asr161os.eda"])
            self.assertEqual(result["size"],len(data))

    def test_rom_fingerprint_only_matches_exact_bytes(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/"asr-648c-lo-1.5b.bin"; p.write_bytes(b"synthetic")
            result=inventory.inspect(p)
            self.assertFalse(result["fingerprint_match"])

    def test_asr_diskwriter_header_is_not_mislabeled_as_raw_or_microcode(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/"asr161os.eda"
            p.write_bytes(b"\r\nASR-10 Disk"+b"\0"*100)
            result=inventory.inspect(p)
            self.assertIn("DiskWriter EDA image",result["format"])
            self.assertFalse(result["raw_floppy_image"])
            self.assertIn("unparsed",result["internal_layout"])

    def test_unknown_eda_suffix_does_not_imply_verified_asr_image(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/"unknown.eda"; p.write_bytes(b"not an identified image")
            result=inventory.inspect(p)
            self.assertEqual(result["format"],"unverified Ensoniq disk-image candidate")

if __name__=="__main__": unittest.main()
