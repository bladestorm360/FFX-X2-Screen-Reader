# GitHub retained-object cleanup handoff

Status: **in progress (~95%)** overall repository cleanup. Current local files,
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
