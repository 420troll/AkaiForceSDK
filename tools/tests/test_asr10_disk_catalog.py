import importlib.util
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location("disk_catalog",ROOT/"tools/asr10_disk_catalog.py")
catalog_module=importlib.util.module_from_spec(spec); spec.loader.exec_module(catalog_module)


class DiskCatalogTest(unittest.TestCase):
    def fixture(self):
        image=bytearray(catalog_module.BLOCK*catalog_module.BLOCKS)
        image[catalog_module.BLOCK+38:catalog_module.BLOCK+40]=b"ID"
        image[2*catalog_module.BLOCK+28:2*catalog_module.BLOCK+30]=b"OS"
        image[4*catalog_module.BLOCK+510:5*catalog_module.BLOCK]=b"DR"
        entry=3*catalog_module.BLOCK
        image[entry+1]=32
        image[entry+2:entry+14]=b"ASR-10 OS   "
        image[entry+14:entry+16]=(1).to_bytes(2,"big")
        image[entry+16:entry+18]=(1).to_bytes(2,"big")
        image[entry+18:entry+22]=(24).to_bytes(4,"big")
        fat=(catalog_module.FAT_START+(24//catalog_module.FAT_ENTRIES_PER_BLOCK))*catalog_module.BLOCK+(24%catalog_module.FAT_ENTRIES_PER_BLOCK)*3
        image[fat:fat+3]=(1).to_bytes(3,"big")
        image[24*catalog_module.BLOCK:25*catalog_module.BLOCK]=b"O"*catalog_module.BLOCK
        return bytes(image)

    def test_catalogs_verified_asr_root_and_file_extent(self):
        rows=catalog_module.catalog(self.fixture())
        self.assertEqual(len(rows),1)
        self.assertEqual((rows[0]["type"],rows[0]["name"],rows[0]["blocks"]),(32,"ASR-10 OS",1))
        self.assertEqual(rows[0]["sha256"],__import__("hashlib").sha256(b"O"*512).hexdigest())

    def test_rejects_non_asr_and_bad_fat(self):
        with self.assertRaises(ValueError): catalog_module.catalog(b"bad")
        image=bytearray(self.fixture())
        fat=(catalog_module.FAT_START+(24//catalog_module.FAT_ENTRIES_PER_BLOCK))*catalog_module.BLOCK+(24%catalog_module.FAT_ENTRIES_PER_BLOCK)*3
        image[fat:fat+3]=b"\0\0\0"
        with self.assertRaises(ValueError): catalog_module.catalog(bytes(image))


if __name__=="__main__": unittest.main()
