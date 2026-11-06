[Русский](README.md) | **English**

# Minotaur Rebalance documentation

Everything in `docs/`, plus `CHANGELOG.md` and `BUILD-NOTES.md`, belongs to the fork. OpenYAMM's own documentation lives in
`devdocs/`, `level_generation/`, `packaging/` and `README.md`; it is maintained upstream and is not translated here.

## Where to start

| Document | Purpose | Language |
|---|---|---|
| [Windows 1.21 update](research/windows-1.21-build.md) | ZIP, base version, save checks, resources and rollback | EN |
| [ROADMAP](ROADMAP.en.md) | Current state, plans, history | [RU](ROADMAP.md), EN |
| [Mod description](MINOTAUR_REBALANCE.en.md) | Mechanics, formulas, diagnostics, testing | [RU](MINOTAUR_REBALANCE.md), EN |
| [Lethal Strike v0.4](design/death-strike.en.md) | The current skill contract | [RU](design/death-strike.md), EN |
| [Lethal Strike and protected targets](design/lethal-strike-protection-task.en.md) | Deferred / Low Priority | [RU](design/lethal-strike-protection-task.md), EN |
| [1.18 notes](minotaur-1.18.md) | Reagent stacks and expanded tooltips | EN |
| [Independent audit](research/independent-audit-2026-11-02.en.md) | Review results and recommendations | [RU](research/independent-audit-2026-11-02.md), EN |
| [Undead immunities](research/undead-condition-immunities.en.md) | Research on Liches and Vampires | [RU](research/undead-condition-immunities.md), EN |
| [MM8 window glow](research/surface-materials-mm8-window-glow.en.md) | Research on Surface Materials | [RU](research/surface-materials-mm8-window-glow.md), EN |

## Repository specifics

- **Sparse checkout.** Many documents, such as most of `docs/research/`, are tracked but absent from disk.
  Read them with `git show HEAD:docs/research/combat-balance.md`. Do not run `git sparse-checkout reapply`.
- **New files.** `docs/` is no longer ignored, so new documents appear in `git status` at once. The directory is outside the sparse
  patterns, so its files are staged with `git add --sparse <path>`. A plain `git add` refuses them.

## How languages are organized

A Russian document keeps its path so links and Git history stay intact. The English version sits next to it with the `.en.md`
suffix: `ROADMAP.md` and `ROADMAP.en.md`. The first line of both versions switches language.

## Maintenance rules

1. A change to an important document updates both versions in the same commit.
2. A new main document is created in both languages at once. Research notes and build logs may be written in one language and
   translated when needed.
3. Section structure, formulas, numbers, identifiers and links match in both versions.
4. Translation never raises a status: planned work does not become implemented, and a hypothesis does not become fact.
5. Consistent terms: Axe, Recovery, Damage Resistance (the in-game name; discussions sometimes say Damage Reduction),
   Grandmaster or GM, Lethal Strike, off-hand, Stats.
6. Before committing documentation, run `python3 tools/check_doc_i18n.py`. Review warnings about a link or code span present in
   only one version; some are expected, such as the link to a document's own language partner.

## Translation backlog

| Priority | Documents | Reason |
|---|---|---|
| 1 | `CHANGELOG.md`, `BUILD-NOTES.md` | Mixed language, updated often |
| 2 | `research/combat-balance.md`, `research/final-review.md`, `design/monster-knowledge.md`, `design/identify-monster.md` | Basis of the balance and deferred features |
| 3 | `research/windows-ci-optimization.md`, `research/upstream-fc4e117-fix-integration.md`, `research/minotaur-incremental-updates.md` | Process and infrastructure |
| 4 | `research/monster-lore.md`, `research/monster-stats.md`, `research/monster-size-classification.md`, `research/reproduce.md`, `research/lethal-strike-teacher-*.md`, `research/damage-resistance-layout-bug.md`, `patches/README.md` | Narrow or historical material |

The remaining `docs/research/` documents are already in English.
