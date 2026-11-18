# Интеграция OpenYAMM 2.7 — 2026-11-17/18

Рабочая ветка: `integration/minotaur-v2.7-20261117` (worktree `/home/plane/projects/openyamm-minotaur-v2.3`).
Windows-сборка — по разрешению пользователя 2026-11-18 (раздел «Windows-поставка»).

## Ревизии и возврат

- База — последняя рабочая поставка пользователя `Minotaur-Update-minotaur-2.4-51ec30f41533-51-1`, исходник
  `51ec30f41533` (ветка `integration/minotaur-v2.4-20261115`).
- Цель — официальный [v2.7](https://github.com/pjasicek/openyamm/releases/tag/v2.7), `71235186` (= `upstream/main` на
  2026-11-17): 14 коммитов после v2.4 (2.5: HUD draw list, китайский, переназначаемая панель соло; 2.6: Manathirst,
  зелья поддержки в приказах, именованные моды, оверлеи сцен, фильтр лута, Map Studio и общий `core/`; 2.7: мешочки
  MAW, замеченные предметы на земле, опасности пошагового боя по ходам).
- Резервная ветка `backup/minotaur-2.4-before-2.7-20261117` → `51ec30f4`. Старая ветка 2.4 не менялась.
- Слияние — merge-коммит `7260195a` (родители `51ec30f4`, `71235186`): 1302 пути, из них конфликтных 18.
- Отдельный коммит после слияния — исправление бутылок и случайные подземелья (см. ниже).

## Загрузка объектов

Partial clone `blob:none`: недостающие blob-ы v2.7 (1364) скачаны пакетами по `AGENTS.md`
(`GIT_NO_LAZY_FETCH=1`, `fetch --stdin`, 8 потоков). Две пачки упирались в обрывы GitHub (`early EOF`, таймауты);
последние файлы взяты напрямую с `raw.githubusercontent.com` с проверкой `git hash-object` и записью в объектный склад.
В sparse-checkout этого worktree добавлены: `core/` (новый верхний каталог 2.7, нужен CMake), сцены/BLV/события 94 карт
пула забега, текстовые данные мира MM9. **Внимание:** `git merge` в sparse-checkout снял с диска ранее вручную
выписанные файлы вне шаблонов (например, `worlds/mm9/state/`); в git они на месте, для проверок выписаны снова.

## Конфликты (18 файлов)

| Файл | Решение |
|---|---|
| `game/maps/SaveGame.cpp` | Писатель мода `OYMINO1/v194` = upstream 193; `case 184 → 183` сохранён, остальные строки мода прежние; новые ворота upstream 184–193 приняты, дубль `SaveMagic` upstream убран |
| `game/party/Party.cpp` (6) | Мешочки (pouches) upstream + таблица предметов мода в каждом размещении (геометрия стаков реагентов); стаки реагентов складываются и внутри мешочков MAW (`addToPouch`) |
| `game/items/InventoryBags.{h,cpp}` | `restoreDefinitionGeometry` мода рядом с `sortPouch` upstream; теги сумок удалены, как в upstream; `previewPlaceInBag` с таблицей предметов |
| `game/gameplay/GameplayRuntimeInterfaces.h`, `game/indoor/IndoorWorldRuntime.{h,cpp}`, `game/outdoor/OutdoorWorldRuntime.{h,cpp}` | `hitReaction` (upstream) и `lethalStrike` (мод) оба сохранены, `hitReaction` раньше; единственный позиционный вызов `lethalStrike` (`killPartyAttackActor`) исправлен |
| `game/gameplay/GameplayActionController.cpp` | Цепные стрелы upstream + талант забега (см. ниже) |
| `game/gameplay/GameplayItemService.cpp`, `game/ui/GameplayPartyOverlayRenderer.cpp` | Версия upstream с передачей таблицы предметов мода |
| `engine/AssetFileSystem.{h,cpp}` | Корни иконок пакета правил и пользовательских портретов мода рядом с именованными модами upstream; удалённый upstream «editor root» убран |
| `game/app/GameApplication.h`, `game/party/SoloTalentState.h`, `tests/CMakeLists.txt`, `AGENTS.md` | Обе стороны |

## Сохранения

- Писатель: `OYMINO1/v194` → схема upstream 193. Читаются: мод 184 (2.4) как 183, мод 180 как 179, старые строки мода
  89/94/95/130/132/140/143/155 и legacy 85 — без изменений; официальные `OYSAVE1` до 193.
- Upstream 2.7 при чтении сохранений < 189/190 переименовывает Pinning Shot в Power Shot и удаляет Attunement (таланты
  соло MAW). У забега свои таланты с теми же id (`attunement`, `pinning_shot`), поэтому для сохранений мода эти миграции
  отложены до чтения правил: выполняются для MAW/Standard и пропускаются для Roguelike (`migrateModSaveTalents`).
- Новая замороженная фикстура писателя #51: `tests/fixtures/minotaur-2.4-v184-roguelike-v1.oysav` (+ `.txt` с
  происхождением; записана немодифицированным бинарником `51ec30f4` до слияния, SHA256 бинарника и файла в txt).
  Проверяется модульным тестом (таланты по id, маршрут, бутылка, круговая запись в 194) и сценарием
  `rulesets/roguelike_v1_save.yml`. Прежние фикстуры (v180, v184-perks, MAW180 и др.) не менялись.

## Вынужденные адаптации к 2.7

- Upstream убрал «пробивающий» полёт стрелы (Piercing Arrows соло MAW стал Chaining Arrows с тем же id). Карточка забега
  `piercing_arrows` теперь через механику 2.7: попадание перескакивает на ближайшего монстра в 10 м, 1/2/3 раза, 50 %
  урона (`CharacterAttackExtras::chain*`). Это изменение поведения таланта, утверждённого 2026-11-17, — вынужденное.
- Ранги талантов — `uint16_t` (как в 2.7).
- Модульные тесты мода линкуют `GameplayActionController.cpp` без экрана: добавлены заглушки новых вызовов 2.7
  (`soloTalents`, зелья поддержки) в `tests/AxeActionTestStubs.cpp`; тестовые миры Axe/Lethal Strike — новая сигнатура.
- CI мода: новая группа `upstream27` (тесты, добавленные в 2.5–2.7) в `tools/minotaur_ci_tests.sh`; в Windows-workflow —
  новая фикстура и сценарий `roguelike_v1_save.yml`.

## Найдено и исправлено после слияния

- 2.7 добавил в `Character::tryPlaceInventoryItemAt` проверку «предмет целиком в границах открытой сумки» до любого
  размещения. Она срабатывала раньше слияния стопок мода: стопка бутылок 1×2, брошенная на нижнюю клетку стопки в
  последнем ряду, больше не сливалась (тест `Inventory hotfix multi cell merge...`). Теперь сначала слияние с
  совместимой стопкой, затем проверка границ (для обмена и обычной укладки, как в upstream).
- Круговые тесты сохранений мода ждали схему 183 после записи текущим писателем — теперь 193 (как при порте 2.4
  179 → 183). Тест upstream «сохранения версии 192 с двумя мешочками» состаривал файл подменой номера в заголовке мода;
  как и прочие такие тесты, теперь ставит официальный заголовок (`stampOfficialSaveVersion`). Тест отказа неизвестных
  оболочек мода расширен версиями 192, 193, 195.

## Исправление: бутылки в лавке лагеря

Лавка «Potions and magic» лагеря продавала строку MM6 `1765` («Potion Bottle»), которая в OpenYAMM не является пустой
бутылкой алхимии (пустая бутылка таблицы — `220`, `potion_settings.txt`, `ItemDefinition::isEmptyAlchemyBottle`). Теперь
продаётся `220`; бутылки `1765` в сохранении забега при загрузке превращаются в `220` (`migrateRunBottles`). Тест
проверяет, что каждый товар запаса лагеря варится (реагент или бутылка алхимии).

## Случайные подземелья

См. [roguelike-rules-v2.md](roguelike-rules-v2.md): 94 карты MM6–MM9 по этапам 1–4, маршрут `[1, 2, 3, 4]` хранится в
сохранении, страж-одиночка ×2, учёт мёртвых на входе, выход с пустой карты, `groupScale 0.5` на больших картах для
тестов. Карты сложнее 4-го этапа и класс C отложены (решения пользователя 2026-11-17).

## Итог проверок (Linux, `safe-heavy-run`, 2026-11-18)

Сборка `cmake --build build --target openyamm openyamm_unit_tests openyamm_runtime_tests` — успешно (пик 6,6 ГиБ).

| Проверка | Результат |
|---|---|
| Группы CI мода (`tools/minotaur_ci_tests.sh build/tests <группа>`) | world 7/7, medusa-importer 49/49, updater 17/17, upstream-ai 155/155, upstream-indoor 20/20, roguelike 47/47, upstream27 42/42, native-ui 28/28, maw24 347/347 |
| gameplay | 291/294: три случая `ScriptedMapRegressionTests` требуют X4-текстур (нет в sparse-checkout, как и при 2.4) |
| mm9 | 182/187: иконки предметов MM9, спрайты маркеров квестов, музыка 90007 — нет на диске |
| upstream24 | 412/431: звуки (`AudioRegressionTests`), модели оружия в руках (`HeldWeaponTests`) — нет на диске; `HouseDialogueRegressionTests` 202/202 после `chmod 644` восстановленного портрета (umask 002 машины) |
| Сценарии релиза и мода из Windows-workflow | все 24 проходят, включая `medusa_start` (8/8), `standard_exit_dialog` (10/10), `mm9/shop_after_world_switch` (6/6), `mm8/plane_gateway_keeps_buffs` (12/12), `roguelike_run` (170/170, до победы), `roguelike_v184_save` (27/27), `roguelike_v1_save` (19/19), `roguelike_worlds` (49/49) |
| Пул забега: старт на каждой из 94 карт, точка входа, счёт врагов, страж | MM6 27 карт (243 шага), MM7 31 (279), MM8 22 (198), MM9 14 (126) — все проходят |
| AddressSanitizer (`build-asan`) | модульные тесты забега/сохранений/инвентаря/стаков/Lethal Strike/правил 143/143, 0 ошибок; сценарии `roguelike_run`, `roguelike_v1_save`, `roguelike_worlds`, пул MM7 (31 новая игра подряд) — 0 ошибок |
| `tools/check_minotaur_workflow.py`, тесты инструментов упаковки, `check_doc_i18n.py`, `git diff --check` | пройдены (12 групп; 23 случая, 12 пропусков только для Windows; 0 ошибок документации) |

Наблюдение: один раз из шести длинных прогонов пула (31 новая игра в одном процессе) процесс упал (segfault) на
команде `run new` перед загрузкой карты 7d32. Не воспроизвелось ни отдельно, ни при двух повторах полного прогона,
ни под AddressSanitizer. Отладчика на машине нет; причина не установлена (вероятно, гонка потоков движка при
многократном перезапуске игры в одном процессе — это предположение). В обычной игре такого сценария (десятки новых
игр без выхода) нет; при повторении на Windows прислать журнал.

Для ASan-сборки шейдеры взяты из обычной сборки: инструментированный компилятор шейдеров bgfx (`fcpp`, сторонний
код) падает на новых шейдерах 2.7 с `stack-buffer-overflow` — это не код мода и не игры.

## Windows-поставка

- `minotaur-windows.yml` переведён на 2.7: `MINOTAUR_UPSTREAM_VERSION` 2.7, `MINOTAUR_UPSTREAM_SHA` `71235186`, версия
  пакета `minotaur-2.7-<SHA>-<run>-<attempt>` (как при переносе 2.4 в `763f7701`).
- Задачи `content` и `windows` теперь выкачивают `/core/` и `/third_party/cgltf/`: upstream добавил их во все задачи
  сборки в 2.6–2.7, а корневой `CMakeLists.txt` делает `add_subdirectory(core)` — без них сборка в CI упала бы на
  конфигурации. Флаг `-DOPENYAMM_BUILD_EDITOR=OFF` убран, как в upstream (в 2.7 такой опции нет).
- Задача `windows` собирает модульные тесты, а они в 2.7 подключают `openyamm_core_compile` (перенесённый из редактора
  код `core/compile/`). Upstream под Windows ни тесты, ни этот код не собирает, и в нём нашлась строка, которая на
  Windows не компилируется: `relativePath.native().starts_with("..")` в `core/compile/EditorDocument.cpp`
  (`native()` там — широкая строка). Заменено на `generic_string()`, на Linux/macOS поведение то же. Остальное новое
  для Windows (`core/compile/`, тесты 2.5–2.7, наш код) просмотрено на ту же ловушку (`path` → `std::string`,
  `c_str()`, POSIX-функции) — других мест нет; локальная проверка компилятором MinGW не выполнялась (запуск
  скачанного компилятора не разрешён в этой сессии), окончательно проверит CI.
- `tools/check_minotaur_workflow.py` раньше сверял с upstream только ресурсы и пропустил это. Теперь он требует, чтобы
  `content` выкачивал исходные пути задачи `content` upstream, а `windows` — задач `windows` и `linux` (каталог
  покрывает вложенные пути). На workflow до правки он выдаёт все шесть пропусков.
- База обновления — установленная у пользователя #51: `packaging/minotaur/bases/minotaur-2.4-51ec30f41533-51-1.json`,
  артефакт `minotaur-update-metadata-51ec30f4…` запуска
  [44083463333](https://github.com/Plane-PPP/openyamm/actions/runs/44083463333) без изменений (`check-base`: версия и
  SHA `51ec30f41533c30683635697bbdbf71ba7887488`, 294 пакета); `latest-baseline.json` указывает на неё. Формат пакетов
  в 2.7 тот же (`content_format` 1), инструменты упаковки не менялись.
- Запуск: `gh workflow run minotaur-windows.yml --repo Plane-PPP/openyamm --ref integration/minotaur-v2.7-20261117
  -f package_mode=update -f previous_build_sha=51ec30f41533c30683635697bbdbf71ba7887488`. Ветка не добавлена в
  push-триггеры, как и ветка 2.4.

## Не проверено

- Windows-сборка, Windows CI, установленный пакет, Vulkan.
- Игра на экране: карточный экран, бой и скрипты карт MM7–MM9 внутри забега, правила мира MM9 в забеге.
- По `AGENTS.md` слияние upstream, форматы сохранений, миграции и инвентарные транзакции требуют независимого ревью
  второго агента (Codex) — не выполнено.
