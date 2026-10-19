# Minotaur Rebalance — Roadmap

Действующее ТЗ: **Lethal Strike v0.4**, 2026-10-15. Оно заменило Death Strike v0.2.
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

| Этап | Статус | Результат |
| --- | --- | --- |
| Исследования таблиц/карт | Research completed | 658 общих шаблонов,198 исходных MM8,184 с кампанийными свидетельствами; [ревизия](research/final-review.md) |
| Контракт v0.4 | Approved | P0 от S/V, потолок30%, GIANT строго ниже60%, Identify Monster не участвует |
| Реестр категорий | Implemented | 48 GIANT /610 NORMAL; MM8 21/177; явные ID, unknown NORMAL + trace warning |
| Навык и обучение | LS-001 fixed / topics manually confirmed | Корректная схема тем; Expert Perception / Master Mace / GM Axe; [детали](research/lethal-strike-teacher-bug.md) |
| Бой и диагностика | Implemented | Один trial после melee, штатная смерть, заметное сообщение, отдельный trace |
| Skills/Stats | LS-002 fixed / layout manually confirmed | Четыре resource rows внутри рамки, исходные Condition/Quick Spell, раздельные Earth/Mind/Body; [детали](research/damage-resistance-layout-bug.md) |
| Автоматические проверки | Automatically tested | 21 focused /30044 assertions;3 runtime /58;187 расширенных /7048. Ограничения suite в BUILD-NOTES |
| Windows packaging | Packaged / success | [Run38784738765](https://github.com/Plane-PPP/openyamm/actions/runs/38784738765); SHA385a1e4; OpenYAMM-windows-Release.zip |
| Ручной тест v0.4 | In progress / hotfix retest pending | Исправление Stats и появление тем подтверждены; LS-003 и боевой прогон остаются |
| Identify Monster | Deferred | Caps/UI/поведение не меняются; knowledge, AC/damage/aura/GM magic отложены |
| Бестиарий/лор | Deferred | Исследования сохранены, парсинг и расширенный интерфейс отложены |

## Ресурсный hotfix 1

LS-001 и LS-002 исправлены отдельными коммитами. Подготовлен [малый патч](patches/Minotaur-v0.4-hotfix-1.zip)
для executable385a1e4, [установка и тест](patches/README.md). Автоматический прогон22 tests/30155 assertions
успешен; пользователь подтвердил вёрстку и появление тем.
Следующая полная сборка из этой ветки включит исправления в engine.zip без отдельного hotfix.

## Следующие этапы

Ресурсные исправления LS-001/LS-002 подтверждены пользователем в части Stats и появления тем.
[LS-003](research/lethal-strike-teacher-dialogue-followup.md): следующая сборка должна получить
тематические тексты пользователя и подсказки следующих учителей. Пока не реализовано.

1. Получить тексты пользователя для рангов и реализовать LS-003; затем подготовить следующую полную сборку.
2. Ручной прогон Windows v0.4: частота/сообщение, GIANT60%, dynamic P0, Damage Resistance, учителя и старые saves.
3. Зафиксировать результаты пользователя отдельно от автоматически проверенного.
4. По результатам отдельным этапом корректировать экспериментальный баланс.

Автоматические проверки и Windows packaging завершены; артефакт существует. Production source —
385a1e4. Hotfix меняет три ресурса поверх этой сборки, не executable;
предшествовавшие hotfix тестовые/документальные коммиты игровой код не меняли.
Подробности и ограничения — [BUILD-NOTES](../BUILD-NOTES.md).

Каждый законченный этап фиксировать отдельным коммитом с обновлением ROADMAP.
Не выполнять merge main. Новый баланс Axe/DR и Identify Monster не входит в задачу.
Актуальный контракт: [Lethal Strike v0.4](design/death-strike.md).

## Перенос v0.4 на OpenYAMM1.5 — In progress,2026-10-19

Официальный тег1.5:1414884155362e613977a3444a3c7f81ed4ee047. До объединения
сохранён fixture старого mod-v85. Обнаружена коллизия номера85 с upstream weather layout;
совместимость требует отдельного декодирования и проверки. Hotfix LS-001/LS-002 сохраняется.
