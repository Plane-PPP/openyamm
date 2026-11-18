# Пул подземелий забега: все внутренние карты MM6–MM9

Дата: 2026-11-17. Ветка `integration/minotaur-v2.4-20261115`, HEAD `51ec30f4` (worktree
`/home/plane/projects/openyamm-minotaur-v2.3`). Только исследование: код, ресурсы, баланс и ветки не менялись,
сборки и игра не запускались. Отчёт опирается на реализацию забега v1 (`docs/research/roguelike-rules-v1.md`) и на
прежний статический каталог Codex (`/home/plane/projects/openyamm-roguelike-notes/codex-dungeons/`, 2026-11-11), но
все числа ниже пересчитаны заново по ресурсам этого HEAD.

**Решения пользователя от 2026-11-17** (ответы на раздел 11) — в разделе 11.1; рекомендации разделов 6 и 9
приведены к ним.

**Обозначения.** «Факт» — прочитано в коде или ресурсах (путь указан). «Расчёт» — посчитано скриптом по ресурсам
по тем же правилам, что у движка, без запуска игры. «Оценка» — моё суждение; его можно оспорить.

## 0. Коротко

- В ресурсах 204 внутренние сцены: MM6 — 50, MM7 — 63, MM8 — 47, MM9 — 44 (факт). После исключения служебных карт,
  арен, «пасхалок» NWC и 19 открытых регионов/городов MM9 остаётся **166 карт**, из них по классам адаптации
  (оценка, раздел 5): **A — 70** (подключаются практически без адаптации), **B — 71**, **C — 14**, **D — 11**.
- Прототип выбирает подземелья так: `rules.route: [1, 2, 3, 3]`, на каждый этап — первая в перемешанном по seed
  списке карта нужного `tier` (`dungeonRunOrder`, `game/content/DungeonRun.cpp:462`). В ступени 1 одна карта, поэтому
  первое подземелье всегда Goblinwatch. Монстры, их уровни и характеристики — оригинальные; меняются только размеры
  групп (`groupScale: 0.5` у пяти больших карт), отключено пополнение, добавлен страж (сильнейший вид ×4 HP).
- Фактическая кривая героя (факт, `run.yml`): вход в этапы 1–4 на уровнях **1 → 13 → 25 → 33**, финал на **41**,
  повышение класса после этапа 2. Средний уровень монстров в выбранных Fable картах: этап 1 — 3,4; этап 2 — 5–12;
  этап 3 — 16–21 (расчёт). Отношение «уровень героя ÷ средний уровень монстров» держится около 1,5–2.
- **Этап 1 можно сделать случайным уже сейчас**: кроме Goblinwatch, по уровням и без внешних зависимостей подходят
  `6d02` Abandoned Temple (MM6), `7d06` Temple of the Moon (MM7), `d05` Abandoned Temple (MM8) и `anskramkeep`
  Anskram Keep (MM9); пограничные — `6t3`, `d01`, `beethoven`.
- **Этап 4** в прототипе повторяет ступень 3. Для него есть большие сложные карты MM6 без внешних зависимостей:
  `6d17` Lair of the Wolf (≈340 монстров), `6t7` Superior Temple of Baa (≈340), `cd2` Castle Darkmoor (≈397),
  `6d19` Agar's Laboratory, `6d16` Warlord's Fortress; с малой адаптацией — `cd1` Castle Alamos (≈492), `6t6`
  Supreme Temple of Baa, `6d18` Gharik's Forge.
- Главные препятствия (раздел 7): страж «сильнейший вид ×4» на картах с уникальным боссом (Cuisinart 880 HP → 3520;
  Dragon King MM9 35 100 HP); карты без враждебных монстров на старте навсегда запирают выход; любой переход в
  другой BLV считается выходом из подземелья (многоуровневые комплексы играются одной картой); время прохождения
  больших карт MM6 без урезания групп — 20–40 минут до стража на одну карту (оценка); мир MM9 живёт по своим
  правилам (`world.yml`), и книги/награды забега там не проверены.

## 1. Источники, метод, ограничения

**Ресурсы (факт).**

- Сцены: `assets_dev/worlds/mm{6,7,8,9}/maps/*.scene.yml` — точки появления (`spawns`, `type_id: 3`), размещённые
  актёры (`initial_state.actors`), сундуки (`initial_state.chests.raw_items_hex`; у MM9 — `loot_containers`),
  предметы на полу (`sprite_objects.raw_containing_item_hex`), двери, стартовые точки MM9. Сцены MM9 — JSON.
- Геометрия: `*.blv` (двоичный формат, раскладка версий 6/7/8 — `game/indoor/IndoorMapData.cpp:306`
  `detectLayout`; MM9 записан в раскладке 8, `IndoorMapDataWriter::buildBytes`, там же `:1070`): число вершин, граней,
  секторов и габарит по вершинам.
- События: `assets_dev/worlds/mm*/events/maps/<карта>.lua` (+ надстройки `<карта>_mmmerge.lua` для 15 карт MM6),
  общий слой MM9 `events/common/mm9_common.lua` (ключи MM9 = квест-биты: `MM9.hasKey` → `evt.HasQuestBit`, `:65`).
- Таблицы: `assets_dev/engine/data_tables/map_stats.txt` (колонки — `game/tables/MapStats.cpp:20–69`),
  `monster_data.txt` (+ `worlds/mm9/data_tables/monster_data.txt`), `items.txt` (+ MM9, id = 10000 + сырой id,
  `worlds/mm9/state/item_ids.yml`), `hostile.txt`, `worlds/mm9/data_tables/map_transitions.txt`.

**Как считалось число монстров (расчёт по правилам движка).** Каждая точка `type_id: 3` даёт целую группу:
индексы 1–3 — слот встречи карты с рангом A/B/C по весам сложности слота, 4–12 — слот и ранг фиксированы
(`resolveIndoorEncounterSpawnDescriptor`, `resolveIndoorEncounterTierLetter`, веса
`IndoorEncounterDifficultyTierWeights` — `game/indoor/IndoorWorldRuntime.cpp:1094`, `:1189–1270`); размер группы —
равномерно из диапазона `map_stats.txt` (`resolveIndoorEncounterSpawnCount`). В таблицах — **математическое
ожидание** (E), а не результат конкретного seed. Плюс размещённые враждебные актёры (для MM6–MM8 враждебность — по
строке `Party` в `hostile.txt` и флагу Aggressor, как `defaultActorHostileToParty`, `IndoorWorldRuntime.cpp:1940`;
для MM9 — `hostility_type == 4` без `mm9_civilian`). Уровни, HP, опыт — `monster_data.txt`. «Сильнейший» — вид с
наибольшим базовым HP среди враждебных, как выбирает стража `IndoorWorldRuntime::mapKillProgress` (`:19622`).
Монстры, вызванные скриптами (`evt.SummonMonsters`), в E не входят — их число вызовов указано отдельно.

**Ограничения.**

- Игра не запускалась: E — среднее по правилам, фактический состав на конкретном seed другой; число монстров у
  MM7/MM8 в объединённой таблице везде «1–3» на точку, у MM6 — 2–6 (факт `map_stats.txt`), поэтому карты MM6 в
  2–4 раза «плотнее».
- Зависимости найдены статически (регулярные выражения по Lua, без исполнения): `HasItem`, `IsQBitSet`,
  `MM9.hasKey/hasItem`, переходы `evt.MoveToMap` и `map_transitions.txt`. «Внешний» = проверяется, но на этой карте
  не выдаётся. Для ~40 карт-кандидатов смысл проверок прочитан вручную (раздел 5); остальные классифицированы
  автоматически.
- Время прохождения — оценка: темп из правил v0 Fable (6–8 убийств в минуту вместе с ходьбой, не проверен), порог
  стража 50 % HP ≈ половина монстров. Формула колонки: E × 0,5 / (6…8) минут.
- Физическую проходимость, ловушки и перспективу камеры по геометрии не проверял; «габарит» — прямоугольник по
  вершинам, а не площадь пола.
- Для чтения ресурсов я **докачал в общий объектный склад git** (`/home/plane/projects/openyamm/.git`) 549 blob-ов
  карт этого HEAD (`*.scene.yml`, `*.blv`, Lua MM9; ~200 МБ) пакетным способом из `AGENTS.md`
  (`GIT_NO_LAZY_FETCH=1`, `fetch --stdin`). Рабочее дерево, sparse-checkout, ветки и индекс не тронуты; выгрузка шла
  во временную папку сессии. Если это нежелательно, объекты уберёт обычная сборка мусора git только после их
  открепления — сообщите, решу отдельно.
- Временные скрипты (`catalog.py`, `derive.py`, `stages.py`, `tables.py`, `itemplace.py`, `gates.py`) лежат в
  папке сессии `/tmp/claude-1000/-home-plane-projects-openyamm-minotaur-v2-3/06ae6141-3c52-4418-8fdb-6c3a16b841ef/scratchpad/scripts/`
  и не сохраняются; при необходимости их можно перенести в `tools/` отдельным решением.

## 2. Как устроен действующий прототип (факт, код)

### 2.1 Выбор четырёх подземелий

- `assets_dev/engine/rulesets/roguelike/run.yml`: пул `dungeons` (8 карт MM6) с `tier`, `groupScale`, `entrance`
  (x, y, z, yaw — взяты из событий наружных карт, ведущих в подземелье: `evt.MoveToMap(..., "6d01.blv")`).
- `dungeonRunOrder(run, seed)` (`game/content/DungeonRun.cpp:462`): перемешивает все карты Фишером–Йетсом от
  `seed ^ randomNameKey("dungeon_run.order")`, затем для каждого числа из `rules.route` берёт **первую** карту с
  таким `tier` и удаляет её из списка. Если карт нужного tier не осталось — берётся любая первая (`order.begin()`),
  то есть ступень молча нарушается. Повторов нет.
- `route: [1, 2, 3, 3]`, пул по tier: 1 — `6d01`; 2 — `6d02`, `6t3`, `6t2`; 3 — `6d07`, `6d15`, `6d13`, `6d12`.
  Отсюда: этап 1 всегда Goblinwatch, этапы 3 и 4 — две разные карты ступени 3.
- Сценарий `tests/scenarios/rulesets/roguelike_run.yml` и `tests/DungeonRunTests.cpp:224` **закладываются на то, что
  первая карта — 6d01** (`"Run started in 6d01.blv, seed 4."`): расширение пула их сломает (раздел 8).

### 2.2 Что меняется относительно оригинальных карт

| Параметр | В забеге | Где |
|---|---|---|
| Состав, уровни, HP, атаки, сопротивления монстров | **без изменений** (Standard) | `RoguelikeRules::Monsters` наследует `MonsterRules`; `resolveMonster`, `spawnTier`, `enterMap` не переопределены |
| Размер групп | `groupScale` карты: `min/max × scale`, округление, не меньше 1; у `6d01`, `6d02`, `6t3`, `6d15`, `6d12` — 0,5 | `adjustMapEncounters`, `RoguelikeRules.cpp:655` |
| Пополнение | отключено (`mapRespawnDays` = 0) | `RoguelikeRules.cpp:680` |
| Прогресс | доля **HP** убитых враждебных монстров первой заселённости; NPC и поздние призывы не весят | `mapKillProgress`, `IndoorWorldRuntime.cpp:19622`; `runClearFraction`, `RoguelikeRules.cpp:562` |
| Страж | при 50 % HP: сильнейший по базовому HP вид карты, ×4 HP (`guardianHealthScale`), у точки входа; без места — выход открывается сразу | `placeDungeonRunGuardian`, `GameApplicationDungeonRun.cpp:925` |
| Выход | любой `MapMove` в другую карту запрещён до смерти стража; после — лагерь, затем **следующее подземелье вместо исходной цели** | `processPendingMapMove`, `GameApplication.cpp:15673`; `redirectMapMoveForDungeonRun`, `GameApplicationDungeonRun.cpp:155` |
| Лут | обычный (сундуки, трупы, уровень сокровищ карты); плюс награда стража: золото 1500/3000/5000 и артефакт (1–2) / реликвия (3–4) мира **текущей карты** | `rules.rewards`, `pickRunGuardianItem`, `RoguelikeRules.cpp:458` |
| Опыт | обычный опыт за убийства остаётся; уровни дают только повышения забега | `RoguelikeRules::Experience` |
| Ловушки, двери, события карты | без изменений | — |

### 2.3 Фактическая кривая прогрессии

Факт `run.yml`: `levels: {perStop: [3, 3, 2, 2], levelsEach: 4}`, повышения равномерно по доле до стража
(`runLevelUpsDue`, `RoguelikeRules.cpp:573`), последнее — смерть стража; `promotionAfterStop: 2`.

| Этап | Уровень героя: вход → повышения → выход | Карты прототипа | Ур. монстров ср. / p90 / max (расчёт) | Страж (база → ×4) |
|---|---|---|---|---|
| 1 | 1 → 5 → 9 → **13** | `6d01` | 3,4 / 6 / 10 | Goblin King 40 → 160 |
| 2 | 13 → 17 → 21 → **25**, затем повышение класса | `6d02`, `6t3`, `6t2` | 5,1–11,8 / 8–27 / 14–27 | 61–153 → 244–612 |
| 3 | 25 → 29 → **33** | `6d07`, `6d15`, `6d13`, `6d12` | 16,1–21,3 / 25–33 / 33–50 | 207–400 → 828–1600 |
| 4 | 33 → 37 → **41** | те же, что этап 3 | 16,1–21,3 | 207–400 → 828–1600 |

Число монстров у карт прототипа (расчёт, E): оригинал / с `groupScale`: `6d01` 218 / 109, `6d02` 220 / 110,
`6t3` 208 / 94, `6t2` 43, `6d07` 71, `6d13` 62, `6d15` 148 / 74, `6d12` 210 / 95 (совпадает с расчётом Fable в
`openyamm-roguelike-notes/правила-v0-приложение.md`, раздел 1).

**Наблюдение (оценка).** На этапе 4 герой 33–41-го уровня встречает те же монстры 16–21-го уровня, что и на
этапе 3, — это провал кривой; отсюда потребность в собственном пуле этапа 4 с монстрами 25–35-го уровня в среднем.

## 3. Критерии этапов (оценка, выведена из кривой Fable)

| Этап | Герой | Средний ур. монстров | p90 | Максимум | Ориентир стража (база) |
|---|---|---|---|---|---|
| 1 | 1 → 13 | ≤ 7 | ≤ 10 | ≤ 18 | ≤ ~90 |
| 2 | 13 → 25 | 4,5–13,5 | ≤ 27 | ≤ 35 | ≤ ~230 |
| 3 | 25 → 33 | 12–22,5 | — | ≤ 50 | ≤ ~400 |
| 4 | 33 → 41 | 18–36 | — | ≤ 80 | ≤ ~550; больше — нужна правка стража |
| 5+ | — | > 36 или max > 80 | | | только с балансировкой |

Карта может попасть в два соседних этапа (колонка «Этап» в каталоге: «23» = этапы 2 и 3).

## 4. Каталог всех внутренних карт

Колонки: **E** — ожидаемое число монстров первой заселённости (расчёт); **Ур.** — средний / 90-й перцентиль /
максимум по ожидаемому числу; **Состав** — виды слота A/B/C с уровнями и ожидаемым числом, затем размещённые
враждебные актёры (имя, уровень × число); **Сильнейший → страж** — базовое HP и HP стража ×4 по текущему правилу;
**Секторы · габарит** — число секторов и прямоугольник по вершинам BLV в единицах движка (для сравнения: Goblinwatch 39 344 × 10 720); **Двери · сундуки** — записи дверей
и непустые сундуки; **Мин. до стража\*** — оценка (см. раздел 1); **Класс** — адаптация (раздел 5).

Исключены из пула (факт по имени/миру, оценка — причина): 12 служебных карт MM6 (`zddb02…10`, `zdtl01/02`,
`zdwj01`, имя `pending`); арены `zarena`, `7d05`, `d42`, `thearena`; «пасхалки» NWC (`znwc`, `7nwc`, `d50`);
19 открытых регионов и городов MM9 из `worlds/mm9/world.yml: openAirMaps` (это наружные области, хоть и BLV).

#### MM6

| Карта | Название | Этап | Монстров (E) | Ур. ср. / p90 / max | Состав (ожид. число) | Сильнейший → страж ×4 | HP всего | Секторы · габарит XY | Двери · сундуки | Мин. до стража* | Класс | Зависимости / заметки |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| `6d01.blv` | Goblinwatch | 1 | 218 | 3.4 / 6 / 10 | Goblin/Goblin Shaman/Goblin King 4–10 (~64); Common Rat/Large Rat/Giant Rat 2–6 (~84); Blood Sucker/Brain Sucker/Soul Sucker 2–8 (~70) | Goblin King 40 → 160 | 2,421 | 59 · 39344×10720 | 38 · 12 | 14–18 | A | внешних проверок не найдено |
| `6d02.blv` | Abandoned Temple | 12 | 220 | 5.1 / 8 / 14 | Bat/Giant Bat/Vampire Bat 3–9 (~75); Spider/Giant Spider/Huge Spider 5–12 (~72); Cobra/King Cobra/Queen Cobra 5–14 (~72); размещены: Huge Spider 12×1 | Queen Cobra 61 → 244 | 3,866 | 67 · 50938×30304 | 10 · 7 | 14–18 | A | внешних проверок не найдено |
| `6d03.blv` | Shadow Guild Hideout | 23 | 61 | 12.9 / 22 / 22 | Thief/Burglar/Rogue 8–18 (~40); Thug/Ruffian/Brigand 8–22 (~21) | Brigand 114 → 456 | 3,518 | 23 · 6144×7936 | 16 · 4 | 4–5 | A | внешних проверок не найдено |
| `6d04.blv` | Hall of the Fire Lord | 2 | 266 | 10.8 / 20 / 35 | Goblin/Goblin Shaman/Goblin King 4–10 (~184); Ogre/Ogre Raider/Ogre Chieftain 15–28 (~81); размещены: Defender of VARN 35×1 | Defender of VARN 227 → 908 | 12,720 | 142 · 11968×13764 | 29 · 11 | 17–22 | A | Amber 2102 и Chest Key 2179 — на этой карте |
| `6d05.blv` | Snergle's Caverns | 3 | 213 | 12.9 / 30 / 30 | Bat/Giant Bat/Vampire Bat 3–9 (~88); Dwarf/Dwarf Warrior/Dwarf Lord 10–30 (~124); размещены: Dwarf Lord 30×1 | Dwarf Lord 180 → 720 | 13,145 | 36 · 11776×17408 | 6 · 1 | 13–18 | C | дверь покоев Снергла (событие 9) — ключ 2108 от NPC в 6d09; за ней призыв 18 дварфов B/C |
| `6d06.blv` | Dragoons' Caverns | 3 | 156 | 14.3 / 22 / 35 | Ooze/Acidic Ooze/Corrosive Ooze 12–25 (~40); Fighter/Soldier/Veteran 14–35 (~8); Thug/Ruffian/Brigand 8–22 (~108) | Veteran 227 → 908 | 10,327 | 71 · 13056×15104 | 35 · 9 | 10–13 | A | внешних проверок не найдено |
| `6d07.blv` | Silver Helm Outpost | 3 | 70 | 16.1 / 27 / 33 | Novice Monk/Initiate Monk/Master Monk 8–27 (~24); Guard/Lieutenant/Captain 11–33 (~18); Swordsman/Expert Swordsman/Master Swordsman 10–24 (~28) | Captain 207 → 828 | 5,550 | 41 · 7680×9216 | 39 · 7 | 4–6 | A | внешних проверок не найдено |
| `6d08.blv` | Shadow Guild | ? | 82 | 11.1 / 18 / 55 | Genie/Djinn/Efreet 33–55 (~2); Thief/Burglar/Rogue 8–18 (~39); Cutpurse/Bounty Hunter/Assassin 3–7 (~39); размещены: Rogue 18×1, Genie 33×1 | Efreet 467 → 1868 | 4,006 | 45 · 13376×12288 | 16 · 13 | 5–7 | B | двери-загадки (dark/arrow/time); выходы в 6d03 и 6d06 — для забега это выход |
| `6d09.blv` | Snergle's Iron Mines | 34 | 104 | 20.2 / 30 / 40 | Dwarf/Dwarf Warrior/Dwarf Lord 10–30 (~48); Ooze/Acidic Ooze/Corrosive Ooze 12–25 (~36); Devil Spawn/Devil Worker 20–40 (~20) | Devil Worker 280 → 1120 | 11,018 | 60 · 19456×11264 | 26 · 12 | 6–9 | B | Cell Key 2192 — в сундуке этой же карты; ключ 2108 выдаёт NPC Ghim Hammond (событие 41) |
| `6d10.blv` | Dragoons' Keep | 3 | 164 | 15.5 / 35 / 35 | Common Rat/Large Rat/Giant Rat 2–6 (~60); Fighter/Soldier/Veteran 14–35 (~104) | Veteran 227 → 908 | 13,356 | 48 · 11008×14848 | 28 · 4 | 10–14 | A | внешних проверок не найдено |
| `6d11.blv` | Corlagon's Estate | 3 | 211 | 13.0 / 19 / 40 | Ghost/Evil Spirit/Specter 9–19 (~126); Power Lich 40–40 (~1); Skeleton/Skeleton Knight/Skeleton Lord 6–14 (~84) | Power Lich 280 → 1120 | 12,057 | 81 · 13312×18752 | 35 · 11 | 13–18 | A | внешних проверок не найдено |
| `6d12.blv` | Silver Helm Stronghold | 34 | 210 | 21.3 / 25 / 50 | Fighter/Soldier/Veteran 14–35 (~72); Swordsman/Expert Swordsman/Master Swordsman 10–24 (~132); Sorcerer 25–25 (~5); размещены: Warlock 50×1 | Warlock 400 → 1600 | 23,631 | 32 · 7424×8960 | 13 · 4 | 13–18 | A | внешних проверок не найдено |
| `6d13.blv` | The Monolith | 34 | 62 | 21.0 / 30 / 40 | Druid/Great Druid/Grand Druid 10–28 (~42); Rock Beast/Earth Spirit/Earth Elemental 25–40 (~12); Water Beast/Water Spirit/Water Elemental 14–36 (~8) | Earth Elemental 280 → 1120 | 7,034 | 27 · 9728×9984 | 11 · 2 | 4–5 | A | внешних проверок не найдено |
| `6d14.blv` | Tomb of Ethric the Mad | 3 | 86 | 17.7 / 40 / 40 | Ghost/Evil Spirit/Specter 9–19 (~8); Lich/Greater Lich/Power Lich 20–40 (~22); Skeleton/Skeleton Knight/Skeleton Lord 6–14 (~55); размещены: Power Lich 40×1 | Power Lich 280 → 1120 | 8,194 | 50 · 11264×11520 | 13 · 8 | 5–7 | A | внешних проверок не найдено |
| `6d15.blv` | Icewind Keep | 34 | 148 | 19.8 / 33 / 33 | Guard/Lieutenant/Captain 11–33 (~72); Ogre/Ogre Raider/Ogre Chieftain 15–28 (~76) | Captain 207 → 828 | 15,124 | 33 · 9728×13312 | 18 · 8 | 9–12 | A | внешних проверок не найдено |
| `6d16.blv` | Warlord's Fortress | 4 | 129 | 31.7 / 60 / 80 | Fighter/Soldier/Veteran 14–35 (~45); Thug/Ruffian/Brigand 8–22 (~51); Death Knight/Doom Knight/Cuisinart 40–80 (~33) | Cuisinart 880 → 3520 | 30,667 | 40 · 9760×7168 | 19 · 18 | 8–11 | A | ключи 2184/2185/2191 выдают события этой карты |
| `6d17.blv` | Lair of the Wolf | 4 | 340 | 23.9 / 40 / 40 | Ooze/Acidic Ooze/Corrosive Ooze 12–25 (~96); Swordsman/Expert Swordsman/Master Swordsman 10–24 (~110); Wolfman/Werewolf/Greater Werewolf 20–40 (~133); размещены: Greater Werewolf 40×1 | Greater Werewolf 280 → 1120 | 45,790 | 124 · 29056×45135 | 39 · 12 | 21–28 | A | Pearl of Purity лежит на этой карте; QBit 1059 убирает только призрака-NPC |
| `6d18.blv` | Gharik's Forge | 4 | 120 | 33.2 / 50 / 50 | Fire Beast/Fire Spirit/Fire Elemental 13–39 (~48); Sorcerer/Magician/Warlock 25–50 (~72) | Warlock 400 → 1600 | 26,890 | 48 · 9472×13056 | 53 · 8 | 7–10 | C | дверь лаборатории (рычаг, событие 16): QBit 1035 из 6d07 + ключ 2107 из сундука 6d07 |
| `6d19.blv` | Agar's Laboratory | 4 | 88 | 31.2 / 50 / 50 | Flying Eye/Terrible Eye/Maddening Eye 30–50 (~48); Agar's Pet/Agar's Monster/Agar's Abomination 13–17 (~40); размещены: Power Lich 40×1 | Maddening Eye 400 → 1600 | 18,969 | 297 · 7168×15616 | 16 · 2 | 6–7 | A | внешних проверок не найдено |
| `6d20.blv` | Caves of the Dragon Riders | 5+ | 108 | 67.9 / 80 / 80 | Wyrm/Giant Wyrm/Great Wyrm 50–70 (~45); Death Knight/Doom Knight/Cuisinart 40–80 (~63) | Cuisinart 880 → 3520 | 73,140 | 21 · 13312×13584 | 0 · 13 | 7–9 | A | внешних проверок не найдено |
| `6t1.blv` | Temple of Baa | 2 | 212 | 9.3 / 14 / 25 | Spider/Giant Spider/Huge Spider 5–12 (~108); Acolyte of Baa/Cleric of Baa/Priest of Baa 8–25 (~44); Skeleton/Skeleton Knight/Skeleton Lord 6–14 (~60) | Priest of Baa 137 → 548 | 8,227 | 50 · 28801×39244 | 20 · 6 | 13–18 | A | все 7 ключей (2175–2183) — в сундуках/событиях этой же карты |
| `6t2.blv` | Temple of the Fist | 2 | 43 | 11.8 / 27 / 27 | Novice Monk/Initiate Monk/Master Monk 8–27 (~27); Common Rat/Large Rat/Giant Rat 2–6 (~16) | Master Monk 153 → 612 | 2,392 | 13 · 3224×4352 | 4 · 3 | 3–4 | A | внешних проверок не найдено |
| `6t3.blv` | Temple of Tsantsa | 2 | 208 | 8.5 / 10 / 14 | Cobra/King Cobra/Queen Cobra 5–14 (~75); Cannibal/Head Hunter/Witch Doctor 6–10 (~66); Cannibal/Head Hunter/Witch Doctor 6–10 (~66) | Queen Cobra 61 → 244 | 6,876 | 67 · 9472×9216 | 24 · 6 | 13–17 | A | внешних проверок не найдено |
| `6t4.blv` | Temple of the Sun | 4 | 17 | 20.8 / 27 / 79 | Minotaur King 79–79 (~1); Novice Monk/Initiate Monk/Master Monk 8–27 (~15); размещены: Master Monk 27×1 | Minotaur King 861 → 3444 | 2,209 | 8 · 6400×2944 | 3 · 9 | 1–1 | A | внешних проверок не найдено |
| `6t5.blv` | Temple of the Moon | 34 | 118 | 18.9 / 35 / 45 | Medusa/Medusa Enchantress/Gorgon 35–45 (~22); Cobra/King Cobra/Queen Cobra 5–14 (~60); Druid/Great Druid/Grand Druid 10–28 (~36) | Gorgon 337 → 1348 | 12,326 | 27 · 15360×9728 | 15 · 3 | 7–10 | B | алтарь Луны (событие 27): квест «в полночь полнолуния» (QBit 1143) — только разговор |
| `6t6.blv` | Supreme Temple of Baa | 4 | 290 | 21.6 / 39 / 60 | Acolyte of Baa/Cleric of Baa/Priest of Baa 8–25 (~180); Devil Spawn/Devil Worker/Devil Warrior 20–60 (~55); Fire Beast/Fire Spirit/Fire Elemental 13–39 (~54); размещены: Priest of Baa 25×1 | Devil Warrior 540 → 2160 | 36,887 | 67 · 14688×11136 | 21 · 7 | 18–24 | B | второй вход/выход через sewer.blv |
| `6t7.blv` | Superior Temple of Baa | 34 | 340 | 19.2 / 28 / 50 | Acolyte of Baa/Cleric of Baa/Priest of Baa 8–25 (~130); Druid/Great Druid/Grand Druid 10–28 (~126); Sorcerer/Magician/Warlock 25–50 (~81); размещены: Priest of Baa 25×2 | Warlock 400 → 1600 | 35,062 | 78 · 22026×20976 | 21 · 9 | 21–28 | A | ключи 2112/2187 носят монстры этой карты |
| `6t8.blv` | Temple of the Snake | 5+ | 74 | 35.4 / 45 / 100 | Medusa/Medusa Enchantress/Gorgon 35–45 (~63); Gold Dragon 100–100 (~1); Peasant/Peasant/Peasant 1–3 (~9); размещены: Peasant 5×1 | Gold Dragon 1300 → 5200 | 18,822 | 23 · 8704×4992 | 4 · 9 | 5–6 | A | внешних проверок не найдено |
| `cd1.blv` | Castle Alamos | 4 | 492 | 30.7 / 50 / 50 | Magyar/Magyar Soldier/Magyar Matron 14–37 (~300); Dust Devil/Twister/Air Elemental 16–33 (~70); Sorcerer/Magician/Warlock 25–50 (~122) | Warlock 400 → 1600 | 96,216 | 181 · 22016×19792 | 13 · 14 | 31–41 | B | ключи 2188/2189 в сундуках карты; плита с паролем JBARD (MMMerge, событие 69) отбрасывает назад |
| `cd2.blv` | Castle Darkmoor | 4 | 397 | 31.3 / 40 / 50 | Flying Eye/Terrible Eye/Maddening Eye 30–50 (~195); Lich/Greater Lich/Power Lich 20–40 (~93); Ogre/Ogre Raider/Ogre Chieftain 15–28 (~108); размещены: Power Lich 40×1 | Maddening Eye 400 → 1600 | 79,366 | 3 · 44558×30000 | 12 · 1 | 25–33 | A | кристалл Delta берётся на месте; QBit 1126 (Oracle) лишь скрывает его |
| `cd3.blv` | Castle Kriegspire | 5+ | 94 | 37.8 / 59 / 79 | Flame Drake/Frost Drake/Energy Drake 24–32 (~22); Minotaur/Minotaur Mage/Minotaur King 39–79 (~54); Agar's Pet/Agar's Monster/Agar's Abomination 13–17 (~18) | Minotaur King 861 → 3444 | 26,843 | 165 · 14560×10112 | 19 · 8 | 6–8 | C | «Guardian of Kriegspire» (событие 13): 50 000 золота за тайный проход, если нет QBit 1364 |
| `hive.blv` | The Hive | 5+ | 134 | 53.6 / 70 / 100 | Devil Spawn/Devil Worker/Devil Warrior 20–60 (~39); Devil Captain/Devil Master/Devil King 30–70 (~93); размещены: Reactor 100×1, Demon Queen 100×1 | Reactor 1300 → 5200 | 62,894 | 81 · 17184×25274 | 32 · 6 | 8–11 | D | финал MM6: выход/концовка требуют Ritual of the Void 2164 (не на карте), EnterHouse-концовка |
| `oracle.blv` | Oracle of Enroth | - | 0 | — | — | — | 0 | 10 · 14080×13952 | 2 · 0 | — | D | нет монстров; сюжетный узел (4 Memory Crystal) |
| `pyramid.blv` | Tomb of VARN | 5+ | 372 | 47.6 / 65 / 70 | Genie/Djinn/Efreet 33–55 (~172); Patrol Unit/Enforcer Unit 50–70 (~6); Defender of VARN/Sentinel of VARN/Guardian of VARN 35–65 (~192); размещены: Defender of VARN 35×1 | Enforcer Unit 700 → 2800 | 142,026 | 216 · 28672×38787 | 23 · 7 | 23–31 | B | все коды и ключи на карте, но длинная цепочка головоломок (12 предметов) |
| `sewer.blv` | Free Haven Sewer | 2 | 163 | 7.4 / 15 / 25 | Common Rat/Large Rat/Giant Rat 2–6 (~112); Thief/Burglar/Rogue 8–18 (~18); Acolyte of Baa/Cleric of Baa/Priest of Baa 8–25 (~33) | Priest of Baa 137 → 548 | 4,922 | 119 · 41344×23336 | 9 · 7 | 10–14 | B | выход в 6t6; квест Принца воров (QBit 1122/1200) — разговор/дверь цилиндра |
| `zarena.blv` | The Arena | — | 0 | — | — | — | 0 | 2 · 8704×8640 | 0 · 0 | — | — | arena |
| `zddb01.blv` | Dragon's Lair | — | 1 | 80.0 / 80 / 80 | размещены: Red Dragon 80×1 | Red Dragon 880 → 3520 | 880 | 2 · 2432×4198 | 0 · 0 | 0–0 | — | test/pending |
| `zdwj02.blv` | Devil Outpost | 4 | 16 | 30.0 / 60 / 60 | Devil Spawn/Devil Worker/Devil Warrior 20–60 (~15); размещены: Devil Warrior 60×1 | Devil Warrior 540 → 2160 | 3,240 | 3 · 3328×2944 | 0 · 2 | 1–1 | A | внешних проверок не найдено |
| `znwc.blv` | New World Computing | — | 55 | 1.8 / 3 / 10 | размещены: Peasant 1×53, Goblin King 10×1, Goblin Shaman 6×1 | Goblin King 40 → 160 | 307 | 66 · 7168×10496 | 56 · 0 | 3–5 | — | NWC easter egg |

#### MM7

| Карта | Название | Этап | Монстров (E) | Ур. ср. / p90 / max | Состав (ожид. число) | Сильнейший → страж ×4 | HP всего | Секторы · габарит XY | Двери · сундуки | Мин. до стража* | Класс | Зависимости / заметки |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| `7d05.blv` | The Arena | — | 0 | — | — | — | 0 | 2 · 8704×8640 | 6 · 0 | — | — | arena |
| `7d06.blv` | The Temple of the Moon | 1 | 62 | 3.6 / 6 / 12 | Giant Bat/Vampire Bat/Inferno Bat 3–9 (~18); Giant Rat/Lightning Rat/Fire Rat 2–6 (~28); Giant Spider/Venemous Spider/Widowmaker 5–12 (~14); размещены: Swordsman 10×3 | Widowmaker 50 → 200 | 757 | 31 · 11520×12608 | 7 · 7 | 4–5 | A | внешних проверок не найдено |
| `7d07.blv` | Thunderfist Mountain | 5+ | 98 | 39.9 / 59 / 79 | Minotaur/Minotaur Headsman/Minotaur Lord 39–79 (~40); Floating Eye/Gazer/Evil Eye 30–50 (~34); Fire Warlock/Water Warlock/Air Warlock 25–50 (~24) | Minotaur Lord 861 → 3444 | 28,770 | 57 · 23424×19088 | 11 · 20 | 6–8 | B | переход в 7d35, 7d36 |
| `7d08.blv` | The Tularean Caves | 3 | 56 | 15.9 / 22 / 37 | Wyvern/Horned Wyvern/Ancient Wyvern 19–37 (~14); Elven Warrior/Elven Defender/Elven Lancer 14–30 (~24); Troglodyte Drone/Troglodyte Soldier/Troglodyte Queen 9–19 (~18) | Ancient Wyvern 247 → 988 | 4,254 | 40 · 24064×24160 | 9 · 7 | 4–5 | B | квест-биты 593,595,1696; переход в 7d32 |
| `7d09.blv` | The Titans' Stronghold | 5+ | 42 | 77.3 / 95 / 100 | Green Dragon/Blue Dragon/Red Dragon 80–100 (~12); Titan/Storm Titan/Blood Titan 65–95 (~30) | Red Dragon 1300 → 5200 | 35,418 | 35 · 33600×35328 | 0 · 12 | 3–4 | B | квест-биты 542,544,1586,1587… |
| `7d10.blv` | The Breeding Zone | 5+ | 60 | 30.7 / 60 / 85 | Gogling/Gog/Magog 7–18 (~14); Young Behemoth/Behemoth/Ancient Behemoth 45–85 (~18); Warden of Fire/Guardian of Flame/Fire Elemental 13–39 (~28) | Ancient Behemoth 977 → 3908 | 13,792 | 33 · 8960×11584 | 16 · 2 | 4–5 | B | переход в 7d26 |
| `7d11.blv` | The Walls of Mist | 4 | 65 | 30.9 / 50 / 55 | Tempest/Cyclone/Air Elemental 16–33 (~22); Djinni/Genie/Efreet 33–55 (~18); Sorcerer/Wizard/Archmage 25–50 (~24); размещены: Lesser Light Elemental 15×1 | Efreet 467 → 1868 | 13,031 | 57 · 41088×43520 | 32 · 4 | 4–5 | B | ключи колонн 1454–1456 в сундуках карты; выход в Celeste |
| `7d12.blv` | Clanker's Laboratory | 4 | 33 | 24.3 / 40 / 50 | Gogling/Gog/Magog 7–18 (~14); Floating Eye/Gazer/Evil Eye 30–50 (~8); Bronze Golem/Brass Golem/Gold Golem 22–50 (~10); размещены: Evil Eye 50×1 | Evil Eye 400 → 1600 | 4,822 | 3 · 6048×4608 | 17 · 14 | 2–3 | B | квест-биты 637 |
| `7d13.blv` | Zokarr's Tomb | 2 | 18 | 11.8 / 14 / 26 | Ghast/Ghoul/Revenant 9–17 (~10); Wight/Wraith/Barrow Wight 14–26 (~8) | Barrow Wight 145 → 580 | 897 | 13 · 4000×4800 | 6 · 0 | 1–2 | C | Zokarr's Tomb: череп 1428 приносится из 7d35 |
| `7d14.blv` | The School of Sorcery | 4 | 39 | 29.4 / 50 / 50 | Bronze Golem/Brass Golem/Gold Golem 22–50 (~6); Sorcerer/Wizard/Archmage 25–50 (~32); размещены: Lesser Light Elemental 15×1 | Gold Golem 400 → 1600 | 7,099 | 18 · 5376×5760 | 16 · 2 | 2–3 | B | квест-биты 657 |
| `7d15.blv` | Watchtower 6 | 4 | 71 | 23.7 / 25 / 50 | Clay Elemental/Stone Elemental/Earth Elemental 25–40 (~26); Lich/Power Lich/Lich King 20–40 (~30); Necromancer/Speaker for the Dead/Queen of the Dead 25–50 (~14); размещены: Shade 9×1 | Queen of the Dead 400 → 1600 | 9,153 | 20 · 13824×15296 | 13 · 4 | 4–6 | B | квест-биты 612 |
| `7d16.blv` | The Wine Cellar | 34 | 23 | 22.0 / 48 / 50 | Giant Bat/Vampire Bat/Inferno Bat 3–9 (~10); Necromancer/Speaker for the Dead/Queen of the Dead 25–50 (~6); Minion Vampire/Vampire/Elder Vampire 20–48 (~4); размещены: Elder Vampire 48×2, Shade 9×1 | Queen of the Dead 400 → 1600 | 3,237 | 11 · 4256×4448 | 8 · 4 | 1–2 | A | внешних проверок не найдено |
| `7d17.blv` | The Tidewater Caverns | 23 | 48 | 12.1 / 18 / 19 | Shade/Specter/Ghost 9–19 (~10); Skeleton/Skeleton Warrior/Skeleton Lord 6–14 (~14); Thief/Rogue/Master Thief 8–18 (~24) | Ghost 93 → 372 | 2,492 | 13 · 13152×11968 | 5 · 4 | 3–4 | B | квест-биты 1607,1608 |
| `7d18.blv` | Lord Markham's Manor | 4 | 10 | 30.5 / 60 / 80 | Initiate of the Sword/Champion of the Sword/Master of the Sword 40–80 (~4); Swordsman/Guard/Adventurer 10–24 (~6) | Master of the Sword 880 → 3520 | 2,329 | 6 · 2944×1792 | 6 · 5 | 1–1 | B | квест-биты 616 |
| `7d19.blv` | Grand Temple of the Moon | 3 | 27 | 17.0 / 30 / 30 | Acolyte of the Moon/Cleric of the Moon/Priest of the Moon 10–30 (~10); Wight/Wraith/Barrow Wight 14–26 (~16); размещены: Priest of the Moon 30×1 | Priest of the Moon 180 → 720 | 2,253 | 19 · 4864×4224 | 30 · 7 | 2–2 | B | квест-биты 554 |
| `7d20.blv` | The Mercenary Guild | 4 | 22 | 34.3 / 60 / 80 | Archer/Bowman/Elite Archer 9–29 (~6); Robber/Raider/Bandit 8–22 (~6); Initiate of the Sword/Champion of the Sword/Master of the Sword 40–80 (~10) | Master of the Sword 880 → 3520 | 5,849 | 3 · 6168×4864 | 29 · 20 | 1–2 | A | внешних проверок не найдено |
| `7d21.blv` | White Cliff Cave | 2 | 34 | 10.0 / 12 / 25 | Emerald Ooze/Sapphire Ooze/Bloodstone Ooze 12–25 (~4); Troglodyte Drone/Troglodyte Soldier/Troglodyte Queen 9–19 (~30) | Bloodstone Ooze 137 → 548 | 1,366 | 26 · 12357×12783 | 1 · 9 | 2–3 | A | внешних проверок не найдено |
| `7d22.blv` | The Hall under the Hill | 12 | 17 | 5.6 / 8 / 8 | 0 (~20); Dragonfly/Fire Dragonfly/Queen Dragonfly 2–8 (~16); размещены: Queen Dragonfly 8×1 | Queen Dragonfly 30 → 120 | 336 | 24 · 11008×20816 | 12 · 2 | 1–1 | B | Faerie Key 1464 на карте; QBit 549 — разговор с королём фей |
| `7d23.blv` | The Lincoln | 5+ | 81 | 64.2 / 90 / 90 | Seeker Droid/Sentinel Droid/Assassin Droid 50–90 (~64); размещены: Seeker Droid 50×8, Assassin Droid 90×5, Sentinel Droid 70×4 | Assassin Droid 1080 → 4320 | 51,012 | 43 · 10904×13120 | 72 · 4 | 5–7 | C | The Lincoln: нужен Wetsuit 1406 (из 7d30/d03) для подводной части |
| `7d24.blv` | Stone City | 23 | 86 | 12.3 / 20 / 30 | Troglodyte Drone/Troglodyte Soldier/Troglodyte Queen 9–19 (~46); размещены: Peasant 2×18, Dwarven Lieutenant 20×10, Dwarven Commander 30×6, Dwarven Soldier 10×6 | Dwarven Commander 180 → 720 | 4,942 | 41 · 17056×13336 | 10 · 13 | 5–7 | B | Stone City: город дварфов; враждебность групп зависит от QBit 611 (Свет) |
| `7d25.blv` | Celeste | 5+ | 21 | 46.2 / 50 / 50 | размещены: Angel Lord 50×16, Greater Roc 39×4, Lesser Light Elemental 15×1 | Angel Lord 400 → 1600 | 7,543 | 31 · 16000×21760 | 4 · 0 | 1–2 | D | город Celeste (магазины, NPC), 9 внешних квест-битов |
| `7d26.blv` | The Pit | 4 | 77 | 30.8 / 50 / 50 | Lich/Power Lich/Lich King 20–40 (~18); Necromancer/Speaker for the Dead/Queen of the Dead 25–50 (~48); Minion Vampire/Vampire/Elder Vampire 20–48 (~10); размещены: Shade 9×1 | Queen of the Dead 400 → 1600 | 15,181 | 40 · 48432×27948 | 0 · 4 | 5–6 | D | город The Pit (магазины, NPC), 8 внешних квест-битов |
| `7d27.blv` | Colony Zod | 5+ | 34 | 50.0 / 70 / 70 | Devil Worker/Devil Warrior/Devil Captain 30–70 (~34) | Devil Captain 700 → 2800 | 14,416 | 28 · 23677×14362 | 42 · 6 | 2–3 | C | Colony Zod: требуется предмет 1089 (реагент) и ключ 1463 с этой карты |
| `7d28.blv` | The Dragon's Lair | 5+ | 7 | 17.2 / 100 / 100 | Giant Rat/Lightning Rat/Fire Rat 2–6 (~6); размещены: Red Dragon 100×1 | Red Dragon 1300 → 5200 | 1,367 | 3 · 5120×5792 | 0 · 0 | 0–1 | A | внешних проверок не найдено |
| `7d29.blv` | Castle Harmondale | 2 | 164 | 7.6 / 17 / 24 | Giant Bat/Vampire Bat/Inferno Bat 3–9 (~10); Goblin/Hobgoblin/Goblin Lord 4–10 (~28); Giant Rat/Lightning Rat/Fire Rat 2–6 (~63); размещены: Guard 17×33, Goblin Lord 10×13, Hobgoblin 6×6, Goblin 4×6, Adventurer 24×4 | Adventurer 129 → 516 | 5,141 | 72 · 16768×9152 | 27 · 10 | 10–14 | D | замок игрока: состояние, магазины и 63 скрытых стража зависят от 19 квест-битов |
| `7d30.blv` | Castle Lambent | 5+ | 49 | 37.7 / 50 / 70 | Angel/Angel Lord/Archangel 30–70 (~24); Bronze Golem/Brass Golem/Gold Golem 22–50 (~10); Sorcerer/Wizard/Archmage 25–50 (~14); размещены: Lesser Light Elemental 15×1 | Archangel 700 → 2800 | 13,550 | 3 · 16832×13536 | 13 · 9 | 3–4 | B | Castle Lambent: замок Света, мирные NPC |
| `7d31.blv` | Fort Riverstride | 3 | 22 | 15.4 / 24 / 35 | Archer/Bowman/Elite Archer 9–29 (~8); Conscript/Soldier/Fighter 14–35 (~6); Swordsman/Guard/Adventurer 10–24 (~8) | Fighter 227 → 908 | 1,638 | 34 · 6272×4416 | 53 · 20 | 1–2 | B | квест-биты 592,594,606,612 |
| `7d32.blv` | Castle Navan | 3 | 70 | 16.1 / 23 / 35 | Dragonfly/Fire Dragonfly/Queen Dragonfly 2–8 (~6); Elven Archer/Elven Scout/Elven Ranger 11–35 (~36); Elven Warrior/Elven Defender/Elven Lancer 14–30 (~20); размещены: Elven Scout 23×8 | Elven Ranger 227 → 908 | 5,556 | 36 · 17168×16640 | 19 · 20 | 4–6 | B | замок эльфов: 18 мирных, враждебность по QBit 612; тронный зал закрыт без QBit 610 |
| `7d33.blv` | Castle Gryphonheart | 4 | 42 | 21.3 / 40 / 80 | Archer/Bowman/Elite Archer 9–29 (~12); Conscript/Soldier/Fighter 14–35 (~22); Initiate of the Sword/Champion of the Sword/Master of the Sword 40–80 (~8) | Master of the Sword 880 → 3520 | 5,231 | 44 · 11904×12480 | 12 · 7 | 3–4 | B | Castle Gryphonheart: 6 мирных, квесты Алисы; Catherine's Key на карте |
| `7d34.blv` | The Red Dwarf Mines | 4 | 39 | 24.0 / 35 / 45 | 0 (~16); Medusa/Queen Medusa/Empress Medusa 35–45 (~14); Emerald Ooze/Sapphire Ooze/Bloodstone Ooze 12–25 (~24); размещены: Empress Medusa 45×1 | Empress Medusa 337 → 1348 | 5,482 | 54 · 29722×21510 | 4 · 5 | 2–3 | C | Red Dwarf Mines: Elixir 1431 / Worn Belt 1451 — не на карте |
| `7d35.blv` | Nighon Tunnels | 3 | 184 | 17.9 / 30 / 50 | Gogling/Gog/Magog 7–18 (~90); Dwarven Soldier/Dwarven Lieutenant/Dwarven Commander 10–30 (~44); Fire Warlock/Water Warlock/Air Warlock 25–50 (~50) | Air Warlock 400 → 1600 | 17,466 | 71 · 35360×22112 | 0 · 0 | 12–15 | B | переход в 7d07, 7d24 |
| `7d36.blv` | Tunnels to Eeofol | 5+ | 148 | 46.7 / 60 / 85 | 0 (~26); Young Behemoth/Behemoth/Ancient Behemoth 45–85 (~64); Medusa/Queen Medusa/Empress Medusa 35–45 (~84) | Ancient Behemoth 977 → 3908 | 55,909 | 77 · 34688×22272 | 1 · 0 | 9–12 | B | переход в 7d07 |
| `7d37.blv` | The Haunted Mansion | 23 | 52 | 13.5 / 19 / 26 | Shade/Specter/Ghost 9–19 (~12); Ghast/Ghoul/Revenant 9–17 (~20); Wight/Wraith/Barrow Wight 14–26 (~20) | Barrow Wight 145 → 580 | 3,154 | 15 · 4608×4672 | 23 · 12 | 3–4 | A | внешних проверок не найдено |
| `7nwc.blv` | The Strange Temple | — | 0 | — | — | — | 0 | 100 · 21987×20968 | 85 · 0 | — | — | NWC easter egg |
| `d01.blv` | The Erathian Sewers | 2 | 46 | 7.4 / 8 / 18 | 0 (~8); Giant Rat/Lightning Rat/Fire Rat 2–6 (~10); Thief/Rogue/Master Thief 8–18 (~36) | Master Thief 86 → 344 | 1,289 | 55 · 13344×15392 | 30 · 4 | 3–4 | A | внешних проверок не найдено |
| `d02.blv` | The Maze | 5+ | 107 | 52.8 / 79 / 79 | Minotaur/Minotaur Headsman/Minotaur Lord 39–79 (~60); Fire Hydra/Air Hydra/Chaos Hydra 45–65 (~32); Fire Warlock/Water Warlock/Air Warlock 25–50 (~14); размещены: Minotaur Lord 79×1 | Minotaur Lord 861 → 3444 | 49,345 | 72 · 16384×24192 | 16 · 7 | 7–9 | A | внешних проверок не найдено |
| `d03.blv` | Castle Gloaming | 4 | 53 | 30.5 / 48 / 50 | Lich/Power Lich/Lich King 20–40 (~8); Necromancer/Speaker for the Dead/Queen of the Dead 25–50 (~26); Minion Vampire/Vampire/Elder Vampire 20–48 (~18); размещены: Shade 9×1 | Queen of the Dead 400 → 1600 | 10,311 | 49 · 24048×29952 | 0 · 9 | 3–4 | B | Castle Gloaming: замок Тьмы, мирные NPC |
| `d04.blv` | The Temple of Baa | 4 | 57 | 23.7 / 50 / 70 | Devil Worker/Devil Warrior/Devil Captain 30–70 (~16); Giant Spider/Venemous Spider/Widowmaker 5–12 (~6); размещены: Acolyte of the Moon 10×19, Priest of the Moon 30×8, Cleric of the Moon 20×8 | Devil Captain 700 → 2800 | 8,820 | 32 · 6336×12352 | 14 · 5 | 4–5 | A | внешних проверок не найдено |
| `mdk01.blv` | Barrow VII | 12 | 26 | 4.7 / 7 / 18 | Gogling/Gog/Magog 7–18 (~8); Giant Rat/Lightning Rat/Fire Rat 2–6 (~18) | Magog 86 → 344 | 422 | 7 · 4032×2322 | 10 · 2 | 2–2 | B | переход в mdr01, mdt01 |
| `mdk02.blv` | Barrow IV | 12 | 52 | 6.8 / 10 / 14 | Giant Bat/Vampire Bat/Inferno Bat 3–9 (~28); Skeleton/Skeleton Warrior/Skeleton Lord 6–14 (~24) | Skeleton Lord 61 → 244 | 1,331 | 10 · 2944×7056 | 6 · 3 | 3–4 | B | квест-биты 704; переход в 7d13, mdk05, mdr02, mdr05 |
| `mdk03.blv` | Barrow II | 2 | 60 | 7.6 / 13 / 17 | Giant Rat/Lightning Rat/Fire Rat 2–6 (~30); Rotted Corpse/Walking Dead/Zombie 9–17 (~30) | Zombie 79 → 316 | 1,827 | 10 · 7680×3632 | 6 · 3 | 4–5 | B | квест-биты 704; переход в mdk04, mdr03, mdr04, mdt05 |
| `mdk04.blv` | Barrow XIV | 2 | 32 | 7.5 / 13 / 17 | Giant Bat/Vampire Bat/Inferno Bat 3–9 (~18); Ghast/Ghoul/Revenant 9–17 (~14) | Revenant 79 → 316 | 928 | 6 · 3744×2944 | 4 · 0 | 2–3 | B | квест-биты 704; переход в mdk03, mdr02, mdr05, mdt04 |
| `mdk05.blv` | Barrow III | 2 | 28 | 6.5 / 13 / 17 | Ghast/Ghoul/Revenant 9–17 (~12); Giant Rat/Lightning Rat/Fire Rat 2–6 (~16) | Revenant 79 → 316 | 698 | 7 · 3584×2064 | 4 · 0 | 2–2 | B | квест-биты 704; переход в mdk02, mdr01, mdr02, mdt04 |
| `mdr01.blv` | Barrow X | 12 | 10 | 5.1 / 7 / 18 | Gogling/Gog/Magog 7–18 (~4); Giant Rat/Lightning Rat/Fire Rat 2–6 (~6) | Magog 86 → 344 | 178 | 9 · 6206×3410 | 5 · 2 | 1–1 | B | переход в mdk01, mdk05 |
| `mdr02.blv` | Barrow XII | 12 | 30 | 6.8 / 10 / 14 | Giant Bat/Vampire Bat/Inferno Bat 3–9 (~16); Skeleton/Skeleton Warrior/Skeleton Lord 6–14 (~14) | Skeleton Lord 61 → 244 | 771 | 5 · 3872×4480 | 6 · 0 | 2–2 | B | квест-биты 705; переход в mdk02, mdk04, mdk05, mdr03 |
| `mdr03.blv` | Barrow V | 2 | 44 | 6.3 / 14 / 14 | Giant Rat/Lightning Rat/Fire Rat 2–6 (~20); Skeleton/Skeleton Warrior/Skeleton Lord 6–14 (~24) | Skeleton Lord 61 → 244 | 1,055 | 6 · 5504×3792 | 4 · 0 | 3–4 | B | квест-биты 705; переход в mdk03, mdr02, mdt01, mdt03 |
| `mdr04.blv` | Barrow XI | 2 | 34 | 7.5 / 13 / 17 | Giant Bat/Vampire Bat/Inferno Bat 3–9 (~22); Rotted Corpse/Walking Dead/Zombie 9–17 (~12) | Zombie 79 → 316 | 989 | 8 · 5776×4992 | 5 · 0 | 2–3 | B | квест-биты 705; переход в mdk03, mdt02, mdt03, mdt05 |
| `mdr05.blv` | Barrow XV | 2 | 8 | 5.5 / 13 / 17 | Giant Rat/Lightning Rat/Fire Rat 2–6 (~6); Rotted Corpse/Walking Dead/Zombie 9–17 (~2) | Zombie 79 → 316 | 166 | 5 · 5824×3600 | 7 · 0 | 0–1 | B | квест-биты 705; переход в mdk02, mdk04, mdt03, mdt04 |
| `mdt01.blv` | Barrow IX | 12 | 4 | 6.3 / 9 / 18 | Gogling/Gog/Magog 7–18 (~2); Giant Bat/Vampire Bat/Inferno Bat 3–9 (~2) | Magog 86 → 344 | 91 | 7 · 3168×2962 | 5 · 2 | 0–0 | B | переход в mdk01, mdr03 |
| `mdt02.blv` | Barrow VI | 2 | 6 | 7.1 / 14 / 26 | Giant Rat/Lightning Rat/Fire Rat 2–6 (~4); Wight/Wraith/Barrow Wight 14–26 (~2) | Barrow Wight 145 → 580 | 175 | 6 · 5440×4272 | 7 · 0 | 0–0 | B | квест-биты 703; переход в 7d13, mdr04, mdt03, mdt05 |
| `mdt03.blv` | Barrow I | 2 | 8 | 9.9 / 14 / 26 | Giant Bat/Vampire Bat/Inferno Bat 3–9 (~4); Wight/Wraith/Barrow Wight 14–26 (~4) | Barrow Wight 145 → 580 | 332 | 11 · 6640×3952 | 6 · 0 | 0–1 | B | квест-биты 703; переход в mdr03, mdr04, mdr05, mdt02 |
| `mdt04.blv` | Barrow VIII | 2 | 10 | 5.6 / 14 / 26 | Giant Rat/Lightning Rat/Fire Rat 2–6 (~8); Wight/Wraith/Barrow Wight 14–26 (~2) | Barrow Wight 145 → 580 | 219 | 9 · 4016×4448 | 7 · 0 | 1–1 | B | квест-биты 703; переход в mdk04, mdk05, mdr05, mdt05 |
| `mdt05.blv` | Barrow XIII | 12 | 14 | 5.9 / 9 / 17 | Giant Bat/Vampire Bat/Inferno Bat 3–9 (~12); Ghast/Ghoul/Revenant 9–17 (~2) | Revenant 79 → 316 | 300 | 6 · 4032×2800 | 5 · 2 | 1–1 | B | квест-биты 703; переход в mdk03, mdr04, mdt02, mdt04 |
| `mdt09.blv` | Wromthrax's Cave | 5+ | 1 | 90.0 / 90 / 90 | размещены: Blue Dragon 90×1 | Blue Dragon 1080 → 4320 | 1,080 | 2 · 22784×16384 | 0 · 0 | 0–0 | B | квест-биты 534,1684 |
| `mdt10.blv` | William Setag's Tower | ? | 8 | 13.2 / 40 / 80 | Initiate of the Sword/Champion of the Sword/Master of the Sword 40–80 (~2); Giant Rat/Lightning Rat/Fire Rat 2–6 (~6) | Master of the Sword 880 → 3520 | 692 | 5 · 3232×3104 | 4 · 1 | 0–1 | C | William's Tower: ключ 1461 у монстра карты, но квест Алисы; 8 монстров |
| `mdt11.blv` | The Hidden Tomb | 4 | 4 | 28.2 / 40 / 48 | 0 (~4); Lich/Power Lich/Lich King 20–40 (~2); Minion Vampire/Vampire/Elder Vampire 20–48 (~2) | Elder Vampire 374 → 1496 | 694 | 8 · 3744×4480 | 0 · 6 | 0–0 | A | внешних проверок не найдено |
| `mdt12.blv` | The Dragon Caves | 5+ | 9 | 93.8 / 100 / 100 | 0 (~12); Green Dragon/Blue Dragon/Red Dragon 80–100 (~8); размещены: Mega-Dragon 100×1 | Red Dragon 1300 → 5200 | 10,484 | 11 · 30030×28780 | 0 · 3 | 1–1 | C | Dragon Egg 1449 на карте; 9 драконов 90–100 |
| `mdt14.blv` | The Bandit Caves | 2 | 12 | 9.1 / 12 / 22 | Robber/Raider/Bandit 8–22 (~8); Thief/Rogue/Master Thief 8–18 (~4) | Bandit 114 → 456 | 432 | 9 · 3672×2992 | 0 · 3 | 1–1 | B | квест-биты 669 |
| `mdt15.blv` | The Small House | 5+ | 2 | 80.0 / 80 / 80 | Tolberti/Robert the Wise 80–80 (~2) | Tolberti 880 → 3520 | 1,760 | 27 · 20224×4928 | 0 · 2 | 0–0 | B | переход в 7d25, 7d26 |
| `t01.blv` | Temple of the Light | 4 | 48 | 25.5 / 50 / 70 | Angel/Angel Lord/Archangel 30–70 (~14); Acolyte of the Sun/Cleric of the Sun/Priest of the Sun 10–30 (~22); Monk/Initiate/Master 8–27 (~12) | Archangel 700 → 2800 | 8,000 | 66 · 14592×22464 | 15 · 4 | 3–4 | B | квест-биты 611; переход в 7d25; нет готовой точки входа |
| `t02.blv` | Temple of the Dark | 4 | 54 | 22.6 / 33 / 48 | Acolyte of the Moon/Cleric of the Moon/Priest of the Moon 10–30 (~28); Minion Vampire/Vampire/Elder Vampire 20–48 (~26) | Elder Vampire 374 → 1496 | 6,837 | 47 · 6912×13312 | 10 · 20 | 3–4 | B | квест-биты 612; переход в 7d26; нет готовой точки входа |
| `t03.blv` | Grand Temple of the Sun | 3 | 32 | 15.6 / 30 / 30 | Acolyte of the Sun/Cleric of the Sun/Priest of the Sun 10–30 (~24); Monk/Initiate/Master 8–27 (~8) | Priest of the Sun 180 → 720 | 2,471 | 16 · 5344×9088 | 16 · 4 | 2–3 | B | квест-биты 556 |
| `t04.blv` | The Hall of the Pit | 3 | 28 | 15.7 / 19 / 33 | Stone Gargoyle/Marble Gargoyle/Obsidian Gargoyle 16–33 (~8); Harpy/Harpy Hag/Harpy Queen 14–19 (~10); Rotted Corpse/Walking Dead/Zombie 9–17 (~10) | Obsidian Gargoyle 207 → 828 | 2,047 | 10 · 6016×19456 | 3 · 3 | 2–2 | B | квест-биты 611,612,707; переход в 7d26 |

#### MM8

| Карта | Название | Этап | Монстров (E) | Ур. ср. / p90 / max | Состав (ожид. число) | Сильнейший → страж ×4 | HP всего | Секторы · габарит XY | Двери · сундуки | Мин. до стража* | Класс | Зависимости / заметки |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| `d05.blv` | Abandoned Temple | 1 | 116 | 4.2 / 7 / 11 | Young Couatl/Couatl/Winged Serpent 1–4 (~62); Serpentman/Serpentman Elder/Serpentman Warrior 5–11 (~54) | Serpentman Warrior 45 → 180 | 1,719 | 33 · 19456×8724 | 112 · 8 | 7–10 | A | QBit 403 — только реплика NPC при наличии героя ростера |
| `d06.blv` | Pirate Outpost | ? | 35 | 12.0 / 31 / 75 | Regnan Crossbowman/Regnan Arbalester/Regnan Lieutenant 18–50 (~10); Regnan Bandit/Regnan Pirate/Regnan Brigadier 18–50 (~4); Pirate Seer/Pirate Magi/Pirate Wizard 1–3 (~20); размещены: Regnan Archmage 75×1 | Regnan Archmage 787 → 3148 | 2,613 | 13 · 9029×6992 | 13 · 7 | 2–3 | B | ключ 619 у главаря пиратов этой карты; переход в d34 |
| `d07.blv` | Smuggler's Cove | 2 | 72 | 10.0 / 13 / 21 | Animalist/Shapeshifter/Lycanthrope 10–17 (~34); Bestial Animalist/Bestial Shapeshifter/Bestial Lycanthrope 13–21 (~10); Ratman/Wererat/Greater Wererat 8–15 (~28) | Bestial Lycanthrope 107 → 428 | 2,901 | 17 · 22938×18776 | 9 · 15 | 4–6 | B | квест-биты 10 |
| `d08.blv` | Dire Wolf Den | 2 | 28 | 11.5 / 19 / 19 | Dire Wolf Yearling/Dire Wolf/Pack Leader 9–19 (~18); Ratman/Wererat/Greater Wererat 8–15 (~10) | Pack Leader 93 → 372 | 1,383 | 15 · 5386×7218 | 1 · 1 | 2–2 | A | внешних проверок не найдено |
| `d09.blv` | Merchant House of Alvar | - | 0 | — | — | — | 0 | 8 · 3456×4416 | 5 · 0 | — | D | нет враждебных монстров |
| `d10.blv` | Escaton's Crystal | 5+ | 42 | 100.3 / 125 / 125 | Emerald Dragon/Sapphire Dragon/Ruby Dragon 75–125 (~28); Crystal Walker/Crystal Sentry/Crystal Guardian 60–100 (~14) | Ruby Dragon 1937 → 7748 | 56,294 | 16 · 17191×9820 | 1 · 1 | 3–4 | A | внешних проверок не найдено |
| `d11.blv` | Wasp Nest | 3 | 142 | 16.4 / 18 / 23 | Wasp Worker/Wasp Sentry/Wasp Warrior 15–23 (~142) | Wasp Warrior 121 → 484 | 10,820 | 13 · 4608×6912 | 0 · 0 | 9–12 | A | внешних проверок не найдено |
| `d12.blv` | Ogre Fortress | 34 | 68 | 19.0 / 28 / 35 | Soldier of Fortune/Mercenary/Mercenary Captain 10–24 (~32); Ogre Mage Apprentice/Ogre Mage/Ogre Magi 20–35 (~20); Ogre Brawler/Ogre Warrior/Ogre Warleader 15–28 (~16) | Ogre Magi 227 → 908 | 6,604 | 24 · 14816×9660 | 4 · 2 | 4–6 | A | ключи тюрьмы 620/621 у монстров карты |
| `d13.blv` | Troll Tomb | 23 | 42 | 12.7 / 19 / 19 | 0 (~2); Smoke Gog/Ember Gog/Fire Gog 10–19 (~42) | Fire Gog 93 → 372 | 2,342 | 38 · 6432×7520 | 136 · 0 | 3–4 | B | Urn of Ashes 603 (не на карте) — только упокоение Vilebite, текст |
| `d14.blv` | Cyclops Larder | 4 | 32 | 24.6 / 48 / 48 | Human Peasant/Human Peasant/Human Peasant 1–3 (~8); Cyclops/Cyclops Hunter/Cyclops Warrior 24–48 (~24) | Cyclops Warrior 374 → 1496 | 5,083 | 17 · 6016×7808 | 10 · 4 | 2–3 | C | Cyclops Larder: Sarcophagus of Korbu 612 из d26 |
| `d15.blv` | Chain of Fire | 4 | 14 | 30.8 / 69 / 69 | Smoke Gog/Ember Gog/Fire Gog 10–19 (~8); Fire Newt/Fire Lizard/Salamander 29–69 (~6) | Salamander 683 → 2732 | 3,344 | 10 · 13224×6404 | 0 · 1 | 1–1 | A | внешних проверок не найдено |
| `d16.blv` | Dragon Hunter's Camp | 4 | 30 | 23.0 / 45 / 45 | Dragon Hunter/Crusader/Dragonslayer 15–45 (~16); Soldier of Fortune/Mercenary/Mercenary Captain 10–24 (~14) | Dragonslayer 337 → 1348 | 4,064 | 17 · 6784×8672 | 1 · 9 | 2–2 | B | лагерь охотников на драконов: союз с драконами/охотниками (QBit 21/22) |
| `d17.blv` | Dragon Cave | 5+ | 38 | 57.3 / 100 / 100 | Dragon/Dragon Flightleader/Great Wyrm 70–100 (~24); Hatchling/Dragonette/Young Dragon 15–60 (~14) | Great Wyrm 1300 → 5200 | 22,924 | 10 · 19840×22528 | 0 · 0 | 2–3 | B | пещера драконов: союз (QBit 21/22) решает враждебность |
| `d18.blv` | Naga Vault | 3 | 22 | 16.0 / 28 / 38 | 0 (~2); Naga/Naga Warrior/Naga Queen 20–38 (~12); Serpentman/Serpentman Elder/Serpentman Warrior 5–11 (~10) | Naga Queen 258 → 1032 | 1,852 | 15 · 8146×3888 | 14 · 2 | 1–2 | A | внешних проверок не найдено |
| `d19.blv` | Necromancers' Guild | 4 | 82 | 25.1 / 30 / 57 | Dark Path Journeyman/Necromancer/Master Necromancer 30–57 (~46); Skeleton Bowman/Skeleton Hunter/Skeleton Archer 11–17 (~18); Vampire Minion/Vampire/Greater Vampire 20–40 (~18) | Master Necromancer 495 → 1980 | 11,959 | 28 · 6732×14360 | 28 · 12 | 5–7 | C | Necromancers' Guild: Nightshade Brazier 604 из d22; фракционные квест-биты |
| `d20.blv` | Mad Necromancer's Lab | 4 | 52 | 34.7 / 57 / 60 | Skeletal Dragon/Bone Dragon/Undead Dragon 30–60 (~24); Dark Path Journeyman/Necromancer/Master Necromancer 30–57 (~18); Skeleton Bowman/Skeleton Hunter/Skeleton Archer 11–17 (~10) | Undead Dragon 540 → 2160 | 12,759 | 28 · 9440×11712 | 11 · 5 | 3–4 | B | Ebonest 539 лежит на карте; QBit 71 — квест героя ростера |
| `d21.blv` | Vampire Crypt | 4 | 18 | 26.4 / 40 / 40 | 0 (~6); Vampire Minion/Vampire/Greater Vampire 20–40 (~18) | Greater Vampire 280 → 1120 | 2,783 | 6 · 8000×9088 | 3 · 4 | 1–2 | A | внешних проверок не найдено |
| `d22.blv` | Temple of the Sun | 3 | 38 | 17.0 / 30 / 30 | Acolyte of the Sun/Cleric of the Sun/Priest of the Sun 10–30 (~38) | Priest of the Sun 180 → 720 | 3,268 | 6 · 4632×2872 | 4 · 0 | 2–3 | B | Temple of the Sun: фракционные квест-биты (19/20/28) меняют враждебность |
| `d23.blv` | Druid Circle | 5+ | 56 | 25.4 / 45 / 85 | Dark Dwarf Grunt/Dark Dwarf Pounder/Dark Dwarf Berserker 17–31 (~12); Charger/Juggernaut/Demolisher 45–85 (~12); Boulder/Boulder/Boulder 12–35 (~32) | Demolisher 977 → 3908 | 9,370 | 20 · 15050×14840 | 5 · 6 | 4–5 | A | внешних проверок не найдено |
| `d24.blv` | Balthazar Lair | 4 | 58 | 24.5 / 30 / 45 | Triton/Triton Warrior/Triton Crusader 20–45 (~58) | Triton Crusader 337 → 1348 | 8,103 | 31 · 8096×8832 | 13 · 0 | 4–5 | B | квест-биты 23 |
| `d25.blv` | Barbarian Fortress | 34 | 53 | 18.7 / 24 / 35 | Soldier of Fortune/Mercenary/Mercenary Captain 10–24 (~10); Ogre Mage Apprentice/Ogre Mage/Ogre Magi 20–35 (~22); Ogre Brawler/Ogre Warrior/Ogre Warleader 15–28 (~20); размещены: Ogre Magi 35×1 | Ogre Magi 227 → 908 | 4,949 | 18 · 5696×8432 | 9 · 10 | 3–4 | A | Dragon Leader's Egg 605 на карте |
| `d26.blv` | The Crypt of Korbu | 4 | 20 | 36.0 / 65 / 65 | Efreeti Soldier/Efreeti Knight/Efreeti Lord 45–65 (~8); Naga/Naga Warrior/Naga Queen 20–38 (~12) | Efreeti Lord 617 → 2468 | 5,322 | 9 · 8960×8880 | 6 · 0 | 1–2 | C | Crypt of Korbu: Remains of Korbu 627 из d14 |
| `d27.blv` | Castle of Air | 5+ | 44 | 65.0 / 80 / 80 | Lesser Air Elemental/Air Elemental/Greater Air Elemental 50–80 (~44) | Greater Air Elemental 880 → 3520 | 27,755 | 18 · 5632×7268 | 11 · 13 | 3–4 | A | Heart of Air на карте |
| `d28.blv` | Tomb of Lord Brinne | 2 | 8 | 8.0 / 8 / 8 | размещены: Dark Elf Peasant 8×8 | Dark Elf Peasant 30 → 120 | 240 | 5 · 2656×2048 | 0 · 0 | 0–1 | A | внешних проверок не найдено |
| `d29.blv` | Castle of Fire | 5+ | 56 | 65.0 / 80 / 80 | Lesser Fire Elemental/Fire Elemental/Greater Fire Elemental 50–80 (~56) | Greater Fire Elemental 880 → 3520 | 35,325 | 15 · 6304×6328 | 8 · 5 | 4–5 | A | Heart of Fire на карте |
| `d30.blv` | War Camp | 4 | 26 | 35.4 / 65 / 79 | Efreeti Soldier/Efreeti Knight/Efreeti Lord 45–65 (~12); Smoke Gog/Ember Gog/Fire Gog 10–19 (~12); Fire Bird/Phoenix/Grand Phoenix 39–79 (~2) | Grand Phoenix 861 → 3444 | 7,194 | 15 · 2304×2304 | 7 · 0 | 2–2 | A | внешних проверок не найдено |
| `d31.blv` | Pirate Stronghold | 4 | 36 | 28.0 / 50 / 50 | Soldier of Fortune/Mercenary/Mercenary Captain 10–24 (~10); Regnan Crossbowman/Regnan Arbalester/Regnan Lieutenant 18–50 (~8); Regnan Bandit/Regnan Pirate/Regnan Brigadier 18–50 (~18) | Regnan Lieutenant 400 → 1600 | 6,374 | 16 · 6324×5144 | 12 · 9 | 2–3 | B | переход в d33 |
| `d32.blv` | Abandoned Pirate Keep | 5+ | 54 | 38.9 / 75 / 75 | Ogre Mage Apprentice/Ogre Mage/Ogre Magi 20–35 (~14); Regnan Sorcerer/Regnan Battlemage/Regnan Archmage 25–75 (~20); Regnan Bandit/Regnan Pirate/Regnan Brigadier 18–50 (~20) | Regnan Archmage 787 → 3148 | 16,022 | 22 · 9694×8477 | 24 · 16 | 3–4 | B | Cannonball of Dominion 662 на карте; квест флота (QBit 36) |
| `d33.blv` | Passage Under Regna | 4 | 80 | 31.8 / 75 / 75 | Ogre Brawler/Ogre Warrior/Ogre Warleader 15–28 (~26); Regnan Sorcerer/Regnan Battlemage/Regnan Archmage 25–75 (~28); Regnan Bandit/Regnan Pirate/Regnan Brigadier 18–50 (~26) | Regnan Archmage 787 → 3148 | 18,774 | 15 · 14944×24160 | 0 · 0 | 5–7 | B | переход в d31, d32 |
| `d34.blv` | Small Sub Pen | 4 | 10 | 30.7 / 50 / 75 | Regnan Crossbowman/Regnan Arbalester/Regnan Lieutenant 18–50 (~4); Regnan Sorcerer/Regnan Battlemage/Regnan Archmage 25–75 (~2); Regnan Bandit/Regnan Pirate/Regnan Brigadier 18–50 (~4) | Regnan Archmage 787 → 3148 | 2,178 | 6 · 3808×3744 | 0 · 0 | 1–1 | C | Small Sub Pen: Pirate Leader's Key 619 из d06 |
| `d35.blv` | Escaton's Palace | 5+ | 36 | 69.5 / 90 / 90 | 0 (~4); Ether Knight/Ether Champion/Ether Lord 40–71 (~16); Chaos Guardian/Chaos Protector/Chaos Overlord 50–90 (~20) | Chaos Overlord 1080 → 4320 | 25,557 | 34 · 11520×11104 | 64 · 5 | 2–3 | A | внешних проверок не найдено |
| `d36.blv` | Prison of the Lord of Air | 5+ | 26 | 71.5 / 100 / 100 | Crystal Walker/Crystal Sentry/Crystal Guardian 60–100 (~8); Lesser Earth Elemental/Earth Elemental/Greater Earth Elemental 50–80 (~8); Ether Knight/Ether Champion/Ether Lord 40–71 (~10) | Crystal Guardian 1300 → 5200 | 19,414 | 10 · 12384×15296 | 4 · 2 | 2–2 | A | внешних проверок не найдено |
| `d37.blv` | Prison of the Lord of Fire | 5+ | 22 | 83.8 / 125 / 125 | Emerald Dragon/Sapphire Dragon/Ruby Dragon 75–125 (~10); Ether Knight/Ether Champion/Ether Lord 40–71 (~8); Lesser Water Elemental/Water Elemental/Greater Water Elemental 50–80 (~4) | Ruby Dragon 1937 → 7748 | 22,395 | 3 · 3840×5696 | 5 · 0 | 1–2 | A | внешних проверок не найдено |
| `d38.blv` | Prison of the Lord of Water | 5+ | 26 | 68.5 / 80 / 90 | Ether Knight/Ether Champion/Ether Lord 40–71 (~8); Lesser Fire Elemental/Fire Elemental/Greater Fire Elemental 50–80 (~12); Fear/Terror/Nightmare 50–90 (~6) | Nightmare 1080 → 4320 | 17,896 | 12 · 7809×9516 | 0 · 5 | 2–2 | A | внешних проверок не найдено |
| `d39.blv` | Prison of the Lord of Earth | 5+ | 32 | 73.6 / 90 / 90 | Lesser Air Elemental/Air Elemental/Greater Air Elemental 50–80 (~12); Fear/Terror/Nightmare 50–90 (~10); Plane Guardian/Plane Protector/Plane Overlord 50–90 (~10) | Nightmare 1080 → 4320 | 24,846 | 5 · 6192×6192 | 1 · 0 | 2–3 | A | внешних проверок не найдено |
| `d40.blv` | Uplifted Library | 4 | 18 | 24.0 / 45 / 45 | Boulder/Boulder/Boulder 12–35 (~8); Triton/Triton Warrior/Triton Crusader 20–45 (~10) | Triton Crusader 337 → 1348 | 2,517 | 9 · 2048×1218 | 5 · 1 | 1–2 | A | Lost Book of Kehl 611 на карте |
| `d41.blv` | Dark Dwarf Compound | 34 | 42 | 22.1 / 31 / 35 | Dark Dwarf Grunt/Dark Dwarf Pounder/Dark Dwarf Berserker 17–31 (~26); Boulder/Boulder/Boulder 12–35 (~16) | Boulder 227 → 908 | 5,038 | 33 · 12192×9280 | 24 · 20 | 3–4 | A | Axe of Balthazar 541 на карте |
| `d42.blv` | Arena | — | 0 | — | — | — | 0 | 2 · 8704×8640 | 6 · 0 | — | — | arena |
| `d43.blv` | Ancient Troll Home | 4 | 28 | 24.1 / 38 / 38 | Young Basilisk/Basilisk/Elder Basilisk 20–38 (~16); Dancing Light/Wisp/Will O' Wisp 15–38 (~12) | Elder Basilisk 258 → 1032 | 3,817 | 13 · 6208×9504 | 0 · 4 | 2–2 | A | внешних проверок не найдено |
| `d44.blv` | Grand Temple of Eep | 3 | 48 | 14.3 / 17 / 21 | Animalist/Shapeshifter/Lycanthrope 10–17 (~24); Bestial Animalist/Bestial Shapeshifter/Bestial Lycanthrope 13–21 (~8); Ratman/Wererat/Greater Wererat 8–15 (~16) | Bestial Lycanthrope 107 → 428 | 3,054 | 27 · 6720×10269 | 22 · 11 | 3–4 | A | внешних проверок не найдено |
| `d45.blv` | Chapel of Eep | 2 | 28 | 9.2 / 10 / 17 | Animalist/Shapeshifter/Lycanthrope 10–17 (~12); Ratman/Wererat/Greater Wererat 8–15 (~16) | Lycanthrope 79 → 316 | 1,015 | 12 · 7392×7840 | 5 · 12 | 2–2 | A | внешних проверок не найдено |
| `d46.blv` | Church of Eep | 23 | 16 | 13.1 / 17 / 21 | Animalist/Shapeshifter/Lycanthrope 10–17 (~6); Bestial Animalist/Bestial Shapeshifter/Bestial Lycanthrope 13–21 (~6); Ratman/Wererat/Greater Wererat 8–15 (~4) | Bestial Lycanthrope 107 → 428 | 917 | 3 · 4288×1792 | 10 · 1 | 1–1 | A | внешних проверок не найдено |
| `d47.blv` | Old Loeb's Cave | 5+ | 14 | 69.8 / 100 / 100 | Dragon/Dragon Flightleader/Great Wyrm 70–100 (~10); Hatchling/Dragonette/Young Dragon 15–60 (~4) | Great Wyrm 1300 → 5200 | 10,376 | 9 · 7360×12768 | 0 · 0 | 1–1 | A | внешних проверок не найдено |
| `d48.blv` | Ilsingore's Cave | 5+ | 10 | 65.5 / 100 / 100 | Dragon/Dragon Flightleader/Great Wyrm 70–100 (~6); Hatchling/Dragonette/Young Dragon 15–60 (~4) | Great Wyrm 1300 → 5200 | 6,763 | 4 · 10556×17624 | 0 · 1 | 1–1 | A | внешних проверок не найдено |
| `d49.blv` | Yaardrake's Cave | 5+ | 10 | 65.5 / 100 / 100 | Dragon/Dragon Flightleader/Great Wyrm 70–100 (~6); Hatchling/Dragonette/Young Dragon 15–60 (~4) | Great Wyrm 1300 → 5200 | 6,763 | 3 · 14912×9556 | 0 · 1 | 1–1 | A | внешних проверок не найдено |
| `d50.blv` | NWC | — | 80 | 32.7 / 31 / 75 | размещены: Regnan Pirate 31×63, Regnan Arbalester 31×11, Regnan Battlemage 50×5, Regnan Archmage 75×1 | Regnan Archmage 787 → 3148 | 16,773 | 81 · 21987×16200 | 86 · 0 | 5–7 | — | NWC easter egg |
| `eleme.blv` | Plane of Earth | 5+ | 96 | 52.8 / 80 / 85 | Lesser Earth Elemental/Earth Elemental/Greater Earth Elemental 50–80 (~54); Charger/Juggernaut/Demolisher 45–85 (~26); Boulder/Boulder/Boulder 12–35 (~16) | Demolisher 977 → 3908 | 45,579 | 34 · 9616×15288 | 5 · 11 | 6–8 | A | Heart of Earth на карте |

#### MM9

| Карта | Название | Этап | Монстров (E) | Ур. ср. / p90 / max | Состав (ожид. число) | Сильнейший → страж ×4 | HP всего | Секторы · габарит XY | Двери · сундуки | Мин. до стража* | Класс | Зависимости / заметки |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| `1000terrors.blv` | Thousand Terrors | 5+ | 205 | 46.2 / 70 / 75 | размещены: Orbus 45×59, Eye 35×38, Lesser Demon 70×35, Oculus 50×24, Flying Icky 11×19 … | Greater Demon 962 → 3848 | 95,685 | 20 · 30280×30167 | 137 · 14 | 13–17 | B | ключи MM9 108; переход в hallofthegods |
| `afterworld.blv` | The Afterworld | — | 1 | 50.0 / 50 / 50 | размещены: Forad Darre 50×1 | Forad Darre 525 → 2100 | 525 | 2 · 28508×23593 | 0 · 0 | 0–0 | — | open-air region/city |
| `anskramkeep.blv` | Anskram Keep | 12 | 86 | 6.3 / 10 / 14 | размещены: Nagate Hatchling 6×30, Imply 4×24, Imp Elder 10×14, Imp 6×11, Airborne Icky 7×5 … | Nagate Elder 72 → 288 | 2,263 | 48 · 13803×16711 | 103 · 7 | 5–7 | A | внешних проверок не найдено |
| `arslegardcity.blv` | Arslegard City | — | 1 | 200.0 / 200 / 200 | размещены: Njam the Meddler 200×1 | Njam the Meddler 6600 → 26400 | 6,600 | 63 · 23920×28754 | 30 · 0 | 0–0 | — | open-air region/city |
| `bathhouse.blv` | The Bathhouse | 5+ | 35 | 38.4 / 60 / 70 | размещены: Jellyspore 15×17, Colloidal Warrior 60×12, Concubine 60×3, Colloidal Soldier 50×1, Colloidal Guardian 70×1 … | Colloidal Guardian 850 → 3400 | 13,802 | 21 · 20248×8581 | 20 · 4 | 2–3 | B | ключи MM9 348 |
| `beethoven.blv` | Beet Hoven | 2 | 156 | 6.2 / 16 / 16 | размещены: Bone Thrasher 3×80, Lobber 7×26, Skeleton 16×18, Rotter 9×18, Airborne Icky 7×14 | Skeleton 87 → 348 | 4,326 | 40 · 23710×49864 | 34 · 11 | 10–13 | B | ключи MM9 14,16,110 |
| `bootcamp.blv` | Ravensford | — | 5 | 1.2 / 2 / 2 | размещены: Lobber Pod 2×5 | Lobber Pod 7 → 28 | 23 | 20 · 45967×38421 | 23 · 0 | 0–0 | — | open-air region/city |
| `chasmofthedead.blv` | Chasm of the Dead | 4 | 239 | 24.9 / 33 / 40 | размещены: Skeleton Master 33×66, Skeleton Warrior 22×60, Skeleton 16×41, Embalmed Mummy 30×20, Petrified Mummy 40×19 … | Petrified Mummy 360 → 1440 | 42,840 | 63 · 28795×48005 | 93 · 16 | 15–20 | B | ключи MM9 46 |
| `connectingtunnels.blv` | Connecting Tunnel | 5+ | 87 | 39.4 / 79 / 79 | размещены: Skeleton Warrior 22×24, Spectre Dagrell 79×13, Annelid 33×12, Skeleton Master 33×12, Bloodsucker 20×10 … | Spectre Dagrell 1057 → 4228 | 34,035 | 32 · 40182×27566 | 2 · 2 | 5–7 | A | внешних проверок не найдено |
| `darkpassageway.blv` | Dark Passageway | 5+ | 94 | 54.5 / 60 / 70 | размещены: Specter 55×39, Reptilian Terror 60×19, Desert Terror 50×16, Apparition 44×6, Amphibious Terror 45×6 … | Sea Monster Female 850 → 3400 | 54,589 | 28 · 24492×30843 | 41 · 7 | 6–8 | C | Capstone of Order 10396 (из guberland) для прохода |
| `dookscastle.blv` | Dook's Castle | 4 | 56 | 34.2 / 50 / 51 | размещены: Half Orc Captain 35×17, Kin Trellborg 35×11, Trellborg 22×9, Ancient Trellborg 50×7, Half Orc Soldier 24×4 … | Winged Aberration 517 → 2068 | 16,030 | 12 · 20972×10957 | 16 · 7 | 4–5 | A | внешних проверок не найдено |
| `dragonstadium.blv` | Dragon's Lair | 5+ | 1 | 500.0 / 500 / 500 | размещены: Dragon King 500×1 | Dragon King 35100 → 140400 | 35,100 | 4 · 12616×14500 | 0 · 2 | 0–0 | D | один Dragon King: 35 100 HP, ур. 500 — финальный бой MM9 |
| `drangheim.blv` | Drangheim | — | 32 | 10.1 / 18 / 22 | размещены: Thrall 12×9, Field Thrall 8×8, Fire Dragonfly 5×7, Cutpurse 8×4, Bandit 22×3 … | Bandit 139 → 556 | 1,583 | 2 · 65024×63942 | 3 · 4 | 2–3 | — | open-air region/city |
| `drangheimcity.blv` | Drangheim City | — | 0 | — | — | — | 0 | 38 · 16712×19005 | 33 · 0 | — | — | open-air region/city |
| `drangheimprison.blv` | Drangheim Prison | 2 | 50 | 7.5 / 10 / 14 | размещены: Nagate Hatchling 6×34, Nagate 10×13, Nagate Elder 14×3 | Nagate Elder 72 → 288 | 1,617 | 92 · 16015×11592 | 106 · 6 | 3–4 | B | тревога/пропуск посетителя (ключи 5006/5007) меняют поведение стражи |
| `dungeonofsecrets.blv` | Dungeon of Secrets | - | 0 | — | — | — | 0 | 11 · 10486×7496 | 158 · 0 | — | D | нет враждебных монстров |
| `frosgard.blv` | Frosgard | — | 49 | 28.0 / 40 / 40 | размещены: Yeti 38×11, Gezzampt 20×11, Grey Wolf 20×8, Black Wolf 40×6, Ice Lobber 20×5 … | Black Wolf 360 → 1440 | 10,389 | 2 · 65024×61176 | 0 · 2 | 3–4 | — | open-air region/city |
| `frosgardcity.blv` | Frosgard City | — | 0 | — | — | — | 0 | 13 · 63969×43254 | 72 · 0 | — | — | open-air region/city |
| `guberland.blv` | Guberland | — | 40 | 10.4 / 22 / 25 | размещены: Fire Dragonfly 5×14, Bandit 22×8, Cutpurse 8×8, Dragonfly 3×6, Nettle Spore 25×3 … | Nettle Spore 169 → 676 | 2,302 | 2 · 57575×65024 | 4 · 0 | 2–3 | — | open-air region/city |
| `guberlandcity.blv` | Guberland City | — | 0 | — | — | — | 0 | 50 · 17039×20972 | 76 · 0 | — | — | open-air region/city |
| `hallofthegods.blv` | Hall of the Gods | - | 0 | — | — | — | 0 | 2 · 20316×12203 | 0 · 0 | — | D | нет враждебных монстров |
| `inventastorca.blv` | Inventa Storca | 4 | 22 | 32.8 / 45 / 65 | размещены: Lich 45×10, Airborne Icky 7×4, Bandit 22×3, Skeleton 16×3, Lich King 65×2 | Lich King 746 → 2984 | 6,456 | 27 · 13189×9298 | 82 · 5 | 1–2 | A | внешних проверок не найдено |
| `isleofashes.blv` | Isle of Ashes | — | 74 | 5.8 / 15 / 65 | размещены: Dragonfly Mite 2×21, Bone Thrasher 3×19, Skeletoid 3×14, Jellyspore 15×10, Dragonfly 3×3 … | Lich King 746 → 2984 | 2,637 | 2 · 65024×65024 | 4 · 3 | 5–6 | — | open-air region/city |
| `klusoshouse.blv` | Kluso's House | 4 | 20 | 34.6 / 33 / 65 | размещены: Guard Captain 33×19, Count 65×1 | Count 746 → 2984 | 5,743 | 11 · 3054×4487 | 26 · 1 | 1–2 | A | внешних проверок не найдено |
| `lichlab.blv` | Lich Lab | 5+ | 172 | 42.1 / 59 / 79 | размещены: Skeleton Master 33×79, Oculus 50×22, Power Lich 55×20, Dagrell 39×19, Lich King 65×11 … | Spectre Dagrell 1057 → 4228 | 68,157 | 66 · 17367×17736 | 98 · 8 | 11–14 | B | предметы 10358/10378 для части комнат; ключи 296/299 — переменные квеста |
| `lindisfarne.blv` | Lindisfarne | — | 57 | 22.9 / 35 / 35 | размещены: Thrall Master 18×29, Magreeb 22×12, Evil Sorcerer 35×9, Armored Magreeb 33×4, Evil Apprentice 25×3 | Evil Sorcerer 289 → 1156 | 8,815 | 2 · 58374×65024 | 13 · 1 | 4–5 | — | open-air region/city |
| `lindisfarnemonastery.blv` | Lindisfarne Monastery | - | 0 | — | — | — | 0 | 8 · 12216×28754 | 40 · 0 | — | D | нет враждебных монстров |
| `mountainpass.blv` | Mountain Pass | — | 106 | 21.3 / 39 / 51 | размещены: Imp Elder 10×36, Grey Wolf 20×15, Winged Oddity 27×13, Red Wolf 30×11, Winged Mutant 39×8 … | Winged Aberration 517 → 2068 | 16,240 | 2 · 52367×45875 | 0 · 1 | 7–9 | — | open-air region/city |
| `ruinedtemple.blv` | Ruined Temple | 4 | 20 | 33.5 / 79 / 79 | размещены: Skeleton 16×6, Skeleton Master 33×4, Skeleton Warrior 22×4, Spectre Dagrell 79×3, Dagrell 39×3 | Spectre Dagrell 1057 → 4228 | 6,339 | 10 · 5850×11346 | 55 · 3 | 1–2 | B | джинн-желание (ключи 5022–5025); левая дверь открывается переменной карты |
| `sturmford.blv` | Sturmford | — | 102 | 6.4 / 15 / 25 | размещены: Dragonfly 3×43, Field Thrall 8×29, Dragonfly Mite 2×11, Basilisk 15×6, Thrall 12×4 … | Nettle Spore 169 → 676 | 3,065 | 2 · 65024×65024 | 12 · 4 | 6–8 | — | open-air region/city |
| `sturmfordcity.blv` | Sturmford City | — | 0 | — | — | — | 0 | 63 · 22067×21686 | 105 · 2 | — | — | open-air region/city |
| `tasaracademy.blv` | Ta'Sar Academy | 3 | 20 | 15.9 / 35 / 39 | размещены: Guard 11×16, Half Orc Captain 35×2, Guard Captain 33×1, Dagrell 39×1 | Dagrell 346 → 1384 | 2,019 | 30 · 11837×13562 | 44 · 4 | 1–2 | B | дверь закрыта до ключа 9516 (собрать 4 книги на столе — книги на карте) |
| `templeofhonk.blv` | Temple of Honk | - | 0 | — | — | — | 0 | 23 · 11632×8883 | 35 · 5 | — | D | нет враждебных монстров |
| `thearena.blv` | The Arena | — | 0 | — | — | — | 0 | 5 · 5898×6881 | 6 · 0 | — | — | arena |
| `thjoradmine.blv` | Thjorad Mine | 23 | 108 | 12.3 / 24 / 24 | размещены: Troglodyte 10×85, Troglodyte Gnoll 24×13, Troglodyte Wren 17×10 | Troglodyte Gnoll 159 → 636 | 6,842 | 60 · 45056×28590 | 42 · 15 | 7–9 | B | Slag Extractor 10399 (из Thjorgard City) для квестовой части |
| `thjorgard.blv` | Thjorgard | — | 50 | 17.4 / 22 / 50 | размещены: Gezzamptling 15×14, Bandit 22×12, Burglar 14×9, Flying Icky 11×5, Gelatinous Spore 20×4 … | Bandit 139 → 556 | 4,734 | 3 · 65024×58767 | 7 · 2 | 3–4 | — | open-air region/city |
| `thjorgardcity.blv` | Thjorgard City | — | 0 | — | — | — | 0 | 89 · 19333×31294 | 145 · 0 | — | — | open-air region/city |
| `thronheim.blv` | Thronheim | — | 60 | 20.7 / 33 / 33 | размещены: Gezzamptling 15×14, Annelid 33×10, Bloodsucker 20×9, Basilisk 15×8, Fibrase Basilisk 23×6 … | Annelid 263 → 1052 | 8,012 | 2 · 60509×65024 | 0 · 3 | 4–5 | — | open-air region/city |
| `thronheimcity.blv` | Thronheim City | — | 1 | 50.0 / 50 / 50 | размещены: Desert Terror 50×1 | Desert Terror 499 → 1996 | 499 | 64 · 30023×21586 | 39 · 1 | 0–0 | — | open-air region/city |
| `traininghall.blv` | Training Hall | 4 | 31 | 31.8 / 40 / 60 | размещены: Ghoul 27×17, Lizard-Orc 40×8, Trellborg 22×4, Lizard-Orc Warrior 60×2 | Lizard-Orc Warrior 684 → 2736 | 8,051 | 14 · 9462×12288 | 54 · 5 | 2–3 | A | внешних проверок не найдено |
| `verhoffinruins.blv` | Verhoffin Ruins | 4 | 42 | 33.5 / 65 / 65 | размещены: Fright 35×14, Zombie 17×13, Terror 65×7, Skeleton Warrior 22×4, Skeleton Master 33×3 … | Terror 746 → 2984 | 12,436 | 31 · 11387×7014 | 75 · 6 | 3–4 | B | Door5 открывает предмет 10559 (не на карте); головоломка плахи |
| `wizardslab.blv` | Wizard's Lab | 4 | 131 | 33.3 / 51 / 70 | размещены: Eye 35×59, Skeleton 16×22, Evil Apprentice 25×17, Winged Aberration 51×13, Evil Grand Sorcerer 50×9 … | Sea Monster Female 850 → 3400 | 36,720 | 16 · 27655×18041 | 90 · 3 | 8–11 | A | внешних проверок не найдено |
| `yanmirsfort.blv` | Yanmir's Fort | 4 | 65 | 28.2 / 38 / 70 | размещены: Yeti 38×27, Thrall 12×15, Ice Lobber 20×9, Ice Lobbercicle 13×5, Thrall Master 18×4 … | Yanmir 850 → 3400 | 15,210 | 28 · 31458×32768 | 73 · 11 | 4–5 | B | клетки закрыты до ключа 7001 |
| `yorwick.blv` | Yorwick | — | 110 | 37.8 / 50 / 55 | размещены: Apparition 44×36, Evil Sorcerer 35×16, Shadow 33×15, Bloodsucker 20×13, Evil Apprentice 25×10 … | Specter 588 → 2352 | 36,518 | 3 · 58767×65024 | 8 · 2 | 7–9 | — | open-air region/city |

\* Оценка: E × 0,5 / (6…8 убийств в минуту). Для карт прототипа с `groupScale: 0.5` время вдвое меньше.

## 5. Сюжетные ограничения и адаптация

### 5.1 Классы

- **A** — враждебные монстры есть с самого начала, точка входа известна, все проверяемые ключи/предметы лежат на этой
  же карте, внешние квест-биты не найдены или влияют только на реплики NPC. Подключение = строка в `run.yml`.
- **B** — карта играется, но: есть переход в другой BLV (для забега это просто ещё один выход), внешние квест-биты
  меняют реплики, награды или враждебность групп, нет готовой точки входа, длинные головоломки, особые правила мира
  MM9. Нужна проверка руками; кода обычно не нужно.
- **C** — заметная часть карты (покои босса, лаборатория, проход) закрыта предметом/квест-битом/ценой, которые на
  карте не получить. Минимальная адаптация: выдать предмет или поставить квест-бит при входе (см. 9.3) — карту
  при этом не перерабатывать.
- **D** — в одиночку не работает: город/замок игрока с состоянием от десятков квест-битов, нет враждебных монстров на
  старте (выход забега тогда не откроется никогда), финальная сюжетная карта или один босс на тысячи HP.

### 5.2 Что именно блокируют зависимости (прочитано вручную)

| Карта | Проверка | Что закрыто | Источник ключа | Минимальная адаптация |
|---|---|---|---|---|
| `6d05` Snergle's Caverns | событие 9 «Door»: `IsQBitSet(1051)`, `HasItem(2108)` | покои Снергла; при открытии — призыв 6×3 дварфов B/C | ключ 2108 выдаёт NPC Ghim Hammond в `6d09` (событие 41, QBit 1025); в ресурсах как предмет не найден | выдать 2108 при входе или принять закрытую часть |
| `6d18` Gharik's Forge | событие 16 «Lever»: QBit 1035 (ставит `6d07`), QBit 1223, `HasItem(2107)` | лаборатория Гарика (в сундуке 5 карты — Hourglass of Time; связь сундука с закрытой частью не проверена) | ключ 2107 — сундук карты `6d07`; QBit 1035 — тоже `6d07` | выдать 2107 и QBit 1035 при входе — или оставить как «межподземельную» связь (5.3) |
| `cd3` Castle Kriegspire | событие 13 «Guardian of Kriegspire» (+ `cd3_mmmerge.lua`): без QBit 1364 просит 50 000 золота | тайный проход внутрь | NPC вне карты | QBit 1364 при входе или снизить цену; иначе класс D |
| `hive` The Hive | событие 60 «Exit»: `HasItem(2164)` Ritual of the Void, QBit 1226; `EnterHouse`-концовка | выход/финал MM6 | вне карты | не брать (финал) |
| `cd1` Castle Alamos | событие 69 (`cd1_mmmerge.lua`): пароль «JBARD», ошибка отбрасывает назад | часть замка; ключи 2188/2189 на карте | пароль известен из диалога вне карты | показать подсказку/записку, ничего не выдавать |
| `6d08` Shadow Guild | события 61–63: загадки (dark / arrow / time) | двери; переходы в `6d03`, `6d06` | на карте | нет |
| `6t5` Temple of the Moon | событие 27 «Altar of the Moon»: QBit 1143/1198 | только разговор у алтаря | вне карты | нет |
| `6d17` Lair of the Wolf | события 55/90: QBit 1059 | призрак-NPC; Pearl of Purity — на карте | — | нет |
| `7d29` Castle Harmondale | 19 внешних квест-битов; события 1, 15–16, 376–378, 416–420 | двери, магазины, 63 скрытых стража | сюжет MM7 | не брать |
| `7d23` The Lincoln | `HasItem(1406)` Wetsuit | подводная часть | `7d30`/`d03` | выдать Wetsuit или не брать |
| `7d34` Red Dwarf Mines | `HasItem(1431)` Elixir, `HasItem(1451)` Worn Belt | квестовые ветки | вне карт | проверить руками |
| `d14` / `d26` (MM8) | Sarcophagus 612 ↔ Remains 627 | гробница Корбу | друг у друга | пара в одном забеге или выдать |
| `d19` Necromancers' Guild | `HasItem(604)` Nightshade Brazier, фракционные биты 19/20/26/28 | лидер гильдии, враждебность | `d22` | проверить фракцию |
| `darkpassageway` (MM9) | `MM9.hasItem(396)` Capstone of Order | проход | `guberland` | выдать или не брать |
| `tasaracademy` (MM9) | ключ 9516 | главная дверь | собирается на карте (4 книги на столе) | нет |
| `yanmirsfort` (MM9) | ключ 7001 | клетки | квест вне карты | проверить |

### 5.3 Связи между подземельями внутри одного забега (факт: квест-биты глобальны)

Забег начинается с новой игры, и все квест-биты сброшены; но биты, которые ставит пройденная в забеге карта,
остаются до конца забега. Найденные связи: `6d07` ставит QBit 1035 и даёт ключ 2107 → открывает лабораторию `6d18`;
`6d09` даёт QBit 1025 (ключ Снергла) → `6d05`; `d14` ↔ `d26` (Корбу); `d22` → `d19` (Brazier); `7d35` → `7d13`
(череп Зокарра). Это не баг, а возможная механика «случайной синергии маршрута» — но только если такие пары могут
выпасть в правильном порядке (сейчас порядок случаен).

### 5.4 Многоуровневые комплексы

Любой переход в другую карту (`evt.MoveToMap` с именем BLV или переход MM9) до стража запрещён, а после стража
ведёт в лагерь и следующее подземелье (факт, 2.2). Поэтому комплексы играются **только своей картой входа**:
Dwarven Barrows MM7 (15 карт `mdk/mdr/mdt01–05` по 4–60 монстров — по отдельности слишком малы), `7d24 ↔ 7d35 ↔
7d07 ↔ 7d36` (Stone City — Nighon Tunnels — Thunderfist — Tunnels to Eeofol), `d31 ↔ d33 ↔ d32` (Regna),
`d06 ↔ d34`, `6d08 → 6d03/6d06`, `6t6 ↔ sewer`, `7d25/7d26` с храмами `t01`–`t04`. Для этапа 4 «длинный комплекс
из двух карт» потребовал бы нового кода (раздел 9.4).

## 6. Распределение по этапам

Колонки: E, средний/max уровень, страж ×4, класс, время до стража (оценка). Жирным — рекомендуемое ядро пула
(класс A, уровни в коридоре, разумный страж).

### Этап 1 (герой 1 → 13)

| Карта | Игра | E | Ур. ср/max | Страж ×4 | Класс | Мин. | Заметки |
|---|---|---|---|---|---|---|---|
| **`6d01` Goblinwatch** | MM6 | 218 | 3,4 / 10 | 160 | A | 14–18 | кодовый замок, ловушки, таймер; в прототипе |
| **`6d02` Abandoned Temple** | MM6 | 220 | 5,1 / 14 | 244 | A | 14–18 | в прототипе этап 2; годится и на 1 |
| **`7d06` Temple of the Moon** | MM7 | 62 | 3,6 / 12 | 200 | A | 4–5 | 3 размещённых Swordsman 10-го уровня; мала |
| **`d05` Abandoned Temple** | MM8 | 116 | 4,2 / 11 | 180 | A | 7–10 | QBit 403 — только реплика; 2 входа |
| **`anskramkeep` Anskram Keep** | MM9 | 86 | 6,3 / 14 | 288 | A (мир MM9) | 5–7 | сценарные ворота с бесами и рычагами — на карте; правила мира MM9 (7.4) |
| `6t3` Temple of Tsantsa | MM6 | 208 | 8,5 / 14 | 244 | A | 13–17 | граница 1/2; ключ камеры 2186 на карте |
| `d01` Erathian Sewers | MM7 | 46 | 7,4 / 18 | 344 | A | 3–4 | граница 1/2; 3 входа |
| `beethoven` Beet Hoven | MM9 | 156 | 6,2 / 16 | 348 | B | 10–13 | граница 1/2; 3 внешних ключа (16/110/14 — ветка квеста) |
| `mdk02` и др. Barrows | MM7 | 4–60 | 5–8 / 14–26 | ≤ 580 | B | 0–4 | комплекс, по отдельности малы |
| `7d22` Hall under the Hill | MM7 | 17 | 5,6 / 8 | 120 | B | 1–1 | мала |

### Этап 2 (герой 13 → 25)

| Карта | Игра | E | Ур. ср/max | Страж ×4 | Класс | Мин. | Заметки |
|---|---|---|---|---|---|---|---|
| **`6t3` Temple of Tsantsa** | MM6 | 208 | 8,5 / 14 | 244 | A | 13–17 | в прототипе |
| **`6t2` Temple of the Fist** | MM6 | 43 | 11,8 / 27 | 612 | A | 3–4 | в прототипе; мала (13 точек) |
| **`6t1` Temple of Baa** | MM6 | 212 | 9,3 / 25 | 548 | A | 13–18 | все 7 ключей — на карте |
| **`6d03` Shadow Guild Hideout** | MM6 | 61 | 12,9 / 22 | 456 | A | 4–5 | Guild Key на карте; граница 2/3 |
| `6d04` Hall of the Fire Lord | MM6 | 266 | 10,8 / 35 | **908** | A | 17–22 | страж — Defender of VARN (уникальный, ур. 35): тяжело для этапа 2 |
| `6d11` Corlagon's Estate | MM6 | 211 | 13,0 / 40 | 1120 | A | 13–18 | Power Lich 40 → страж 1120; граница 2/3 |
| `sewer` Free Haven Sewer | MM6 | 163 | 7,4 / 25 | 548 | B | 10–14 | выход в `6t6`; квест Принца воров |
| **`7d37` Haunted Mansion** | MM7 | 52 | 13,5 / 26 | 580 | A | 3–4 | |
| **`7d21` White Cliff Cave** | MM7 | 34 | 10,0 / 25 | 548 | A | 2–3 | мала |
| `7d17` Tidewater Caverns | MM7 | 48 | 12,1 / 19 | 372 | B | 3–4 | QBit 1607/1608 — сундук для жрецов |
| `7d24` Stone City | MM7 | 86 | 12,3 / 30 | 720 | B | 5–7 | город дварфов; враждебность групп от QBit 611 |
| **`d08` Dire Wolf Den** | MM8 | 28 | 11,5 / 19 | 372 | A | 2–2 | мала |
| **`d45` Chapel of Eep** | MM8 | 28 | 9,2 / 17 | 316 | A | 2–2 | мала |
| `d07` Smuggler's Cove | MM8 | 72 | 10,0 / 21 | 428 | B | 4–6 | QBit 10 меняет враждебность |
| `d13` Troll Tomb | MM8 | 42 | 12,7 / 19 | 372 | B | 3–4 | урна 603 — только текст |
| `drangheimprison` | MM9 | 50 | 7,5 / 14 | 288 | B | 3–4 | тревога/пропуск меняют стражу |
| `thjoradmine` Thjorad Mine | MM9 | 108 | 12,3 / 24 | 636 | B | 7–9 | Slag Extractor из другой карты |

### Этап 3 (герой 25 → 33, после повышения класса)

| Карта | Игра | E | Ур. ср/max | Страж ×4 | Класс | Мин. | Заметки |
|---|---|---|---|---|---|---|---|
| **`6d07` Silver Helm Outpost** | MM6 | 71 | 16,1 / 33 | 828 | A | 4–6 | в прототипе; ставит QBit 1035/ключ 2107 для `6d18` |
| **`6d15` Icewind Keep** | MM6 | 148 | 19,8 / 33 | 828 | A | 9–12 | в прототипе |
| **`6d13` The Monolith** | MM6 | 62 | 21,0 / 40 | 1120 | A | 4–5 | в прототипе |
| **`6d12` Silver Helm Stronghold** | MM6 | 210 | 21,3 / 50 | 1600 | A | 13–18 | в прототипе; граница 3/4 |
| **`6d06` Dragoons' Caverns** | MM6 | 156 | 14,3 / 35 | 908 | A | 10–13 | Harp, Flute |
| **`6d10` Dragoons' Keep** | MM6 | 164 | 15,5 / 35 | 908 | A | 10–14 | артефакт Mordred в сундуке 10 |
| **`6d14` Tomb of Ethric the Mad** | MM6 | 86 | 17,7 / 40 | 1120 | A | 5–7 | Ethric's Skull у Power Lich |
| `6d09` Snergle's Iron Mines | MM6 | 104 | 20,2 / 40 | 1120 | B | 6–9 | ключ Снергла для `6d05` |
| `6t5` Temple of the Moon | MM6 | 118 | 18,9 / 45 | 1348 | B | 7–10 | алтарь в полночь — только разговор |
| `6d05` Snergle's Caverns | MM6 | 213 | 12,9 / 30 | 720 | C | 13–18 | покои закрыты ключом 2108 |
| `7d35` Nighon Tunnels | MM7 | 184 | 17,9 / 50 | 1600 | B | 11–15 | комплекс; Zokarr's Axe на полу |
| `7d08` Tularean Caves | MM7 | 56 | 15,9 / 37 | 988 | B | 4–5 | переход в `7d32` |
| **`d11` Wasp Nest** | MM8 | 142 | 16,4 / 23 | 484 | A | 9–12 | однородная: осы |
| **`d12` Ogre Fortress** | MM8 | 68 | 19,0 / 35 | 908 | A | 4–6 | ключи тюрьмы у монстров |
| **`d25` Barbarian Fortress** | MM8 | 53 | 18,7 / 35 | 908 | A | 3–4 | |
| **`d41` Dark Dwarf Compound** | MM8 | 42 | 22,1 / 35 | 908 | A | 3–4 | Ring of Planes (артефакт) |
| **`d44` Grand Temple of Eep** | MM8 | 48 | 14,3 / 21 | 428 | A | 3–4 | |
| `d22` Temple of the Sun | MM8 | 38 | 17,0 / 30 | 720 | B | 2–3 | фракция Солнца/некромантов |
| `t03`, `t04`, `7d19` (MM7 храмы) | MM7 | 27–32 | 15–17 / 30–33 | 720–828 | B | 2 | промо-квесты, малы |
| `tasaracademy` | MM9 | 20 | 15,9 / 39 | 1384 | B | 1–2 | мала |

### Этап 4 (герой 33 → 41) — приоритет большим картам MM6

| Карта | Игра | E | Ур. ср/max | Страж ×4 | Класс | Мин. | Заметки |
|---|---|---|---|---|---|---|---|
| **`6d17` Lair of the Wolf** | MM6 | 340 | 23,9 / 40 | 1120 | A | 21–28 | 124 сектора, 29 × 45 тыс. — самая обширная из «чистых»; Pearl of Purity/Putrescence |
| **`6t7` Superior Temple of Baa** | MM6 | 340 | 19,2 / 50 | 1600 | A | 21–28 | ключи у жрецов Баа; 21 проверка навыка; сундук с артефактным слотом |
| **`cd2` Castle Darkmoor** | MM6 | 397 | 31,3 / 50 | 1600 | A | 25–33 | 21 внутренний телепорт; Deck of Fate; артефактный слот |
| **`6d19` Agar's Laboratory** | MM6 | 88 | 31,2 / 50 | 1600 | A | 6–7 | 297 секторов — лабиринт; мало монстров на площадь |
| `6d16` Warlord's Fortress | MM6 | 129 | 31,7 / 80 | **3520** | A | 8–11 | страж — Cuisinart 880 (ур. 80), не одиночка: остаётся ×4 = 3520 (принято, 11.1-2) |
| `cd1` Castle Alamos | MM6 | 492 | 30,7 / 50 | 1600 | B | 31–41 | самая населённая; пароль JBARD; долго без урезания групп |
| `6t6` Supreme Temple of Baa | MM6 | 290 | 21,6 / 60 | 2160 | B | 18–24 | второй выход в `sewer`; Memory Crystal Alpha |
| `6d18` Gharik's Forge | MM6 | 120 | 33,2 / 50 | 1600 | C | 7–10 | лаборатория закрыта (ключ/бит из `6d07`) |
| `6d12` Silver Helm Stronghold | MM6 | 210 | 21,3 / 50 | 1600 | A | 13–18 | граница 3/4 |
| **`wizardslab` Wizard's Lab** | MM9 | 131 | 33,3 / 70 | 3400 | A (мир MM9) | 8–11 | страж — Sea Monster Female 850 |
| **`dookscastle` Dook's Castle** | MM9 | 56 | 34,2 / 51 | 2068 | A (мир MM9) | 3–5 | артефакт Everstrike в сундуке |
| `chasmofthedead` | MM9 | 239 | 24,9 / 40 | 1440 | B | 15–20 | 21 скрытый враг появляется по скрипту |
| `yanmirsfort` | MM9 | 65 | 28,2 / 70 | 3400 | B | 4–5 | Yanmir 850 HP |
| `7d11` Walls of Mist, `7d15` Watchtower 6, `d03` Gloaming, `d04` Temple of Baa, `t01`, `t02` | MM7 | 48–71 | 23–31 / 50–70 | 1500–2800 | A–B | 3–6 | средние по размеру; `d04` — Cloak of the Sheep |
| `d19`, `d20`, `d24`, `d33` | MM8 | 52–82 | 24–35 / 45–75 | 1350–3150 | B–C | 3–6 | фракционные квесты MM8 |

**Только с балансировкой (5+, обозначаю проблему, не предлагаю переработку).** `pyramid` Tomb of VARN
(372 монстра, ср. 47,6, классический финал MM6 — самая большая и интересная карта, но уровни выше кривой на ~15 и
24–31 мин до стража); `cd3` Castle Kriegspire (ср. 37,8, Minotaur King 861 → 3444, цена 50 000 — C);
`6t8` Temple of the Snake (Gold Dragon 1300, ур. 100 → страж 5200); `6d20` Caves of the Dragon Riders
(ср. 67,9); `7d36` Tunnels to Eeofol (148, ср. 46,7); `d02` The Maze (107, ср. 52,8); `7d07` Thunderfist
Mountain (ср. 39,9); MM9 `lichlab` (172, ср. 42,1), `1000terrors` (205, ср. 46,2), `connectingtunnels`,
`darkpassageway` (C); драконьи логова MM7/MM8, планы стихий MM8 (ср. 65–100). Для них нужна либо 5-я ступень
маршрута, либо масштаб уровней/HP — это решение пользователя (раздел 11).

### Приоритет: подключить практически без адаптации

Этап 1: `6d01`, `6d02`, `7d06`, `d05` (+ `anskramkeep` после проверки мира MM9). Этап 2: `6t3`, `6t2`, `6t1`,
`6d03`, `7d37`, `7d21`, `d08`, `d45`. Этап 3: `6d07`, `6d15`, `6d13`, `6d12`, `6d06`, `6d10`, `6d14`, `d11`, `d12`,
`d25`, `d41`, `d44`. Этап 4: `6d17`, `6t7`, `cd2`, `6d19` (+ `6d16` после правки стража; `wizardslab`,
`dookscastle` после проверки мира MM9). Это 5 + 8 + 12 + 4 карт, все точки входа известны (раздел 7.2).

## 7. Технические ограничения подключения MM6–MM9

### 7.1 Выбор маршрута

- `DungeonRunStop.tier` — одно число (`DungeonRun.h:31`); карта не может стоять в двух этапах. Нужен список
  (`tiers: [2, 3]`) и выбор `find_if` по вхождению — 10 строк в `dungeonRunOrder`.
- Пустая ступень молча подменяется любой картой (`DungeonRun.cpp:479–482`). При пулах в 5–15 карт это редкость, но
  лучше отказ при загрузке `run.yml` (как сейчас для длины `route`, `:67`).
- Seed: порядок — функция `rootSeed` (`randomNameKey("dungeon_run.order")`); добавление карт в `run.yml` меняет
  маршрут существующих сохранений. Незавершённый забег v1 после обновления может «продолжиться» в другую карту:
  `dungeonRunStop` пересчитывает маршрут при каждом переходе (`GameApplicationDungeonRun.cpp:156`). Нужно
  сохранять выбранный маршрут в сохранении (именованные переменные `run.route.<n>`), как уже хранятся `run.reward.*`.

### 7.2 Точки входа и страж

- Вход и место стража — `entrance` из `run.yml`. Для MM6–MM8 их можно взять так же, как Fable: из событий наружных
  карт `evt.MoveToMap(x, y, z, yaw, …, "<карта>.blv")` — я нашёл хотя бы один такой вызов для всех кандидатов из
  раздела 6 (метод воспроизводит значения прототипа: `6d01` 601, 6871, 177, 1400). Без готового входа: `t01`, `t02`
  (входят из городов MM7 с иными аргументами) — нужен ручной подбор.
- MM9: вход — `ArrivalX/Y/Z` и `FacingYawUnits` из `worlds/mm9/data_tables/map_transitions.txt` или
  `party_start_points` сцены (есть у всех кандидатов MM9). Переходы MM9 идут через тот же `pendingMapMove`
  (`game/mm9/Mm9PositionedTransitionRuntime.cpp:142`), значит печать выхода и перенаправление работают; но у
  переходов MM9 свой диалог `AskPlayer` — проверить, как он сочетается с меню лагеря.
- Страж = вид с наибольшим **базовым** HP среди враждебных первой заселённости (`mapKillProgress`). На картах с
  одиночным уникальным монстром это он: Defender of VARN (`6d04`), Cuisinart (`6d16`), Minotaur King (`6t4`, `cd3`),
  Gold Dragon (`6t8`), Efreet (`6d08`), Dragon King 35 100 HP (`dragonstadium`). Нужно поле `guardian` в стопе:
  `{monster: <id>|strongest|commonest, healthScale: …}` или потолок HP стража относительно этапа.
- Если на старте враждебных нет (`countedHealth == 0`), `mapProgress` возвращает пусто, страж не зовётся, а
  `dungeonRunExitSealed` держит выход закрытым навсегда (`GameApplicationDungeonRun.cpp:651`). Карты класса D
  с 0 враждебных (`templeofhonk`, `lindisfarnemonastery`, `dungeonofsecrets`, `oracle`, `d09`) брать нельзя;
  для остальных — защитная проверка при загрузке стопа.
- Скрытые монстры (флаг Invisible) не входят в знаменатель до появления: `7d29` (63), `chasmofthedead` (21),
  `thjoradmine` (20), `7d23` (17). Знаменатель растёт по ходу — порог стража «уезжает» (известное ограничение v0).

### 7.3 Многокарточные подземелья и выходы

- Любой выход в другую карту запечатан до стража и ведёт в лагерь после (2.2) — для однокарточных подземелий это
  правильно, для комплексов (5.4) отрезает продолжение. Межкарточный переход внутри «одного стопа» потребует
  списка разрешённых карт стопа (`maps: [...]`) в `holdMapMoveForDungeonRunCamp`/`redirectMapMoveForDungeonRun` и
  учёта прогресса по нескольким картам (`mapKillProgress` считает только текущую). Это новый код — в эту версию не
  советую.
- Внутренние телепорты (`evt.MoveToMap` без имени карты или `"0"`) не затронуты (`isSameMapTeleport`).

### 7.4 Мир MM9

Факт `worlds/mm9/world.yml`: обзор монстров 360° и ×1,5 (`mm9Sight`), провокация (`mm9Provoke`), строй
(`formations`), Fleet Foot вместо Fly, `friendlyActorsBlockParty`, нет наёмников MM6–MM8, артефакты только из
«чёрных» сундуков (`chestArtifactsOnly`), автосохранение по прибытии, предметы MM9 — id 10000+. Следствия:

- Бой на картах MM9 ощутимо жёстче при тех же уровнях (оценка); пул MM9 лучше давать с отдельной пометкой.
- Книги школ из карточек выдаются по миру **текущей карты** (`spellbookItemIdForSpell(spellId, worldId)`,
  `GameApplicationDungeonRun.cpp:997`, `:1362`): на карте MM9 они либо станут предметами MM9, либо не найдутся и
  останутся «в долгу» — не проверено.
- Награда стража ищется сначала в мире текущей карты (`pickRunGuardianItem` с `worldIdOfMap`,
  `GameApplicationDungeonRun.cpp:1102`): на MM9 — артефакты MM9, на MM7/MM8 — свои. Это не ошибка, но меняет
  «MM6's world first» из `roguelike-rules-v1.md`.
- Лагерь открывает дома MM6 по id (`rules.shops`); на карте другого мира не проверялось.
- Ключи MM9 = квест-биты (`mm9_common.lua:65`): внешние ключи (`beethoven`, `drangheimprison`, `tasaracademy`,
  `lichlab`, `ruinedtemple`) в основном меняют сцены и диалоги; разобраны вручную только перечисленные в 5.2.

### 7.5 Ресурсы и сборка

- В sparse-checkout этого worktree лежит геометрия только 9 карт MM6 (+ по одной MM7/MM8) — на диске
  `assets_dev/worlds/mm6/maps`. Остальные BLV и сцены есть в дереве git (HEAD), а пакет релиза их содержит
  (полный релиз, без сужения). Для **локальных** безголовых сценариев на новых картах нужно материализовать blob-ы
  пакетно (процедура `AGENTS.md`; заметка «partial-clone-no-lazy-fetch»): блобы сцен и BLV этого HEAD я уже
  докачал в объектный склад, остаётся выписать нужные файлы и добавить их в sparse-список worktree. Безголовой карте
  нужны ещё эффекты/снаряды (список — в той же заметке). Недостающие в worktree файлы (по API GitHub, факт): BLV MM6 ~18 МБ, MM7 ~30 МБ, MM8 ~18 МБ, MM9 ~139 МБ;
  `.lighting` ещё ~730 МБ.
- Сохранения: формат не меняется при добавлении карт; дельта каждой посещённой карты пишется как обычно.

### 7.6 Различия данных между играми

- Размеры групп в объединённой `map_stats.txt`: MM6 — диапазоны «2–4 … 4–6» (группа ~3–5), MM7/MM8 — везде «1–3»
  (факт). Карты MM6 поэтому в 2–4 раза плотнее при той же площади.
- Уровень сокровищ (`Tres`, колонка 11): у всех 51 строк MM6 — 6, у MM7/MM8 — 0–6 (факт). Случайные слоты сундуков
  пересчитываются через него (`sampleRemappedTreasureLevel`, `game/gameplay/ChestRuntime.cpp`), так что лут MM6 в
  забеге богаче, чем у карт MM7/MM8 того же этапа (оценка влияния — не проверена).
- Слоты «−7» в сундуках = артефакт (`RandomChestItemMaxLevel = 7`, `ChestRuntime.cpp:32`). Такие слоты есть в
  `6t6`, `6t7`, `cd1`, `cd2`, `cd3`, `pyramid`, `7d08`, `7d09`, `7d12`, `7d27`, `d10`, `d15`, `d18`, `d27`, `d29`,
  `d35`, `d38`, `d40`, `d48` и в «чёрных» сундуках MM9 (`1000terrors` ×5, `bathhouse`, `chasmofthedead`,
  `dookscastle`, `drangheimprison`, `lichlab`, `traininghall`, `verhoffinruins`, `yanmirsfort`) — расчёт по
  `raw_items_hex`/`loot_containers`. Вместе с наградой стража это может дать два артефакта за этап.

## 8. Риски

| Риск | Вероятность / вес (оценка) | Чем снять |
|---|---|---|
| Длительность: большие карты MM6 этапа 4 — 20–40 мин до стража; забег с оригинальными группами ≈ 60–80 мин против цели 30–60 мин (решение пользователя 2026-11-12, `исследование.md`) | высокая | решение пользователя: `groupScale`, порог `clear.share` по этапу или принять длинный забег |
| Страж из уникального босса (×4 HP) | высокая на 10+ картах | поле `guardian` в стопе |
| Карта без враждебных на старте = вечная печать | низкая при отборе A, катастрофическая при ошибке | проверка при загрузке стопа + исключение D |
| Изменение пула меняет маршрут старых сохранений | высокая | сохранять маршрут в сохранении |
| Сценарий/тесты ждут `6d01` первым | факт | отдельный тестовый `run.yml` или фиксированный маршрут для `run new <seed>` |
| Мир MM9: книги, лагерь, награды, AI | средняя | безголовый сценарий на `anskramkeep` и `wizardslab` перед включением MM9 |
| Межкарточные связи открывают закрытые части только при удачном порядке | средняя | принять как синергию или выдавать ключ при входе |
| Ручная классификация покрывает ~40 карт; остальные — автоматически | средняя | ручной проход каждой карты пула в игре |

## 9. Рекомендации для следующей версии

### 9.1 Формат `run.yml` (минимальные изменения кода)

```yaml
dungeons:
  - map: 6d17.blv
    stages: [4]            # вместо tier; карта может стоять в нескольких этапах
    entrance: {x: -9600, y: 22127, z: 1, yaw: 512}   # outb2.lua
    groupScale: 0.5        # тестовое значение для карт с E ≳ 145 (решение 11.1-1)
rules:
  route: [1, 2, 3, 4]
  # страж: ×4, а если сильнейший вид при первой заселённости один на карте — ×2 (решение 11.1-2)
  clear: {share: 0.5, guardianHealthScale: 4, uniqueGuardianHealthScale: 2}
```

Классы C в пул не входят (решение 11.1-4), поэтому `onEnter` из 9.3 в этой версии не нужен.

- `dungeonRunOrder`: выбор по `stages` (вхождение), отказ при пустом этапе, маршрут фиксируется в сохранении при
  старте (`run.route.0..3`).
- `placeDungeonRunGuardian` (`GameApplicationDungeonRun.cpp:914`): посчитать в первой заселённости актёров вида
  `progress.strongestMonsterId` (враждебные, `spawnedAfterFill == false`, NPC не в счёт); ровно один — множитель
  `uniqueGuardianHealthScale`, иначе `guardianHealthScale`. Число удобно вернуть из `mapKillProgress`
  (новое поле `strongestMonsterCount` в `MapKillProgress`, `game/rules/ExperienceRules.h`).
- `groupScale: 0.5` прототипа оставить; новым большим картам задать так же (список — 11.1-1).
- Проверка стопа при входе: если `countedHealth == 0` после первой заселённости — сразу открыть выход и сообщить
  (или считать стоп неудачным).

### 9.2 Порядок подключения

1. Этап 1 случайный: `6d01`, `6d02`, `7d06`, `d05` (все класс A; две — другие миры MMMerge, проверка переходов
   континентов на старте новой игры с MM7/MM8-картой).
2. Этап 4 собственный: `6d17`, `6t7`, `cd2`, `6d19`; `6d16` — после поля `guardian`.
3. Этапы 2–3 расширить картами A из раздела 6.
4. Отдельной партией: MM9 (`anskramkeep`, `dookscastle`, `wizardslab`) после безголового сценария на мир MM9.
5. Классы B и C — по одной, с ручным прохождением.

### 9.3 Минимальная адаптация для класса C (без переработки карт)

`onEnter.qbits` / `onEnter.items` — выдать при входе в стоп то, что в оригинале приносили извне:
`6d05` → предмет 2108; `6d18` → QBit 1035 + предмет 2107; `cd3` → QBit 1364; `7d23` → предмет 1406;
`d34` → 619; `d14`/`d26` → 612/627; `darkpassageway` → 10396. Реализуется через уже существующие
`Party`-вызовы (квест-биты, выдача предмета) в `redirectMapMoveForDungeonRun`/при загрузке стопа.

### 9.4 Чего не делать сейчас

Многокарточные стопы, переработку карт, сокращение групп ниже прототипа, 5-ю ступень маршрута — до решения
пользователя (раздел 11).

## 10. Квестовые и уникальные предметы

Источник места: сундук (индекс в `initial_state.chests`, «ловушка» = флаг Trapped, событие, открывающее сундук, из
`openedChestIds`), пол (`sprite_objects`, координаты), носитель (`carried_item_id` актёра, вид по таблице монстров),
событие (`AddValue(InventoryItem)`/`MM9 give`, номер и условия). Всё — факт по ресурсам. **Назначение** — по описанию
в `items.txt` и комментариям квест-битов (факт); **роль в забеге** — предложение (оценка), эффекты не реализованы.
Принцип ролей: механика забега, которой нет у навыков, талантов и карточек (не «+N к характеристике»).

### 10.1 Отобранные предметы

| Предмет (id) | Карта | Где и как | Условия доступа | Исходное назначение | Возможная роль в забеге |
|---|---|---|---|---|---|
| Hourglass of Time (2053) | `6d18` Gharik's Forge | сундук 5 (ловушка), событие 48 | карта этапа 4; лаборатория за дверью с ключом 2107 + QBit 1035 (из `6d07`) — связь с сундуком не проверена | квест «песочные часы» (QBit 1029) | раз за подземелье остановить всех монстров на 5 с; продвигается только через межподземельную связь |
| Ethric's Skull (2084) | `6d14` Tomb of Ethric | у Power Lich ур. 40 (960, 8576, 768) | убить сильнейшего монстра карты | квест черепа Этрика | каждое 10-е убийство поднимает монстра союзником на 20 с (подъём мёртвых — нет в навыках) |
| Pearl of Putrescence (2078) / Pearl of Purity (2079) | `6d17` Lair of the Wolf | Putrescence — у Greater Werewolf ур. 40 (7040, −18560, 2304); Purity — событие 56: «You find a Pearl» после разговора с призраком (событие 90 ставит QBit 1166; всё на карте) | убить вожака-оборотня; Purity — пока призрак не отпущен (QBit 1059) | выбор «чёрная/белая жемчужина», призрак (QBit 1059) | взаимоисключающий выбор: Purity — партия невосприимчива к болезни и яду в забеге; Putrescence — убитые взрываются ядовитым облаком |
| Ankh (2068) | `6d12` Silver Helm Stronghold | у Warlock ур. 50 (2752, −3456, 0) | убить сильнейшего чародея | улика культа Баа | одноразовое воскрешение погибшего героя прямо в подземелье (у Cheat Death — только для себя) |
| Horn of Ros (2055) | `6t8` Temple of the Snake | у монстра «Peasant» ур. 5 (3776, 960, 0) | карта 5+ (Gold Dragon); носитель — по таблице «Peasant» | рог дракона Роса: «чувствует раны противника» | полоска HP и слабое сопротивление монстра видны всегда; +урон по раненым ниже 25 % |
| Harp (2099), Flute (2098) | `6d06` Dragoons' Caverns | сундуки 7 и 8 (ловушки), события 43, 44 | обычные сундуки | квест арфы (QBit 1027) | сыграть: группа в 6 м засыпает на 6 с, раз в подземелье |
| Chime of Harmony (2095) | `6t1` Temple of Baa | сундук 5 (ловушка), событие 31 | открыть сундук | реликвия Баа (квест) | +1 карточка в каждом предложении до конца забега |
| Sacred Chalice (2054) | `6t4` Temple of the Sun | событие 13 «Cabinet», при `MapVar(6)` и не взятом QBit 1075 | шкаф после события карты; Minotaur King рядом | квест Серебряной чаши | после смерти стража лечит и возвращает в сознание всю партию |
| Crystal Skull (2086) | `6d04`, `pyramid` | `6d04`: сундук 2 (ловушка), событие 42; `pyramid`: сундуки 4, 5 | открыть сундук | квест черепа (VARN) | метка стража и оставшихся монстров на мини-карте |
| Deck of Fate (2067) | `cd1`, `cd2` | `cd1`: сундуки 0, 2–4; `cd2`: на полу (−15808, 17088, 600) и др. | — | игровая колода MM6 (случайный эффект по клику) | в лагере тянуть карту судьбы: случайный редкий талант или проклятие на следующее подземелье |
| Magic Lamp (2103) | `pyramid` Tomb of VARN | на полу (−15296, −6976, 810) и ещё 11 мест | карта 5+ | лампа джинна MM6 | одно желание: выбрать 1 из 3 реликвий |
| Memory Crystals α/β/δ/ε (2170, 2171, 2172, 2173) | `6t6`, `cd1`, `cd2`, `cd3` | события: `6t6` 32 (алтарь, QBit 1077), `cd1` 59 «Exit» (QBit 1078), `cd2` 57 «Crystal», `cd3` 62 | этап 4 / 5+ | кристаллы памяти Оракула (финал MM6) | коллекция: 2 кристалла в одном забеге — показать маршрут следующих этапов и переброс одного |
| Cobra Egg (2094) | `6d02`, `6t5` | на полу, 6 и 11 штук (координаты в 10.2) | собрать | товар (200 зол.) | инкубатор: через подземелье вылупляется кобра-союзник |
| Spider Queen's Heart (2101) | `6d02` Abandoned Temple | у Huge Spider ур. 12 (−29280, −10240, −2201) | убить королеву пауков | трофей квеста | удары оружием оплетают цель (замедление) |
| Mordred (2020, артефакт) | `6d10` Dragoons' Keep | сундук 10 (ловушка), событие 29 | этап 3 | артефакт Братства, квест (QBit 1028) | уже артефакт: оставить как есть; сундук — «охраняемый» источник |
| Control Cube (2076) | `pyramid` | сундук 1, событие 41 | 5+ | куб управления VARN | вызвать робота-стража Enforcer Unit союзником на одно подземелье |
| Lich Jar (1417 MM7 / 628 MM8) | `7d11`, `d20` | `7d11`: сундук 0, события 176/195; `d20`: сундуки 0–3 | этап 4 | превращение в лича (промоушен MM7/MM8) | герой становится личом: иммунитет к ядам/болезням, лечение ранит |
| Medusa's Mirror (1341) | `7d34` Red Dwarf Mines | у Empress Medusa ур. 45 (−3200, 18176, −1024) | убить императрицу медуз (C) | иммунитет к окаменению | для героини-Медузы: её взгляд окаменяет монстров на 3 с |
| Hermes' Sandals (1331, артефакт) | `mdt12` Dragon Caves | у Mega-Dragon ур. 100 (−1104, 1755, 0) | дракон 100-го уровня | артефакт | награда за «запредельный» стоп: оставить артефактом |
| Minotaur's Axe (1353) | `d02` The Maze | у Minotaur Lord ур. 79 (−192, 6976, 0) | 5+ | трофей | — (особый предмет, роль не нужна) |
| Vampire's Cape (1352) | `7d16` Wine Cellar | у **скрытого** Elder Vampire ур. 48 (−1920, 3648, −512) | монстр невидим до события | особый плащ | кража жизни ночью; днём штраф |
| Clanker's Amulet (1339) | `7d12`, `7d14` | `7d12`: у Evil Eye ур. 50; `7d14`: сундук 1 (ловушка) | этап 4 | инструмент алхимика | зелья варятся без бутылки и дают двойной эффект |
| Season's Stole (1440) | `t04` Hall of the Pit | сундук 0 (ловушка), событие 176 | QBit 707 — квест вне карты | смена времён года (MM7) | смена «сезона» забега: стихийная уязвимость всех монстров следующего подземелья |
| Dragon Egg (1449) | `mdt12` | на полу (18920, 10728, 0), (894, −3812, 218) | 5+ | квест яйца дракона | вылупляется дракончик-союзник |
| Hearts of Air / Fire / Earth (608, 606, 609) | `d27`, `d29`, `eleme` | сундуки 10/19/19 (ловушки) и на полу | 5+ (планы стихий) | квесты сердец стихий MM8 | стихийная аура: все удары героя получают урон стихии |
| Prophecies of the Sun / Snake (626, 652) | `d05` Abandoned Temple (MM8) | Sun — на полу (11776, 2432, −384); Snake — сундук 0 (ловушка), событие 81 | этап 1 | пророчества MM8 | «пророчество»: показать карту следующего этапа и разрешить один переброс |
| Ebonest (539) | `d20` Mad Necromancer's Lab | сундук 2 (ловушка), событие 83 | этап 4 | копьё драконоборца | ×3 урона по драконам и стражам-драконам |
| Royal Wasp Jelly (635) | `d11` Wasp Nest | на полу, 12 мест | этап 3 | квестовое желе | съесть: +1 уровень героя без карточки (единственный способ вне повышений) |
| Drum of Victory (615) | `d18` Naga Vault | сундук 2 (ловушка), событие 83 | этап 3 | квест барабана | после смерти стража — бафф партии на следующее подземелье |
| Lost Book of Kehl (611) | `d40` Uplifted Library | сундук 0 (ловушка), событие 81 | этап 4 | книга Кела | выучить одно заклинание любой школы без навыка |
| Black Orb of Knowledge (10250) | `wizardslab` (MM9) | событие NPC Robinssen за 2000 золота (`wizardslab.lua:416`) | золото | квестовый шар | показ всех сундуков и ловушек карты |
| Verhoffin's Book (10242) | `verhoffinruins` (MM9) | событие «Book» (`verhoffinruins.lua:154`) | — | квест Верхоффина | — |
| Thjorad (10197, артефакт) | `lindisfarnemonastery` (MM9) | событие использования (`lindisfarnemonastery.lua:117`) | карта без враждебных (D) | артефакт-молот | — |
| Артефакты MM9 в сундуках: Spencer's Obedience (10198), Everstrike (10184), Gungnir (10189), Black's Heart (10180) | `connectingtunnels`, `dookscastle`, `inventastorca`, `klusoshouse` | сундуки с ловушками, координаты в 10.2 | этапы 4/5+ | артефакты | уже артефакты; охраняемые сундуки |

### 10.2 Полный список найденных особых и квестовых предметов

Все предметы с редкостью Artifact/Relic/Special или «Equip Stat = N / A» (квестовые), найденные в сундуках, на
полу, у монстров и в событиях внутренних карт (без открытых регионов MM9; события MM9 — только `RegisterEvent`,
выдачи через `handlers[]` перечислены вручную в 10.1).

| Предмет (id) | Редкость | Карта | Этап карты | Где и как |
|---|---|---|---|---|
| Candelabra (2069) | квестовый | `6d02.blv` Abandoned Temple | 12 | сундук 2 (ловушка) событие 26 |
| Cobra Egg (2094) | особый | `6d02.blv` Abandoned Temple | 12 | на полу (-24320,-7680,-2201); на полу (-24320,-7808,-2201); на полу (-24064,-7680,-2201) …(+3) |
| Spider Queen's Heart (2101) | квестовый | `6d02.blv` Abandoned Temple | 12 | у монстра Huge Spider ур.12 (-29280,-10240,-2201) |
| Guild Key (2180) | квестовый | `6d03.blv` Shadow Guild Hideout | 23 | сундук 3 (ловушка) событие 19 |
| Crystal Skull (2086) | квестовый | `6d04.blv` Hall of the Fire Lord | 2 | сундук 2 (ловушка) событие 42,42 |
| Amber (2102) | квестовый | `6d04.blv` Hall of the Fire Lord | 2 | сундук 1 (ловушка) событие 41; сундук 4 событие 44; сундук 5 (ловушка) событие 45 …(+3) |
| Chest Key (2179) | квестовый | `6d04.blv` Hall of the Fire Lord | 2 | у монстра Defender of VARN ур.35 (1472,-128,-4704) |
| Flute (2098) | квестовый | `6d06.blv` Dragoons' Caverns | 3 | сундук 8 (ловушка) событие 44 |
| Harp (2099) | квестовый | `6d06.blv` Dragoons' Caverns | 3 | сундук 7 (ловушка) событие 43 |
| Key to Gharik's Laboratory (2107) | квестовый | `6d07.blv` Silver Helm Outpost | 3 | сундук 1 (ловушка) событие 16 |
| Cell Key (2192) | квестовый | `6d09.blv` Snergle's Iron Mines | 34 | сундук 1 (ловушка) событие 27 |
| Mordred (2020) | артефакт | `6d10.blv` Dragoons' Keep | 3 | сундук 10 (ловушка) событие 29 |
| Whisp wrappings (665) | квестовый | `6d11.blv` Corlagon's Estate | 3 | сундук 0 (ловушка) событие 75; сундук 1 событие 42; сундук 2 событие 43 …(+8) |
| Crystal of Terrax (2077) | квестовый | `6d11.blv` Corlagon's Estate | 3 | сундук 9 (ловушка) событие 50 |
| Ankh (2068) | квестовый | `6d12.blv` Silver Helm Stronghold | 34 | у монстра Warlock ур.50 (2752,-3456,0) |
| Ethric's Skull (2084) | квестовый | `6d14.blv` Tomb of Ethric the Mad | 3 | у монстра Power Lich ур.40 (960,8576,768) |
| Dragon Tower Keys (2106) | квестовый | `6d15.blv` Icewind Keep | 34 | сундук 5 (ловушка) событие 25 |
| Warlord's Key (2184) | квестовый | `6d16.blv` Warlord's Fortress | 4 | событие 45 «Chest of Drawers» при IsAtLeast(MapVar(2) |
| Storage Key (2185) | квестовый | `6d16.blv` Warlord's Fortress | 4 | событие 45 «Chest of Drawers» при IsAtLeast(MapVar(2) |
| Storage Key (2191) | квестовый | `6d16.blv` Warlord's Fortress | 4 | событие 46 «Chest of Drawers» при IsAtLeast(MapVar(3) |
| Pearl of Putrescence (2078) | квестовый | `6d17.blv` Lair of the Wolf | 4 | у монстра Greater Werewolf ур.40 (7040,-18560,2304) |
| Pearl of Purity (2079) | квестовый | `6d17.blv` Lair of the Wolf | 4 | событие 56 «nil» при IsQBitSet(QBit(1044)); IsQBitSet(QBit(1166)) |
| Hourglass of Time (2053) | квестовый | `6d18.blv` Gharik's Forge | 4 | сундук 5 (ловушка) событие 48 |
| Temple Gong (2082) | квестовый | `6t1.blv` Temple of Baa | 2 | на полу (5760,0,976) |
| Chime of Harmony (2095) | квестовый | `6t1.blv` Temple of Baa | 2 | сундук 5 (ловушка) событие 31 |
| Bathhouse Key (2175) | квестовый | `6t1.blv` Temple of Baa | 2 | сундук 0 (ловушка) событие 23,23 |
| Store Room Key (2176) | квестовый | `6t1.blv` Temple of Baa | 2 | событие 32 «Statue» при IsAtLeast(MapVar(12) |
| Treasure Room Key (2177) | квестовый | `6t1.blv` Temple of Baa | 2 | сундук 5 (ловушка) событие 31 |
| Secret Door Key (2178) | квестовый | `6t1.blv` Temple of Baa | 2 | сундук 2 (ловушка) событие 28 |
| Bathhouse Key (2181) | квестовый | `6t1.blv` Temple of Baa | 2 | сундук 1 (ловушка) событие 27 |
| Store Room Key (2182) | квестовый | `6t1.blv` Temple of Baa | 2 | сундук 3 (ловушка) событие 29 |
| Treasure Room Key (2183) | квестовый | `6t1.blv` Temple of Baa | 2 | сундук 4 (ловушка) событие 30 |
| Bone (2085) | квестовый | `6t3.blv` Temple of Tsantsa | 2 | на полу (-256,-1408,0); на полу (256,-1408,0); на полу (-64,-1536,0) …(+5) |
| Cell Key (2186) | квестовый | `6t3.blv` Temple of Tsantsa | 2 | сундук 6 (ловушка) событие 16 |
| Sacred Chalice (2054) | квестовый | `6t4.blv` Temple of the Sun | 4 | событие 13 «Cabinet» при IsAtLeast(MapVar(6); IsQBitSet(QBit(1075)) |
| Cobra Egg (2094) | особый | `6t5.blv` Temple of the Moon | 34 | на полу (768,3072,256); на полу (832,3008,256); на полу (5248,3392,0) …(+8) |
| Temple Gong (2082) | квестовый | `6t6.blv` Supreme Temple of Baa | 4 | на полу (8240,768,1120); на полу (3712,848,1888); на полу (3280,1280,1888) …(+4) |
| Memory Crystal Alpha (2170) | квестовый | `6t6.blv` Supreme Temple of Baa | 4 | событие 32 «Oracle Memory Crystal» при IsQBitSet(QBit(1077)) |
| Temple Gong (2082) | квестовый | `6t7.blv` Superior Temple of Baa | 34 | сундук 0 (ловушка); сундук 1 событие 27; сундук 2 событие 38 …(+4) |
| High Sorcerer's Key (2112) | квестовый | `6t7.blv` Superior Temple of Baa | 34 | у монстра Priest of Baa ур.25 (3072,-9344,0) |
| High Cleric's Key (2187) | квестовый | `6t7.blv` Superior Temple of Baa | 34 | у монстра Priest of Baa ур.25 (-9344,-2624,0) |
| Horn of Ros (2055) | квестовый | `6t8.blv` Temple of the Snake | 5+ | у монстра Peasant ур.5 (3776,960,0) |
| Deck of Fate (2067) | особый | `cd1.blv` Castle Alamos | 4 | сундук 0 (ловушка) событие 30; сундук 2 (ловушка) событие 42; сундук 3 событие 43 …(+2) |
| Memory Crystal Beta (2171) | квестовый | `cd1.blv` Castle Alamos | 4 | событие 59 «Exit» при IsQBitSet(QBit(1078)) |
| Treasure Room Key (2188) | квестовый | `cd1.blv` Castle Alamos | 4 | сундук 2 (ловушка) событие 42 |
| Teleporter Key (2189) | квестовый | `cd1.blv` Castle Alamos | 4 | сундук 0 (ловушка) событие 30 |
| Deck of Fate (2067) | особый | `cd2.blv` Castle Darkmoor | 4 | на полу (-15808,17088,600); на полу (-15808,16832,600); на полу (-13344,23968,-180) …(+2) |
| Memory Crystal Delta (2172) | квестовый | `cd2.blv` Castle Darkmoor | 4 | событие 57 «Crystal» при HasItem(2172); IsQBitSet(QBit(1126)) |
| Horseshoe (2083) | квестовый | `cd3.blv` Castle Kriegspire | 5+ | сундук 1 событие 41; сундук 2 (ловушка) событие 42; сундук 4 (ловушка) событие 44 …(+8) |
| Jeweled Egg (2100) | квестовый | `cd3.blv` Castle Kriegspire | 5+ | сундук 2 (ловушка) событие 42 |
| Memory Crystal Epsilon (2173) | квестовый | `cd3.blv` Castle Kriegspire | 5+ | событие 62 «nil» при HasItem(2173); IsQBitSet(QBit(1128)) |
| Hive Sanctum Key (2190) | квестовый | `hive.blv` The Hive | 5+ | сундук 2 (ловушка) событие 42 |
| Control Cube (2076) | квестовый | `pyramid.blv` Tomb of VARN | 5+ | сундук 1 событие 41 |
| Crystal Skull (2086) | квестовый | `pyramid.blv` Tomb of VARN | 5+ | сундук 4 (ловушка) событие 44; сундук 5 (ловушка) событие 45 |
| Magic Lamp (2103) | особый | `pyramid.blv` Tomb of VARN | 5+ | на полу (-15296,-6976,810); на полу (-15296,-5440,810); на полу (-15296,-3840,810) …(+9) |
| Water Temple Key (2193) | квестовый | `pyramid.blv` Tomb of VARN | 5+ | сундук 2 (ловушка) событие 42 |
| Flame Door Key (2194) | квестовый | `pyramid.blv` Tomb of VARN | 5+ | сундук 4 (ловушка) событие 44 |
| Back Door Key (2195) | квестовый | `pyramid.blv` Tomb of VARN | 5+ | у монстра Defender of VARN ур.35 (-8704,-18816,93) |
| Bibliotheca Chest Key (2196) | квестовый | `pyramid.blv` Tomb of VARN | 5+ | сундук 5 (ловушка) событие 45 |
| VARN Chest Key (2197) | квестовый | `pyramid.blv` Tomb of VARN | 5+ | сундук 3 (ловушка) событие 43 |
| Sewer Key (2198) | квестовый | `sewer.blv` Free Haven Sewer | 2 | событие 12 «Sewer Grate» при IsAtLeast(MapVar(2); IsAtLeast(MapVar(5) |
| Wealthy Hat (1433) | квестовый | `7d06.blv` The Temple of the Moon | 1 | сундук 5 событие 25 |
| Floor Tile (w/ moon insignia) (1438) | квестовый | `7d06.blv` The Temple of the Moon | 1 | на полу (-4160,6336,-96) |
| Case of Soul Jars (1404) | квестовый | `7d07.blv` Thunderfist Mountain | 5+ | сундук 1 (ловушка) событие 176 |
| The Perfect Bow (1344) | особый | `7d09.blv` The Titans' Stronghold | 5+ | сундук 1 (ловушка) событие 176 |
| Lich Jar (1417) | квестовый | `7d11.blv` The Walls of Mist | 4 | сундук 0 событие 176,195 |
| West Pillar Key (1454) | квестовый | `7d11.blv` The Walls of Mist | 4 | сундук 2 (ловушка) событие 177 |
| Central Pillar Key (1455) | квестовый | `7d11.blv` The Walls of Mist | 4 | сундук 3 (ловушка) событие 178 |
| East Pillar Key (1456) | квестовый | `7d11.blv` The Walls of Mist | 4 | сундук 4 (ловушка) событие 179 |
| Clanker's Amulet (1339) | особый | `7d12.blv` Clanker's Laboratory | 4 | у монстра Evil Eye ур.50 (3312,-448,624) |
| Clanker's Amulet (1339) | особый | `7d14.blv` The School of Sorcery | 4 | сундук 1 (ловушка) событие 176 |
| Vampire's Cape (1352) | особый | `7d16.blv` The Wine Cellar | 34 | у монстра Elder Vampire ур.48 (-1920,3648,-512) скрыт |
| Vase (1426) | квестовый | `7d18.blv` Lord Markham's Manor | 4 | событие 376 «nil» при IsQBitSet(QBit(653)); IsQBitSet(QBit(724)) |
| Moon Cloak (1350) | особый | `7d19.blv` Grand Temple of the Moon | 3 | у монстра Priest of the Moon ур.30 (2688,4992,32) |
| Eagle Statuette (1420) | квестовый | `7d19.blv` Grand Temple of the Moon | 3 | сундук 1 (ловушка) событие 176 |
| Altar Piece (1479) | квестовый | `7d19.blv` Grand Temple of the Moon | 3 | сундук 0 (ловушка) событие 195 |
| Lieutenant's Cutlass (1340) | особый | `7d20.blv` The Mercenary Guild | 4 | у монстра Master of the Sword ур.80 (2112,5440,-64) |
| Heart of the Wood (1402) | квестовый | `7d20.blv` The Mercenary Guild | 4 | сундук 3 (ловушка) событие 178 |
| Arcomage Deck (1453) | квестовый | `7d21.blv` White Cliff Cave | 2 | на полу (192,11776,203) |
| Faerie Ring (1348) | особый | `7d22.blv` The Hall under the Hill | 12 | сундук 1 (ловушка) событие 176 |
| Red Apple (1432) | квестовый | `7d22.blv` The Hall under the Hill | 12 | событие 454 «Fruit Tree» при IsAtLeast(MapVar(52); событие 455 «Fruit Tree» при IsAtLeast(MapVar(53); событие 456 «Fruit Tree» при IsAtLeast(MapVar(54) …(+3) |
| Faerie Key (1464) | квестовый | `7d22.blv` The Hall under the Hill | 12 | на полу (-525,-6770,-4096); у монстра Queen Dragonfly ур.8 (-1024,-7680,-4096) |
| Oscillation Overthruster (1407) | квестовый | `7d23.blv` The Lincoln | 5+ | событие 376 «nil» при HasItem(1407); IsAtLeast(MapVar(4); IsQBitSet(QBit(633)) |
| Large Round Crystal (664) | квестовый | `7d24.blv` Stone City | 23 | сундук 13 (ловушка) событие 189 |
| Lady Carmine's Dagger (1342) | особый | `7d25.blv` Celeste | 5+ | у монстра Peasant ур.3 (-1792,17664,0) |
| Colony Zod Key (1463) | квестовый | `7d27.blv` Colony Zod | 5+ | событие 376 «nil» |
| Golem left leg (1444) | квестовый | `7d29.blv` Castle Harmondale | 2 | у монстра Gold Golem ур.50 (-5120,-128,0) скрыт |
| Big Tapestry (1422) | квестовый | `7d30.blv` Castle Lambent | 5+ | событие 376 «nil» при IsQBitSet(QBit(611)); IsQBitSet(QBit(694)); IsQBitSet(QBit(711)) |
| Archibald Ironfist Painting (1424) | квестовый | `7d33.blv` Castle Gryphonheart | 4 | событие 378 «Portrait» при IsQBitSet(QBit(777)) |
| Roland Ironfist Painting (1425) | квестовый | `7d33.blv` Castle Gryphonheart | 4 | событие 377 «Portrait» при IsQBitSet(QBit(776)) |
| Catherine's Key (1462) | квестовый | `7d33.blv` Castle Gryphonheart | 4 | сундук 2 (ловушка) событие 177 |
| Medusa's Mirror (1341) | особый | `7d34.blv` The Red Dwarf Mines | 4 | у монстра Empress Medusa ур.45 (-3200,18176,-1024) |
| Zokarr's Axe (1351) | особый | `7d35.blv` Nighon Tunnels | 3 | на полу (18816,14592,0) |
| Zokarr IV's Skull (1428) | квестовый | `7d35.blv` Nighon Tunnels | 3 | событие 196 «nil» при IsQBitSet(QBit(740)) |
| Ghost Ring (1347) | особый | `7d37.blv` The Haunted Mansion | 23 | сундук 1 (ловушка) событие 177 |
| Angel Statue Painting (1423) | квестовый | `7d37.blv` The Haunted Mansion | 23 | событие 376 «Portrait» при IsQBitSet(QBit(778)) |
| Minotaur's Axe (1353) | особый | `d02.blv` The Maze | 5+ | у монстра Minotaur Lord ур.79 (-192,6976,0) |
| Angel Statuette (1421) | квестовый | `d02.blv` The Maze | 5+ | сундук 2 (ловушка) событие 177 |
| Haldar's Remains (1430) | квестовый | `d02.blv` The Maze | 5+ | сундук 1 (ловушка) событие 176 |
| Case of Soul Jars (1404) | квестовый | `d03.blv` Castle Gloaming | 4 | сундук 0 (ловушка) событие 176,195 |
| Big Tapestry (1422) | квестовый | `d03.blv` Castle Gloaming | 4 | событие 376 «nil» при IsQBitSet(QBit(612)); IsQBitSet(QBit(694)); IsQBitSet(QBit(711)) |
| Cloak of the Sheep (1332) | артефакт | `d04.blv` The Temple of Baa | 4 | у монстра Priest of the Moon ур.30 (3712,7168,-1536) |
| Barrow Key (1458) | квестовый | `mdk01.blv` Barrow VII | 12 | сундук 0 (ловушка) событие 176 |
| Lantern of Light (1450) | квестовый | `mdk03.blv` Barrow II | 2 | сундук 2 (ловушка) событие 177 |
| Barrow Key (1459) | квестовый | `mdr01.blv` Barrow X | 12 | сундук 0 (ловушка) событие 176 |
| Barrow Key (1457) | квестовый | `mdt01.blv` Barrow IX | 12 | сундук 0 (ловушка) событие 176 |
| Villain's Blade (1343) | особый | `mdt10.blv` William Setag's Tower | ? | у монстра Master of the Sword ур.80 (2688,-960,1408) |
| William's Tower Key (1461) | квестовый | `mdt10.blv` William Setag's Tower | ? | у монстра Master of the Sword ур.80 (1664,512,1408) скрыт |
| Shadow's Mask (1346) | особый | `mdt11.blv` The Hidden Tomb | 4 | сундук 6 событие 181 |
| Hermes' Sandals (1331) | артефакт | `mdt12.blv` The Dragon Caves | 5+ | у монстра Mega-Dragon ур.100 (-1104,1755,0) |
| Dragon Egg (1449) | квестовый | `mdt12.blv` The Dragon Caves | 5+ | на полу (18920,10728,0); на полу (894,-3812,218) |
| Signet Ring (1408) | квестовый | `mdt14.blv` The Bandit Caves | 2 | сундук 0 (ловушка) событие 176,195 |
| Altar Piece (1478) | квестовый | `t01.blv` Temple of the Light | 4 | сундук 1 (ловушка) событие 176 |
| Altar Piece (1479) | квестовый | `t02.blv` Temple of the Dark | 4 | сундук 0 (ловушка) событие 195 |
| Sun Cloak (1349) | особый | `t03.blv` Grand Temple of the Sun | 3 | у монстра Priest of the Sun ур.30 (-2112,3648,256) |
| Knight Statuette (1419) | квестовый | `t03.blv` Grand Temple of the Sun | 3 | сундук 1 (ловушка) событие 176 |
| Season's Stole (1440) | квестовый | `t04.blv` The Hall of the Pit | 3 | сундук 0 (ловушка) событие 176 |
| Prophecies of the Sun (626) | квестовый | `d05.blv` Abandoned Temple | 1 | на полу (11776,2432,-384); на полу (10944,-3328,-640) |
| Idol of the Snake (630) | квестовый | `d05.blv` Abandoned Temple | 1 | сундук 19 (ловушка) событие 100 |
| Prophecies of the Snake (652) | квестовый | `d05.blv` Abandoned Temple | 1 | сундук 0 (ловушка) событие 81 |
| Terminus (502) | артефакт | `d06.blv` Pirate Outpost | ? | сундук 7 событие 88 |
| Pirate Leader's Key (619) | квестовый | `d06.blv` Pirate Outpost | ? | у монстра Regnan Archmage ур.75 (5792,-3107,-704) |
| Anointed Herb Potion (616) | квестовый | `d07.blv` Smuggler's Cove | 2 | сундук 4 (ловушка) событие 85 |
| Forged Vouchers (650) | квестовый | `d07.blv` Smuggler's Cove | 2 | сундук 9 (ловушка) событие 90; сундук 11 (ловушка) событие 92 |
| Green Apple (655) | квестовый | `d07.blv` Smuggler's Cove | 2 | сундук 7 (ловушка) событие 88; сундук 8 (ловушка) событие 89; сундук 9 (ловушка) событие 90 …(+4) |
| Bone of Doom (637) | квестовый | `d08.blv` Dire Wolf Den | 2 | сундук 1 (ловушка) событие 82 |
| Royal Wasp Jelly (635) | квестовый | `d11.blv` Wasp Nest | 3 | на полу (1920,896,1350); на полу (128,-1280,384); на полу (-1024,-2304,384) …(+9) |
| Prison Key (620) | квестовый | `d12.blv` Ogre Fortress | 34 | у монстра Ogre Warleader ур.28 (-13952,512,-848) |
| Prison Key (621) | квестовый | `d12.blv` Ogre Fortress | 34 | у монстра Ogre Warleader ур.28 (-17408,-2176,-1088) |
| Remains of Korbu (627) | квестовый | `d14.blv` Cyclops Larder | 4 | сундук 2 (ловушка) событие 83 |
| Snake (523) | реликвия | `d16.blv` Dragon Hunter's Camp | 4 | сундук 7 (ловушка) событие 88 |
| Drum of Victory (615) | квестовый | `d18.blv` Naga Vault | 3 | сундук 2 (ловушка) событие 83 |
| Eclipse (516) | артефакт | `d19.blv` Necromancers' Guild | 4 | сундук 0 (ловушка) событие 81 |
| Ebonest (539) | особый | `d20.blv` Mad Necromancer's Lab | 4 | сундук 2 (ловушка) событие 83 |
| Puzzle Box (613) | квестовый | `d20.blv` Mad Necromancer's Lab | 4 | сундук 1 (ловушка) событие 82 |
| Lich Jar (628) | квестовый | `d20.blv` Mad Necromancer's Lab | 4 | сундук 0 (ловушка) событие 81; сундук 1 (ловушка) событие 82; сундук 2 (ловушка) событие 83 …(+2) |
| Vial of Grave Earth (614) | квестовый | `d21.blv` Vampire Crypt | 4 | на полу (-231,7064,-222) |
| Nightshade Brazier (604) | квестовый | `d22.blv` Temple of the Sun | 3 | на полу (-2400,-624,-901) |
| Druid Circlet of Power (638) | квестовый | `d23.blv` Druid Circle | 5+ | сундук 3 (ловушка) событие 84 |
| Dragon Leader's Egg (605) | квестовый | `d25.blv` Barbarian Fortress | 34 | сундук 1 (ловушка) событие 82 |
| Sarcophagus of Korbu (612) | квестовый | `d26.blv` The Crypt of Korbu | 4 | событие 131 «nil» при IsQBitSet(QBit(81)) |
| Heart of Air (608) | квестовый | `d27.blv` Castle of Air | 5+ | сундук 10 (ловушка) событие 91; на полу (1657,182,776) |
| Flute (634) | квестовый | `d28.blv` Tomb of Lord Brinne | 2 | событие 451 «Tomb of Lord Brinne» при IsQBitSet(QBit(237)) |
| Heart of Fire (606) | квестовый | `d29.blv` Castle of Fire | 5+ | сундук 19 (ловушка) событие 100; на полу (544,4384,2152) |
| Mace of the Sun (538) | особый | `d32.blv` Abandoned Pirate Keep | 5+ | сундук 3 событие 84 |
| Cannonball of Dominion (662) | квестовый | `d32.blv` Abandoned Pirate Keep | 5+ | сундук 19 (ловушка) событие 100 |
| Lost Book of Kehl (611) | квестовый | `d40.blv` Uplifted Library | 4 | сундук 0 (ловушка) событие 81 |
| Ring of Planes (519) | артефакт | `d41.blv` Dark Dwarf Compound | 34 | сундук 13 (ловушка) событие 94 |
| Axe of Balthazar (541) | особый | `d41.blv` Dark Dwarf Compound | 34 | сундук 1 (ловушка) событие 82 |
| Infused Metal Rod (663) | квестовый | `d41.blv` Dark Dwarf Compound | 34 | сундук 12 (ловушка) событие 93 |
| Wheel of Frelandeau (658) | квестовый | `d44.blv` Grand Temple of Eep | 3 | сундук 1 (ловушка) событие 82 |
| Log of Eldenbrie (659) | квестовый | `d45.blv` Chapel of Eep | 2 | сундук 1 (ловушка) событие 82 |
| Ball of Dunduck (660) | квестовый | `d46.blv` Church of Eep | 23 | сундук 1 (ловушка) событие 82 |
| Noblebone Bow (542) | особый | `d49.blv` Yaardrake's Cave | 5+ | сундук 0 (ловушка) событие 81 |
| Heart of Earth (609) | квестовый | `eleme.blv` Plane of Earth | 5+ | сундук 19 (ловушка) событие 100; на полу (1280,13280,-1520) |
| Spencer's Obedience (10198) | артефакт | `connectingtunnels.blv` Connecting Tunnel | 5+ | сундук TreasureChest1 (ловушка) (-944,-13271,-870) |
| Everstrike (10184) | артефакт | `dookscastle.blv` Dook's Castle | 4 | сундук TreasureChest0 (ловушка) (13148,4301,-2540) |
| Gungnir (10189) | артефакт | `inventastorca.blv` Inventa Storca | 4 | сундук Chest1 (ловушка) (-576,-4188,-1687) |
| Black's Heart (10180) | артефакт | `klusoshouse.blv` Kluso's House | 4 | сундук TreasureChest1 (ловушка) (-1616,-961,87) |

## 11. Решения пользователя

### 11.1 Принято 2026-11-17

1. **Длинный забег — да, но пока с уменьшенными вдвое группами для тестов** (уточнение того же дня: так быстрее
   набираются уровни при тестировании). `groupScale: 0.5` прототипа у `6d01`, `6d02`, `6t3`, `6d15`, `6d12`
   остаётся. Новым картам — по тому же признаку, что у прототипа (моё прочтение: Fable уменьшил карты от ≈148
   монстров в ожидании и не трогал карты до ≈71): `groupScale: 0.5` при E ≳ 145. Из пула это `6t1` (212), `6d04`
   (266), `6d06` (156), `6d10` (164), `6d11` (211), `6d17` (340), `6t7` (340), `6t6` (290), `cd1` (492), `cd2`
   (397), `sewer` (163), `7d35` (184), `beethoven` (156), `chasmofthedead` (239); `d11` (142) — на границе.
   Значение тестовое; к оригинальным группам вернуться после тестов.
2. **Уникальный босс — ×2 HP вместо ×4.** Определение для реализации (моё предложение): вид стража, который при
   первой заселённости карты стоит в **одном** экземпляре. Определять по факту заселения, а не по таблице: у
   случайных слотов сильнейший ранг C может не выпасть вовсе, и стражем станет другой вид. Расчёт по ожиданию
   (раздел 4) — карты пула, где сильнейший обычно одиночка: `6d04` Defender of VARN 227 → **454**, `6d11` Power
   Lich 280 → 560, `6d12` Warlock 400 → 800, `6t4` Minotaur King 861 → 1722, `7d12` Evil Eye 400 → 800, `7d14`
   Gold Golem 400 → 800, `7d16` Queen of the Dead 400 → 800, `d18` Naga Queen 258 → 516, `d06` Regnan Archmage
   787 → 1574, MM9: `wizardslab` Sea Monster Female 850 → 1700, `yanmirsfort` Yanmir 850 → 1700, `klusoshouse`
   Count 746 → 1492, `tasaracademy` Dagrell 346 → 692. **Не покрывается:** `6d16` Warlord's Fortress — Cuisinart
   там не один (≈11 в ожидании), страж остаётся 880 × 4 = 3520 — **так и оставить** (решение того же дня).
3. **Все карты всех версий.** Пул — карты MM6, MM7, MM8 и MM9 классов A и B по этапам раздела 6 (B — после
   ручной проверки каждой). Не входят по технической причине: класс D (выход не откроется или карта сюжетная),
   исключённые в разделе 4 (служебные, арены, NWC, открытые регионы MM9).
4. **Класс C пока не трогать.** `6d05`, `6d18`, `cd3`, `7d13`, `7d23`, `7d27`, `7d34`, `d14`, `d19`, `d26`,
   `d34`, `darkpassageway`, `mdt10`, `mdt12` в пул не включать; `onEnter` не делать.
5. **Докачанные blob-ы оставить** в общем объектном складе `/home/plane/projects/openyamm/.git`.

### 11.1а Реализовано (2026-11-18)

Решения 11.1 вошли в правила забега v2 на OpenYAMM 2.7: [roguelike-rules-v2.md](roguelike-rules-v2.md) (пул из 94 карт
по этапам, маршрут хранится в сохранении, страж-одиночка ×2, `groupScale 0.5` на картах от ~145 монстров). При
реализации добавлено то, чего исследование не предусмотрело: мёртвые на входе монстры не засчитываются этапу
(Inventa Storca, 9 трупов в сцене MM9), карта без врагов не запирает выход. Отличия пула от таблиц раздела 6: в пул
не вошли карты меньше ~15 монстров и `t01`/`t02` (нет события входа).

### 11.2 Ещё открыто

1. **Межподземельные связи** (`6d07` → `6d18`, `6d09` → `6d05` и др.): после решения 11.1-4 принимающие карты
   (класс C) в пул не входят, так что вопрос отложен до возврата к классу C.
2. **5-я ступень / «запредельные» карты** (`pyramid`, `cd3`, `6t8`, `lichlab`, `1000terrors`…): отдельный режим
   «финал повышенной сложности» или пока не трогать?
3. **Уникальные предметы** (пользователь прочитает 10.1 и ответит). Какие из предложенных в 10.1 ролей развивать в механики забега, и должны ли они
   оставаться на своих картах (охраняемых местах) или стать общим пулом наград?
