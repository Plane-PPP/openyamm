# Roguelike rules v1 on Minotaur / OpenYAMM 2.4

Status: built 2026-11-17 on `integration/minotaur-v2.4-20261115` after `473788e4`, checked headless on Linux. The task
is `docs/research/roguelike-next-prep/fable-next-version-task.md` (local, not committed); the user approved the talent
table below on 2026-11-17. What v0 was: [roguelike-rules-v0.md](roguelike-rules-v0.md). Every number in `run.yml` is a
test value.

## What changed from v0

| Area | v0 (2.4, `473788e4`) | v1 |
| --- | --- | --- |
| Offers per level-up | 3, fixed in code | `rules.levels.offers: 4` |
| Card identity | perk level at `eventVariables[0xF100 + run.yml index]` | stable ids: `skill:<Skill>`, `talent:<id>`; ranks in `Character::talentRanks` |
| Numeric perks | six stats, HP, SP, AC, resistances, melee and bow damage | removed; talents instead (below) |
| Choice | text box with buttons | card screen (approved mockup); the text box stays for `run pick` and scenarios |
| Level-up | levels and a card | also heals: a hero who is out comes round, HP = max(HP + 25 %, 25 %) of the maximum, capped |
| Camp Rest | the map's rest price (3 in Goblinwatch); hit and spell points filled by hand | `rules.camp.restFood: 2`; the start's 2 food pay for one rest; the game's own Rest and Heal on the rest screen until the next morning (hotfix) |
| Camp alchemy shop | the house's own stock | plus small stacks of MM6 herbs and bottles (`shops[].stock`) |
| Guardians | open the way out | also a reward, once each: gold and an artifact (1-2) or a relic (3-4) |

## Talents (run.yml `talents`)

Adapted from the MAW solo run's talents (`assets_dev/engine/data_tables/talents.yml`) for a party of four: all
passive, no action bar. A card offers the next rank; the requirement is checked when the card is dealt.

| Id | Needs | Ranks | Effect |
| --- | --- | --- | --- |
| `sweeping_blows` | melee weapon or Armsmaster, Expert | 3 | a melee swing also strikes 1/2/3 more in a 120° arc, 50 % damage |
| `onslaught` | — | 3 | after a kill, weapon attacks 15/25/35 % faster for 10 s |
| `shield_wall` | Shield, Expert | 3 | with a shield: −15/25/35 % damage taken, 15/25/35 % chance to take an ally's blow or arrow |
| `multishot` | Bow, Expert | 3 | every bow shot adds 2/4/6 side arrows fanned ±12°, 50 % damage |
| `piercing_arrows` | Bow, Expert | 3 | arrows fly on through 1/2/3 more monsters, 50 % damage |
| `pinning_shot` | Bow, Expert | 3 | an arrow's hit slows the target by half for 2/3/4 s |
| `explosive_arrows` | Bow | 1 | v0's Carnage arrows (kept as it was) |
| `attunement` | any school | 3 | spells +10/20/30 % damage, −10/20/30 % SP cost |
| `overload` | any school, Expert | 3 | area spells +10/20/30 % per monster near the target beyond the first, at most 3 |
| `spell_surge` | any school, Master | 3 | every 5th/4th/3rd spell is free and 50 % stronger |
| `binding_circle` | any school, Expert | 3 | when struck by a monster within 5 m: every hostile within 5 m is held 2/2.5/3 s; once in 20 s |
| `resolve` | — | 3 | +10/20/30 to all resistances; at rank 3 monsters' special attacks do not take hold |
| `bloodthirst` | — | 3 | heals 5/10/15 % of damage dealt by weapons and spells |
| `unbroken` | — | 1 | the part of one hit above 30 % of max HP is halved |
| `cheat_death` | — | 1 | once per dungeon a fatal hit leaves 10 % HP, then 3 s of −95 % damage |

Left out (they need a button or a party move): Charge, Blink, Cleave (its role is Sweeping Blows'), and the percent
passives Weapon Training, Marksman, Steady Aim, Quickfoot.

## Where it lives

- `assets_dev/engine/rulesets/roguelike/run.yml`: `levels.offers`, `levels.heal`, `camp.restFood`, `shops[].stock`,
  `rewards.guardians`, `icons` (a card picture per skill), `talents`.
- `game/content/DungeonRun.{h,cpp}`: `DungeonRunTalent` (numbers per rank: `values`, `values2`, `share`, `seconds`,
  `cooldown`, `radius`, `arc`, `targets`, `spells`), `DungeonRunShopStack`, `DungeonRunGuardianReward`.
- `game/rules/roguelike/RoguelikeRules.{h,cpp}`: deal and apply by id, `migrateLegacyRunPerks`, the level-up heal,
  `pickRunGuardianItem`, `grantRunItems`, and the rules objects the run installs over Standard's: Combat, Spells,
  Classes and Items (shop stacks). Each calls Standard first, then the talent.
- `game/rules/roguelike/RoguelikeTalents.{h,cpp}`: every talent's effect as a function of the hero's rank.
- Shared hook added: `CombatRules::characterAttackExtras` (Standard: none). `GameplayActionController` reads it on an
  ordinary attack for the sweep (extra blows before the paying blow, as MAW solo's), the side arrows and the pierce.
  MAW solo keeps its own path (`PartyAttackConfig` talent fields); nothing in MAW's code changed.
- `game/ui/GameplayRunCardPicker.{h,cpp}`, `game/ui/RunCardPresentation.{h,cpp}`, `game/events/RunCardDialog.h`: the
  card screen. `EventDialogContent::runCards` makes the dialogue renderer and input draw the screen instead of the
  choice box; confirming runs the dialog's own `EventChoice` action, so the card is given only by
  `answerDungeonRunChoice`. Esc closes it; the card stays owed and the screen returns after 20 s.
- `game/app/GameApplicationDungeonRun.cpp`: the reward (`grantDungeonRunGuardianReward`, `deliverDungeonRunItems`),
  attaching the screen, the camp's own rest price, the debug `run buy <item>` and a fuller `run` status.
- Russian texts: `assets_dev/languages/ru/text/engine-roguelike.po` (marked machine translation for review).

## State and saves

The save format is unchanged (OYMINO1/v184). New state: `run.rewarded` (last stop rewarded), `run.reward.<stop>`
(the item given there), `run.items`, `run.item.<n>` (owed items waiting for room) in the named globals;
`eventVariables[0xF0F2]` (the stop where Cheat Death last saved the hero). Timers (Onslaught, Spell Surge's count,
Binding Circle's rest, Cheat Death's cut) are not saved and start fresh on load, as MAW solo's.

Old unfinished runs (user decision: keep them): on load `migrateLegacyRunPerks` moves each `0xF100 + i` level to the
id of v0's perk `i` (`might` … `explosive_arrows`) in `talentRanks` and erases the variable, once. The amounts are
already in `permanentBonuses` and stay; the removed perks are not in v1's table, so they are never offered again and
have no effect of their own. An owed card of such a save is dealt in v1's form. Frozen fixture:
`tests/fixtures/minotaur-2.4-v184-roguelike-perks.oysav` (written by `473788e4`, see its `.txt`).

## Economy notes

- Rewards: 1500 / 3000 / 5000 gold after guardians 1-3, none after the 4th (no camp follows it); its relic is the
  trophy. The artifact or relic is of the exact rarity (`ItemDefinition::rarity`, not `ItemRuntime::isArtifact`),
  MM6's world first, never one the party carries or the run gave before, fixed by the seed and the stop.
- Rest at the camp: 2 food. The start gives 2, so the first camp's rest is paid; later rests need found food. It is
  the game's Rest and Heal (`GameplayScreenRuntime::beginRestAction`, no food taken there and no monsters-near check;
  the camp pays and sets `RestScreenState::foodCost` so the screen shows its price): the hourglass, then
  `Party::restAndHealAll` (the unconscious come round; the dead, petrified and eradicated stay). It lasts until 9:00
  the next morning, at least 8 hours, so the shops are open after it (MM6's magic shops keep 8-16). The camp's menu
  waits while the rest screen is open. The first v1 version only filled hit and spell points of heroes above zero,
  with no rest screen (Windows playtest of patch #50).
- Herb stacks: 3 / 4 / 5 of each of MM6's three herbs and of empty bottles at camps 1-3, sold one at a time at the
  herb's own price (the scenario's three Widoweeps Berries and three bottles cost 13 gold in all).
- Against the camp's prices (the test's complaint was too little gold for training): the camp has no mastery
  teachers, masteries come from cards. Gold buys books and goods. MM6 book values are 100-400 for a school's Normal
  spells and 500-1000 for its Expert ones; the camp guilds sell at ×1.5 (Initiate, camp 1) and ×2.5-3 (Adept,
  camps 2-3). With the rewards the party has about 2000 gold at camp 1 (four or five Normal books), about 5000 at
  camp 2 and 10000 at camp 3 (two to four Expert books, or armour and potions), before any gold found on the way.
  Seen in the scenario (seed 4, no looting): 500 → 2000 → 4987 → 9987.

## Known limits

- The card screen was drawn and clicked only in headless form (layout and selection logic are unit-tested); its look
  in RU/EN at 4:3, 16:9 and 21:9 must be checked on Windows.
- Shield Wall's cover catches blows and arrows aimed at one hero, not spells or area attacks.
- Binding Circle springs on a hit by a monster standing within its radius (a melee or point-blank blow); a far arrow
  does not spring it.
- Side arrows of Multishot do not pierce and do not home; a bursting arrow's side arrows burst at half strength.
- The guardian keeps its kind's name (the coloured "Guardian" prefix is not done).

## Checks (Linux, `safe-heavy-run`, 2026-11-17)

- Unit: the `roguelike` group (`DungeonRunTests`, `RoguelikeRulesTests`, `RoguelikeTalentTests`): 40 cases, 7580
  assertions, all pass. Every talent's effect has a case; also four unique offers and their determinism, class caps
  and the Medusa heroine's, storage by id, the migration of the frozen v184 save (ranks by id, bonuses not doubled,
  rewritten and read back), the level-up heal, the camp shop's stacks, the guardian item's exact rarity without
  repeats, a full pack's queue, the card screen's texts.
- The other CI groups (`tools/minotaur_ci_tests.sh build/tests`): world, medusa-importer, updater, upstream-ai,
  upstream-indoor, native-ui and maw24 pass. gameplay (3), mm9 (7) and upstream24 (16) fail only on assets this
  sparse checkout lacks (sounds, MM9 icons and maps), the same kind of failures as the 2.4 integration's local run
  (which had more).
- Scenarios: `rulesets/roguelike_run.yml` 162 steps to victory (heal, card screen, talents, Earth Magic books, both
  rewards checks with a save and load, herbs bought, Rest for 2 food, four guardians' items and gold);
  `rulesets/roguelike_v184_save.yml` (the frozen v184 save plays on to the next level-up); and 15 more of CI's list
  pass. `mm9/shop_after_world_switch.yml` and `mm8/plane_gateway_keeps_buffs.yml` cannot load their maps here (not
  checked out).
- Not done: the AddressSanitizer pass of the checklist (before a push), anything on a screen or on Windows.
