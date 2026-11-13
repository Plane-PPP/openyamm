# Minotaur / Medusa / импортёр на OpenYAMM2.3

Статус2026-11-13: основа2.3 с Медузой и импортёром собрана и прошла выбранные Linux-профили.
Скелет Roguelike ещё не переносился; нужен локальный checkpoint готовой основы после разрешения коммита. Это не релиз.

## Источники и изоляция

Основной план Fable: `/home/plane/projects/openyamm-roguelike-notes/codex-minotaur-2.2-prompt.md`.
Пользователь затем повысил цель до2.3 и разрешил реализацию. Target `f4a6b8032759a218505532b2600ac56b6d80c7af`,
Minotaur base `54efa0e975420b7544837244ec5d465923cf1dc4`. Общая база upstream1.21 `c24a0aaa6bb6f7956c84beed1bd44c88e62453c4`.
GitHub latest и объект тега `e285296b5819d7d7b069944bd16a04899e23274b` проверены13ноября.
Все изменения — в `/home/plane/projects/openyamm-minotaur-v2.3`, ветка `integration/minotaur-v2.3-20261113`.
Backup `backup/minotaur-before-v2.3-20261113`; исходные ветки, worktree, сохранения и незакоммиченные файлы не изменялись.
Коммиты/публикация ещё не разрешены. Полные результирующие SHA появятся только после разрешённых коммитов.

## Что перенесено

Medusa: `2c1ae605,b572b956,ba85f7ac,33c9e666,1f1cb19b,b096d52f`.
Импортёр: `74d5f6be,2028de43,5927df31,1536a668,38310474,c8a2a217`; тестовый `72200416` исключён.
Патчи применены без коммитов. Обе корневые папки иконок сохраняются при смене мира; пользовательские иконки
дополняют штатные пакеты и не подменяют их. Современный C2 и ограничения доступных миров сохранены.
Импорт кукол сохраняет тип и геометрию базовой строки; меняются лишь поддерживаемые body/hand изображения.

Medusa контент0.2.2: `347fb2185a29dfd3d2eca47e61f2a13e476cf65b`. Архив этой ревизии подключён локально из
ignored build/medusa-source; чужие untracked-портреты не использованы. Старт и возрождение Antagarich направлены
на `out10.odm`, `3114,-11055,513`, yaw0 через continent_settings.patch.txt; карточка лишь показывает выбор.
Контентный репозиторий не публикуется и не обновляется. Desktop импортёр описан в `devdocs/CUSTOM_PORTRAITS.md`.

## Сохранения

Пишется `OYMINO1/v180` с upstream-схемой179 и дополнительным Minotaur combatLevel. Это отдельное пространство
версий от `OYSAVE1`. Добавлена явная строка155→154; прежние89/94/95/130/132/140/143 и legacy85 сохранены.
Оба пути чтения — полного save и summary — используют явное соответствие. Новая запись не предназначена для старого exe.
Fixture `tests/fixtures/minotaur-1.21-v155.oysav` — подлинный прежний writer,11030байт,
SHA256 `6c7bd4a8b2600e00e97068b50307c46e197541a6f3261d1043d4fdbbc8697e04`.
Происхождение и SHA старого binary — в соседнем TXT. Частный campaign-save не использован.

## Бой и другие изменения общей основы

- Согласованные числовые функции Lethal Strike, Axe и Minotaur DR сохранены; независимый reviewer сравнил9ключевых
  функций с прежней базой. LS остаётся ближним навыком Minotaur/MinotaurLord, не перком стрел.
- Upstream разделил flat meleeWeaponDamageBonus и rangedWeaponDamageBonus; диагностика Axe теперь читает melee.
  Элементальные зачарования роллятся отдельно на каждый hit и сопротивляются своим элементам. Axe GM удваивает
  только физическую часть: тест обновлён на flatphysical9 плюс отдельный Fire9, без возвращения старого смешанного поля.
- Сохранены upstream Carnage/explosive-arrow path и элементальные компоненты стрел; не заменены старой реализацией.
- Пробуждение соседей indoors использует upstream радиус128 вместо1024; это унаследованное изменение, не новый баланс Minotaur.
- Upstream battle orders/upkeep заменили единственное поле attackSpellName; tests используют setAttackSpell.
- Critical и lethalStrike — разные аргументы runtime. LS terminal transition по-прежнему идёт через штатную смерть,
  XP/loot/hooks, а защищённый CanDamage=false actor теперь не сообщает ложную успешную LS-смерть.
- Upstream quick/auto группируются по прохождениям; map-layout identity может сбросить snapshot изменённой карты.
  Старые snapshots MM6/7/8 без identity обрабатываются по upstream policy. Согласованность run variables при сбросе
  карты требует отдельного решения режима; сохранение старых незавершённых забегов не обещано.

Это перечисление унаследованного поведения, а не согласование новых чисел баланса. При желании пользователя
вернуть конкретное поведение1.21 нужен отдельный явный выбор и проверка.

## Проверки и ограничения

До слияния: старый fixture-capture test1/1,18assertions; runner20261113T120847Z-8292b754c3.
Новый CMake configure прошёл; новая программа и test targets ещё собираются. Ошибки старых API в тестах
устраняются; успешный новый test run пока не заявлен. Общие Python проверки: tools/check_doc_i18n.py —0errors,
8прежних warnings до обновления документов; повторить после финальных правок.
Независимый read-only review проверил save mapping и формулы; найденный indoor guard исправлен с runtime assertion.
Все heavy-команды только safe-heavy-run; OPENYAMM_USE_VULKAN=OFF. Локальные зависимости скопированы, исходные build не изменялись.
Отсутствующие sparse-ресурсы и непроверенные ручные/GPU/Windows случаи перечислить после прогонов отдельно.

## Осталось

Завершить Linux проверки и report. Скелет Roguelike — в новой линии от результата, без изменения правил.
Пять исходных коммитов из ТЗ проверить на эквивалентность upstream и перенести только нужное; при переделке остановиться.
Перед commit выполнить doc-i18n и независимый review затронутых важных мест. Windows baseline/одна или две
сборки выбираются пользователем отдельно; текущая baseline1.21 не заменяется предположением.

## Итог Linux-проверок основы

Все три цели собраны в отдельном build: openyamm, openyamm_unit_tests, openyamm_runtime_tests.
Последняя native-сборка: runner20261113T131830Z-fce2ace06b, peak3647.3MiB, exit0.
Профили запускались последовательно под safe-heavy-run; disjoint unit-source selections суммируются в269cases/49 765assertions:

| Профиль | Результат | Лог в build/ |
|---|---|---|
| Minotaur, saves, inventory, Medusa, importer, AssetFileSystem |184/184,46894assertions|integration-focused-tests.log|
| Battle orders, projectiles/Carnage, updater, settings |82/82,1635assertions|integration-upstream-tests.log|
| Стрелы/элементы из GameplayRuleRegressionTests |3/3,1236assertions|integration-arrow-tests.log|
| Native LS, MM6 detection/factions/flying, creation |26/26,3328assertions|integration-runtime-tests.log|
| Python updater tools |8passed,11Windows-onlyskipped|integration-updater-python.log|
| Doc-i18n |0errors,8pre-existingwarnings|tools/check_doc_i18n.py|

Unit184 runner20261113T132859Z-e226399bce; final native+upstream profiles runner20261113T133347Z-9b374d153d.
Прежние неуспешные прогоны не скрыты: сначала недоставало PNG/manifests/maps, затем выявлены неполные
synthetic downgrade fixtures. Исправлены тестовые envelopes, swim-memory offsets, trailing statistics и позиция
legacy attack-spell для157; строгий loader не ослаблен. Chedian expectation обновлён по явной таблице (Sorcerer),
Between Time сохранил Necromancer. В production никаких классовых/балансных таблиц ради тестов не меняли.
Native regression проверяет, что CanDamage=false блокирует LS, сохраняя HP и обычную upstream-семантику удара.

Команды воспроизведения (из интеграционного worktree; установить PATH/LD_LIBRARY_PATH постоянного toolchain):

```sh
safe-heavy-run check
safe-heavy-run run -- cmake --build build --target openyamm_unit_tests openyamm_runtime_tests openyamm
safe-heavy-run run -- ./build/tests/openyamm_unit_tests --source-file='*LethalStrikeTests.cpp,*AxeRebalanceTests.cpp,*ReagentStackTests.cpp,*InspectSupplementTests.cpp,*BottleStackHotfixTests.cpp,*InventoryGeometryHotfixTests.cpp,*ItemNamingHotfixTests.cpp,*RulesetTests.cpp,*CustomPortraitTests.cpp,*ProjectMedusaTests.cpp,*AssetFileSystemTests.cpp,*GameplaySaveLoadUiSupportTests.cpp,*MapLayoutIdentityTests.cpp,*PlaythroughStatisticsTests.cpp'
safe-heavy-run run -- ./build/tests/openyamm_unit_tests --source-file='*BattleOrdersTests.cpp,*GameplayProjectileServiceTests.cpp,*UpdaterTests.cpp,*MenuSettingsTests.cpp'
safe-heavy-run run -- ./build/tests/openyamm_unit_tests --source-file='*GameplayRuleRegressionTests.cpp' --test-case='*arrow*,*ranged*bonus*,*elemental weapon damage*'
safe-heavy-run run -- ./build/tests/openyamm_runtime_tests --source-file='*LethalStrikeWorldTests.cpp,*IndoorActorFactionRuntimeTests.cpp,*IndoorActorDetectionRuntimeTests.cpp,*IndoorActorFlyingRuntimeTests.cpp,*NewGameScreenTests.cpp'
python3 tools/check_doc_i18n.py
```

Полный suite, Windows installer11skips, реальная пользовательская кампания, визуальные/GPU и Android проверки
не выполнены. Этот Linux SDL headless-профиль не подтверждает desktop визуалы. Никаких Vulkan build/CI/push/release.

## Независимое review и точки продолжения

review_v23 читал merge/save/combat и Medusa/importer/C2 без изменения файлов и запуска тяжёлых задач.
Подтверждены9сохранённых числовых функций LS/Axe/DR. Найденные дефекты стыковки исправлены: false-positive LS-kill
у защищённого indoor actor; центровка start-card по старой трёхконтинентной сетке. Проверка synthetic saves
выявила ошибки test helpers; запись production/save whitelist не расширялась.

Точки для последующей проверки (номера текущего worktree):
- game/maps/SaveGame.cpp:26,313,6211,6487 — envelope/schema и два пути чтения.
- tests/LethalStrikeTests.cpp — frozen155 migration и отказ неизвестным mod-envelopes.
- tests/RulesetTests.cpp:531 — явно synthetic official fixtures; сравнение valid official177/refused mod177.
- game/indoor/IndoorWorldRuntime.cpp — раздельные critical/lethalStrike и CanDamage guard;
  tests/IndoorActorFactionRuntimeTests.cpp — assertion protected kill.
- engine/AssetFileSystem.cpp:676 — сохранение обеих icon roots при смене мира; combined regression в AssetFileSystemTests.
- game/ui/screens/NewGameScreen.cpp:1077,2746 — importer candidates и custom start cards;
  game/ui/screens/NewGameScreenPartySheet.cpp — custom doll и header старта.

Итоговые source SHA не выдумываются: HEAD всё ещё54efa0e9, изменения подготовлены без коммитов.
Для следующего шага нужен разрешённый локальный merge-коммит основы и новая ветка от него под скелет.
Эквивалент136db0ce уже присутствует в upstream startNewSessionFromCharacterCreation (m_pMapSceneRuntime guard).
Остальные a70116a7,cbc4fd40,007c79e5,d3bc5643 ещё не применялись. Старую feature/roguelike-run не двигать.

## Lighting pre-commit check

`safe-heavy-run run -- python3 tools/lighting/stale_bakes.py`: final result **0 stale lighting bakes**, exit0,
runner20261113T140013Z-2863e99337 (build/integration-lighting-audit.log). Only materialized out01 bake was checked;
this is not a full audit of sparse-hidden bakes. Initial report listed200absent source BMPs, not changed local art.
An independent partial source-assets checkout now matches the target gitlink
`323e0ca32f453b35a8aab9085c0012e8bfcdc5d8`, materializing exactly those200files. Recheck passed with their original
hashes; no bake regenerated, no model/source art changed. No STALE_LIGHTING_BAKES.txt remains after the clean run.

Whitespace check of the integration delta against v2.3 passes. The full cached merge retains three upstream EOF
blank-line warnings already present in v2.3: tests/Mm9ActorRulesTests.cpp:2098,
tests/playthrough/mm9/17_sturmford_pledge.yml:108 and tests/scenarios/mm9/ai/guards.yml:44; those files were not edited.
