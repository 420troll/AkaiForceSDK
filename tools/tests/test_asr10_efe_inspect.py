import importlib.util
import sys
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools"))
spec=importlib.util.spec_from_file_location("efe",ROOT/"tools/asr10_efe_inspect.py")
efe=importlib.util.module_from_spec(spec); spec.loader.exec_module(efe)


class EfeInspectTest(unittest.TestCase):
    def fixture(self):
        data=bytearray(64)
        data[:4]=bytes.fromhex("e0000000")
        data[8:10]=(5).to_bytes(2,"big")
        for i,c in enumerate(b"A B C D E F G H I J K L "):
            data[10+i]=c
        data[34:38]=(58).to_bytes(4,"big")
        data[38:40]=(46).to_bytes(2,"big")
        # Original synthetic ADD and END instructions, encoded as six bytes.
        data[46:52]=(0x000000000000).to_bytes(6,"big")
        data[52:58]=(0x0000fff5ffffff).to_bytes(6,"big")
        return bytes(data)

    def test_header_pointer_and_instruction_statistics(self):
        row=efe.inspect(self.fixture())
        self.assertEqual(row["name"],"ABCDEFGHIJKL")
        self.assertEqual(row["version"],5)
        self.assertEqual(row["fx_ucode"],46)
        self.assertEqual(row["candidate_words"],2)
        self.assertEqual(row["end_opcode_words"],1)
        self.assertTrue(row["fits_es5510_160_word_program_limit"])

    def test_rejects_out_of_bounds_ucode_pointer(self):
        data=bytearray(self.fixture()); data[38:40]=(60).to_bytes(2,"big")
        with self.assertRaises(ValueError): efe.inspect(bytes(data))


if __name__=="__main__": unittest.main()
