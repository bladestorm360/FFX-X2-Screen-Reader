"""Combined offline checks for every trial; does not execute a probe."""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

import hashlib
import json
from pathlib import Path
import sys
import unittest

DIRECTORY = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(DIRECTORY))
from atel_model import binding, compile_script, instructions
from compile_trials import ROOT, enrich, entry_graph, fnv32
from trial_designs import TRIALS
from validation_host import describe_state, script_source
from field_text import TextEvidence

EXTRACTION = evidence_path(ROOT / "Docs/Scripts/Offline/output/FFX/20261003_audit")
MANIFEST = Path(sys.argv.pop(1)).resolve() if len(sys.argv) > 1 else None


def declaration(index, offset, location=6, fmt=5, count=1):
    low = offset | (location << 25) | (fmt << 28)
    return dict(index=index, offset=offset, reference_location=location,
                reference_format=fmt, raw_low=low, raw_high=count)


def worker(entries, jumps=(), decls=None, index=0):
    return dict(index=index, type=3, entries=list(entries), jumps=list(jumps),
                ints=[], floats=[], declarations=decls or [declaration(0, 4)],
                event_data_offset=0x200, private_data_offset=0x400, shared_data_offset=0x600)


def fixture(code, workers):
    return dict(script="fixture0", sha256="fixture", code=bytes(code).hex(),
                english_strings=[], area_names=[], workers=workers)


class ControlFlowTests(unittest.TestCase):
    def test_high_bit_width_and_truncation(self):
        self.assertEqual(instructions(bytes([0xAE, 1, 0, 0x3C]))[0], (0xAE, 1, 3))
        with self.assertRaises(ValueError):
            instructions(bytes([0xAE, 1]))

    def test_array_assignment_keeps_index_before_value(self):
        model = compile_script(fixture([0xAE, 2, 0, 0xAE, 9, 0, 0xA3, 0, 0, 0x3C],
                                       [worker([0], decls=[declaration(0, 4, count=3)])]))
        write = model["workers"][0]["blocks"][0]["statements"][0]
        self.assertEqual((write["element"], write["value"]), ("2", "9"))

    def test_worker_local_jump_tables(self):
        model = compile_script(fixture([0xB0, 0, 0, 0x3C, 0xB0, 0, 0, 0x3C],
                                       [worker([0], [3]), worker([4], [7], index=1)]))
        self.assertEqual(model["workers"][0]["blocks"][0]["edges"][0]["target"], 3)
        self.assertEqual(model["workers"][1]["blocks"][0]["edges"][0]["target"], 7)

    def test_jsr_preserves_worker_and_continuation(self):
        model = compile_script(fixture([0xB3, 1, 0, 0x3C, 0x34],
                                       [worker([0]), worker([4], index=1)]))
        links = entry_graph(model)["links"]
        self.assertEqual(links, [dict(source="0:0", target="1:0", pc=0, kind="subroutine")])
        self.assertEqual(model["workers"][0]["blocks"][0]["edges"][0]["target"], 3)

    def test_unknown_native_call_does_not_inherit_literals(self):
        code = [0xAE, 23, 0, 0xB5, 255, 255, 0x25,
                0xAE, 0, 0, 0xAE, 5, 0, 0xB5, 100, 0, 0x25, 0x3C]
        model = compile_script(fixture(code, [worker([0])]))
        calls = [s for s in model["workers"][0]["blocks"][0]["statements"] if s["kind"] == "call"]
        self.assertFalse(calls[0]["signature_known"])
        self.assertEqual(calls[1]["args"], ["0", "5"])

    def test_native_property_location_not_low_nibble(self):
        d = declaration(0, 4, location=6, fmt=5)
        self.assertEqual(binding(worker([0], decls=[d]), 0), "L6:map:000204:T5")
        # The native mask is (low >> 25) & 7. A low-nibble enum would yield 12.
        self.assertEqual((d["raw_low"] >> 25) & 7, 6)
        self.assertEqual((d["raw_low"] >> 24) & 15, 12)

    def test_bad_jump_does_not_become_a_solution(self):
        with self.assertRaises(ValueError):
            compile_script(fixture([0xB0, 1, 0, 0x3C], [worker([0], [3])]))


class AllTrialsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if MANIFEST is None:
            raise ValueError("Pass the generated manifest.json path")
        cls.manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
        cls.events = {s["script"]: s for s in json.loads((EXTRACTION / "maps_v3.json").read_text(encoding="utf-8"))["events"]}

    def test_six_complete_design_assets(self):
        self.assertEqual(len(self.manifest["trials"]), 6)
        self.assertEqual({t["name"] for t in self.manifest["trials"]}, {t["name"] for t in TRIALS})
        for trial in self.manifest["trials"]:
            with self.subTest(trial=trial["name"]):
                design = json.loads((MANIFEST.parent / trial["design_file"]).read_text(encoding="utf-8"))
                model = json.loads((MANIFEST.parent / trial["flow_file"]).read_text(encoding="utf-8"))
                self.assertTrue(design["normal_route"] and design["treasure_route"] and design["reset_policy"])
                self.assertTrue(design["completion_witnesses"] and design["treasure_witnesses"] and design["treasure_flag_witnesses"])
                original = self.events[trial["script"]]
                decoded = instructions(bytes.fromhex(original["code"]))
                for w in model["workers"]:
                    for b in w["blocks"]:
                        self.assertIn(b["start"], decoded)
                        for edge in b["edges"]:
                            self.assertIn(edge["target"], decoded)
                        for s in b["statements"]:
                            self.assertIn(s["pc"], decoded)
                            op, arg, _ = decoded[s["pc"]]
                            if s["kind"] == "call":
                                self.assertIn(op, (0xB5, 0xD8))
                                self.assertEqual(arg, s["id"])
                            if s["kind"] == "write":
                                self.assertIn(op, (0xA0, 0xA1, 0xA3, 0xA4))
                                self.assertEqual(arg, s["variable"])
                nodes = {n["id"] for n in design["entry_graph"]["nodes"]}
                for link in design["entry_graph"]["links"]:
                    self.assertIn(link["source"], nodes)
                    self.assertIn(link["target"], nodes)

    def test_runtime_fingerprints_and_aliases(self):
        text = TextEvidence(EXTRACTION / "text")
        for name, descriptor in self.manifest["scripts"].items():
            with self.subTest(script=name):
                event = enrich(self.events[name], EXTRACTION, text)
                self.assertEqual(fnv32(bytes.fromhex(event["code"])), descriptor["code_fnv32"])
                self.assertEqual(event["sha256"], descriptor["sha256"])
                for field in descriptor["fields"]:
                    d = event["workers"][field["worker"]]["declarations"][field["variable"]]
                    self.assertEqual((field["low"], field["high"]), (d["raw_low"], d["raw_high"]))
                    self.assertIn((field["low"] >> 25) & 7, (0, 6))
                    self.assertGreater(field["high"] & 0xFFFF, 0)
                    self.assertLessEqual(field["high"] & 0xFFFF, 1024)

    def test_bevelle_and_zanarkand_special_cases(self):
        scripts = self.manifest["scripts"]
        self.assertIn("dome0200", scripts)
        roles = {f["role"]: f for f in scripts["bvyt1100"]["fields"]}
        self.assertIn("pedestal_slot_1", roles)
        self.assertIn("pedestal_slot_2", roles)
        self.assertEqual(roles["sphere_matrix"]["high"] & 0xFFFF, 72)
        masks = [t["treasure"]["mask"] for t in TRIALS]
        self.assertEqual(masks, [1, 2, 4, 8, 16, 32])

    def test_host_never_substitutes_a_live_walkthrough(self):
        source = script_source(self.manifest)
        self.assertNotIn("/* CLOISTER_MANIFEST */ {}", source)
        self.assertNotIn("setInterval", source)
        self.assertNotIn("Tolk_", source)
        text = describe_state(None, self.manifest)
        self.assertIn("No current trial snapshot", text)


if __name__ == "__main__":
    unittest.main(verbosity=1)
