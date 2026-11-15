# Интеграция OpenYAMM 2.4 — 2026-11-15

Рабочая ветка: `integration/minotaur-v2.4-20261115`.
Статус: merge подготовлен, сборка и проверки идут. Этот документ обновляется результатами;
успешная компиляция сама по себе не означает успешного прохождения кампаний.

## Ревизии, исходные материалы и возврат

- Рабочий checkout: `/home/plane/projects/openyamm-minotaur-v2.3`, первоначальная ветка
  `integration/minotaur-v2.3-roguelike-20261113`, HEAD `4788e9b51049b27ad8280c3a8860af7a4b4f2b52`.
- Последний интегрированный upstream: `v2.3`, `f4a6b8032759a218505532b2600ac56b6d80c7af`.
- **Протестированный полный прототип 2.3:** `da1bb1624580ceb7caa3626d637c020727f5aa46`,
  ветка `feature/roguelike-v0`, соседний worktree `/home/plane/projects/openyamm-medusa/.roguelike-v23`.
  Его git status был чист. Текущий checkout первоначально содержал только скелет; обновлять его без
  включения da1bb162 означало бы потерять реализованные правила v0.
- Actions API подтвердил source SHA da1bb162 и успех предыдущей поставки:
  [run43598461895](https://github.com/Plane-PPP/openyamm/actions/runs/43598461895),
  [Windows ZIP](https://github.com/Plane-PPP/openyamm/actions/runs/43598461895/artifacts/14115874232),
  9 944 108 882 байт. Это подтверждает источник CI, но не SHA конкретной установки игрока.
- База объединена merge-коммитом `51a1647a`: сохранён и единственный отдельный commit4788e9b5.
  Конфликт старого Windows workflow разрешён в пользу полного pipeline da1bb162 с MM6–MM9 и языками;
  более поздний pipeline уже содержит нужные отборы тестов.
- Цель: [официальный v2.4](https://github.com/pjasicek/openyamm/releases/tag/v2.4),
  `f052579667e2233d6318d9855dce29097e8ca9ef`. Выполнен `git fetch upstream --tags`, тег проверен.
- Резервные ветки: `backup/integrated-2.3-before-2.4-20261115` →4788e9b5,
  `backup/roguelike-tested-2.3-20261115` →da1bb162. Исходные ветки/worktrees не изменялись.
  Для возврата использовать отдельный worktree/установку этой ревизии и старые копии saves;
  новый save184 не предназначен для старого exe.
- Незакоммиченные `INTEGRATION-STATUS.md`, `PREPARATION.md` сохранены; это исторические журналы 2.3,
  их старые указания об отсутствии разрешений не относятся к текущему заданию.
  Четыре отчёта первого тестирования сохранены; исходные `01-playtest-report.md`,
  `02-technical-investigation.md`, `04-upstream-bug-report.md` не редактируются.
  Исходный handoff03 сохранён как `03-fable-handoff-2.3.md`, актуальный03 получает вступление 2.4.

## Что пришло из upstream

Полный merge до точного тега, включая код, тесты, ресурсы, скрипты и упаковку, а не выборочное
копирование release notes. Изменения относятся к 12 upstream-коммитам после v2.3.

- MAW solo: один герой, first-person weapon/shield, action bar, таланты и их взаимодействия,
  сохранение solo/action bar/talent ranks. Финальные исправления leech Cleave/Sweeping Blows и
  тройного учёта предметов дракона включены до f052579.
- Auto Walk (Num Lock), Free Cursor (средняя кнопка), защита клика при открытии окна.
- Знаки известных/доступных книг заклинаний; предупреждение об окончании зелий; эффекты на health bars;
  собственный HP в условиях battle orders; перетаскивание Progress scroll.
- Исправления расхода хода зельем, стрельбы через склон, дальности обнаружения ещё не вступивших в бой
  монстров, MM8 plane gateway времени, MM9 potion keys/звуков, сортировки с переносом в следующий bag.
- MAW item popup order/colours, catalyst/keep mixed potions, stack sales/scroll shops,
  опыт charmed monsters, regeneration aura для MM9 hirelings.
- Сопутствующие изменения renderer/model loader/cache, сцен/эффектов, таблиц, переводов,
  packs и first-person view model включены. Vulkan-сборка не запускалась.

## Содержательные конфликты и сохранённые механики

После включения полной базы v0 возникло пять конфликтных файлов upstream:

| Файл | Решение |
|---|---|
| `game/app/GameSettings.cpp` | Сохранены четыре Minotaur debug/text поля и новые view_model_path/pin_inbox_path |
| `game/maps/SaveGame.cpp` | Сохранены отдельные mod/official envelopes и старые mappings; добавлены поля и gates183 |
| `game/ui/GameplayHudOverlaySupport.cpp` | Сохранены ReagentStacks и собственный LS tooltip, добавлены upstream solo/turn headers |
| `tests/AlchemyBenchRegressionTests.cpp` | Оба набора тестов: наши bonuses/once-per-character и upstream keep-mixed/catalyst |
| `tests/RulesetTests.cpp` | Новые upstream downgrade helpers, official magic для synthetic saves и отказ неизвестного mod177 |

Автослияния отдельно просмотрены. Независимый read-only review сравнил с da1bb162:
семь основных функций LS/Axe/DR и `PotionRules::mixingAlchemyLevel` совпали побайтно.
ReagentStacks, CustomPortraits, DungeonRun, RoguelikeRules и run.yml не менялись.
Roguelike по-прежнему заменяет Experience/Monsters и использует общие standard combat/items/skills/
spells/classes/potions/crafting. Medusa сохраняет Нигон и pinned content0.2.2/347fb218;
геометрия импортируемых кукол берётся из исходной строки, пользовательские icon mounts сохранены.

Выявленный новый стык: solo extra blows — дополнительные цели **одного** действия, без новой recovery.
Им нельзя давать новый LS-roll. Ограничение `!talentExtraBlow` сохраняет прежний контракт основной атаки,
не меняет damage/leech upstream, формулу/проценты/пулы мода. Добавлен адресный controller test
с одинаковыми seeds, основным и дополнительным ударами и клоном без LS.

Известная новая UI-неточность: до получения школы карточкой героиня79 Roguelike может видеть
CannotLearn у книги, потому что прогноз нового marker смотрит обычный class cap.
После получения фактического навыка marker и чтение работают по навыку. Изменение общей SkillRules
или прямое связывание inventory с GameSession ради прогноза не внесено. Это не решение о новом балансе.

## Схема сохранений

- Writer `OYMINO1/v184` → upstream183. Официальный `OYSAVE1/v183` читается как183.
- Явно добавлено `OYMINO1/v180` →179; сохранены89/94/95/130/132/140/143/155 и legacy85.
  Mod180 и official180 — **разные схемы**: official180 уже не хранит retired MAW alchemist.
- Upstream gates: retired alchemist180, solo/action bars181, talent ranks182, ownHealth183.
  Сначала определяется mapped schema, затем читаются поля. Дополнительный combatLevel мода сохранён.
- Подлинный frozen production180: `tests/fixtures/minotaur-2.3-v180-roguelike.oysav`,
  407 887 байт. Записан неизменённым бинарником da1bb162, с его прежними assets, до новой сборки.
  Происхождение, SHA256 binary/save и protected run находятся в соседнем TXT. Не генерировать заново.
- Старый сценарий для freeze прошёл37/37 шагов: seed4, Goblinwatch, четыре герояL13,12 выбранных карт,
  книги и живой guardian. Данные игрока не использованы. Новый тест проверяет summary, run variables,
  карты/инвентарь/portrait/equipment/spells и roundtrip в184, default solo/actionbar/talents.
- Образцы старых mod132/140/143/155 и official saves продолжают проверяться. Замена прежнего ожидания
  schema179 на183 относится только к новому writer, не к утверждению чтения исходного образца180.

## Критический переход Roguelike

Исходное наблюдение и доказательство 2.3 остаются в02 §3 без переписывания.
Upstream guardClicksAfterWindowOpened подавляет клик, который открыл окно, и краткий следующий клик.
Он не меняет ранний перехват pending map move в Camp и не потребляет deferred close departure dialog.
Следовательно автоматического исправления P0 от upstream по исходникам нет.

Минимальная правка: когда подтверждено, что это межкарточный выход в Camp, вызвать
`GameplayScreenRuntime::closeActiveDialogForMapMove()` **перед** restore/open Camp.
Этот API извлекает и обнуляет старый callback перед вызовом; позднее он не закроет новое окно.
Обычные переходы и same-map teleports продолжают свой прежний путь. Правила Camp/Rest/магазинов,
пулы карт и наград не перепроектировались. Сохранён pending CampMove и унаследованный retry.

Сценарий `tests/scenarios/rulesets/roguelike_run.yml` теперь использует event51 Goblinwatch,
реальный MapTransitionConfirm/OK, EventChoice магазина, Cancel и EventChoice Next.
Headless driver учитывает обычный ответ ruleset на следующем кадре и отличает удержанный Camp move
от неудачного map move; assert отдельно требует фактическое окно Camp, не только внутренний flag.
Старые bypass-команды остальных этапов сценария остаются историческим покрытием.

**Результат реального runtime-пути: ожидается, не заявлен до прогона.**
Не доказанные первым исследованием немедленный Close и все Windows double-click/renderer эффекты
не объявляются исправленными без отдельных проверок.

Сохраняется прежний отдельный риск: entered/guardian меняются перед успешной загрузкой карты;
отказ загрузки не гарантирует rollback. Это не изменялось: требуется следующая отдельная итерация.

## Автоматические проверки

Лимиты: `/home/plane/AGENTS.md`, safe-heavy-run check успешен; profile openyamm,
MemoryHigh16GiB/Max18GiB, swap<=1GiB, CPU<=4, jobs4. Один тяжёлый запуск одновременно.
Локальный CMake: `/home/plane/.local/share/openyamm-toolchain/root/usr/bin/cmake`,
LD_LIBRARY_PATH: `/home/plane/.local/share/openyamm-toolchain/root/usr/lib/x86_64-linux-gnu`.
SDL этого build настроен headless/offscreen (X11/Wayland OFF), OPENYAMM_USE_VULKAN=OFF.

- Freeze: `safe-heavy-run run --label v24-freeze-v23-kept -- bash build/v24-freeze.sh`,
  run20261115T132449Z-f5fb1c4687,37/37 passed. Сценарий эквивалентен первым37 шагам старого run.yml.
- Сборка: `safe-heavy-run run --label v24-linux-build -- env LD_LIBRARY_PATH=/home/plane/.local/share/openyamm-toolchain/root/usr/lib/x86_64-linux-gnu /home/plane/.local/share/openyamm-toolchain/root/usr/bin/cmake --build build --target openyamm openyamm_unit_tests openyamm_runtime_tests`,
  run20261115T132626Z-5cbe2aa646, выполняется.
- `python3 tools/check_minotaur_workflow.py`: passed, MM6/MM7/MM8/MM9 и9 групп тестов.
- Остальные test results/Windows source SHA/artifact будут добавлены по фактическому завершению.

## Windows-поставка

Используется существующий `.github/workflows/minotaur-windows.yml`, обновлённый на2.4/f052579;
version `minotaur-2.4-<actual SHA>-<run>-<attempt>`. Новая ветка не добавлена в push-trigger:
сборка запускается явно один раз через workflow_dispatch. Полный пакет содержит MM6–MM9, языки,
Medusa, Roguelike, portraits/paperdolls/maps/scripts и verifier проверяет own rulesets file-for-file.
Параллелизм Windows определяется по ресурсам runner и ограничен4; content cooker parallel4.
Выбран full package для отдельной установки и ручного тестирования; baseline прежних updates
не заменяется автоматически. Main, теги, GitHub Releases и официальный updater не публикуются.

## Что остаётся Fable и ручной проверке

Исходные пожелания и решения01/02/03 остаются в полном объёме. Никакие новые задачи геймдизайна
не добавлены: четыре предложения карт, качественные перки, Artifact/Relic+gold, Guardian prefix,
Camp/Rest/food, guild memberships, stat tooltip и Add Portrait overlap остаются отдельной итерацией.
Фиолетовый прогресс сохранён. Индексные perk save variables запрещено переставлять без миграции.

На Windows вручную: отдельная установка2.4, старые saves в копии, импорт portrait/doll и reload,
Medusa/Нигон; создание обычной партии и Roguelike/Гоблинвотч; настоящая дверь до/после босса,
OK/Cancel/Escape/повторный клик, Camp→shop→Close→Next, autosave on/off, карты2–4, defeat/victory,
гильдии/книги, карточки и фиолетовый HUD; обычные MM6/MM7/MM8/MM9 exits, предметы/бой/новый MAW solo.
Полное ручное прохождение всех кампаний, личные saves, реальный GPU/FPS и Android не подтверждены
headless/unit-тестами и не объявляются проверенными.
