# Windows Minotaur Rebalance 1.21: verified ZIP update

## Result

[Successful Windows run 42215979271](https://github.com/Plane-PPP/openyamm/actions/runs/42215979271),
run 28, attempt 1, completed 2026-11-06 12:59:33 UTC.

- Executable source: `c9949bbc7301240adda39e1503ff2985729d470b`.
- Official v1.21: `c24a0aaa6bb6f7956c84beed1bd44c88e62453c4`.
- New version: `minotaur-1.21-c9949bbc7301-28-1`.
- Exact supported base: `minotaur-1.20-c80ebaa9634d-25-1`.
- [Download the Windows ZIP](https://github.com/Plane-PPP/openyamm/releases/download/minotaur-1.21-c9949bbc7301-28-1/Minotaur-Update-minotaur-1.21-c9949bbc7301-28-1.zip).
- Filename: `Minotaur-Update-minotaur-1.21-c9949bbc7301-28-1.zip`.
- Size: **1,932,865,419 bytes**, about **1.80 GiB**.
- SHA-256: `8292edc36394fa15a7783bdb51a5ec66690f926fe08efdaaa89da04c40f9c723`.
- [Permanent GitHub Release](https://github.com/Plane-PPP/openyamm/releases/tag/minotaur-1.21-c9949bbc7301-28-1), published 2026-11-07 03:43:29 UTC.
- Release assets have no Actions retention expiry. The original Actions copy expires on 2026-11-13.
- [Publication workflow](https://github.com/Plane-PPP/openyamm/actions/runs/42335135110) succeeded.

The ZIP includes 53 changed packs (1,909,044,068 bytes), 123 program/dependency files, installer and documentation.
It reuses 113 unchanged packs, **3,959,503,161 bytes / 3.69 GiB**, from the installed base. The substantial delta is
mainly the official MM6/MM7/MM8 model and icon updates; unchanged heavy packs are not shipped again.

[Baseline and size/checksum metadata](https://github.com/Plane-PPP/openyamm/actions/runs/42215979271/artifacts/13503977226)
and [runner measurements](https://github.com/Plane-PPP/openyamm/actions/runs/42215979271/artifacts/13502979297)
are retained for 90 days. The next baseline was copied from this actual successful artifact, not reconstructed.

## Tests and package checks

| Set | Cases passed | Assertions |
|---|---:|---:|
| Gameplay / Minotaur / saves / inventory / UI | 258 | 49,851 |
| Selected native gameplay runtime | 7 | 163 |
| Updater engine | 15 | 145 on Windows; 146 on Linux |
| AI / paths / bolster / character sheet | 147 | 2,204 |
| Indoor detection / flying / faction | 18 | 1,097 |

Sets overlap and are not a count of unique full-suite coverage. Both platforms passed. Platform-conditional updater
checks account for the assertion-count difference. The complete unit suite was not run.
Windows PowerShell 5.1 passed all 17 packaging/installer tests, package-validator regression and 3 monitor tests.

CI verifies indexed content, the thin program package and payload hashes before building the delta.
The GitHub artifact digest matches the CI report's complete ZIP SHA-256. Metadata/resource archive digests were
independently matched after downloading. An HTTP Range inspection of the real ZIP checked its complete central
directory (184 entries), manifest, base/source/target, installer CRCs and payload names/sizes against the manifest.
Full duplicate download and local rehash of all 1.8 GiB did **not** finish: both HTTP/2 and HTTP/1.1 were abnormally
slow on this VM; the latter ended at a bounded 180-second network timeout. This was not a memory guard/OOM.
No claim of a complete local payload rehash is made. The downloadable artifact and CI verification succeeded.

## Permanent publication

The publication runner downloaded the **complete original ZIP**, checked its size and full SHA-256, uploaded it
unchanged to a draft, verified all four release asset sizes/digests, and only then published the release.
The release tag points to executable source `c9949bbc7301240adda39e1503ff2985729d470b`.
The ZIP, `SHA256SUMS.txt`, update report and baseline are persistent Release attachments.
The mod was not rebuilt. Medusa's release was not edited. Publication requests `make_latest=false`;
GitHub's latest-stable lookup can still select Minotaur when the other release is a prerelease.
The first publication attempt left a draft because GitHub's by-tag REST lookup excludes drafts; ID-based verification
completed the same draft successfully in the workflow linked above.

## Resources

Windows runner: 4 vCPU, 15.995 GiB RAM, adaptive parallelism selected 4. Ubuntu limits were not copied to it.
The Windows measurements sample the **whole runner**, not compiler RSS.

| Phase | Duration | Peak used RAM | Mean CPU | Peak CPU |
|---|---:|---:|---:|---:|
| Program, shaders and cooker | 15m 56.8s | 7.826 GiB | 92.13% | 100% |
| Unit/runtime test compilation | 14m 04.9s | 8.244 GiB | 98.73% | 100% |
| Content packaging | 2m 39.6s | 3.071 GiB | 18.80% | 29.30% |
| Delta ZIP | 8.0s | 3.985 GiB | 27.62% | 33.20% |

Windows job: **45m 48s**, including checkout/setup/packaging. Minimum observed available memory: **7.751 GiB**.
Every monitored phase returned 0 / completed; no guard stop. Windows disk swap, Linux PSI and cgroup events are
unavailable in this monitor and are not reported as zero. [Detailed measurements](openyamm-1.21-windows-resources.csv).

Ubuntu main incremental port build: 9m 25.7s, **4.682 GiB cgroup peak**, mean **3.761 CPU cores** (about 47% of 8 vCPU),
observed swap and memory PSI 0, no OOM. Subsequent focused rebuilds and tests also used `safe-heavy-run`, jobs 4,
with the existing 16/18 GiB and 1 GiB swap limits. No Claude process or system setting was changed.
[Permanent local-operation statistics](openyamm-1.21-local-resources.csv).
These differing scopes/cache states are not a cold-build performance comparison.

## Installation and rollback

1. Close the game. Retain an external copy of `saves` and `settings.ini`.
2. Check that `app/current` names the exact supported base above.
3. Extract the ZIP contents into the game root. Open its `Minotaur-Update-...` folder and run
   `Apply-MinotaurUpdate.cmd`. The installer also supports extraction directly into the root.
4. If directory detection is uncertain, run it with the actual installation path:
   `Apply-MinotaurUpdate.cmd -InstallationRoot "D:\Games\OpenYAMM"`.
   To validate without installing, append `-CheckOnly`.
5. Wait for the success message and launch the ordinary root `OpenYAMM.exe`.

The installer verifies the base and hashes before switching. It copies saves/settings to
`backups/minotaur/<previous-version>-<unique-id>` and does not overwrite the originals.
Previous app/packs and the rollback pointer stay after a healthy start; the root launcher is still refreshed.
An "already installed" message means the active version equals this target and its payload verified.

Load a copied old save, check party/class/skills/spells/equipment/stacks/quests/location, save to a **new slot**, close,
restart and reload. New mod schema 155 is not promised readable by the old 143 executable.
To roll back: close the game, set `app/current` to the version in `app/previous`, restore the pre-update saves/settings.
The official numeric app updater cannot replace the offline `minotaur-*` namespace. Continue using mod ZIP patches.

## Scope and commits

- `8ee68e726b190564e9f33aaeb948497a34165463`: normal v1.21 merge and compatibility integration.
- `c9949bbc7301240adda39e1503ff2985729d470b`: retain rollback while refreshing launcher, broaden CI regression checks.
- The later documentation/baseline commit does not change the executable source SHA above.

The earlier run 42209078766 passed but is superseded; only the final artifact linked here is recommended.
[Integration, save mapping, conflicts and independent review](openyamm-1.21-integration.md).
Medusa was not merged, modified or built. Its concurrent commits belong to Claude.

Private MM8 campaign saves, real-GPU visuals, 4:3/16:9/ultrawide and Android UI were not manually certified.
Frozen previous-production 132/140/143 fixtures round-trip; inventory geometry/counts and brewing are automatic.
New upstream resistance supplements remain English. Bespoke teacher flavour/next-teacher dialogues,
Identify Monster redesign and bestiary work remain deferred. No new Minotaur balance mechanic was added.
