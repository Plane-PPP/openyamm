# Minotaur on OpenYAMM 1.7

Official tag `v1.7`: `069df4a3f63c2dcaeed7b70694529154e7ccc205`, released 2026-10-22.
The feature branch retains Lethal Strike, Axe/DR balance and LS-001/LS-002 fixes.
LS-003 flavour and teacher hints remain Planned; Identify Monster/bestiary remain Deferred.

## Save migration

Upstream 1.7 uses OYSAVE1/version93. The mod writes OYMINO1/version94.
The old production OYMINO1/version89 is decoded explicitly as upstream88 plus
the mod combat-level field, then receives upstream journal/autonote/history migrations.
Official OYSAVE1 through93 does not contain the mod field. Both known older v85
layouts and v84 remain supported, with exact byte consumption required.

The production 1.5 writer at8434ba7 generated minotaur-1.5-v89.oysav before merging.
Three load/save cycles verify character/skills/inventory/gold/bank/quests/reserve/visited maps.
Fixtures are synthetic; the actual user campaign save has not been supplied.
Keep the old install and saves; unpack separately, copy only saves, save in a new slot.
Do not copy old settings/assets or the obsolete hotfix overlay. New saves do not roll back.

## Packaging

1.7 requires the versioned launcher/app and indexed content packs rather than engine.zip
and per-world ZIPs. The mod verifier checks launcher/game x64 PE, complete packs/hashes
and exact embedded teacher/Stats resources. Stock auto-updates default to never, to avoid
replacing the mod with an official binary. Windows build/manual testing are pending.

## Validation during integration

25 focused tests / 30394 assertions pass, including both progress formats.
Broader regression initially found sparse missing model/map/UI assets and an upstream
recovery enchant test using a positional InventoryItem initializer predating hardened.
The latter shifts the enchant field and is not evidence of an Axe/DR balance change.
Final validation results and Windows artifact are recorded after completion.

## Final automatic checks (v0.4 + Luck)

26 focused tests /30530 assertions,3 runtime /58,223 selected regressions /7506 passed.
The broad run excludes missing sparse MM6/MM9/model cases and the pre-existing shifted
recovery-enchant test. It is not a full-suite claim. Both actual death paths and new-game
innate skill creation pass. Package validator regression and model-profile packaging tests pass.
Local stale-bake checker reports0 with sparse assets (not a full content lighting audit).
Luck is current base+permanent+magical Luck, clamped at0; each complete10 adds0.5 p.p.
to P0 before KL/cap at every rank. CSV includes Luck0/90 (11520 scenarios).
