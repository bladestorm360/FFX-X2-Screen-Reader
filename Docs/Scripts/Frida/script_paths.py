"""Supply host paths to Frida scripts that run inside a different process."""

import json
from pathlib import Path
import subprocess
import sys
from tempfile import TemporaryDirectory

FRIDA = Path(__file__).resolve().parent
sys.path.insert(0, str(FRIDA.parent / "Offline"))
from evidence_paths import evidence_root, outside_git


def script_paths(path: Path) -> str:
    group = path.relative_to(FRIDA).parts[0]
    capture = outside_git(evidence_root() / "frida-output" / group)
    capture.mkdir(parents=True, exist_ok=True)
    paths = {
        "FFX_CAPTURE_DIR": capture.as_posix() + "/",
        "FFX_SCRIPT_ROOT": FRIDA.parent.as_posix() + "/",
    }
    return "".join(f"globalThis.{name} = {json.dumps(value)};\n"
                   for name, value in paths.items())


def load_script(path: Path | str) -> str:
    path = Path(path).resolve()
    source = path.read_text(encoding="utf-8-sig")
    # Append a second script at load time without changing the source's directives.
    return script_paths(path) + f"Script.evaluate({json.dumps(path.name)}, {json.dumps(source)});\n"


if __name__ == "__main__":
    script, process, output, *options = sys.argv[1:]
    capture_folder = outside_git(output).parent
    with TemporaryDirectory(prefix="frida-script-", dir=capture_folder) as staging:
        config = Path(staging) / "paths.js"
        config.write_text(script_paths(Path(script).resolve()), encoding="utf-8")
        result = subprocess.run([
            "frida", "-n", process, "-l", str(config), "-l", script, "-o", output, *options
        ])
    sys.exit(result.returncode)
