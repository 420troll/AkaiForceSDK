import importlib.util
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location("normalize",ROOT/"tools/asr10_openasr_trace_normalize.py")
normalize=importlib.util.module_from_spec(spec); spec.loader.exec_module(normalize)

class DeviceTraceNormalizeTest(unittest.TestCase):
    def test_byte_lane_data_and_post_remap_synthetic_marker_are_preserved_once(self):
        text="\n".join([
            "T551ESP\tW\t5.400000000\t18\t04",
            "T551CAP_TEST_SYNTHETIC_BEGIN\tsec=8.000000000\taddress=00FC3030\tvalue=005A",
            "T551ESP\tW\t8.000000000\t18\t5A",
            "T551CAP_TEST_SYNTHETIC_END\tsec=8.000000000",
            "T551ESP\tCOMBINED_COMMIT\t8.100000000\t00\t00000000F000\t000001",
            "T551ESP\tPA4\t8.200000000\t18FC\t1\t1\t1",
        ])
        out,audit=normalize.convert(text)
        self.assertEqual(audit["device_host_event_count"],2)
        self.assertEqual(audit["synthetic_test_event_count"],1)
        self.assertEqual(audit["events"][1]["data"],0x5a)
        self.assertEqual(audit["events"][1]["address"],0x00fc3030)
        self.assertEqual(audit["semantic_commits"][0]["instruction_hex"],"00000000f000")
        self.assertIn("T551CAP_TEST_SYNTHETIC_BEGIN",out)
        self.assertIn("T551CAP_SYNTHETIC\t2",out)
        self.assertIn("T551_PA data=18FC run=1 received=1 loaded=1",out)

    def test_rejects_cpu_tap_only_trace(self):
        with self.assertRaisesRegex(ValueError,"no T551ESP"):
            normalize.convert("T551CAP\\tv1\\t1")

if __name__=="__main__": unittest.main()
