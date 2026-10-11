# Minotaur Rebalance — следующая Windows-сборка

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
