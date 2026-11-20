import importlib.util
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location("eda",ROOT/"tools/asr10_eda_to_img.py")
eda=importlib.util.module_from_spec(spec); spec.loader.exec_module(eda)


class EdaExpansionTest(unittest.TestCase):
    def fixture(self):
        header=bytearray(eda.BLOCK_SIZE)
        header[:len(eda.HEADER)]=eda.HEADER
        # All blocks skipped except block 0 and 9; MSB-first skip bits.
        table=memoryview(header)[eda.SKIP_TABLE_OFFSET:eda.SKIP_TABLE_OFFSET+eda.SKIP_TABLE_SIZE]
        table[:]=b"\xff"*eda.SKIP_TABLE_SIZE
        table[0] &= 0x7f
        table[1] &= 0xbf
        stored=bytes([0x31])*eda.BLOCK_SIZE+bytes([0xa4])*eda.BLOCK_SIZE
        return bytes(header)+stored+b"\0"

    def test_expands_skip_map_with_fixed_asr_geometry(self):
        image,used=eda.expand_eda(self.fixture())
        self.assertEqual(used,2)
        self.assertEqual(len(image),1_638_400)
        self.assertEqual(image[:512],bytes([0x31])*512)
        self.assertEqual(image[9*512:10*512],bytes([0xa4])*512)
        self.assertEqual(image[512:1024],eda.SKIPPED_BLOCK)

    def test_rejects_wrong_media_and_unexplained_tail(self):
        with self.assertRaises(ValueError): eda.expand_eda(b"bad")
        with self.assertRaises(ValueError): eda.expand_eda(self.fixture()+b"extra")

    def test_rejects_truncated_stored_block(self):
        raw=bytearray(self.fixture())
        raw[eda.BLOCK_SIZE:]=raw[eda.BLOCK_SIZE:eda.BLOCK_SIZE+512]
        with self.assertRaises(ValueError): eda.expand_eda(bytes(raw))


if __name__=="__main__": unittest.main()
