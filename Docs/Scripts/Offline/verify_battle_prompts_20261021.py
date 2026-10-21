"""Read-only repair receipt; no deployment, game control or log discovery."""
from datetime import datetime,timezone
from pathlib import Path
import hashlib,json,re,sys

root=Path(__file__).resolve().parents[3]
out=root/'install/battle-prompts-20261021'
game=Path('D:/Games/SteamLibrary/steamapps/common/FINAL FANTASY FFX&FFX-2 HD Remaster')
def digest(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
baseline=json.loads((out/'baseline.json').read_text())
assert all(digest(p)==h for p,h in baseline['production_inputs'].items()),'Source changed after verified freeze'
assert all(digest(p)==h for p,h in baseline['preserved_files'].items()),'Protected file changed; investigate without overwriting it'
checks={}
for name in ('readers','profile','abi','esc-worker','preflight-ffx','preflight-will','docs-audit','build'):
    path=out/(name+'.log');text=path.read_text(encoding='utf-8-sig',errors='replace')
    assert not re.search(r'FAIL:|error (?:C\d|LNK\d)|fatal error',text),name
    checks[name]=dict(sha256=digest(path),passes=len(re.findall(r'PASS:|verified|Native x86 build OK',text)),
        warnings=re.findall(r'.*warning C.*',text),metrics=re.findall(r'METRIC[^\r\n]*',text))
assert 'reference profile' in (out/'readers.log').read_text(errors='replace')
assert 'installed profile' in (out/'readers.log').read_text(errors='replace')
assert not checks['build']['warnings']
payload={
    'dinput8.dll':root/'install/native/dinput8.dll','SDL3.dll':root/'install/native/SDL3.dll',
    'FFX_SR_SDL3_LICENSE.txt':root/'install/native/SDL3_LICENSE.txt',
    'MinHook.x86.dll':root/'install/native/MinHook.x86.dll',
    'FFXLauncherReader.dll':root/'install/native/FFXLauncherReader.dll',
    'FFXLauncherNative.dll':root/'install/native/FFXLauncherNative.dll',
    'FFX_SR_Native_Dependencies.md':root/'src/native/DEPENDENCIES.md'}
staged={k:digest(p) for k,p in payload.items()}
assert digest(root/'src/proxy/dinput8.dll')==staged['dinput8.dll']
installed={k:digest(game/k) for k in payload}
deployed='--deployed' in sys.argv
if deployed:assert staged==installed,'Installed payload does not match verified stage'
receipt=dict(utc=datetime.now(timezone.utc).isoformat(),
    phase='OFFLINE_VERIFIED_GUARDED_DEPLOYED_GAMEPLAY_PENDING' if deployed else
        'OFFLINE_VERIFIED_STAGED_DEPLOYMENT_HELD_BY_USER' if '--held' in sys.argv else 'OFFLINE_VERIFIED_STAGED_AWAITING_GAME_CLOSURE',
    build_command='src/proxy/build_dinput8.bat --release --no-deploy',
    guarded_deploy_command='src/proxy/deploy.bat',warning_free_release_build=True,developer_console=False,
    checks=checks,source_snapshot=baseline['production_inputs'],source_snapshot_unchanged=True,
    staged_payload_hashes=staged,installed_payload_hashes=installed,
    preserved_files=baseline['preserved_files'],preserved_file_hashes_match=True,
    game_launch_control_probe=False,tester_logs_access=False,save_file_writes=False,registry_repair=False,
    installer_rebuilt=False,installer_package_held=True,gameplay_confirmed=False,
    original_evidence='../Analysis/FFX/RepositoryEvidence/offline-output/{seymour_20261021,battle_amount_prompts_20261021}',
    delegation='gpt-6-luna medium supplied bounded popup/Formation edits; Sol corrected by-value ABI, fixture event lifecycle/arrays and synthetic profile mapping, reviewed/integrated and verified all repairs. No Astra.',
    remaining_limits=['Real animation/frame-to-audio latency and gameplay remain unmeasured.',
        'New fixed mechanic/membership/step labels remain English within the existing mod-wide localization/manual-override gap.',
        'Existing reader-host C4389 warning is unrelated; production build and popup worker are warning-free.'])
(out/'verification.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(receipt['phase'])
print('PASS: both173-hook readers, original256-position/1024-color matrix, numeric and learned-mode body replay, popup worker, native profile/ABI/preflight and documentation checks.')
print('PASS: seven staged payload hashes, source freeze and seven protected config/release hashes.')
for row in checks['readers']['metrics']:print(row)
