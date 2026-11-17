import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[4]))
from tools.es5510_decode import decode_word
from tools.es5510_rom_forensics import analyze_rom, scan_stream


def enc(a=0, b=0, c=0, d=0, op=0, select=0, ram=0, skip=0, accum=0):
    return ((d & 255) << 40) | ((c & 255) << 32) | ((b & 255) << 24) | \
        ((a & 255) << 16) | ((op & 15) << 12) | ((select & 15) << 8) | \
        ((skip & 1) << 7) | ((accum & 1) << 6) | ((ram & 7) << 3)


class RomForensicsTests(unittest.TestCase):
    def test_mame_field_decode(self):
        word = enc(a=3, b=4, c=5, d=6, op=9, select=12, ram=5, skip=1, accum=1)
        got = decode_word(word)
        self.assertEqual((got["a"], got["b"], got["c"], got["d"]), (3, 4, 5, 6))
        self.assertEqual(got["alu_op"], "MOV")
        self.assertEqual(got["operand_select"], 12)
        self.assertTrue(got["skip"])
        self.assertTrue(got["mac_accumulate"])
        self.assertEqual(got["ram_control"], "delay-read-dump-fifo")

    def test_synthetic_aligned_program_finds_end_span(self):
        words = [enc(op=0), enc(a=0, b=1, op=0), enc(op=15)]
        payload = b"FIXTURE!" + b"".join(w.to_bytes(6, "big") for w in words)
        # Explicitly analyze the byte offset where our original fixture starts.
        result = scan_stream(payload, 8, tuple(range(6)))
        self.assertEqual(result["complete_words"], 3)
        self.assertEqual(result["end_opcode_count"], 1)
        self.assertEqual(result["end_terminated_spans_1_to_160"], 1)
        self.assertEqual(result["end_span_median_words"], 3)

    def test_rom_scan_emits_metadata_not_instruction_bytes(self):
        words = [enc(op=0), enc(op=15)]
        payload = b"Small Room Rev\0" + b"".join(w.to_bytes(6, "big") for w in words)
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "synthetic-ucode.bin"
            path.write_bytes(payload)
            report = analyze_rom(path)
        self.assertEqual(report["size_bytes"], len(payload))
        self.assertEqual(report["ascii_algorithm_label_occurrences"][0]["label"], "Small Room Rev")
        self.assertNotIn("instructions", report)
        self.assertIn("be48", report["raw_48bit_packing_hypotheses"])

    def test_version_diff_metadata(self):
        a = b"abcdefgh" * 3
        b = bytearray(a)
        b[3] ^= 0x80
        with tempfile.TemporaryDirectory() as tmp:
            pa, pb = Path(tmp) / "a.bin", Path(tmp) / "b.bin"
            pa.write_bytes(a); pb.write_bytes(b)
            report = analyze_rom(pa, pb)
        self.assertEqual(report["comparison"]["changed_bytes"], 1)
        self.assertEqual(report["comparison"]["first_changed_offset"], 3)

    def test_label_table_geometry_is_detected_without_decoding_payload(self):
        records = []
        for i in range(50):
            label = ("3Effect %02d" % i).encode("ascii")
            records.append(label.ljust(51, b"\x00"))
        payload = b"".join(records)
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "fixture.bin"
            path.write_bytes(payload)
            report = analyze_rom(path)
        table = report["label_table_hypotheses"][0]
        self.assertEqual(table["candidate_records"], 50)
        self.assertEqual(table["stride_bytes"], 51)
        self.assertEqual(table["end_exclusive"], 2550)
        self.assertNotIn("instructions", report)

    def test_revision_diff_maps_changes_against_detected_label_region(self):
        records = [("3Effect %02d" % i).encode("ascii").ljust(51, b"\x00")
                   for i in range(50)]
        a = bytearray(b"".join(records) + b"tail-data")
        b = bytearray(a)
        b[2] ^= 1
        b[2552] ^= 1
        with tempfile.TemporaryDirectory() as tmp:
            pa, pb = Path(tmp) / "v1.bin", Path(tmp) / "v2.bin"
            pa.write_bytes(a); pb.write_bytes(b)
            report = analyze_rom(pa, pb)
        comparison = report["comparison"]
        self.assertEqual(comparison["named_region_changes"][0]["changed_bytes"], 1)
        self.assertEqual(comparison["changed_bytes_outside_named_regions"], 1)


if __name__ == "__main__":
    unittest.main()

