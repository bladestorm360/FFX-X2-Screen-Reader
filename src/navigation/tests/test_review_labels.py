"""Evidence regressions for objects/events and the six authored trial routes."""
import sys
import unittest
from pathlib import Path

SRC = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(SRC / "navigation"), str(SRC / "puzzles")]
from identity_labels import object_identity, conversation_identity, model_identity
from finish_targets import TRIAL_LABELS
from puzzle_routes import ROUTES


class ReviewLabels(unittest.TestCase):
    def test_confirmed_party_costumes(self):
        self.assertEqual(model_identity(0x29, "RIKK_A", ""), "Rikku")
        self.assertEqual(model_identity(0x2b, "YUNA_B", ""), "Yuna")
        self.assertEqual(model_identity(0x65, "TIDU_H", ""), "Tidus")
        self.assertNotEqual(model_identity(0x500b, "RIKK_A", ""), "Rikku")

    def test_collectible_and_readable_books(self):
        self.assertEqual(object_identity([], [{"call": 0x10d}], {0x500b})[0], "Primer")
        self.assertEqual(object_identity(["A history book"], [], {0x500b})[0], "")
        self.assertEqual(object_identity(["Obtained Al Bhed Primer XIV"], [], {0x2001})[0], "")

    def test_tutorial_sprite_requires_its_own_menu(self):
        self.assertEqual(object_identity([], [{"call": 0x401d}], {0x5010})[0], "Tutorial terminal")
        self.assertEqual(object_identity([], [], {0x5010})[0], "")

    def test_conversations_use_speakers(self):
        self.assertEqual(conversation_identity(["<control:13:30>\nHello", "<control:13:32>\nHello"]), "Tidus talks to Auron")
        self.assertEqual(conversation_identity(["<control:13:36>\nTidus and Auron are nearby"]), "Conversation with Rikku")
        self.assertEqual(conversation_identity(["Tidus talks to Auron later"]), "")

    def test_every_step_uses_an_authored_path_target(self):
        scripts = {"besaid": "bsyt0100", "kilika": "klyt1000", "djose": "djyt0600", "macalania": "mcyt0500", "bevelle": "bvyt1100", "zanarkand": "dome0700"}
        for trial, steps in ROUTES.items():
            labels = dict(TRIAL_LABELS[scripts[trial]])
            if trial == "zanarkand": labels.update({w: f"Floor puzzle panel {w-55}" for w in range(56,85)})
            for text, worker, *_ in steps:
                self.assertIn(worker, labels, f"{trial}: {text}")

    def test_corrected_sphere_destinations(self):
        def target(trial, instruction):
            return next(s[1] for s in ROUTES[trial] if instruction in s[0])
        self.assertEqual(target("kilika", "north wall recess"),31)
        self.assertEqual(target("kilika", "now-empty lower Kilika"),35)
        self.assertEqual(target("macalania", "movable pedestal on the lower floor"),1)
        self.assertEqual(target("macalania", "lower bridge support recess"),43)
        self.assertEqual(target("macalania", "empty lower treasure recess"),40)
        self.assertEqual(target("bevelle", "former Glyph Sphere source"),34)
        self.assertNotIn(20, [s[1] for s in ROUTES["macalania"]])
        self.assertEqual(target("djose", "central lift"),63)
        self.assertEqual(target("zanarkand", "previously held the Kilika Sphere"),49)

    def test_macalania_worker_roles_and_routes(self):
        labels = TRIAL_LABELS["mcyt0500"]
        self.assertEqual(labels[37], "Upper Macalania Sphere recess")
        self.assertEqual(labels[38], "Ramp Macalania Sphere recess")
        upper = [step for step in ROUTES["macalania"] if "upper Macalania Sphere recess" in step[0] or "upper recess" in step[0]]
        ramp = [step for step in ROUTES["macalania"] if "ramp Macalania Sphere recess" in step[0]]
        self.assertTrue(upper)
        self.assertTrue(ramp)
        self.assertTrue(all(step[1] == 37 for step in upper))
        self.assertTrue(all(step[1] == 38 for step in ramp))
        self.assertEqual(ROUTES["macalania"][7][1], 37)  # step 8: upper sphere pickup
        self.assertEqual(ROUTES["macalania"][9][1], 38)  # step 10: ramp recess removal

        guide = (SRC / "native" / "puzzle_guide_data.hpp").read_text(encoding="utf8")
        macalania = guide.split('{"mcyt0500", {', 1)[1].split('\n    }},', 1)[0]
        upper_rows = [line for line in macalania.splitlines() if "Upper Macalania Sphere recess" in line]
        ramp_rows = [line for line in macalania.splitlines() if "Ramp Macalania Sphere recess" in line]
        self.assertEqual(len(upper_rows), len(upper))
        self.assertEqual(len(ramp_rows), len(ramp))
        for line in upper_rows + ramp_rows:
            if "Upper Macalania Sphere recess" in line:
                self.assertIn(", 37, {0.0, 40.0, 110.0}", line)
                self.assertIn('L"Upper Macalania Sphere recess"', line)
            if "Ramp Macalania Sphere recess" in line:
                self.assertIn(", 38, {-88.0, 40.0, 62.0}", line)
                self.assertIn('L"Ramp Macalania Sphere recess"', line)


if __name__ == "__main__":
    unittest.main(verbosity=0)
