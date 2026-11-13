# Roguelike skeleton on Minotaur / OpenYAMM2.3

Статус: перенос скелета завершён и проверен.8unit-тестов/599assertions и все48шагов исходного headless-сценария прошли. Правила забега не менялись.

## Ветки и происхождение

Проверенная основа: `530d75a3b65cf7b2b7e0c4020cdd936fcc7fb66b`, ветка `integration/minotaur-v2.3-20261113`.
Она объединяет исходный Minotaur54efa0e9 с upstream2.3 `f4a6b8032759a218505532b2600ac56b6d80c7af`,
Медузой и импортёром; её проверки описаны в [отчёте основы](minotaur-2.3-integration.md).

Новая ветка: `integration/minotaur-v2.3-roguelike-20261113` от530d75a3, тот же изолированный worktree
`/home/plane/projects/openyamm-minotaur-v2.3` и инкрементальный build.
Перенесены `a70116a7,cbc4fd40,007c79e5,d3bc5643` из исходного
`d3bc56430fd84a3194f52b7080317b52ac9a1b4e`; исходная `feature/roguelike-run` не двигалась.
`136db0ce` пропущен: в2.3 уже есть тот же `m_pMapSceneRuntime` guard для indoor-start.

## Сохранённое поведение

Неизменённые ruleset.yml и run.yml: четыре героя из default seed, старт Goblinwatch, прежний пул8MM6-карт,
тот же seeded shuffle, каждый выход в следующее подземелье, сохранение seed и `dungeon_run.entered`.
Условие зачистки, стражи, перки, рост навыков, магазин и баланс в эту задачу не входят. Пул остаётся из8карт,
это старый технический скелет, не реализация четырёхкартовых правил v0.

Формат сохранений не изменён относительно основы: `OYMINO1/v180` → upstream179, старый155 →154.
Скелет хранит счётчик в существующих named global vars; сериализованные поля не добавлены.

## Адаптация к2.3

Единственный содержательный merge-конфликт старта: сначала выбирается первый dungeon, затем новая статистика
прохождения получает мир его фактической карты; сохранены markAssisted для preserveDebugLoadout и сброс map context.
`run`/`run status` объявлены чтением через readOnlyWith. Отладочный `run new` помечает созданный playthrough
assisted после успешного старта: прежний observer помечал лишь уходящую сессию. Это стыковка с новой статистикой,
не изменение правил забега. Независимый review_v23 нашёл и подтвердил эти необходимые адаптации.
Windows workflow перенесён, target — новая integration-ветка, проверки включаются наличием run.yml.
Никакого workflow dispatch/push/publication не выполнялось.

## Проверки

Сборка всех3целей passed, runner20261113T140912Z-7f8c13e24a, peak5846.7MiB.
8DungeonRunTests/599assertions passed, runner20261113T141104Z-13728afe5d (сценарий того запуска не прошёл инициализацию).
Все heavy-проверки только safe-heavy-run. Headless использует отдельный user-data каталог, x1 asset profile и
абсолютные пути к development assets/scenario. Original saves/settings не используются.
Текстовые эффекты и готовые atlas/PNG берутся из Git, не генерируются и не редактируются. Промежуточные ошибки
инициализации из-за sparse-ресурсов не объявляются успешным сценарием.

Команды:

```sh
safe-heavy-run run -- cmake --build build --target openyamm_unit_tests openyamm_runtime_tests openyamm
safe-heavy-run run -- ./build/tests/openyamm_unit_tests --source-file='*DungeonRunTests.cpp'
safe-heavy-run run -- ./build/game/openyamm --asset-scale 1 --user-data /tmp/openyamm-roguelike-v23-NEW --asset-root /home/plane/projects/openyamm-minotaur-v2.3/assets_dev --headless-run-scenario /home/plane/projects/openyamm-minotaur-v2.3/tests/scenarios/rulesets/roguelike_run.yml
```

Для каждого сценарного прогона брать новую user-data папку. Фактическая последняя папка записана в
build/roguelike-user-directory.txt; логи build/roguelike-unit-tests.log и build/roguelike-scenario.log.

## Унаследованные ограничения и границы проверки

- До загрузки следующей карты счётчик увеличивается. При неудачной загрузке старый скелет восстанавливает карту,
  но не откатывает счётчик; следующий выход может пропустить dungeon. Это прежний риск, здесь не переделывалась
  transactional transition machinery. Для исправления нужен отдельный негативный runtime-тест и явная работа над скелетом.
- Исходные48шагов не проверяют поражение и выход из последнего dungeon; обычный defeat hook перенесён,
  сценарный helper поражения обходит его. Ручная игра/визуалы, Windows и Android не проверены.
- Старые незавершённые забеги могут потерять согласованность map snapshots/named vars при upstream map-layout reset;
  политика миграции таких забегов не добавлялась.

Независимое read-only review охватило start/redirect/save/load/standard isolation и playthrough hooks.
Итоговый SHA новой линии записать после успешно проверенного и разрешённого локального коммита;
не заменять его исходным SHA d3bc или SHA основы.

## Финальная проверка и handoff

Исходный сценарий прошёл все48шагов: runner20261113T171418Z-6e2194b042, exit0, peak265.4MiB.
Подтверждены старт6d01,7переходов,8карт без повторов, seed4 и порядок, сохранение/загрузка в последнем
подземелье, новый seed1 со стартом в6d01. Unit-пул и YAML-сценарий не редактировались для получения успеха.
`run.yml`, `ruleset.yml`, DungeonRun.{h,cpp}, DungeonRunTests.cpp и сценарий побайтно совпадают с d3bc5643.

После переноса повторены выбранные регрессии: **277unit cases/50 364assertions**, все passed
(192/47 493,82/1 635,3/1 236); **26native cases/3 328assertions**, все passed.
Runner20261113T171520Z-083be49414, exit0, peak184.9MiB. Точные source filters совпадают с отчётом основы,
в первый профиль добавлен `*DungeonRunTests.cpp`. Логи build/roguelike-regression-{unit,upstream,arrows,runtime}.log.
Полный unit/runtime suite не заявляется. Lighting-check0stale на материализованном out01, doc-i18n0errors
и8унаследованных warnings; diff--check для дельты от530d75a3 passed.
Финальное независимое review_v23: блокирующих замечаний по проверенной дельте нет; исправления новой статистики
подтверждены чтением. Reviewer не выдавал самостоятельный test-run за выполненный.

Headless-сценарий содержит ожидаемые предупреждения sparse-окружения: отсутствуют Music/{40,42,43,44,49}.mp3
и engine/models/mm6_deco_{barrel,irontorch,torch}.glb. Все48 функциональных assert прошли; звук/декорации/GPU
этот прогон не подтверждает. Не добавлялись fallback или подменные файлы. Обязательные FX/projectile ресурсы
получены из Git; Cooking/Blender/новая генерация не запускались.

Источник первоначального задания Fable: `/home/plane/projects/openyamm-roguelike-notes/codex-minotaur-2.2-prompt.md`.
Передать Фейблу: основа530d75a3b65cf7b2b7e0c4020cdd936fcc7fb66b; имя текущей ветки выше; итоговый полный SHA
фиксируется после локального коммита в `INTEGRATION-STATUS.md` рядом с этим worktree и в ответе пользователю.
Новая запись сохранений180/179, старый155/154; стандартные формулы Minotaur и общий upstream бой — в отчёте основы.
Windows package/новая baseline/CI/push/release и правила v0 требуют отдельного продолжения; ничего из этого не запускалось.
