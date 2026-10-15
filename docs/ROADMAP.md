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

## Текущие этапы

| Этап | Статус | Результат |
| --- | --- | --- |
| Исследования таблиц/карт | Research completed | 658 общих шаблонов,198 исходных MM8,184 с кампанийными свидетельствами; [ревизия](research/final-review.md) |
| Контракт v0.4 | Approved | P0 от S/V, потолок30%, GIANT строго ниже60%, Identify Monster не участвует |
| Реестр категорий | Implemented | 48 GIANT /610 NORMAL; MM8 21/177; явные ID, unknown NORMAL + trace warning |
| Навык и обучение | Implemented | ID40, Normal1, идемпотентная миграция, M/GM caps, три MM8 Axe-учителя с сохранением услуг |
| Бой и диагностика | Implemented | Один trial после melee, штатная смерть, заметное сообщение, отдельный trace |
| Skills/Stats | Implemented | Динамический P0, Damage Resistance под AC, RMB, English/Russian mod texts |
| Автоматические проверки | Automatically tested | 21 focused /29884 assertions;3 runtime /46;187 расширенных /7048. Ограничения suite в BUILD-NOTES |
| Windows packaging | Authorized / pending | Существующий Minotaur Windows build; успех/артефакт фиксировать только по Actions |
| Ручной тест v0.4 | Pending user | Новая сборка не считается проверенной до внутриигрового прогона |
| Identify Monster | Deferred | Caps/UI/поведение не меняются; knowledge, AC/damage/aura/GM magic отложены |
| Бестиарий/лор | Deferred | Исследования сохранены, парсинг и расширенный интерфейс отложены |

## Следующие этапы

1. Зафиксировать завершённые автоматические проверки, SHA и ограничения в BUILD-NOTES.
2. Отправить коммиты в feature/minotaur-rebalance и дождаться реального успеха Windows workflow.
3. Проверить в игре частоту/сообщение, GIANT60%, dynamic P0, Damage Resistance, учителей и старые saves.
4. По пользовательским результатам отдельным этапом корректировать экспериментальный баланс.

Каждый законченный этап фиксировать отдельным коммитом с обновлением ROADMAP.
Не выполнять merge main. Новый баланс Axe/DR и Identify Monster не входит в задачу.
Актуальный контракт: [Lethal Strike v0.4](design/death-strike.md).
