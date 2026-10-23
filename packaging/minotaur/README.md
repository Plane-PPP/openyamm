# Minotaur offline incremental releases

Player instructions: [README-RU.txt](README-RU.txt). Architecture and provenance:
[research](../../docs/research/minotaur-incremental-updates.md).

The existing versioned1.10 installation is the initial base. The application and changed
logical content packs travel in one ZIP; unchanged packs stay on the player's disk.
No update server or private signing keys are required for this manually downloaded channel.
Do not enable the stock online updater for the mod.

## CI

`Minotaur Windows build` now defaults to `package_mode=update`. It:

1. Runs Python generation tests and the actual Windows PowerShell5.1 installer tests.
2. Reads the base named by `latest-baseline.json`, or `baseline_version` input.
3. Builds an immutable app version named from upstream version, source SHA, run number and attempt.
4. Prepares upstream content packs separately; packages/verifies a small THIN app archive.
5. Produces `Minotaur-Update-<version>.zip`, its size/SHA256 report and `minotaur-baseline.json`.
6. Uploads only the incremental ZIP by default. `full` or `both` explicitly requests a full install.

`tests_only=true` runs the real Windows installer tests without compiling or packaging the game.
`previous_build_sha`, when supplied, must match the chosen base (it cannot silently select another base).
The upstream version prefix in the configure step is updated when integrating a new upstream release.

After a successful release, download the SMALL `minotaur-update-metadata-<sha>` artifact:
copy `minotaur-baseline.json` to `bases/<version>.json`, update `latest-baseline.json`, and commit
it with the build report/ROADMAP. Never replace the previous base before the new artifact succeeds.
This lets the next release use the actual published baseline after old Actions artifacts expire.
Historical base files can be selected to generate a direct update from an older supported install.

## Local generation

Use the staged versioned app plus separately prepared content, not a full5GB outer ZIP:

```sh
python3 tools/minotaur_updates.py build \
  --stage build-windows/release/Release/OpenYAMM \
  --content build-windows/minotaur-content \
  --base packaging/minotaur/bases/minotaur-1.10.json \
  --source-sha FULL_COMMIT_SHA \
  --output build-windows/minotaur-update/Minotaur-Update-VERSION.zip \
  --baseline-output build-windows/minotaur-update/minotaur-baseline.json
```

The bootstrap tool reads a small number of HTTP ranges from the old ZIP64 artifact; it refuses
servers ignoring Range rather than silently downloading the full archive. It reads only current
version, revision and index, verifies ZIP CRC and expected source SHA, and never forwards the
GitHub token to the signed blob URL. Baseline1.10 is already committed, so bootstrap is not needed
for normal builds.

## Data and failure rules

- Compatibility is an exact base app version and the index's logical id/package/mount/content_id set.
- Every reused installed pack and new payload file is checked against its size and SHA256 before switching.
- Compression-only ZIP changes reuse the installed file/size/SHA; future patches compare logical identities.
- Changed packs cannot overwrite an existing content-addressed filename with different bytes: a new name is used.
- Immutable app files stage in `update/`, then move to `app/<target>`. `app/previous` records the base;
  `app/current` switches atomically only after file verification. A valid interrupted staging can resume.
- Applying the same verified update again succeeds without resetting progress or copying files.
- `saves/`, `settings.ini`, logs and other player files are outside the payload and are not replaced.
- Root BUILD-REVISION.txt is managed release metadata; it is refreshed after activation. Release notes are in
  the patch folder. The old root launcher starts the new app; the new app updates the launcher after becoming healthy.
- The existing upstream rollback is available until the first healthy launch. After that the engine prunes previous
  app versions and unused packs. This is not a promise of backwards-compatible saves or permanent rollback.
- No manual Windows gameplay claim follows from filesystem installer tests.
