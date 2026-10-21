"""Workspace launcher entry; implementation lives in tracked src/puzzles."""
from pathlib import Path
import runpy
runpy.run_path(str(Path(__file__).resolve().parents[4] / 'src/puzzles/validation_host.py'), run_name='__main__')
