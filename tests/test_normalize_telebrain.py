import importlib.util
import unittest
from pathlib import Path

MODULE_PATH = Path(__file__).parents[1] / "tools" / "normalize_telebrain.py"
spec = importlib.util.spec_from_file_location("normalize_telebrain", MODULE_PATH)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

def row(line, label, number, offset, kind="Pos Int", raw="0,127", shown="0,99"):
    return {"line": line, "name_raw": label, "display_order_raw": str(number) if number is not None else "bad",
            "display_order": number, "offset_raw": str(offset) if offset is not None else "bad",
            "offset": offset, "type_raw": kind, "internal_range_raw": raw,
            "display_range_raw": shown, "mod_role": None, "editable": True}

def source(tables):
    return {"format": "trollfx.telebrain.local-catalog.v1", "tables": tables}

class NormalizeTests(unittest.TestCase):
    def test_preserves_two_number_spaces_and_unknowns(self):
        result = module.normalize(source({"23": {"source_filename": "23.csv", "source_sha256": "x", "rows": [row(2, "Regen", 6, 4, "Signed Frac", "-127,127", "-98,99")]}}))
        p = result["tables"][0]["rows"][0]
        self.assertEqual((p["display_parameter_number"], p["internal_offset"]), (6, 4))
        self.assertEqual(p["raw_bounds"], [-127, 127])
        self.assertEqual(p["display_bounds"], [-98, 99])
        self.assertIsNone(p["default"])
        self.assertIsNone(p["unit"])

    def test_malformed_row_is_diagnostic_not_silently_fixed(self):
        result = module.normalize(source({"1": {"rows": [row(2, "", None, None, "", "n/a", "n/a")]}}))
        self.assertTrue(any(d["code"] == "malformed_row_fields" for d in result["diagnostics"]))
        self.assertIsNone(result["tables"][0]["rows"][0]["display_parameter_number"])

    def test_duplicate_display_id_and_offset_are_reported(self):
        rows = [row(2, "A", 1, 4), row(3, "B", 1, 4)]
        result = module.normalize(source({"1": {"rows": rows}}))
        codes = {d["code"] for d in result["diagnostics"]}
        self.assertIn("duplicate_display_parameter_number", codes)
        self.assertIn("duplicate_internal_offset", codes)

    def test_ambiguous_reverse_mapping_is_explicit_and_searchable(self):
        result = module.normalize(source({"54": {"rows": [row(2, "Reverse Attack", 4, 2)]}}))
        self.assertTrue(result["tables"][0]["associations_ambiguous"])
        self.assertIn("ambiguous_algorithm_mapping", {d["code"] for d in result["diagnostics"]})
        self.assertEqual(module.search(result, "reverse")[0]["table_id"], 54)

    def test_duplicate_algorithm_table_keys_are_detected(self):
        class DuplicateDict(dict):
            def items(self): return [("1", {"rows": []}), ("1", {"rows": []})]
        result = module.normalize({"format": "trollfx.telebrain.local-catalog.v1", "tables": DuplicateDict()})
        self.assertIn("duplicate_table_id", {d["code"] for d in result["diagnostics"]})

if __name__ == "__main__":
    unittest.main()
