## OpenYAMM 1.20 integration — merged; validation in progress

Official stable v1.20: 1add1d374848648887c08707199921335b4ba440, published 2026-10-30.
Backup: backup/minotaur-before-v1.20-2026-10-31 at b776797. Normal merge preserves history.
Retain upstream search/bag fallback/death explosions and the mod's stack/Minotaur APIs.
Mod143 wraps upstream142; mod140 explicitly continues to decode upstream139, including the save-list summary path.
Native multi-cell geometry and safe layout migration implemented. Linux189/48,755 and native7/159 passed.
Additional save/inventory selection165/21,008 passed (overlapping). Visual preparation blocked by incomplete
local terrain resources; no rendered screenshots claimed. Next: Windows CI/package, then user campaign/visual retest.
[Integration audit](research/openyamm-1.20-stack-geometry.md).

## Historical1.18 UI & inventory hotfix — superseded geometry; other fixes retained

Known-by-rules item names and stack identification compatibility corrected; genuine unknowns preserved.
Shared bounded stack icon geometry fixes Wasp Wing/Gog Blood overflow. Empty bottles stack to 50;
brewing consumes 1 bottle and 1 reagent with atomic cursor placement and safe MAW closed-bag handling.
Seven stat tooltips keep original/current/next information without threshold tables; HP shares the SP/AC/DR edge.
169 gameplay cases / 39,730 assertions and 4 native world/layout cases / 123 assertions passed.
Additional inventory/chest/corpse selection: 89 cases / 6,820 assertions passed.
[Causes, compatibility and manual checklist](research/ui-inventory-hotfix.md).
[Successful Windows run and31.27MiB ZIP](research/windows-ui-inventory-hotfix-build.md) over the prior1.18 package.
Next: user campaign/visual retest. Confirmed next-update baseline recorded; existing deferred design scope unchanged.

## Expanded inspection — Implemented / automatically tested / Windows ZIP packaged

Real Standard attribute bonuses/thresholds and armor/weapon Recovery now supplement original descriptions.
Read-only equip previews, ruleset Recovery overrides and bounded wheel/touch scrolling included.
156 selected cases /36,020 assertions and three native Lethal Strike cases /58 assertions passed.
Linux and Windows checks passed; [Windows package and resource report](research/windows-1.18-build.md).
[137MiB update ZIP](https://github.com/Plane-PPP/openyamm/actions/runs/40863165794/artifacts/12894798568)
updates minotaur-1.14-c991da37a2de-20-1 to minotaur-1.18-c0ccf0a877bc-23-1.
A separate local inventory/loot selection passed83 cases /4,076 assertions; debug-Lich test scope corrected.
Next stage: user campaign-save, inventory and visual retest; then separately scoped corrections.
Identify Monster, bestiary and teacher flavor/next-teacher hints remain Deferred.
[Current 1.18 notes](minotaur-1.18.md).

## Reagent stacks — Implemented / automatically tested on Linux and Windows

Up to 50 ordinary same-property reagents per stack; atomic pickup, manual merge/remainder, split controls,
sorting, single-unit brewing, sale/buy pricing and quantity persistence added.
21 selected cases /1187 assertions passed. [Current 1.18 notes](minotaur-1.18.md).
Expanded tooltips and final unit/runtime/Windows packaging checks passed; manual gameplay remains pending.

## OpenYAMM v1.18 integration — Implemented / automatically tested / packaged

Merged official v1.18 `4ab07ed519a66feab24599d69bc0ae19971250e2` into the feature branch,
from backup `backup/minotaur-before-v1.18-2026-10-29` (`9dcf1c7`).
Official Alchemy/item/follower, black-potion and Mind Resistance fixes retained; no duplicate bonus.
Levitate setter duplication removed; pre-movement sync and queued-effect guards retained.
Adapted Lethal Strike death handling to ShieldHitModifiers without changing balance.
Cybernetics keeps upstream ID48; Lethal Strike moves to49, named saved skills and teachers preserved.
Mod save140 maps to upstream 139; production mod 132 maps explicitly to131; older mappings retained.
Original 1.14 binary generated a frozen v132 fixture; 85 selected cases /32,239 assertions passed.
Reagent stacks, expanded tooltips and Windows packaging completed in the stages above.
Historical intermediate counts below describe their own stage, not the current package.

## История: подготовка обновления upstream — заменено интеграцией v1.18 выше

Проверены наши Alchemy/Mind Resistance/Levitate против upstream `fc4e117` (2026-10-28).
[План объединения и проверки](research/upstream-fc4e117-fix-integration.md).
Найдены 4 конфликтных фрагмента в 3 файлах Levitate и скрытые дубликаты setter, включая
автоматически объединённый заголовок. Полного merge/реализации/сборки не было.
Alchemy/Mind Resistance пока сохранить; объединить Levitate без дубликатов, сохраняя новые
upstream floorBurn/сопротивления/FireImmune и наши first-update/queued-tick guards.
Реальная интеграция и все её проверки остаются Pending. Последняя Windows-сборка ниже не менялась.

# Windows gameplay fixes — packaged / manual retest pending

Alchemy equipment bonuses, Levitate environmental protection and Mind Resistance duration:
Implemented / Automatically tested on Linux and Windows / Windows ZIP packaged.
81 selected Windows cases / 32,164 assertions passed; installer and package checks passed.
[Package, measurements and instructions](research/windows-gameplay-fixes-build.md).
Source `c991da37a2de3b6c74a5ed838e38537fb6a5cc2b`, successful run40541424092.
22.2 MB update for installed `minotaur-1.14-0b4fc6589a50-19-1`; 132 content packs reused.
Next-update baseline: `minotaur-1.14-c991da37a2de-20-1`.
Remote Windows runner had 4 vCPU / ~16 GiB and used jobs=4; peak RAM 7.40 GiB.
Local Ubuntu limits unchanged; no jobs=8 experiment. Next: user's actual-save installation/gameplay retest.
Identify Monster, bestiary and teacher flavour/next-teacher hints remain deferred/pending as previously recorded.

# Initial OpenYAMM 1.14 port / Minotaur v0.4 + Luck — previous package

New official1.14 appeared during1.13 validation; exact1.14 now supersedes that unpublished
intermediate port. The update still targets installed `minotaur-1.10-b7e80f0583df-14-1`.
Installer root detection fixed; Minotaur balance retained. New save OYMINO1/v132 maps to
schema131; production mod95 still explicitly maps to94. [Compatibility](research/openyamm-1.14-compatibility.md).
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
[Compatibility review](research/openyamm-1.13-compatibility.md). Windows packaging and manual gameplay pending.

# Minotaur Rebalance — Roadmap

Действующее ТЗ: **Lethal Strike v0.4 + Luck**, 2026-10-15 с дополнением2026-10-22. Оно заменило Death Strike v0.2.
Identify Monster и бестиарий — Deferred. Исследование69bed40 сохранено; оно не ограничивает
прямо утверждённые в v0.4 формулы/категории/учителей.

## Подтверждённая база: Windows build #3

SHA142a54e01da50d9faf258e38919e88dba9e8de29,
[Actions run #3](https://github.com/Plane-PPP/openyamm/actions/runs/38148278640).
Пользователь на Windows11 проверил двуручный GM Axe ×2, два одноручных топора, off-hand и куклу,
описания Axe, Recovery по логам и пассивную физическую защиту против стрекоз на Emerald Island.
Это ограниченная ручная проверка перечисленных механик, не всех сочетаний/сохранений/ситуаций.
Axe, Armsmaster и формула пассивной защиты в v0.4 сохраняются.
Подробности: [CHANGELOG](../CHANGELOG.md), [описание мода](MINOTAUR_REBALANCE.md).

## Этапы разработки

Предыдущий подтверждённый Windows-пакет:1.7+Luck, source a1b8deb; успешный run39898467800.
Сборки1.5 и исходная v0.4 ниже сохранены как история.

| Этап | Статус | Результат |
| --- | --- | --- |
| Исследования таблиц/карт | Research completed | 658 общих шаблонов,198 исходных MM8,184 с кампанийными свидетельствами; [ревизия](research/final-review.md) |
| Контракт v0.4 | Approved | P0 от S/V и бонуса текущей Удачи (2026-10-22), потолок30%, GIANT строго ниже60%, Identify Monster не участвует |
| Реестр категорий | Implemented | 48 GIANT /610 NORMAL; MM8 21/177; явные ID, unknown NORMAL + trace warning |
| Навык и обучение | LS-001 fixed / topics manually confirmed | Корректная схема тем; Expert Perception / Master Mace / GM Axe; [детали](research/lethal-strike-teacher-bug.md) |
| Бой и диагностика | Implemented | Один trial после melee, штатная смерть, заметное сообщение, отдельный trace |
| Skills/Stats | LS-002 fixed / layout manually confirmed | Четыре resource rows внутри рамки, исходные Condition/Quick Spell, раздельные Earth/Mind/Body; [детали](research/damage-resistance-layout-bug.md) |
| Автоматические проверки | Automatically tested on1.7+Luck | 26 focused /30530 assertions;3 runtime /58;223 selected /7506. Ограничения suite в BUILD-NOTES |
| Windows packaging on1.5 | Packaged / success | [Run39297183309](https://github.com/Plane-PPP/openyamm/actions/runs/39297183309); SHAbad9c27; OpenYAMM-windows-Release.zip |
| Ручной тест v0.4 | In progress / hotfix retest pending | Исправление Stats и появление тем подтверждены; LS-003 и боевой прогон остаются |
| Identify Monster | Deferred | Caps/UI/поведение не меняются; knowledge, AC/damage/aura/GM magic отложены |
| Бестиарий/лор | Deferred | Исследования сохранены, парсинг и расширенный интерфейс отложены |

## Ресурсный hotfix 1

LS-001 и LS-002 исправлены отдельными коммитами. Подготовлен [малый патч](patches/Minotaur-v0.4-hotfix-1.zip)
для executable385a1e4, [установка и тест](patches/README.md). Автоматический прогон22 tests/30155 assertions
успешен; пользователь подтвердил вёрстку и появление тем.
Сборка1.5 включала исправления в engine.zip;1.7 включает их в indexed content packs без отдельного hotfix.

## Следующие этапы

Ресурсные исправления LS-001/LS-002 подтверждены пользователем в части Stats и появления тем.
[LS-003](research/lethal-strike-teacher-dialogue-followup.md): следующая сборка должна получить
тематические тексты пользователя и подсказки следующих учителей. Пока не реализовано.

1. Получить тексты пользователя для рангов и реализовать LS-003; затем подготовить следующую полную сборку.
2. Ручной прогон Windows v0.4: частота/сообщение, GIANT60%, dynamic P0, Damage Resistance, учителя и старые saves.
3. Зафиксировать результаты пользователя отдельно от автоматически проверенного.
4. По результатам отдельным этапом корректировать экспериментальный баланс.

Историческая исходная v0.4: автоматические проверки и Windows packaging завершены. Production source —
385a1e4. Hotfix меняет три ресурса поверх этой сборки, не executable;
предшествовавшие hotfix тестовые/документальные коммиты игровой код не меняли.
Подробности и ограничения — [BUILD-NOTES](../BUILD-NOTES.md).

Каждый законченный этап фиксировать отдельным коммитом с обновлением ROADMAP.
Не выполнять merge main. Новый баланс Axe/DR и Identify Monster не входит в задачу.
Актуальный контракт: [Lethal Strike v0.4](design/death-strike.md).

## Перенос v0.4 на OpenYAMM1.5 — Packaged / user save retest pending,2026-10-19

Официальный тег1.5:1414884155362e613977a3444a3c7f81ed4ee047. До объединения
сохранён fixture старого mod-v85. Обнаружена коллизия номера85 с upstream weather layout;
совместимость обеспечена отдельным декодированием и проверена fixtures. Hotfix LS-001/LS-002 сохранён.

Интеграция и совместимость проверены:25 focused/30294,3 runtime/58,205 selected/7329.
[Отчёт](research/openyamm-1.5-compatibility.md). Оба hotfix встроены; отдельный старый overlay
на новую сборку не переносить. Windows packaging успешно завершён (run39297183309). Следующий этап: пользовательская проверка копии save.
LS-003 остаётся Planned до текстов пользователя и отдельной реализации, перенос1.5 не блокирует.

Дополнительно проверены3 цикла миграции fixture реального по структуре прогресса v0.4:
inventory/XP/skill points/quest bits/NPC overrides/банк/резерв/посещённые карты сохраняются.
Тестовый commit не менял executable первого run cbaea4b; успешный последующий пакет1.5 — bad9c27.

Проверка пакета приведена к составу1.5 (engine+MM6/MM7/MM8); сверяет x64 PE и актуальные
ресурсы hotfix в engine.zip. Первый run39287322676 завершился на устаревшем Verify,
не на компиляции. Повторный run39297183309 успешно завершён; hotfix-ресурсы внутри пакета сверены с checkout.

Windows artifact: [OpenYAMM-windows-Release.zip](https://github.com/Plane-PPP/openyamm/actions/runs/39297183309/artifacts/12211627532).
Новая сборка ещё не проверена пользователем; переносить копии saves в отдельную установку.

## OpenYAMM 1.7 integration — 2026-10-22

Exact v1.7 merged; explicit production 1.5 save migration and embedded hotfixes retained.
25 focused tests /30394 assertions passed. [Compatibility](research/openyamm-1.7-compatibility.md).
Luck implementation, final regressions and Windows packaging completed; next is user save retest.

Бонус Удачи реализован: +0.5 п.п. за полные10 единиц на всех рангах до KL/cap.
Динамический tooltip и trace используют общий расчёт. CSV включает Luck0/90.
Проверки и упаковка1.7 завершены; ручной прогон ожидается.

Автоматически проверено на1.7+Luck:26 focused/30530,3 runtime/58,223 selected/7506.
Windows packaging завершён; пользовательский save пока не предоставлен.

CI adapted to upstream1.7 release target/launcher/indexed content and Python YAML dependency.
Результат Windows job зафиксирован ниже после фактического завершения.

Windows run39830857374: native compile/link passed, packaging failed because CMake
selected hosted Python without PyYAML despite MSYS2 Python YAML being installed.
Retry pins Python3_EXECUTABLE to UCRT64 and checks import before configure.

Native MinGW launcher now links its C++ runtime statically: root launcher must not depend
on runtime DLLs located only beside the game in app/<version>. Retry39894785306 stopped early
to include this packaging fix before compilation; gameplay code unchanged.

Final tooltip review removed duplicate English Luck prose (already supplied by skill_des.txt);
Russian custom description retains the same rule. Formula/save code unchanged.

## Windows 1.7 + Luck — Packaged / manual retest pending

[Run 39898467800](https://github.com/Plane-PPP/openyamm/actions/runs/39898467800) completed successfully: 2026-10-23T04:35:10Z.
Source `a1b8debf0198779d1ea418555c50bb50c21e8028`; previous confirmed1.5 `bad9c2798b3320f16a108661c588d536b98770da`.
[Download OpenYAMM-windows-x64-Release.zip](https://github.com/Plane-PPP/openyamm/actions/runs/39898467800/artifacts/12478309627), 5,750,960,934 bytes.
Windows compile/link, versioned package, x64 launcher/game PE, all indexed content hashes,
MM6/MM7/MM8 presence and exact embedded teacher/Stats resources passed; artifact uploaded.
No old hotfix overlay is required. User manual playthrough and actual save remain untested.

## Перенос на последнюю официальную версию —1.10

Перед интеграцией сохранён production fixture формата мода1.7/v94.
Далее: объединить точный v1.10, проверить сейвы1.5/1.7, прежние механики и собрать Windows.

Точный v1.10 интегрирован; новая schema95 сохраняет поддержку mod89/94.
[Отчёт](research/openyamm-1.10-compatibility.md). Далее — проверки и Windows CI.

Preflight1.10:26 focused/30630,3 runtime/58,226 selected/7531 passed.
Temporary Linux dependency cache is being restored after the host restart.
Windows CI uses a clean checkout; the additional local save test subsequently passed.

Все финальные локальные проверки1.10 passed:27 focused/30639,3 runtime/58,227 selected/7540.
Тестовый executable инкрементально пересобран/перелинкован; полный Linux rebuild не заявляется.
Windows run39941527807 source13d2cf3 успешно завершён; артефакт проверен. Далее ручной прогон.

## Windows1.10 — Packaged / manual retest pending

[Run39941527807](https://github.com/Plane-PPP/openyamm/actions/runs/39941527807) completed successfully at 2026-10-23T11:18:35Z.
Source `13d2cf3bc1e1b0c005a15f4a253bc0dba875522b`; previous confirmed package was1.7/a1b8deb.
[Download OpenYAMM-windows-x64-Release.zip](https://github.com/Plane-PPP/openyamm/actions/runs/39941527807/artifacts/12498430051), 5,751,530,172 bytes.
Compile/link, launcher/game x64 PE, new model-emissive shader, indexed content hashes,
MM6/MM7/MM8 presence and exact embedded teacher/Stats resource comparison passed.
Artifact uploaded. Manual Windows gameplay and the user's actual save remain untested.
Use a separate install, copy only saves, keep originals and save in a new slot.

## Инкрементальные ZIP-обновления — исследовано2026-10-23

Пользователь выбрал ZIP-патчи. [Оригинальная архитектура и план](research/minotaur-incremental-updates.md).
База1.10 сохранена из опубликованного ZIP HTTP-диапазонами (221884 байта вместо5.75ГБ).
Следующий этап: генератор патчей, Windows-установщик, CI и проверки; игровая механика не меняется.

Инкрементальный выпуск реализован: генератор по content_id, SHA-проверки, Windows PowerShell5.1
установщик с атомарным переключением, версия на каждый build, update/full/both в CI.
Локально7 Python-тестов passed;8 Windows-сценариев ожидают CI.
Далее: Windows-проверка установщика, первый реальный ZIP-патч и публикация новой базы.

Windows tests39960808276 обнаружили недоступный Get-FileHash в дочернем PowerShell5.1.
Установщик переведён на стандартный .NET SHA256 без зависимости от модуля PowerShell. Повторный CI ожидается.

Повтор39961282682 выявил некорректный аргумент backup у File.Replace под Windows5.1.
Для атомарной замены передаётся реальный временный путь; после успеха резервный файл удаляется.
CI запускает дочерний5.1 с его штатным каталогом модулей и без интерактивных запросов.

Удаление временного backup после успешной атомарной замены — best effort: ошибка очистки
не должна сообщать о неуспехе уже активированной версии.

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

1.13 validation: first Windows game build passed; unit build exposed an old Axe test-world
method signature, now adapted to shield-tier API. 17 updater tests passed. Added explicit
EnergyGuns/LethalStrike coexistence save test. Retry remains required before publishing.

1.13 second validation: all game/unit sources compiled; the Windows unit link required
winhttp/bcrypt, now declared for the standalone unit target. Gameplay test results remain
pending until the next run actually executes them.

1.13 gameplay checks executed: 29 applicable cases passed in run40318306385. The broad Axe
name filter also picked an unrelated MM9 Bloodaxe quest requiring unshipped MM9 assets;
exclude MM9* from the MM6/MM7/MM8 mod job and retry packaging.
