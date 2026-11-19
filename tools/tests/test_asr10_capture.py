import importlib.util
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location("capture",ROOT/"tools/asr10_capture_reconstruct.py")
capture=importlib.util.module_from_spec(spec); spec.loader.exec_module(capture)

def row(seq,addr,data,kind="host_w",pc=0x1234):
    return f"T551CAP\tv1\t{seq}\t{addr:08X}\t{data:08X}\tFFFFFFFF\t{pc:08X}\t{kind}\t241f9331477b07b072330ca6ebd2d7a2e2cc832e\t8be57cdc5f44ab830dff44c06dd39f24190742d4\t0.{seq}"

class CaptureTest(unittest.TestCase):
    def test_latches_commit_in_event_order_and_never_authenticates_fixture(self):
        lines=["T551CAP_META\t1\tfixture=synthetic",row(1,0xFC6823,0,"padat_w")]
        for i,b in enumerate((0,0,0)): lines.append(row(2+i,0xFC3000+i,b))
        for i,b in enumerate((0,0,0,0,0xF0,0)): lines.append(row(5+i,0xFC3003+i,b))
        lines += [row(11,0xFC31C1,0,"combined_commit_w"),
                  row(12,0xFC6823,0x10,"padat_w")]
        got=capture.reconstruct("\n".join(lines))
        self.assertEqual(got["instructions"],[{"index":0,"word_hex":"00000000f000"}])
        self.assertEqual(got["gprs"],[{"register":0,"value_hex":"000000"}])
        self.assertFalse(got["validated_factory_program"])
        self.assertEqual(got["source_class"],"synthetic_test_fixture")
        self.assertEqual(got["capture_status"],"incomplete")

    def test_rejects_malformed_rows(self):
        with self.assertRaises(capture.TraceError): capture.reconstruct("T551CAP\tbroken")

    def test_no_event_is_not_a_capture(self):
        with self.assertRaises(capture.TraceError): capture.reconstruct("# empty")

if __name__=="__main__": unittest.main()
