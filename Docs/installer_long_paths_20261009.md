# Windows installer long-path repair — 2026-10-09

Sol investigated, implemented, reviewed and tested alone. The reported defect
was reproduced by a synthetic Steam/game directory longer than MAX_PATH. The
installer previously used legacy .NET path handling and ordinary IO calls; a
valid nested game/backup path therefore failed during installation.

`InstallerRuntime.cs` opts into modern .NET Framework 4.6.2 path handling before
any IO; the asInvoker manifest declares longPathAware. `InstallerPaths.Full`
first removes an explicit native prefix, then normalizes a plain canonical
path. Native IO adds the appropriate extended local or UNC prefix. Path
traversal, containment/reparse refusal and case-insensitive identity checks are
preserved. Display/reports/registry filter paths stay ordinary and readable.

`InstallerFileSystem.cs` centralizes file/directory IO. Discovery, stock hashing,
payload staging, migration backups/removal, rollback and optional repair backup
IO use it. Raw streams/FileInfo/XML serialization also receive native paths;
XML reads use streams, avoiding URI interpretation of extended paths. Normal
Install/Check still do not edit registry values, including LongPathsEnabled.
Agent tests use only the existing in-memory registry backend.

The final suite passes 558 synthetic assertions. It covers a Unicode/ampersand
game path over 350 characters and nested backup paths over 580 characters, all 13
payload checksums, discovery dedup including explicit native prefixes, native
and ordinary local/UNC normalization, repeated updates, unique backups,
settings/XML retention, nonstock refusal, contained traversal/reparse guards and
rollback after an injected late failure. Actual compiled embedded-resource
installers pass repeated installs at short and long fixture paths. The actual
13-file tester payload is separately built/tested in the standalone EXE using a
synthetic stock catalog; its native game code is never executed by these checks.

[.NET path handling](https://github.com/microsoft/dotnet/blob/main/Documentation/compatibility/long-path-support.md)
and [Microsoft normalization guidance](https://learn.microsoft.com/en-us/dotnet/framework/migration-guide/mitigation-path-normalization)
support the opt-in behavior. This removes the legacy 260-character constraint;
filesystem component and extended-path limits still exist.

Build/test logs and the original installer backup are retained in
`install/status-wakka-followup-20261009/`. The corrected reviewable EXE is
`install/windows-installer/InstallFFXScreenReader.exe`; final SHA256 and delivery
details are in `verification.json`. The existing 0.5.1-Test Build ZIP was not
refreshed. Live accessible UI and tester-install acceptance remain user-side.
The first final real-payload rerun refused its populated reused fixture as
designed; a fresh uniquely named fixture passes. Original proof folders were
preserved, with no recursive deletion.

Final delivery: both tester/developer native builds pass; a fresh final proxy
passes the offline forwarding fixture. Guarded deployment validates closed
processes, compiled FFX/Will profiles, launcher bootstrap and all 13 installed
hashes. The separately verified tester EXE is `install/windows-installer/InstallFFXScreenReader.exe`, SHA256 `e31263e2c30ab5a10df4d61a805951bbe1eb77d7b109e63dddb6eb030c08518f`.
The existing release ZIP is unchanged; gameplay acceptance remains user-side.
