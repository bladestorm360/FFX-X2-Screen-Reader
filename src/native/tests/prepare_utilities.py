"""Stage real archived bytecode for isolated native utility fixtures."""
import json
import hashlib
import struct
import sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(ROOT/"Docs/Scripts/Offline"))
from evidence_paths import evidence_root, evidence_relative
EVIDENCE=evidence_root()
e=EVIDENCE/"offline-output/FFX/20261003_audit"
maps=json.loads((e/"maps_v3.json").read_text(encoding="utf8"))
scripts=["bsyt0100","klyt1000","djyt0600","mcyt0500","bvyt1100","dome0700","bvyt0900","mtgz0700"]
target=EVIDENCE/"test-build"
target.mkdir(parents=True,exist_ok=True)
for script in scripts:
    event=next(r for r in maps["events"] if r["script"]==script)
    (target/(script+".bin")).write_bytes(bytes.fromhex(event["code"]))
sys.path.insert(0,str(ROOT/"Docs/Scripts/Offline"))
from cloister_inventory import script_chunk, declarations
event=next(r for r in maps["events"] if r["script"]=="cdsp0000")
source=e/"files"/event["source"]
archive=source.read_bytes()
assert hashlib.sha256(archive).hexdigest()==event["sha256"]
atel=script_chunk(archive)
header=struct.unpack_from("<I",atel,0x38)[0]
rows=declarations(atel,header)
assert any(r["raw_low"]==0x20000a00 and r["raw_high"]==1 for r in rows)
assert not any(r["raw_low"]==0x280 and r["raw_high"]==1 for r in rows)
(target/"cdsp0000_atel.bin").write_bytes(atel)
(target/"cdsp0000_provenance.json").write_text(json.dumps({
    "source":str(evidence_relative(source,EVIDENCE)),"source_sha256":event["sha256"],
    "atel_sha256":hashlib.sha256(atel).hexdigest(),"atel_bytes":len(atel),
    "worker0_header":header,"worker0_declarations":rows},indent=2)+"\n",encoding="utf8")
print("Staged eight archived bytecode fixtures and original Salvage Ship ATEL/declarations; no game access.")
