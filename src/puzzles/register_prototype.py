"""Register the user-run prototype in the existing dynamic Frida launcher.

Only writes workspace launcher entries. It never starts Frida or the game.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DIRECTORY = ROOT / "Docs/Scripts/Frida/FFX"


def register():
    DIRECTORY.mkdir(parents=True, exist_ok=True)
    entry = DIRECTORY / "cloister_trials.js"
    host = DIRECTORY / "cloister_trials_host.py"
    entry.write_text("// @python\n// @desc All six Cloisters: event-only state validation; no speech or game writes\n"
                     "// Loaded by the Python host from src/puzzles/state_reader.js.\n"
                     "throw new Error('Run cloister_trials through run_frida.bat with its Python host');\n", encoding="utf-8")
    host.write_text("\"\"\"Workspace launcher entry; implementation lives in tracked src/puzzles.\"\"\"\n"
                    "from pathlib import Path\nimport runpy\n"
                    "runpy.run_path(str(Path(__file__).resolve().parents[4] / 'src/puzzles/validation_host.py'), run_name='__main__')\n",
                    encoding="utf-8")
    print("Registered cloister_trials in run_frida.bat's FFX menu. No probe executed.")


if __name__ == "__main__":
    register()
