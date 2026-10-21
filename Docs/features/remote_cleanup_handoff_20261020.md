# GitHub retained-object cleanup handoff

## October21 replacement completed

Status: complete for the reviewed local content/history and canonical replacement
repository. The user manually deleted old private ID1191559099 after the CLI
DELETE returned403. Authenticated owner/name404 was verified before creation of
new private ID1428963798 at the same URL. Clean master/three tags were restored;
all32 known removed objects (23 blobs, seven commits and two tag objects) return
authenticated404. Only then was the replacement made public and verified1.0
published. Anonymous access to the known source blob also returns404. No claim
about physical GitHub backup erasure is made.

The user explicitly chose to leave permissions as they are on the replacement.
No invitations to former collaborators were sent. External recovery and receipts
remain in `Analysis/FFX/RepositoryCleanup_20261021`, including
`manual-deletion-verified.json`, `replacement-verification.json` and the verified
bundle/working-copy archive. [Final release evidence](../release.md) records
the public release, unchanged tagged payload and later GitHub README formatting.

Everything below describes the superseded blocker and retained recovery plan.

## Historical October21 replacement plan, before manual deletion

The user rejects retaining an affected private archive and explicitly authorizes
deleting/recreating the old repository if needed, following BDFFHD's precedent.
The BDFFHD record confirms manual deletion by the owner, followed by authenticated
404/new repository ID/restored clean history/known-object404 checks. It does not
provide a supported individual GitHub object-delete operation.

The exact FFX target remains private repository ID1191559099,
`bladestorm360/FFX-X2-Screen-Reader`, with clean old master734bd42 and the two
clean old tags. Issues, pull requests, forks, stars, releases, hooks, deploy keys,
Actions secrets/variables and environments are zero. The owner and two existing
write collaborators were recorded outside Git for access recovery. Local current
master5f395cd and annotatedv1.0/source625af61 are preserved, with a clean worktree.

External recovery is verified at
`Analysis/FFX/RepositoryCleanup_20261021`: the clean-history bundle passes
`git bundle verify`; the tracked working-copy ZIP passes7-Zip integrity. Recovery
hashes/refs, remote identity/settings/counts and collaborator roles are recorded.
The verified1.0 ZIP and installed payload remain unchanged.

The authorized DELETE attempt returned HTTP403, "Must have admin rights to
Repository." No account/token permissions were changed and no browser workaround
was attempted. The user explicitly says they will delete it manually. Waiting
for that action; do not recreate against an existing repository or change its
visibility. After confirmed authenticated owner/name404, create a fresh private
repository at the same canonical URL, restore only clean master/three tags,
verify its new identity and every known removed old object/commit is inaccessible,
then make it public and publish the hash-verified1.0 draft release. No claim of
physical GitHub backup erasure is implied by those checks.

The earlier support draft and old cleanup facts below remain historical evidence.

Historical pre-replacement status: **in progress (~95%)**. Current local files,
all local Git objects, and published master/two tag histories are sanitized.
A metadata-only API check still retrieves one known removed decompiled-body
blob. Full host-side removal is not complete and is not implied by a force push.

## Verified facts

- Repository: `bladestorm360/FFX-X2-Screen-Reader`, private; zero forks and zero pull requests
  returned by the all-states query. No uploaded GitHub release assets exist.
- Current master: `734bd42a6c200eff8cd8c3a495bedf9845f9ab2d`.
- Current annotated0.1.1-ShotgunBuild tag: `2ed8c50ca50609a307ac3595f301cbc60763ec65`.
- Current annotated0.1.2-ShotgunBuild tag: `18db063e45a9bbb3c179c0c576166bca7cbe1b2c`.
- Known retained old blob: `f00dfd426cbdcc02855379a4a8fbce5b868e044f`, historical
  `Docs/gameArchitecture.md`, size48985 bytes. The preserved external original
  contains a decompiled game function. Only identity/size were requested remotely;
  no source was printed or copied back into Git.
- External receipts: `Analysis/FFX/RepositoryCleanup_20261020/history-plan.json`,
  `remote-verification.json`, `remote-old-object-check.json` and
  `remote-repository-metadata.json`. The plan preserves all seven old/new commit
  mappings; the original whole repository archive stays outside every Git tree.

## Prepared request for the repository owner

The private repository identified above previously contained third-party game
implementation fragments in historical documentation. The owner authorized
sanitizing all seven commits and atomically replacing master and both old tags.
Those histories are now clean, and the remote has no forks, pull requests or
uploaded release assets. The known old blob identified above remains retrievable
through the Git blob API. Please advise whether GitHub can purge unreachable
historical objects/cached views for this repository, or what supported host-side
removal route is available. Current mod work and its source must be preserved.
This is third-party source cleanup, not a credential exposure report.

## Service limit and next step

GitHub's [documented removal process](https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/removing-sensitive-data-from-a-repository)
separates history rewriting from server garbage collection/cache removal. Its
support policy says non-sensitive data is not ordinarily eligible. Therefore
support eligibility and removal are unconfirmed; this draft does not promise a
purge. No ticket/message was sent and no repository deletion was performed.
Sending this prepared request requires explicit user authorization. Do not
invent an object-delete API, repeatedly rewrite unchanged refs or delete/recreate
the repository as an implicit extension of the three-ref approval.

Keep this blocker visible in project memory and the content/status register.
After a supported host-side action, recheck the exact old blob metadata and
remote refs before calling complete removal verified. Preserve the local mod
work, attributed evidence and external history archive throughout.
