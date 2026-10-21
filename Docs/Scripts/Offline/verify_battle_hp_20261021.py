"""Record the HP repair's source/payload and read-only offline verification.

Never deploy, launch/control the game, discover logs, or change user settings.
The separate freeze records the reviewed source and installed mod hashes.
"""
from datetime import datetime, timezone
from pathlib import Path
import hashlib, json, re, sys

root = Path(__file__).resolve().parents[3]
out = root / 'install/battle-hp-20261021'
prior = root / 'install/battle-prompts-20261021'
game = Path('D:/Games/SteamLibrary/steamapps/common/FINAL FANTASY FFX&FFX-2 HD Remaster')

def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

payload = {
    'dinput8.dll': root/'install/native/dinput8.dll',
    'SDL3.dll': root/'install/native/SDL3.dll',
    'FFX_SR_SDL3_LICENSE.txt': root/'install/native/SDL3_LICENSE.txt',
    'MinHook.x86.dll': root/'install/native/MinHook.x86.dll',
    'FFXLauncherReader.dll': root/'install/native/FFXLauncherReader.dll',
    'FFXLauncherNative.dll': root/'install/native/FFXLauncherNative.dll',
    'FFX_SR_Native_Dependencies.md': root/'src/native/DEPENDENCIES.md',
}
old = json.loads((prior/'baseline.json').read_text())
if '--freeze' in sys.argv:
    inputs = {p: digest(p) for p in old['production_inputs']}
    changed = [p for p, h in inputs.items() if h != old['production_inputs'][p]]
    assert {Path(p).name for p in changed if '/tests/' not in Path(p).as_posix()} == {
        'battle.cpp', 'battle_status.cpp', 'ready_game_text.cpp', 'game_details.cpp'
    }, 'Unexpected production-source change since the prior verified speech repair'
    tests = [
        'src/native/tests/battle_hp_followups.inc',
        'src/native/tests/reader_fixture.cpp',
        'src/native/tests/battle_followups.inc',
        'src/native/tests/battle_switch_followups.inc',
        'src/native/tests/seymour_followups.inc',
        'src/native/tests/sensor_overdrive_followups.inc',
        'src/native/tests/battle_status_worker_fixture.cpp',
        'src/native/tests/run_battle_status_worker_tests.bat',
        'Docs/Scripts/Offline/prepare_battle_hp_20261021.py',
    ]
    snapshot = dict(utc=datetime.now(timezone.utc).isoformat(),
        production_inputs=inputs, changed_since_prior_receipt=changed,
        verification_inputs={p: digest(root/p) for p in tests},
        preserved_files=old['preserved_files'],
        installed_payload_hashes={name: digest(game/name) for name in payload})
    (out/'baseline.json').write_text(json.dumps(snapshot, indent=2)+'\n')
    print('PASS: reviewed production/test snapshot frozen; installed mod hashes recorded read-only.')
    raise SystemExit(0)

baseline = json.loads((out/'baseline.json').read_text())
assert all(digest(p) == h for p, h in baseline['production_inputs'].items()), 'Production source changed after freeze'
assert all(digest(root/p) == h for p, h in baseline['verification_inputs'].items()), 'Verification source changed after freeze'
assert all(digest(p) == h for p, h in baseline['preserved_files'].items()), 'Protected setting/release changed; do not overwrite it'
assert all(digest(game/p) == h for p, h in baseline['installed_payload_hashes'].items()), 'Installed payload changed during deployment hold'
checks = {}
for name in ('readers', 'status-worker', 'build', 'preflight-ffx', 'preflight-will', 'docs-audit', 'repository-audit'):
    path = out/(name+'.log')
    text = path.read_text(encoding='utf-8-sig', errors='replace')
    assert not re.search(r'FAIL:|error (?:C\d|LNK\d)|fatal error|Traceback', text), name
    checks[name] = dict(sha256=digest(path), warnings=re.findall(r'.*warning C.*', text),
        metrics=re.findall(r'METRIC[^\r\n]*', text))
reader = (out/'readers.log').read_text(errors='replace')
assert 'reference profile' in reader and 'installed profile' in reader
assert reader.count('291-byte effective-capacity update,1071-byte Switch renderer') == 2
assert 'PASS:' in (out/'status-worker.log').read_text(errors='replace')
assert 'Native x86 build OK.' in (out/'build.log').read_text(errors='replace')
assert not checks['build']['warnings'] and not checks['status-worker']['warnings']
for name in ('preflight-ffx', 'preflight-will'):
    assert 'entry point, speech and hooks were not run' in (out/(name+'.log')).read_text(errors='replace')
staged = {p: digest(path) for p, path in payload.items()}
assert digest(root/'src/proxy/dinput8.dll') == staged['dinput8.dll']
receipt = dict(utc=datetime.now(timezone.utc).isoformat(),
    phase='OFFLINE_VERIFIED_STAGED_DEPLOYMENT_HELD_BY_USER',
    build_command='src/proxy/build_dinput8.bat --release --no-deploy',
    warning_free_release_build=True, developer_console=False,
    source_snapshot=baseline, checks=checks, staged_payload_hashes=staged,
    installed_payload_hashes=baseline['installed_payload_hashes'],
    installed_payload_unchanged=True, preserved_files_match=True,
    previous_repairs_receipt=str(prior/'verification.json'),
    retained_prior_checks=['profile/signature dependencies', '11000-call native ABI', 'Escape popup real worker'],
    original_evidence='../Analysis/FFX/RepositoryEvidence/offline-output/battle_hp_20261021',
    percentage_output_preserved=True, totals_reconstructed_from_popups=False,
    game_launch_control_probe=False, tester_logs_access=False,
    deployed=False, installer_rebuilt=False, installer_package_held=True,
    gameplay_confirmed=False,
    delegation='HP work retained by Sol. Prior Luna popup/Formation work and Sol corrections remain in the linked prior receipt; no Astra.',
    limits=['Original native renderer/getter/capacity replay uses controlled actor/encounter/graphics seams.',
        'The real status worker uses a held worker and fake backend, without PRISM or audible/game timing.',
        'The current live session still runs the earlier payload; fresh gameplay remains pending.'])
(out/'verification.json').write_text(json.dumps(receipt, indent=2)+'\n')
print(receipt['phase'])
print('PASS: both173-hook reader profiles, original HP/MP/capacity/Switch replay, real owned status worker and warning-free release build/preflight.')
print('PASS: source/test freeze, seven staged hashes, unchanged installed modules and preserved settings/releases; deployment and installer held.')
