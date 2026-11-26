# Интеграция OpenYAMM 3.4 — 2026-11-26

## Ревизии и возврат

- База `integration/minotaur-v3.2-20261124`, `3675ce0d2404a87369da410a3446655f08eecb18`.
  Локальный перенос выполнен в `integration/minotaur-v3.4-20261126`.
- Цель `v3.4`, коммит `91540f225bc7f07dc5ba03262e53b076b1cf60d4`, объект тега
  `ea91a8bda0000c24a1c433eb38192defe21179e6`. GitHub Releases API подтвердил публикацию
  2026-11-25 в 20:00:34 UTC. `git ls-remote --tags` подтвердил обе ревизии. [Релиз v3.4](https://github.com/pjasicek/openyamm/releases/tag/v3.4)
- Merge `257ccdbdbd00415ffcf7935eb6c65ebaecdc436a` имеет родителей из задания. После него перенесён
  `261678b4bc0bb1e4f89e02eae96e419202a0bdce` из `feature/medusa-ruleset`, локальный коммит
  `74df21fb770500d7dd58538f5d338733e2355073`. Для возврата на 3.2 есть
  `backup/minotaur-before-v3.4-20261126`. Использовать нужно бинарник 3.2 и сейвы до обновления.
- Из 3.3–3.4 пришли общая вкладка добычи и сводка подбора, обучение сразу на несколько уровней, изменения
  движения и взаимодействия с декорациями, исправления карт, портретов, MAW, Xeen и Map Studio.

## Объекты и ресурсы

В sparse-шаблоны этого дерева добавлены `core/` и `openyamm-studio/`. Пустой индекс и файлы базы подготовлены
через `GIT_NO_LAZY_FETCH=1 git read-tree -m -u HEAD`. Для целей сборки и пересечений слияния недоставало
288 объектов. Они загружены восемью пакетами `git fetch --stdin` через HTTP/1.1. Объект `SaveGame.cpp` предварительно
сверен с Git-хешем и записан через `git hash-object -w`. Проверка полного индекса в partial clone потребовала
отключить поиск переименований для отсутствующих объектов за пределами sparse.

Изменённые ресурсы скачаны отдельно. После тайм-аутов пакетов для этих файлов использован raw GitHub
с проверкой хеша и дозагрузкой по Range для больших файлов освещения. Логи в `build/port`.
Зависимости скопированы в собственные `build/source-cache` и `build/sysroot`. В `build/port/test-assets`
использованы файлы v3.4 из Git и неизменённые по Git дереву ресурсы 3.2 и Map Studio через жёсткие ссылки
на входные файлы. Medusa использует ранее установленный пакет из тестового набора базы.

## Конфликты

| Файл | Решение |
|---|---|
| `game/app/GameApplication.cpp` | При первом переносе сохранено исключение Roguelike. По решению пользователя оба переключателя действуют в Roguelike при создании партии, привязке сцены и обновлении настроек. |
| `game/gameplay/HouseInteraction.cpp` | Добавлена массовая тренировка 3.4. Предложение сброса талантов остаётся доступно соло и защищает таланты Roguelike. |
| `game/maps/SaveGame.cpp` | Сохранены отдельные оболочки и старые mappings мода. Писатель обновлён до v203, v202 сопоставлен со схемой 201. Добавлено поле мешочка схемы 202. |
| `tests/RulesetTests.cpp` | Согласованы старые тестовые форматы. Сохранён вырез пустого идентификатора инстанса, учтено новое поле мешочка и сохранён официальный заголовок синтетических образцов. |

Просмотрены 31 автоматически слитое пересечение. Список всех 35 и diff находятся в
`build/port/overlap` и `build/port/overlap-merged.diff`. Проверены цепочки добавления стаков и геометрии сумок,
кнопки мешочков и обработка продажи мусора, вызовы событий, стандартные переходы и лагерь Roguelike.
Код корней именованных модов, импортёр портретов и кукол, расчёт основной атаки, правила Roguelike и реализация
стаков совпадают с базой. Дерево `openyamm-studio/` совпадает с v3.4.

## Сохранения и адаптации

- `OYMINO1/v203 → 202`, прежний `OYMINO1/v202 → 201`. Схема 202 добавляет содержимое вкладки добычи.
  `OYSAVE1/v202` читает оригинальную схему 202. Сохранены все прежние строки `saveSchemaVersion`
  и миграции, включая отдельную обработку старых талантов MAW и Roguelike.
- Замороженный `tests/fixtures/minotaur-3.2-v202-roguelike.oysav` записан неизменённой Linux-сборкой
  точной базы до слияния. Исходный сценарий прошёл 170 шагов. Фикстура сделана сразу после первого стража
  Гоблинвотча и однократной выдачи артефакта 2029. Ревизия, SHA256 бинарника и сейва и защищённый запуск
  указаны в соседнем TXT. Сценарий загрузки и round trip проверяют маршрут, награду, таланты и новый формат.
- В Roguelike автоматический подбор учитывает `lootPouch`, а распределение зелий, реагентов, бутылок и
  квестовых предметов учитывает `partyPouches`. Содержимое нового мешочка сохраняется в формате v203.
  Команда `evt.JoinParty` перенесена поверх v3.4 и использует его код внешности персонажа из ростера.
- Формулы Axe, Damage Resistance и Lethal Strike сохраняются из базы. Код авторитетной проверки Lethal Strike
  при кооперативе сохранён. Обновление массовой тренировки не отменяет право сброса талантов только в соло.

## Workflow и упаковка

Версия и полный SHA обновлены в `minotaur-windows.yml`. Выборки исходников и ресурсов по-прежнему покрывают
desktop v3.4. В checkout задания упаковки добавлена замороженная фикстура v202, в сценарии добавлена её загрузка.
`tools/minotaur_ci_tests.sh` включает 14 групп с новой `upstream34`. База обновления соответствует установленному
Minotaur 3.2 #54. Windows-сборка и упаковка ожидают отдельного запуска.

## Что найдено и исправлено

- Тестовые копии старых сохранений теперь удаляют добавленное в схеме 202 поле при моделировании схем до 199.
  В тестах повторной записи старых фикстур ожидается схема 202. Тест неизвестной оболочки проверяет следующий
  неподдерживаемый v204. Отдельный прогон проверил байты заголовка, записанного игровым бинарником после загрузки v202.
- Пакеты изменённых ресурсов прерывались по тайм-ауту. Все 558 первоначально отсутствовавших объектов набора
  были получены напрямую с проверкой Git-хешей. Крупные файлы освещения догружены по Range.
  В локальной копии `assets_dev/worlds/mm8/icons_x2/npc1130.png` восстановлены права 0644, ожидаемые тестом.
  Содержимое и режим в Git совпадают с базой.

## Проверки на Linux

Три цели собраны. Проверки выполнены с ограничениями `safe-heavy-run` и логами в `build/port`.

| Проверка | Команда и результат |
|---|---|
| Лимиты | `safe-heavy-run check`, успешно. Профиль openyamm, 4 задания, до 18 ГиБ RAM. |
| Сборка базы | `safe-heavy-run run --label v34-freeze-base-build -- bash build/port/build-base.sh`, три цели, код 0. Запуск `20261126T010211Z-299c470079`. |
| Запись фикстуры до слияния | `safe-heavy-run run --label v34-freeze-v202 -- bash build/port/freeze.sh`, 170/170 шагов, заголовок `OYMINO1/v202`. Запуск `20261126T011733Z-f94443d0bf`. |
| Сборка v3.4 | `safe-heavy-run run --label v34-linux-build -- bash build/port/build.sh openyamm openyamm_unit_tests openyamm_runtime_tests`, код 0, пик 5446.1 МиБ. Запуск `20261126T011845Z-25a3ce4a13`. |
| Группы CI | `safe-heavy-run run --label v34-ci-groups -- python3 build/port/with-unit-assets.py bash tools/minotaur_ci_tests.sh build/tests`, 14/14, код 0. Запуск `20261126T013037Z-7ad422f137`. |
| Сценарии Windows workflow на Linux | `safe-heavy-run run --label v34-scenarios -- bash build/port/test-scenarios.sh`, 27/27 сценариев и 540/540 шагов, код 0. Запуск `20261126T013131Z-b755dceee0`. |
| Пул Roguelike | `safe-heavy-run run --label v34-pool -- bash build/port/test-scenarios.sh build/port/pool/pool_mm{6,7,8,9}.yml`, 94/94 карт и 846/846 шагов, код 0. Запуск `20261126T013244Z-f775cf625c`. |
| Заголовок нового писателя | `safe-heavy-run run --label v34-writer -- bash build/port/check-writer.sh`, 14/14 шагов, заголовок повторно записанного сейва `OYMINO1/v203`, код 0. Запуск `20261126T013613Z-342860037e`. |
| Упаковка | `safe-heavy-run run --profile light --label v34-packaging -- bash build/port/packaging-checks.sh`, код 0. Проверены workflow, синтаксис групп, обновления, сборка и проверка пакета, runtime assets, runtime packs и замеры Windows. Пропущены 12 случаев установщика, требующих PowerShell 5.1 на Windows. |
| Документация и пробелы | `safe-heavy-run run --profile light --label v34-doc-i18n -- python3 tools/check_doc_i18n.py`, 0 ошибок, 8 прежних предупреждений. `git diff --check` и `git diff --cached --check` прошли. |

Группы и число прошедших случаев. Gameplay 299, world 7, medusa-importer 49, updater 17, upstream-ai 159,
upstream-indoor 24, Roguelike 49, MM9 190, upstream24 447, upstream27 42, upstream32 6 unit и 35 runtime,
upstream34 103 unit, 2 script и 5 runtime, native-ui 32, maw24 359. Падений или пропусков из-за ресурсов нет.
Выборки повторяют фильтры Windows workflow. Пул прошёл на MM6 27/243, MM7 31/279, MM8 22/198 и MM9 14/126
(карты и шаги). Пул проверяет старт, позицию, прогресс и стража с помощью команды `actor kill`.

## Независимое ревью

- Формат и миграции. `game/maps/SaveGame.cpp:370`, `game/maps/SaveGame.cpp:2014`,
  `game/maps/SaveGame.cpp:2489`, `game/maps/SaveGame.cpp:2548`, `game/maps/SaveGame.cpp:6732`,
  `game/maps/SaveGame.cpp:7314` и `tests/RoguelikeTalentTests.cpp:1103`. Сверить обе оболочки
  версии 202, сохранение маршрута и талантов, пустой мешочек при чтении v202.
- Инвентарь. `game/app/GameApplication.cpp:11683`, `game/app/GameApplication.cpp:12992`,
  `game/party/Party.cpp:4218`, `game/party/Party.cpp:4340`, `game/party/Party.cpp:6761`,
  `game/ui/GameplayHudOverlaySupport.cpp:2555`. Сверить автоматическую выдачу добычи, перенос вещей
  между вкладками, объединение стаков и настройки при повторной привязке партии.
- Тренировка и бой. `game/gameplay/HouseInteraction.cpp:1665`, `game/gameplay/HouseInteraction.cpp:2602`,
  `game/indoor/IndoorWorldRuntime.cpp:21878` и `game/outdoor/OutdoorWorldRuntime.cpp:20560`.
  Сверить массовую тренировку, доступ к сбросу талантов и проверку авторитета Lethal Strike.
- Medusa. `game/events/EventRuntime.cpp:5163`, `game/party/Party.cpp:5489`,
  `game/app/GameApplication.cpp:13002`. Сверить границы партии, выбор портрета и источник строки ростера.

## Оставшиеся проверки и решение пользователя

Независимый ревьюер ещё не проверял перенос. Сборка и установщик Windows, Vulkan, запуск Map Studio,
ручные кампании и живая сессия кооператива остаются непроверенными. Проверка сценариев использовала
локальный каталог ресурсов Linux. Установленный Windows-пакет 3.2 #54 остаётся базой обновления.

26 ноября пользователь утвердил оба переключателя v3.4 в Roguelike. Меню паузы показывает обычное описание
«Мешочки партии» для правил без собственных мешочков. Сценарий забега включает оба переключателя и проверяет,
что меч из сундука, купленный реагент и пустая бутылка доступны каждому из четырёх героев как общие предметы.

## Повторные проверки после решения пользователя

| Проверка после решения пользователя | Команда и результат |
|---|---|
| Сборка всех трёх целей | `safe-heavy-run run --label v34-pouches-build -- bash build/port/build.sh openyamm openyamm_unit_tests openyamm_runtime_tests`, код 0. Запуск `20261126T065704Z-9a312f9537`. |
| Сценарий мешочков | `safe-heavy-run run --label v34-pouches-focused -- env PORT_SCENARIO_RESULTS="$PWD/build/port/pouches-focused" bash build/port/test-scenarios.sh tests/scenarios/rulesets/roguelike_run.yml`, 177/177, код 0. Запуск `20261126T070025Z-d26a664d10`. |
| Группы | `safe-heavy-run run --label v34-pouches-groups -- python3 build/port/with-unit-assets.py bash tools/minotaur_ci_tests.sh build/tests`, 14/14, код 0. Запуск `20261126T070043Z-0a813bb472`. |
| Сценарии | `safe-heavy-run run --label v34-pouches-scenarios -- env PORT_SCENARIO_RESULTS="$PWD/build/port/pouches-scenario-results" bash build/port/test-scenarios.sh`, 27/27 и 547/547 шагов, код 0. Запуск `20261126T070143Z-b9a5ce2a51`. |
| Пул с обоими переключателями | `safe-heavy-run run --label v34-pouches-pool -- env PORT_SCENARIO_RESULTS="$PWD/build/port/pouches-pool-results" bash build/port/test-scenarios.sh build/port/pool-pouches/pool_mm{6,7,8,9}.yml`, 94/94 и 846/846 шагов, код 0. Запуск `20261126T070308Z-bc0c02c721`. |
| Документация и пробелы | `safe-heavy-run run --profile light --label v34-pouches-i18n -- python3 tools/check_doc_i18n.py`, 0 ошибок, 8 прежних предупреждений. `GIT_NO_LAZY_FETCH=1 git -c diff.renames=false diff --check` и такая же проверка с `--cached` прошли. |
| Освещение в sparse-дереве | `safe-heavy-run run --profile light --label v34-pouches-lighting -- python3 tools/lighting/stale_bakes.py --check`, код 1. Проверка отметила `assets_dev/worlds/mm8/maps/out01.lighting`: отсутствуют все 200 исходных BMP из `assets_dev/worlds/mm8/textures/`. Полный список в `build/port/pouches-lighting.log`. |
