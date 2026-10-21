"""Real offline map/dictionary regressions; never reads the installed game."""
import json
from pathlib import Path
import unittest
from extract_maps import parse_event
from field_text import TextEvidence

ROOT = Path(__file__).parent / "output/FFX/20261003_audit"


class ExtractionTests(unittest.TestCase):
    def test_every_extracted_event_parses_including_sparse_chunks(self):
        files = list((ROOT / "files").rglob("*.ebp"))
        self.assertEqual(len(files), 397)
        for file in files:
            with self.subTest(file=file.name):
                event = parse_event(file.read_bytes())
                self.assertTrue(event["workers"])
                self.assertEqual(len(bytes.fromhex(event["code"])), event["code_length"])

    def test_area_name_domain_and_exact_regression(self):
        text = TextEvidence(ROOT / "text")
        self.assertEqual(text.macro(11, 340), "Zanarkand Stadium")
        self.assertEqual(text.macro(11, 344), "Zanarkand - Overpass")
        self.assertEqual(text.macro(11, 0), "None")
        self.assertEqual(text.macro(11, 371), "<macro:11:371>")
        maps = json.loads((ROOT / "maps_v3.json").read_text(encoding="utf-8"))
        self.assertEqual(set(maps["room_areas"]["371"]), {340})

    def test_speaker_evidence_is_native_header_not_a_mentioned_name(self):
        text = TextEvidence(ROOT / "text")
        file = next((ROOT / "files").rglob("swin0400.ebp"))
        strings = text.field_strings(file)
        self.assertTrue(strings[33].startswith("O\u2019aka\n"))


if __name__ == "__main__":
    unittest.main()
