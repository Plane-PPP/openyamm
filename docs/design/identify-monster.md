# Identify Monster: существующая механика и расширение

> **Deferred по ТЗ Lethal Strike v0.4, 2026-10-15.** Существующее поведение Identify Monster
> не меняется. Knowledge, новые caps/формулы/боевые бонусы, GM magic и бестиарий не реализуются
> в этой версии и не являются зависимостями Lethal Strike. Ниже сохранён исследовательский проект.


Статус: **Research / Proposed**. Назначение: дополнить уже существующий навык, сохранив просмотр,
тексты, обучение и ограничения. Источник: commit `142a54e01da50d9faf258e38919e88dba9e8de29`,
срез 2026-10-14. Этапы [ROADMAP](../ROADMAP.md#этапы-разработки): R1, R3, R4, R6.

## Подтверждённое текущее поведение OpenYAMM

Есть три отдельных слоя, которые нельзя смешивать.

| Ранг | Описание в skill_des.txt | Успех существующей речевой реакции | Фактический просмотр в текущем renderer |
| --- | --- | --- | --- |
| Нет навыка | Нет обещанного бонуса | SpeechId::None | Все доступные ниже поля |
| Normal | HP и AC | `I+10 >= MonsterLevel` | Те же поля |
| Expert | Дополнительно тип и урон атак | `2*I+10 >= MonsterLevel` | Те же поля |
| Master | Дополнительно заклинания | `3*I+10 >= MonsterLevel` | Те же поля |
| Grandmaster | Дополнительно сопротивления | Всегда успешно | Те же поля |

Источники: [английские тексты](../../assets_dev/engine/data_tables/english/skill_des.txt),
[GameMechanics::resolveIdentifyMonsterSpeech](../../game/gameplay/GameMechanics.cpp),
[GameplayPartyOverlayRenderer::renderActorInspectOverlay](../../game/ui/GameplayPartyOverlayRenderer.cpp).
Renderer без проверки mastery рисует имя/preview, текущие/максимальные HP, AC, типы/урон обеих атак,
заклинания и их уровень/мастерство, все десять сопротивлений и активные состояния.
Runtime данные поступают из `actorInspectState`; сопротивления берутся из базового pStats.
Шаблонный Level/XP не выводится этим блоком как отдельная строка. Отсутствующий layout/preview
может препятствовать отображению, но это не mastery gating.

При нажатии RMB indoor/outdoor выбирают `bestPartyWideUtilitySkillMemberIndex("IdentifyMonster")`
и вызывают речевую реакцию по базовому уровню шаблона. При успехе монстр считается сильным,
если `MonsterLevel >= character.level - 5`, иначе слабым; при неуспехе — IdentifyMonsterFail.
В текущей реализации это **не записывает постоянное знание** и не скрывает поля окна.
Источники: [IndoorGameView](../../game/indoor/IndoorGameView.cpp),
[OutdoorGameView](../../game/outdoor/OutdoorGameView.cpp), [Party](../../game/party/Party.cpp).
Utility selection использует собственный score и обычно требует canAct; его нельзя переиспользовать
как единственный источник новых bonuses: лучший score не обязательно знает нужную разновидность.

**Расхождение описания и фактического UI уже существует.** Требование сохранить оригинальную
последовательность раскрытия трактуется как сохранение описанного контракта и запрет ухудшать
существующий просмотр. В этой задаче gating не добавляется. До реализации нужно отдельно решить,
является ли восстановление ступеней самостоятельной задачей. Новое знание не становится условием
для доступа к уже видимым старым полям. Нельзя объявить, что все версии MM ведут себя одинаково.

## Финальная проверка 2026-10-15

[Ревизия с источниками](../research/final-review.md) подтверждает отсутствие UI gating и
действующие caps. Разрешение противоречия с build #3 требует данных установленного пакета
и className/raceId персонажа. Предложение для GM после продвижения: только Lich M→G;
не применено. Для естественного Lich учитывается Undead/race6, а не предположение Human.
Кампанийное покрытие рассчитано отдельно по 184 подтверждённым данными разновидностям,
с процентами для PL10/20/30/50 и I1/5/10; таблица ниже на 198 — инвентарный baseline.

## Реальные ограничения классов: обнаруженный конфликт

[class_skills.txt](../../assets_dev/engine/data_tables/class_skills.txt),
[race_skills.yml](../../assets_dev/engine/data_tables/race_skills.yml),
[ClassSkillTable::getEffectiveCap](../../game/tables/ClassSkillTable.cpp):
`clamp(max(baseCap, raceMin) + raceAdd)`, но raceAdd применяется только при baseCap≠None.
Race/exception правила требуют отдельной проверки на конкретном персонаже.

| Класс | Identify Monster base cap | Для естественной расы класса | Другие важные caps |
| --- | --- | --- | --- |
| Minotaur | None (`-`) | None, race3 не имеет правила IdentifyMonster | Axe E→M race3; Armsmaster M; Perception M |
| MinotaurLord | None (`-`) | None | Axe M→GM race3; Armsmaster M; Perception GM |
| Necromancer | Master | Master для Human | Perception E; нет Armsmaster |
| Lich | Master | Master для Undead/race6 (также Human) | Не GM Identify Monster |
| Knight | Normal | Normal для Human | Armsmaster M |
| Champion / BlackKnight | Normal | Normal для Human | Armsmaster GM |
| Vampire | Master | Master race1 | — |
| Nosferatu | GM | GM race1 | Действующий естественный GM-исследователь |
| Dragon / GreatWyrm | Master | GM при race5 (`add:1`) | Расовый cap отличается от base |
| Ranger / Hunter | Master | Master для Human | — |
| BountyHunter / RangerLord | GM | GM для Human | — |
| Sorcerer / Wizard / ArchMage / Dark Archmage | Master | Master для Human | — |

Пользователь подтвердил внутриигровой **Expert Identify Monster у игрового минотавра**.
Это факт тестовой среды пользователя, который **не объясняется** текущими class/race tables.
Возможные проверяемые причины: ранее сохранённый навык, другая mounted table, package override,
особый персонаж или способ выдачи навыка. Ни одна причина пока не установлена.
Наличие навыка в старом save не доказывает возможность его заново выучить у учителя.

Принятое дизайнерское ограничение: Minotaur остаётся максимум **Expert**, без M/GM;
MinotaurLord не расширяется без отдельного решения. В этом этапе таблицы не исправляются даже
ради устранения конфликта. Реальный максимум Lord по текущей таблице — None.
Necromancer может быть Master-исследователем и передавать AC/damage, но естественного GM для него
текущая таблица не даёт. Концепция «некромант с GM» пока условный сценарий: потребует отдельного
решения о caps; альтернативы без их изменения — Nosferatu или Dragon race5 как GM-специалист.
Бонусы универсальны для всех легально доступных классов, остальных caps не менять.

Обучение сохраняет существующие правила:
[Party::canMemberLearnSkill](../../game/party/Party.cpp) проверяет effective cap;
[MasteryTeacherDialog](../../game/gameplay/MasteryTeacherDialog.cpp) — cap, класс/продвижение,
порог skill и предыдущий ранг. Базовые пороги E4/M7/GM10; таблица teacher_topics может задавать
дополнительные Req.Skill/Gold. Формульные GM5 или Master5 ниже — только сравнение коэффициентов,
а не подтверждение доступности обучения на таком уровне.

## Дополнительное постоянное изучение

Память знания отдельна от оригинального просмотра. Игрок явно выбирает персонажа и действие
«Изучить» в существующем окне информации; это не автоматическая запись при sight/kill/RMB речи.
Вид A/B/C сохраняется навсегда для данного Character, включая выход в Adventurers Inn.
Повторное успешное изучение того же ключа не создаёт повторного бонуса/сообщения.
[Хранение и UI](monster-knowledge.md).

```text
IdentificationPower = PlayerLevel + masteryMultiplier * IdentifyMonsterLevel
IdentificationDifficulty = max(0, MonsterLevel - PlayerLevel)
success = power >= difficulty
masteryMultiplier = Normal:1, Expert:2, Master:3, Grandmaster:4
```

Детерминированно, при неудаче запись не добавляется; после развития повторная попытка разрешена.
Персонаж должен владеть навыком: отсутствие навыка не трактовать как Normal0 с автоматическим
успехом на низком уровне. Выбор base/effective I нужно согласовать; аналитика использует base.
Порог эквивалентен `MonsterLevel <= 2*PlayerLevel + multiplier*I`.
Это **новая проверка знания**, она не заменяет старую формулу речевой реакции или описанные ступени.
GM теперь не гарантирует запись знания о любом монстре: гарантирована только старая речевая реакция.

Пример PL30/GM10 против ML90: power70, difficulty60, success. PL30/E10 достигает ML80,
M10 — ML90, GM10 — ML100. PL35/GM15 достигает ML130 и покрывает все 198 нативных вариантов;
PL35/E15 — ML100 и 197/198. Самый высокий Ruby Dragon125 разделяет эти пороги.
PL50/E5 покрывает 197/198; поздно вклад mastery в доступность почти насыщен.
Не увеличивать коэффициенты до игровых тестов: уже высокое покрытие, ранний GM сдерживает цена.

[Полная матрица 19800 строк](../research/identification-matrix.csv): каждая разновидность при
PL10/20/30/35/50, I1/5/10/15/20 и четырёх рангах. Колонка `mastery_numeric_requirement_met`
отмечает минимальную численную доступность ранга; class cap, цена и учитель отдельно.

| Player level | I | N:предел (доступно) | E | M | GM |
| --- | --- | --- | --- | --- | --- |
| 10 | 5 | 25 (98/198) | 30 (112/198) | 35 (120/198) | 40 (133/198) |
| 10 | 10 | 30 (112/198) | 40 (133/198) | 50 (155/198) | 60 (166/198) |
| 10 | 15 | 35 (120/198) | 50 (155/198) | 65 (171/198) | 80 (188/198) |
| 10 | 20 | 40 (133/198) | 60 (166/198) | 80 (188/198) | 100 (197/198) |
| 20 | 5 | 45 (141/198) | 50 (155/198) | 55 (156/198) | 60 (166/198) |
| 20 | 10 | 50 (155/198) | 60 (166/198) | 70 (177/198) | 80 (188/198) |
| 20 | 15 | 55 (156/198) | 70 (177/198) | 85 (190/198) | 100 (197/198) |
| 20 | 20 | 60 (166/198) | 80 (188/198) | 100 (197/198) | 120 (197/198) |
| 30 | 5 | 65 (171/198) | 70 (177/198) | 75 (180/198) | 80 (188/198) |
| 30 | 10 | 70 (177/198) | 80 (188/198) | 90 (194/198) | 100 (197/198) |
| 30 | 15 | 75 (180/198) | 90 (194/198) | 105 (197/198) | 120 (197/198) |
| 30 | 20 | 80 (188/198) | 100 (197/198) | 120 (197/198) | 140 (198/198) |
| 35 | 5 | 75 (180/198) | 80 (188/198) | 85 (190/198) | 90 (194/198) |
| 35 | 10 | 80 (188/198) | 90 (194/198) | 100 (197/198) | 110 (197/198) |
| 35 | 15 | 85 (190/198) | 100 (197/198) | 115 (197/198) | 130 (198/198) |
| 35 | 20 | 90 (194/198) | 110 (197/198) | 130 (198/198) | 150 (198/198) |
| 50 | 5 | 105 (197/198) | 110 (197/198) | 115 (197/198) | 120 (197/198) |
| 50 | 10 | 110 (197/198) | 120 (197/198) | 130 (198/198) | 140 (198/198) |
| 50 | 15 | 115 (197/198) | 130 (198/198) | 145 (198/198) | 160 (198/198) |
| 50 | 20 | 120 (197/198) | 140 (198/198) | 160 (198/198) | 180 (198/198) |

## Новые боевые эффекты: только известные разновидности

| Ранг | Дополнительный эффект | Источник |
| --- | --- | --- |
| Normal | +I AC только против известной разновидности | Лично знающий владелец навыка |
| Expert | +I damage к melee, shooting и атакующей магии; сохраняет AC | Личный |
| Master | AC и damage доступны всей активной партии | Каждый M/GM, лично знающий эту цель |
| Grandmaster | `P_double_spell = I%` на ×2 атакующего заклинания | Максимальный лично знающий GM в активной партии |

AC и damage: **независимый максимум** между подходящими личными и партийными источниками,
без суммирования. Личный Normal15 даёт AC15, но не damage15; известный Master10 даёт партии
AC10/damage10. Итог для этого персонажа AC15/damage10. Expert12 лично знающий получает
AC12/damage12, даже если рядом Master10. Master15, не знающий вид, не подходит.
Разные исследователи могут дополнять coverage. Выход специалиста из активной партии немедленно
убирает его aura, сохраняя его знания. Смерть/бессознательность специалиста: политика canAct
пока открыта, не подменять требование присутствия в партии автоматически.
GM chance — отдельный max среди GM, поэтому Master20 не блокирует знающего GM10.
Death Strike всегда использует **свои I и knowledge**, даже если личный damage заменён командным.

## Корректные точки начисления

**AC:** в `resolveMonsterImpactHitCheck` общего
[GameplayCombatController](../../game/gameplay/GameplayCombatController.cpp), после определения
источника-монстра и защитника, до hit roll. Не менять Character::armorClass/общий character sheet.
Учесть melee и немагические missiles, для которых движок уже проверяет AC. Магия/AoE, обходящие
AC, не получают новую защиту от AC сами по себе. Предлагается добавлять knowledge AC после
bolster-adjustment, чтобы +I не масштабировался вторично; порядок ещё требует согласования.

**Melee:** общий [GameplayActionController](../../game/gameplay/GameplayActionController.cpp)
и `resolveMeleeAppliedDamage`/`resolveAxeMeleeWeaponDamage`. Один плоский бонус на итоговое действие
по цели, не на руку/elemental enchant/on-hit. Предложение: separate additive component после
существующего GM Axe ×2/target slayer multiplier, но до сопротивления Physical. Тогда +I сохраняет
буквальный смысл +I, не становится +2I или несколькими I. Это новый порядок только для нового
бонуса; старый Axe/Recovery не меняется. Иное размещение требует явного расчёта/решения.

**Shooting:** при разрешённом impact в
[GameplayProjectileService::buildProjectileDirectActorImpact](../../game/gameplay/GameplayProjectileService.cpp)
и world adapters, где цель уже известна и бросок попадания завершён. Release не знает окончательную
цель и не является корректной точкой. Несколько стрел одного действия используют общий action ID:
плоский бонус не начисляется повторно за каждый внутренний компонент по одной цели.

**Magic:** [GameplaySpellService](../../game/gameplay/GameplaySpellService.cpp),
[GameplaySpellActionController](../../game/gameplay/GameplaySpellActionController.cpp),
[PartySpellSystem](../../game/party/PartySpellSystem.cpp), projectile impacts и direct world requests.
Нельзя менять только `rollSpellDamage`: там ещё нет фактической цели/личного knowledge, и не все
источники урона проходят одну функцию. Бонус по реальному spell-target transaction добавляется
один раз, того же типа урона, до сопротивлений, без обхода иммунитетов. Нулевой/полностью
резистированный урон требует явного правила; рекомендуется не создавать damage в immune target.

## GM и многокомпонентная магия

Предпочтительно одна проверка `(castInstanceId, targetActorStableIdentity)`, max GM I снимается
в snapshot источников при первом допустимом impact. Один результат ×1/×2 используется для всех
компонентов данного применения на данной цели. Для другой цели отдельная проверка.
`projectileId`, spellId или animation tick не являются cast ID: разные компоненты имеют разные ID,
а один и тот же spellId может применяться несколько раз параллельно.

| Путь | Что исследовано | Безопасное расширение |
| --- | --- | --- |
| Shrapmetal | PartySpellSystem: MultiProjectile, минимум Expert, 5 projectile requests | Один cast ID для всех пяти; отдельный cached roll на цель; flat +I один раз |
| Многоударные/несколько снарядов | Отдельные impact события; цель известна поздно | Общий transaction context с used-flat и multiplier по цели |
| AoE | Один impact может породить несколько target damages | У каждой цели собственная запись; иммунитеты каждого target отдельно |
| Многоцелевые direct spells | World spell requests обходят release обычной melee | Тот же target transaction resolver для direct и projectile paths |
| Периодический урон/Immolation | Отдельные updateImmolation в обоих worlds | Сохраняемый effect/cast ID, результат на цель за весь эффект; не новый roll на каждый tick |
| Повторный вход цели в AoE | Возможны поздние/повторные impacts | Существующий multiplier, новый trial не делать |

Текущий [ProjectileState](../../game/gameplay/GameplayProjectileService.h) сохраняет projectileId,
source member, spellId и damage, **не имеет общего castInstanceId/target roll cache**.
Причина сложности — асинхронные world impacts, direct/AoE/periodic обходы и save/load живых
снарядов. Поэтому GM нельзя безопасно «включить одной строкой» в каждом impact.

Предлагаемая архитектура: DamageApplicationContext (action/cast/effect ID, source, origin,
component ID, target generation) → общий target modifier resolver → прежние сопротивления и
world damage hooks → единственное death finalization. Хранить ×2 decision, consumed flat bonus
и lifecycle reference count до завершения всех components. Кэш живых casts/effects сериализовать,
иначе загрузка позволяет повторить roll или +I. При удалении/замене actor не переиспользовать
запись по одному actor index. Для DoT длительный cache ограничен временем эффекта.

Предложение ×2 применять к обычному spell damage **до сопротивлений**, новый +I не удваивать;
вариант `(D+I)*2` рассматривается отдельно в [балансе](../research/combat-balance.md).
Единая проверка не запрещает ×2 всех пяти Shrapmetal hits на одной цели, но исключает
пять независимых trial и повторное +I. Минимальная безопасная альтернатива до transaction layer:
GM отложить, реализовать N/E/M; либо явный ограниченный список single-hit spells как отдельный
согласованный этап. Молчаливое per-projectile поведение не соответствует проекту.

## Диагностика и нерешённые вопросы

Предложить выключенный `identify_monster_trace` в существующем
[GameplayDebugTrace](../../game/debug/GameplayDebugTrace.cpp). Записи study: actor/template key,
персонаж/level/I/rank, power/difficulty/result, already_known; combat: AC/damage, личный/партийный
source ID, candidates/max, class/race, cast/target ID, flat_consumed, GM chance/roll/multiplier,
damage_before/after/resisted. Выключение не меняет RNG, HP, party state.

До реализации требуется сверить class-table конфликт с установленным build #3, выбрать base/effective skills,
bolster level, canAct policy, порядок additive AC/damage и обработку periodic damage.
Оригинальные описания/обучение/речь/просмотр сохраняются; новые функции пока отсутствуют.

Связи: [Death Strike](death-strike.md), [знание и UI](monster-knowledge.md),
[статистика](../research/monster-stats.md), [баланс](../research/combat-balance.md).
