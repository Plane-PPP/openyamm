# OpenYAMM1.18 / Minotaur — Windows ZIP packaged (2026-10-29)

Official base4ab07ed519a66feab24599d69bc0ae19971250e2; update baseline remains
minotaur-1.14-c991da37a2de-20-1. Merge, reagent stacks and numeric inspection supplements implemented.
156 selected Linux cases /36,020 assertions plus3 native Lethal Strike cases /58 assertions passed.
Native game, unit and runtime targets compile under safe-heavy-run/jobs4, no swap/PSI/OOM events.
Windows workflow retains adaptive runner parallelism, adds the new tests and emits minotaur-1.18 versions.
[Successful Windows run40863165794](https://github.com/Plane-PPP/openyamm/actions/runs/40863165794), source
c0ccf0a877bc74fca0aa84a1594ed099d318cbdd. Windows:156 cases /36,020 assertions plus3 native cases /58 assertions.
[Minotaur-Update-minotaur-1.18-c0ccf0a877bc-23-1.zip](https://github.com/Plane-PPP/openyamm/actions/runs/40863165794/artifacts/12894798568)
is143,681,237 bytes (137.03MiB); SHA-256211612350913e54b36d93a016cf6ba0fb57c82afdba138077390ab475b4ac667.
9 changed packs,123 reused (5.27GiB). Windows runner4vCPU/15.99GiB, jobs4; observed RAM peak7.855GiB,
minimum available8.140GiB. Program18m39s, unit/runtime compilation16m36s, Windows job47m59s.
All monitored phases completed with exit0; installer/package/monitor checks passed. No manual retest claimed.
[Full build report and installation](docs/research/windows-1.18-build.md).
[Implementation, compatibility and installation](docs/minotaur-1.18.md).

## Windows gameplay fixes ZIP — 2026-10-27

Implemented / Automatically tested on Windows / Packaged; manual gameplay pending.
Source `c991da37a2de3b6c74a5ed838e38537fb6a5cc2b`.
[Successful Actions run](https://github.com/Plane-PPP/openyamm/actions/runs/40541424092).
[Download ZIP](https://github.com/Plane-PPP/openyamm/actions/runs/40541424092/artifacts/12753968279):
`Minotaur-Update-minotaur-1.14-c991da37a2de-20-1.zip` (22,234,588 bytes).
Base `minotaur-1.14-0b4fc6589a50-19-1`; target `minotaur-1.14-c991da37a2de-20-1`.
SHA256 `de2984f9ea537bdb719bd9524dd2aeaa624db58fad172be70960ca294110f14b`.
No changed resource packs; 132 packs / 5,777,255,868 bytes reused.

Alchemy equipment bonuses, Levitate lava/water protection and Mind Resistance duration included.
No Minotaur balance or save schema changes. 81 selected Windows cases / 32,164 assertions passed;
installer, package-validation and native resource-monitoring checks passed.
Actual GitHub Windows runner: 4 vCPU / ~16 GiB, jobs=4. Peak whole-runner RAM 7.404 GiB,
minimum available 8.591 GiB; compilation CPU means 91.44% / 98.75%, peak 100%.
Game compilation 19m40s; unit compilation 15m47s; Windows job total 50m34s.
All monitored commands exited 0; no guard stop/retry. No local Ubuntu compilation or jobs=8 experiment.
[Full measurements and test instructions](docs/research/windows-gameplay-fixes-build.md).
Back up saves; extract the ZIP and run Apply-MinotaurUpdate.cmd for the existing 1.14 mod install.

## 2026-10-27 — earlier local validation, no Windows package at that stage

Implemented / Automatically tested locally; not packaged: Alchemy equipment bonuses, Levitate environmental protection and Mind Resistance duration.
The restored Linux unit build passed under safe-heavy-run, jobs=4: 260.385 s, peak 4.09 GiB, no swap,
pressure or limit events. 81 selected cases / 32,164 assertions passed. Toolchain is now persistent under
`/home/plane/.local/share/openyamm-toolchain`; no system packages installed and no build data removed.
[Investigation and validation](docs/research/2026-10-27-gameplay-fixes.md).
The last released Windows ZIP and packaging baseline are unchanged. These fixes are not yet manually tested.

# OpenYAMM 1.14 / Minotaur v0.4 + Luck — Windows ZIP packaged

New official1.14 appeared during1.13 validation; exact1.14 now supersedes that unpublished
intermediate port. The update still targets installed `minotaur-1.10-b7e80f0583df-14-1`.
Installer root detection fixed; Minotaur balance retained. New save OYMINO1/v132 maps to
schema131; production mod95 still explicitly maps to94. [Compatibility](docs/research/openyamm-1.14-compatibility.md).
Windows build/tests and ZIP publication succeeded; manual gameplay pending. Earlier1.13 entries below are history.

## Windows 1.14 — Packaged / manual retest pending

[Successful run40325494796](https://github.com/Plane-PPP/openyamm/actions/runs/40325494796), source `0b4fc6589a5003142c2a0a1d3dd82da6d5542d43`.
[Download ZIP patch](https://github.com/Plane-PPP/openyamm/actions/runs/40325494796/artifacts/12654334120): **910,392,473 bytes (910.4 MB)**.
Base `minotaur-1.10-b7e80f0583df-14-1` → `minotaur-1.14-0b4fc6589a50-19-1`.
Changed23 resource packs (888,146,682 bytes); reused109 packs (4,889,109,186 bytes).
The delta includes updated shared icons/models/world items, core/maps and the new ruleset pack.
84.17% less download than the previous5,751,530,172-byte full1.10 archive; no new full1.14 archive was built.
SHA256: `8bc8d0182468ee147f4d50090653156b3e99e0f4b100cc62c4c0ee42c70c4407`.

Automatically tested on Windows:29 selected gameplay cases /30,791 assertions;17 updater tests,
including10 actual PowerShell5.1 filesystem scenarios; full/thin validator regressions.
Native x64 game/launcher, shaders, all resource hashes and embedded teacher/Stats resources verified.
This is selected coverage; MM9 data is outside this package and its Bloodaxe quest is excluded.
The user's actual save and manual gameplay/installation of this1.14 patch remain untested.
Back up saves, run the **new patch folder's** Apply-MinotaurUpdate.cmd and save into a new slot.
Installer supports both nested extraction and scripts placed directly in the game root.
The next build defaults to this newly confirmed baseline; old baselines remain available.

# OpenYAMM 1.13 / Minotaur v0.4 + Luck — integration, Windows checks pending

Port to latest official v1.13; previous balance retained. ZIP update base:
`minotaur-1.10-b7e80f0583df-14-1` (user confirmed installation on 2026-10-26).
Installer now detects extraction directly into the game root as well as a child folder.
Lethal Strike moves to skill ID48 to preserve new upstream IDs; named saved skills are retained.
New save header OYMINO1/v130 maps to schema129; old mod95 remains explicitly schema94.
[Compatibility review](docs/research/openyamm-1.13-compatibility.md). Windows packaging and manual gameplay pending.

# Minotaur incremental Windows updates — first patch packaged

Release workflow now defaults to incremental ZIPs. Initial base is the existing1.10
package source13d2cf3, run39941527807 (131 content packs). No gameplay code or balance changed.
The small app bundle is always delivered; only changed logical resource packs accompany it.
Full installation remains available through package_mode=full/both.
[Player installation](packaging/minotaur/README-RU.txt), [maintainer guide](packaging/minotaur/README.md).
15 tests passed on Windows, including8 real Windows PowerShell5.1 installation scenarios.
A new package must not be called successful or manually played before actual results.
The previous full packages below remain historical installation bases.

# Minotaur v0.4 + Luck на OpenYAMM1.10 — Windows packaged

Official latest v1.10:3f06332001c7a841cc2c2d161f192a1252b25713.
Previous confirmed Windows source:a1b8debf0198779d1ea418555c50bb50c21e8028, run39898467800.
Full rebuild requested; previous fixes and balance retained, including Luck bonus.
Save reader supports production mod1.5/v89 and1.7/v94; writes OYMINO1/v95.
[Compatibility and installation](docs/research/openyamm-1.10-compatibility.md).
Use a separate folder, copy only saves, save into a new slot; do not copy old assets/settings.
Automatically tested:27 focused/30639,3 runtime/58,227 selected/7540 passed (groups overlap).
Final changed test was compiled/relinked incrementally after the host dependency cache loss;
no fresh full Linux executable rebuild is claimed. Windows build succeeded; manual verification pending.
LS-003 Planned, Identify Monster/bestiary Deferred. No merge into main.

## Historical builds

# Minotaur v0.4 + Luck на OpenYAMM1.7 — Windows packaged

Официальный v1.7:069df4a3f63c2dcaeed7b70694529154e7ccc205.
Предыдущая подтверждённая сборка на1.5:bad9c2798b3320f16a108661c588d536b98770da, run39297183309.
LS-001/LS-002 встроены; LS-003 Planned, Identify Monster/bestiary Deferred.
Изменение баланса по запросу2026-10-22: `B=0.5*floor(max(0,currentLuck)/10)`
добавляется к P0 всех рангов до KL и предела30%. Удача90: +4.5 п.п.
Axe и Damage Resistance не изменены.

Сохранения OYMINO1/v89 (мод1.5) читаются как upstream88+combatLevel;
новая запись OYMINO1/v94 включает upstream93. [Подробности](docs/research/openyamm-1.7-compatibility.md).
Распаковать отдельно, сохранить резервную копию прежней установки, перенести только saves.
Запустить OpenYAMM.exe, загрузить копию и сохранить в новый слот. Не переносить старые
assets/settings/hotfix. Новые saves не предназначены для старого executable.
Автообновление на официальный OpenYAMM по умолчанию отключено, чтобы не заменить мод.
Automatically tested:26 focused/30530 assertions,3 runtime/58,223 selected/7506.
Выборка неполная; sparse MM6/MM9/model cases и старый positional recovery-enchant
test исключены (его поле enchant сдвинул upstream hardened, игровой баланс не меняли).
Package verifier и model-profile packaging passed; stale-bake checker0 на sparse assets.
Windows packaged successfully; manual verification pending. See published result below.

---

## Историческая сборка1.5

### Minotaur Rebalance v0.4 на OpenYAMM1.5 — историческая сборка

Upstream tag1.5:1414884155362e613977a3444a3c7f81ed4ee047 (release2026-10-17).
Предыдущий Windows executable:385a1e447aadb17cdf3b0db36691a92889f23786,
[run38784738765](https://github.com/Plane-PPP/openyamm/actions/runs/38784738765).
Последующий hotfix1 (79b01d3/c259af9/4fcdbb0) уже встроен в engine resources.
Баланс Lethal Strike v0.4/Axe/DR не меняется. LS-003 не реализован, тексты ожидаются.

Implemented / Automatically tested: Linux executable и оба test targets собраны;
25 focused/30294 assertions;3 runtime/58;205 selected/7329 passed (пересекаются).
[Save compatibility review](docs/research/openyamm-1.5-compatibility.md): старый mod85
загружается и пересохраняется; upstream85/88 различаются; новый header OYMINO1/version89.
Конкретный save пользователя и новая Windows-сборка ещё не Manually tested.

Первая Windows-попытка: [run39287322676](https://github.com/Plane-PPP/openyamm/actions/runs/39287322676),
source SHAcbaea4b. Компиляция и упаковка прошли; Verify упал из-за старого требования
mmmerge.zip. Тег1.5 содержит только миры MM6/MM7/MM8. Upload не выполнялся.
Проверка исправлена: точный состав1.5, x64 PE, побайтовая сверка трёх hotfix-ресурсов
в staged engine.zip с checkout. Python regression подтверждает отсутствие требования
mmmerge и отказ при отсутствии MM8. Повторный Windows packaging: [run39297183309](https://github.com/Plane-PPP/openyamm/actions/runs/39297183309),
source SHA`bad9c2798b3320f16a108661c588d536b98770da`; **completed/success**.
Job Windows x64 release:2026-10-19T17:47:56Z →18:21:03Z,33m7s.
[OpenYAMM-windows-Release.zip](https://github.com/Plane-PPP/openyamm/actions/runs/39297183309/artifacts/12211627532),
artifact ID12211627532,4 764 095 432 байта. Compile/Verify/Upload успешно завершены.
Проверка staged engine.zip подтвердила побайтовое совпадение Stats/NPC hotfix-ресурсов
с checkout; PE x64 и обязательные пакеты/шейдеры/документы проверены.
Новый полный пакет готов к ручному тестированию, но не считается Manually tested.
[Метаданные run и проверок](docs/research/windows-v0.4-openyamm-1.5-build.json),
подтверждены2026-10-20. Независимая полная загрузка ZIP не заявляется.
Техническая правка workflow: весь tools/ для новых1.5 texture cooker sources,
cook-on-build OFF для committed cooked assets. CI job/публикация прежние.
Локальная sparse проверка stale lighting:0; полной проверки всех lighting assets не заявляется.

Установка: новая отдельная папка, резервная копия старых saves; переносить только копии saves,
после загрузки сохранить в новый слот. Не копировать старые assets/assets.zip/engine.zip/settings:
старый overlay скроет ресурсы1.5. Старые executable не читают новый mod-format.

## История предыдущих пакетов и hotfix (до переноса1.5)

# Lethal Strike v0.4 — Windows test build

> **LS-002 исправлен ресурсным hotfix:** четыре строки HP/SP/AC/DR помещены внутри
> прежней рамки, Condition/Quick Spell возвращены на исходные позиции, Earth/Mind/Body
> имеют разные Y. Автоматическая геометрия проверена; пользователь подтвердил исправленную вёрстку.

> **LS-001 исправлен в ресурсном hotfix:** темы загружаются; Expert у Silk Nightwalker
> (Expert Perception), Master у Robert Morningstar (Master Mace), GM у Garic Senjac (GM Axe).
> Исходная сборка без патча сохраняет дефект. Ручная проверка патча ещё не выполнена.

ТЗ пользователя 2026-10-15 разрешает реализацию, commit/push и запуск существующего
`Minotaur Windows build`. Merge main и GitHub Release не выполняются. Старые разрешения/планы
ниже относятся к build #3; они не являются статусом новой сборки.

Implementation commit: `a9b27e758fd136b0132732421f2997d6679296d1`.
Предыдущая подтверждённая сборка: `142a54e01da50d9faf258e38919e88dba9e8de29`,
[run #3](https://github.com/Plane-PPP/openyamm/actions/runs/38148278640).
Новая Windows-сборка **успешна**:
[Actions run38784738765](https://github.com/Plane-PPP/openyamm/actions/runs/38784738765),
workflow_dispatch; job `Windows x64 release` завершилась2026-10-15 в18:50:12UTC (30m5s).
Packaged source SHA: `385a1e447aadb17cdf3b0db36691a92889f23786`.
Артефакт: [OpenYAMM-windows-Release.zip](https://github.com/Plane-PPP/openyamm/actions/runs/38784738765/artifacts/11999735772),
ID11999735772, размер3 564 147 209 байт. Compile, Verify Windows package и Upload прошли.

Последующие383a137/daa86f9 меняют только тесты и отчёты; игровой код/ресурсы совпадают
с packaged SHA385a1e4. Финальная запись в документации также не требует пересборки.
Новая сборка **готова к ручному тестированию**, но не считается Manually tested.
Дополнительная read-only проверка опубликованного ZIP подтвердила BUILD-REVISION.txt
(текущий и предыдущий SHA), обязательные файлы, x64 PE-заголовок executable, документацию
и отключённые по умолчанию trace-настройки. Полное содержимое executable и вложенных
архивов этой проверкой не проверялось; результат —
[windows-v0.4-build.json](docs/research/windows-v0.4-build.json).

## Ресурсный hotfix 1 — LS-001 / LS-002

Для пакета SHA385a1e4 подготовлен [Minotaur-v0.4-hotfix-1.zip](docs/patches/Minotaur-v0.4-hotfix-1.zip)
(58 679 байт); [инструкция](docs/patches/README.md). Новый executable не нужен:
штатный AssetFileSystem монтирует assets/assets.zip перед engine.zip. В исходном пакете
assets/assets.zip отсутствует; после распаковки он добавляется, engine.zip сохраняется.

Изменены только npc.txt, npc_topic.txt, character.yml. LS-001: шестиколоночные темы,
Expert у Expert Perception Silk Nightwalker (Alvar, goto3/event31), Master у Master Mace
Robert Morningstar (Garrote Gorge, goto5/event13), GM у GM Axe Garic Senjac (goto35/event225).
LS-002: четыре resource rows внутри исходной рамки без сдвига соседних блоков,
Condition/Quick Spell на исходных позициях, Earth/Mind/Body с разными координатами.

Hotfix Automatically tested:22 tests/30155 assertions, включая Axe/DR regression;
реальные загруженные темы, действия диалога и повышение через них, GM cap,
геометрия строк/рамок и lookup overlay по canonical/legacy UI paths.
Архив воспроизводим; вложенный overlay и hashes сверены с тремя исходными ресурсами.
Windows hotfix **частично Manually tested**: пользователь подтвердил Stats и появление тем. Полная пересборка/новый Actions run не запускались;
source SHA executable остаётся385a1e4, hotfix отмечен MINOTAUR-HOTFIX-1.json.
Старые результаты сборки и тестов ниже относятся к базовой v0.4.

## Реализовано

- Полноценный Lethal Strike ID40, численный рост, Normal1 для всех Minotaur/Lord, старые saves,
  M/GM caps и три дополнительных teacher topics без изменения Axe-услуг.
- v0.4: N0.60S; E0.90S+0.56V; M1.10S+0.64V; GM1.30S+0.72V, базовые S/V, без Identify Monster.
- KL от актуальных уровней, cap30%, GIANT строго ниже60%HP с полным шансом; один melee trial.
- Явная terminal death без фиктивного урона, заметное High-priority сообщение4секунды.
- Динамический RMB P0 и Damage Resistance под Armor Class с общей действующей формулой.
- Независимый lethal_strike_trace и тестовые команды skill/player level/player class/actor health/lethal chance.
- Optional minotaur_russian_text для новых русских mod-текстов, English по умолчанию.
- Save v85 хранит cached outdoor combat Level; чтение v84 и ранее сохранено. Синтетический
  v84 fixture создан старым production-сериализатором и не содержит пользовательских данных.

## Автоматические проверки

Linux Release, C++20, `OPENYAMM_USE_VULKAN=OFF`.
Собраны `openyamm`, `openyamm_unit_tests`, `openyamm_runtime_tests`.
В локальном неполном dependency checkout отсутствовали3 стандартных GLSL binaries bgfx;
они загружены из закреплённой версии a73c12db8f502022d292cc6fb8872482997b0664 в build/_deps,
без изменения проекта/CI. Неизменённый character_inspect.yml восстановлен из Git для проверки layout.

- Focused: **21 test /30044 assertions, passed** — новые формулы/пороги/классификация,
  выдача/рост/найм/миграция и настоящий v84 reader, обучение/продвижение, динамический P0,
  layout Stats, одна проверка за melee и инвариантность trace, исключения shooting/spells/objects;
  все12 предыдущих Axe/Minotaur-тестов включены.
- Runtime: **3 tests /58 assertions, passed** — реальное создание Minotaur через NewGameScreen
  сверх выбранных skills; реальные indoor/outdoor terminal paths: HP0, dying, XP, death feedback,
  outdoor MonsterKilled queue;100%-й Wolf's Eye drop создаёт один реальный world item в обоих
  adapters, повторная смерть отвергается без повторных XP/events/drops.
- Extended: **187 tests /7048 assertions, passed** — attack/recovery/equip/Armsmaster/settings/
  save/mastery/skill/bolster. Прогоны пересекаются; числа не суммируются как независимое покрытие.
- CSV/compiled registry: повторная генерация дала одинаковые SHA256 всех11 аналитических/генерируемых
  файлов; утверждённые658 категорий и5760 вероятностных сценариев. Относительные ссылки проверены.
- `git diff --check` успешен с `core.whitespace=cr-at-eol` для родных CRLF TXT. Старые class caps
  побайтно совпали; добавлена только строка LethalStrike.

Команды (при необходимости локальный cmake находится в /tmp/openyamm-cmake/usr/bin,
с LD_LIBRARY_PATH=/tmp/openyamm-cmake/usr/lib/x86_64-linux-gnu; это особенность среды):

```sh
cmake --build build --target openyamm_unit_tests openyamm_runtime_tests openyamm --parallel 4
./build/tests/openyamm_unit_tests --test-case='*Lethal Strike*,*Axe Master*,*Axe GM*,*Axe trace*,*Axe traced*,*minotaur*'
./build/tests/openyamm_runtime_tests --test-case='*Lethal Strike*'
./build/tests/openyamm_unit_tests --test-case='*attack*,*recovery*,*equip*,*Armsmaster*,*settings*,*save*,*mastery*,*skill*,*bolster*' --test-case-exclude='*font*,*goblinwatch*,*merged continent weather*,*android settings*,*development and release settings*,*MM6 promotion sidecars*,*Every source MM9 RUDE skill-training*,*outdoor facet overlays reconstruct old saves*,*mm6 Shrine of the Gods*'
python3 tools/generate_lethal_strike_research.py
python3 docs/research/final-review.py
git diff --check
```

Первый расширенный прогон имел5 failures от отсутствующих sparse assets: MM6 promotion sidecars,
MM9 trainer table, MM6 common scripts (2 cases), character inspect layout. Layout восстановлен;
остальные4 вне текущей задачи исключены точными именами. Предшествующее расхождение
combat_target_panel development/release и недоступные fonts/maps/Android tests также исключены.
**Полный suite не заявляется успешным.** Соответствующий CI не изменялся.

Дополнительные проверки ненулевого loot и разрешённой пары Mace main / Axe off-hand выполнены
отдельными тестовыми этапами после dispatch;
production source Windows run385a1e4 остался тем же. Новых изменений игрового кода не было.

## Ручная проверка и ограничения

Новый билд **не Manually tested**. Проверить куклу/оружие, реальный звук/анимацию/добычу и квест,
читабельность сообщения4секунды и русских glyphs, RMB P0, Stats при разных разрешениях,
всех учителей с сюжетными ограничениями, реальные старые saves и частоту способности.
Автотест world death использует синтетические actors и детерминированный item loot fixture без graphics/audio;
он не является ручным прослушиванием звука или проверкой всех конкретных квестов/loot tables.
Формат v85 старым executable не читается: работать с копиями тестовых сохранений.
Для старого outdoor save, не хранившего боевой Level, миграция использует текущие mounted bolster rules.
MM9 actor canReceiveDamage=false остаётся защищённым. Unknown templates — NORMAL с trace warning.

[Подробные команды и сценарии проверки](docs/MINOTAUR_REBALANCE.md#lethal-strike-v04--implemented),
[действующий контракт](docs/design/death-strike.md), [ROADMAP](docs/ROADMAP.md).
Identify Monster (включая caps/UI), knowledge, spell bonuses и бестиарий — Deferred.
Старые расчёты v0.2 сохранены как исторические, не описание реализованного баланса.

---

## Исторические записи build #3

# Minotaur Rebalance — следующая Windows-сборка

> Исторические секции ниже описывают подготовку build #3. Актуальная запись после выпуска:
> [Actions run #3](https://github.com/Plane-PPP/openyamm/actions/runs/38148278640),
> workflow_dispatch 2026-10-11, Windows x64 release success,
> SHA `142a54e01da50d9faf258e38919e88dba9e8de29`, артефакт `OpenYAMM-windows-Release.zip`.
> Метаданные run/артефакта подтверждены API 2026-10-14. Пользователь сообщил результаты
> ручных тестов Windows11: [CHANGELOG](CHANGELOG.md#minotaur-windows-build-3--ручные-испытания).
> Фраза «ещё не выполнено» ниже относится к моменту подготовки. Историческое разрешение
> packaging/push не разрешало новый запуск на этапе исследования; приведённое там ограничение
> «только документация» относилось к исследованию. Новый запуск v0.4 отдельно разрешён текущим ТЗ.
> От предыдущего подтверждённого `4c3d726...` входят commits `6869538...` и `142a54e...`.
> Сам установленный ZIP и BUILD-REVISION.txt пользователя отдельно не сверялись.

Windows packaging в ветке `feature/minotaur-rebalance` разрешён пользователем. GitHub Release не создаётся.
HEAD до текущих изменений: `4c3d7267346b03dafb76a53ecf99e5c1863101de`.
Commit реализации и тестов: `6869538e3eb22954ba4f224ad0f44890e703dba1`.
Точный SHA packaged revision дописывается workflow ниже и в BUILD-REVISION.txt из GITHUB_SHA.
Предыдущая подтверждённая Actions-сборка: `4c3d7267346b03dafb76a53ecf99e5c1863101de`,
run https://github.com/Plane-PPP/openyamm/actions/runs/38039764246; успешна, ZIP artifact присутствует.
Относительно неё добавлены обе диагностики, новые Master/GM Axe, игровые тексты, тесты
и документация; пассивное сопротивление уже было в предыдущей сборке и сохраняется.

## Состав следующего пакета

- Пассивная физическая защита Minotaur / MinotaurLord без изменения формулы.
- Существующий `minotaur_damage_trace`.
- Master Axe off-hand; GM одноручный Recovery; GM двуручный ×2 физического урона.
- Новый `axe_damage_trace`, актуальные тексты и документация.

## Проверки до запуска Actions

- Локальная сборка `openyamm_unit_tests` успешна (C++20, Linux).
- Целевой прогон Axe + Minotaur: **12 тестов / 10633 проверки, все успешны**.
  Он включает 7 существующих Minotaur-тестов и 5 новых Axe-тестов.
- Расширенный профильный прогон: **95 тестов / 5532 проверки, все успешны**.
  Прогоны пересекаются; их числа не следует складывать как независимое покрытие.
- Прямой headless-тест общего GameplayActionController: фиксированные seed 0–15,
  одноручный/двуручный/смешанный набор, с Haste и без него, оба состояния диагностики.
  Сравниваются урон, Recovery и бинарное сохранение состояния партии;
  проверяются реальные промежуточные значения в axe_attack.
- Синтаксическая проверка GameApplication.cpp успешна; GameplayActionController.cpp
  также собран и связан в unit target с реальным общим контроллером и заглушками представления.
- YAML существующего workflow и синтаксис его shell-скриптов проверены, packaging не вызывался.
- `git -c core.whitespace=cr-at-eol diff --check` успешен; родной CRLF skill_des.txt сохранён.
- Предыдущий этап Minotaur-диагностики отдельно: 7 тестов / 1146 проверок.
- Ручное тестирование, Windows-компиляция и Windows packaging **не выполнены**.

Команды целевого и расширенного прогонов (выполнять последовательно):

```sh
cmake --build build --target openyamm_unit_tests --parallel 4
./build/tests/openyamm_unit_tests --test-case='*Axe Master*,*Axe GM*,*Axe trace*,*Axe traced*,*minotaur*'
./build/tests/openyamm_unit_tests --test-case='*attack*,*recovery*,*equip*,*Armsmaster*,*settings*' --test-case-exclude='*font*,*goblinwatch*,*merged continent weather*,*android settings*,*development and release settings*'
```

В текущем окружении CMake запускался из `/tmp/openyamm-cmake/usr/bin/cmake` с его
`LD_LIBRARY_PATH`; это локальное расположение инструмента, не требование проекта.

Ограничения расширенного прогона: sparse checkout не содержит карт/шрифтов/Android-профиля,
нужных исключённым тестам. До исключения также воспроизведён уже существовавший сбой
`development and release settings expose the same options`:
`[gameplay] combat_target_panel` есть только в release-профиле. Разница подтверждена
и в исходном HEAD, поэтому не исправлена посторонним изменением. Полный suite не заявляется успешным.

Для ручного прогона: проверить куклу и сохранение обеих рук, Master/GM учителей,
GM10/20/30 с Speed/Haste/Armsmaster, двуручные попадания по сопротивляющимся целям,
магические зачарования, обе диагностики и реальные HP. Sword слева у обычного минотавра
недоступен из-за существующего Expert-потолка. Подготовка персонажей и команды логов —
в docs/MINOTAUR_REBALANCE.md.

## Выпуск

Использовать `.github/workflows/minotaur-windows.yml`, job Windows x64 release,
существующий target `openyamm_windows_release_zip`.
После разрешённого commit/push запускается существующий workflow (push или workflow_dispatch).
Артефакт назван `openyamm-minotaur-${github.sha}-windows-x64`; тесты в этом workflow отключены,
поэтому локальные автоматические проверки не заменяются packaging.
Перед выпуском зафиксировать новый SHA и проверить diff относительно SHA предыдущего реального пакета.

## Готовность

Изменения подготовлены для следующего **тестового** Windows packaging существующим workflow:
документация добавлена в sparse checkout и пакет, наличие файлов проверяется в ZIP.
`BUILD-REVISION.txt` генерируется workflow из фактического `GITHUB_SHA`.
При ручном workflow_dispatch указать `previous_build_sha` предыдущего подтверждённого пакета:
тогда файл также содержит его SHA и `git diff --name-status` между сборками.
Без этого параметра сравнение явно помечается как ожидающее, а предыдущий SHA не выдумывается.
Windows-компилятор, полный ZIP и ручная игра ещё должны подтвердить результат.
Commit, push и запуск packaging разрешены пользователем для этого этапа. GitHub Release не разрешён и не создаётся.

## Файлы изменений относительно предыдущей сборки

Список включает изменения предыдущего этапа Minotaur-диагностики, вошедшие в commit реализации.

- `.github/workflows/minotaur-windows.yml`
- `.gitignore`
- `AGENTS.md`
- `README.md`
- `assets_dev/engine/data_tables/english/skill_des.txt`
- `cmake/CreateOpenYammReleasePackage.cmake`
- `game/app/GameApplication.cpp`
- `game/app/GameSettings.cpp`
- `game/app/GameSettings.h`
- `game/debug/GameplayDebugTrace.cpp`
- `game/debug/GameplayDebugTrace.h`
- `game/gameplay/GameMechanics.cpp`
- `game/gameplay/GameMechanics.h`
- `game/gameplay/GameplayActionController.cpp`
- `game/gameplay/GameplayCombatController.cpp`
- `settings.ini`
- `settings_release.ini`
- `tests/CMakeLists.txt`
- `tests/GameplayRuleRegressionTests.cpp`
- `tests/MenuSettingsTests.cpp`
- `BUILD-NOTES.md`
- `CHANGELOG.md`
- `docs/MINOTAUR_REBALANCE.md`
- `docs/ROADMAP.md`
- `tests/AxeActionTestStubs.cpp`
- `tests/AxeRebalanceTests.cpp`

## Установка поверх предыдущей версии

1. Закрыть игру; сделать резервную копию папки saves и settings.ini из установленной версии.
2. Распаковать новый ZIP во временную папку и перенести содержимое OpenYAMM в папку игры
   с заменой executable, библиотек, runtime и assets (включая engine.zip и world ZIP).
3. Сохранить прежние saves и пользовательский settings.ini: не заменять их пакетными настройками.
   Новые параметры диагностики при отсутствии в старом INI выключены по умолчанию.
4. Запустить игру и загрузить копию сохранения. Старый exe с новыми assets отдельно не использовать.

## Следующая сборка: тексты и навигация учителей

Пользователь составит тематические тексты по рангам. Добавить отдельные подсказки с именем
и местом следующего специалиста — [LS-003](docs/research/lethal-strike-teacher-dialogue-followup.md). Пока Planned, не Implemented.
Исправления hotfix1 уже в исходных ресурсах и войдут в engine.zip следующего полного пакета
из feature/minotaur-rebalance; отдельно ставить старый hotfix на новый пакет не потребуется.

## Windows 1.7 + Luck — Packaged / manual retest pending

[Run 39898467800](https://github.com/Plane-PPP/openyamm/actions/runs/39898467800) completed successfully: 2026-10-23T04:35:10Z.
Source `a1b8debf0198779d1ea418555c50bb50c21e8028`; previous confirmed1.5 `bad9c2798b3320f16a108661c588d536b98770da`.
[Download OpenYAMM-windows-x64-Release.zip](https://github.com/Plane-PPP/openyamm/actions/runs/39898467800/artifacts/12478309627), 5,750,960,934 bytes.
Windows compile/link, versioned package, x64 launcher/game PE, all indexed content hashes,
MM6/MM7/MM8 presence and exact embedded teacher/Stats resources passed; artifact uploaded.
No old hotfix overlay is required. User manual playthrough and actual save remain untested.

## Windows1.10 — Packaged / manual retest pending

[Run39941527807](https://github.com/Plane-PPP/openyamm/actions/runs/39941527807) completed successfully at 2026-10-23T11:18:35Z.
Source `13d2cf3bc1e1b0c005a15f4a253bc0dba875522b`; previous confirmed package was1.7/a1b8deb.
[Download OpenYAMM-windows-x64-Release.zip](https://github.com/Plane-PPP/openyamm/actions/runs/39941527807/artifacts/12498430051), 5,751,530,172 bytes.
Compile/link, launcher/game x64 PE, new model-emissive shader, indexed content hashes,
MM6/MM7/MM8 presence and exact embedded teacher/Stats resource comparison passed.
Artifact uploaded. Manual Windows gameplay and the user's actual save remain untested.
Use a separate install, copy only saves, keep originals and save in a new slot.

## Первый ZIP-патч — Packaged / manual retest pending

[Run40000571371](https://github.com/Plane-PPP/openyamm/actions/runs/40000571371) успешно завершён2026-10-23. Source `b7e80f0583df4e594718cb3de63f87c4a0793fc7`.
[Скачать первый патч](https://github.com/Plane-PPP/openyamm/actions/runs/40000571371/artifacts/12522948534): 20,316,780 байт (20.3МБ) вместо
5,751,530,172 байт полной1.10, сокращение загрузки99.65%.
База `minotaur-1.10` → `minotaur-1.10-b7e80f0583df-14-1`.
Переиспользован 131 пакет (5,766,643,023 байт на диске),
изменённых ресурсных пакетов0. Поставляется программа и установщик.
Windows: 15 тестов passed, включая 8 сценариев Windows PowerShell 5.1.
Отдельно прошли проверки validator full/thin.
Compile/PE/шейдеры/хеши/встроенные исправления учителей и Stats проверены.
API размер/SHA256 артефакта совпадают с отчётом сборщика. Полный ZIP независимо не скачивался.
Ручной запуск игры и установка на реальную пользовательскую папку ещё не проверены.
Новая база сохранена в репозитории; следующая сборка по умолчанию использует её.
