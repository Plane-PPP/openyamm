**Русский** | [English](README.en.md)

# Документация Minotaur Rebalance

Всё в `docs/`, а также `CHANGELOG.md` и `BUILD-NOTES.md` принадлежит форку. Документация OpenYAMM находится в `devdocs/`,
`level_generation/`, `packaging/` и `README.md`, она ведётся в upstream и здесь не переводится.

## С чего начать

| Документ | Назначение | Язык |
|---|---|---|
| [Windows-патч 1.21](research/windows-1.21-build.md) | ZIP, исходная версия, сейвы, ресурсы и откат | EN |
| [ROADMAP](ROADMAP.md) | Текущее состояние, планы, история | RU, [EN](ROADMAP.en.md) |
| [Описание мода](MINOTAUR_REBALANCE.md) | Механики, формулы, диагностика, тестирование | RU, [EN](MINOTAUR_REBALANCE.en.md) |
| [Lethal Strike v0.4](design/death-strike.md) | Действующий контракт навыка | RU, [EN](design/death-strike.en.md) |
| [Lethal Strike и защищённые цели](design/lethal-strike-protection-task.md) | Отложено, Deferred / Low Priority | RU, [EN](design/lethal-strike-protection-task.en.md) |
| [Заметки 1.18](minotaur-1.18.md) | Стаки реагентов и расширенные подсказки | EN |
| [Независимый аудит](research/independent-audit-2026-11-02.md) | Результаты ревью и рекомендации | RU, [EN](research/independent-audit-2026-11-02.en.md) |
| [Иммунитеты нежити](research/undead-condition-immunities.md) | Исследование личей и вампиров | RU, [EN](research/undead-condition-immunities.en.md) |
| [Свечение окон MM8](research/surface-materials-mm8-window-glow.md) | Исследование Surface Materials | RU, [EN](research/surface-materials-mm8-window-glow.en.md) |

## Особенности репозитория

- **Sparse checkout.** Многие документы, например большая часть `docs/research/`, на диске отсутствуют, но отслеживаются.
  Читать их: `git show HEAD:docs/research/combat-balance.md`. Не выполняйте `git sparse-checkout reapply`.
- **Новые файлы.** `docs/` больше не игнорируется, новые документы сразу видны в `git status`. Каталог находится вне шаблонов
  sparse, поэтому файлы в нём добавляются так: `git add --sparse <путь>`. Обычный `git add` их отклоняет.

## Как устроены языки

Русский документ сохраняет свой путь, чтобы не ломать ссылки и историю Git. Английская версия лежит рядом с суффиксом
`.en.md`: `ROADMAP.md` и `ROADMAP.en.md`. Первая строка обеих версий переключает язык.

## Правила поддержки

1. Изменение важного документа обновляет обе версии в том же коммите.
2. Новый основной документ создаётся сразу на двух языках. Исследования и журналы сборок можно писать на одном языке и
   добавлять перевод по необходимости.
3. Структура разделов, формулы, числа, идентификаторы и ссылки в обеих версиях совпадают.
4. Статусы не повышаются при переводе: запланированное не становится реализованным, гипотеза не становится фактом.
5. Единые термины: Axe, Recovery, Damage Resistance (название в игре; в обсуждениях встречается Damage Reduction),
   Grandmaster или GM, Lethal Strike, off-hand, Stats.
6. Перед коммитом документации запускайте `python3 tools/check_doc_i18n.py`. Предупреждение о ссылке или коде, которые есть
   только в одной версии, нужно просмотреть: часть из них ожидаема, например ссылка на собственную языковую пару.

## Что осталось перевести

| Приоритет | Документы | Причина |
|---|---|---|
| 1 | `CHANGELOG.md`, `BUILD-NOTES.md` | Смешанный язык, часто обновляются |
| 2 | `research/combat-balance.md`, `research/final-review.md`, `design/monster-knowledge.md`, `design/identify-monster.md` | Основание баланса и отложенные функции |
| 3 | `research/windows-ci-optimization.md`, `research/upstream-fc4e117-fix-integration.md`, `research/minotaur-incremental-updates.md` | Процесс и инфраструктура |
| 4 | `research/monster-lore.md`, `research/monster-stats.md`, `research/monster-size-classification.md`, `research/reproduce.md`, `research/lethal-strike-teacher-*.md`, `research/damage-resistance-layout-bug.md`, `patches/README.md` | Узкие или исторические материалы |

Остальные документы `docs/research/` уже написаны на английском.
