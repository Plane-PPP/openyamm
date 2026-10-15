# Death Strike v0.2

Уточнение 2026-10-15: [финальная ревизия](../research/final-review.md) отделяет
198 исходных descriptor records от 184 разновидностей с кампанийными свидетельствами,
уточняет caps/UI, корреляции и вопросы визуальной классификации. Прежние расчёты по 198
сохранены как инвентарный baseline; они не являются статистикой частоты встреч.


Статус: **Proposed**, формула проектируется и рассчитана, **не реализована**.
Назначение: уникальный развиваемый за обычные очки навык игрового Minotaur/Minotaur Lord.
Этапы [ROADMAP](../ROADMAP.md#этапы-разработки): R2, R5, R6.

## Подтверждённый прототип и принятый контракт

В [monster_data.txt](../../assets_dev/engine/data_tables/monster_data.txt) merged MM7-шаблон
**ID303 Minotaur Lord / AMinotaur C / Level79 / HP861** содержит `Bonus=Dead`.
[MonsterTable](../../game/tables/MonsterTable.cpp) разбирает Dead;
[GameplayCombatController](../../game/gameplay/GameplayCombatController.cpp) сопоставляет его
`CharacterCondition::Dead`. Это атака **монстра по персонажу**; её проверки состояния, защиты
и сопротивления нельзя механически перенести в новый навык персонажа против монстра.

Death Strike — уточнённое название прежнего планового **Lethal Strike**, а не вторая способность.
Normal/Expert/Master/Grandmaster, обычные очки и стоимость уровня; конкретные учителя,
пороги обучения и начальное получение навыка ещё должны быть определены.
Доступность класса и расы следует разделить: предложение ограничивает Minotaur/MinotaurLord,
а не все классы с race_id=3, если пользователь отдельно не решит иначе.

Одна проверка на успешно завершённую **melee-атаку** по живой цели. Не зависит от оружия:
одноручное, двуручное, две руки и Unarmed имеют один trial за действие. Промах, стрельба,
заклинание, удар по world object и уже погибшая от обычного урона цель не получают trial.
Успешное попадание с нулевым итоговым уроном не названо исключением в концепции:
будущая реализация должна проверять hit/action, а не только `damage > 0` текущего контроллера.
Нужно отдельно согласовать поведение на полностью неуязвимой/скриптовой цели.

## Формула v0.2 — без округления промежуточных процентов

`S` — численный Death Strike; `V` — Perception; `I` — личный Identify Monster атакующего.
В расчётах это базовые вложенные уровни. Использование effective skill с enchantments пока
открытый вопрос; нельзя незаметно заменить базовый уровень и изменить баланс.
Если вариант A/B/C лично не изучен, **только вклад I равен нулю**; S и V остаются.
Party Master/GM не даёт чужой I для Death Strike.

| Мастерство | P0, проценты |
| --- | --- |
| Normal | `0.30*S` |
| Expert | `0.45*S + 0.14*V + 0.07*I` |
| Master | `0.55*S + 0.16*V + 0.08*I` |
| Grandmaster | `0.65*S + 0.18*V + 0.09*I` |

`delta = PlayerLevel - MonsterLevel`.

```text
delta >= 0: KL = min(1.4, 1 + delta / 60.0)
delta <  0: KL = 30.0 / (30.0 + 1.5 * abs(delta))
NORMAL, hp > 0: KG = 1
GIANT, 2*hp >= runtimeMaxHp: KG = 0
GIANT, 0 < 2*hp < runtimeMaxHp: KG = 0.25
P_percent = min(15.0, P0 * KL * KG)
```

HP берётся после обычного урона и его актуальных сопротивлений/hooks. Разность уровней и HP
должны браться из согласованного runtime facts контракта; базовые шаблоны использованы только
для исследования. В production использовать дробный roll, например `uniform_real [0,100) < P`.
Не применять целочисленный `roll(100) < truncated(P)` и не потреблять RNG при пропуске проверки.

## Примеры и порядок применения потолка

| S/V/I | Ранг | P0 | P равные уровни NORMAL | GIANT ниже 50% |
| --- | --- | --- | --- | --- |
| 5/5/5 | Expert | 3.30% | 3.30% | 0.825% |
| 10/5/5 | Master | 6.70% | 6.70% | 1.675% |
| 10/10/10 | GM | 9.20% | 9.20% | 2.30% |
| 15/10/10 | GM | 12.45% | 12.45% | 3.1125% |
| 20/20/20 | GM | 18.40% | 15.00% | 4.60% |

При delta−20 KL=0.5; delta−40=1/3; delta−60=0.25; delta+24 и выше KL=1.4.
Для GM20/20/20 и delta+24 GIANT получает **6.44%**, а не 3.75%:
потолок 15 применяется **после KG**, в точном соответствии с v0.2.
[Полная сетка вероятностей](../research/death-strike-probabilities.csv) и
[моделирование](../research/combat-balance.md) показывают пользу S5–10 и цену S15–20.
I от партийного исследователя не заменяет личное знание даже при активном бонусе damage.

## Техническая последовательность

[GameplayActionController::executePartyAttack](../../game/gameplay/GameplayActionController.cpp)
(ветка melee рядом с `resolveMeleeAppliedDamage`/`applyPartyAttackMeleeDamage`) уже объединяет
руки и обычный удар; здесь должен жить общий для indoor/outdoor decision после применения
обычного урона. На текущем этапе функция только исследована, новая проверка не добавлена.

1. Завершить обычный урон ровно один раз, восстановить target facts и проверить живую цель.
2. Рассчитать личные S/V/I, known, KL/KG, вероятность; один roll для eligible target.
3. При успехе передать **reason=DeathStrike** общей операции завершения жизни actor.
4. Один переход alive→dying/dead, единственные XP, reputation/quest hooks и генерация corpse/loot.
5. Result содержит `deathStrikeSucceeded` и окончательный killed; feedback использует готовый result.

Путь смерти сейчас размазан между
[IndoorWorldRuntime::applyPartyAttackMeleeDamage](../../game/indoor/IndoorWorldRuntime.cpp)
и [OutdoorWorldRuntime](../../game/outdoor/OutdoorWorldRuntime.cpp), отдельными projectile/spell
и actor-death путями. Безопасная будущая архитектура: общий контракт terminal event и единственный
владелец death finalization для каждого world adapter. Нельзя «добить» повторным обычным уроном
с громадным числом: это может повторить Pain Reflection, on-hit hooks, lifesteal и feedback.
Нельзя использовать `CharacterCondition::Dead` для actor как прямой перенос prototype.

При success showHitStatus через
[GameplayCombatController::handlePartyAttackPresentation](../../game/gameplay/GameplayCombatController.cpp)
должен выбрать сообщение **вместо последующего перезаписывания стандартной строкой**:

- `Minotaur delivers a Death Strike!`
- `Минотавр наносит смертельный удар!`

Текущий formatter использует английские строки; новые строки нуждаются в существующем
локализационном ресурсе в следующем этапе. Локализация пока не изменялась. Неудачный trial не
производит отдельного сообщения. Обычная XP/loot обработка не вызывается повторно из UI.

## Регистрация навыка и совместимость

Будущие точки: [SkillData.cpp](../../game/party/SkillData.cpp) (canonical names/aliases/display),
[ClassSkillTable](../../game/tables/ClassSkillTable.cpp) (cap/learning),
[Party](../../game/party/Party.cpp) (обучение/повышение),
[MasteryTeacherDialog](../../game/gameplay/MasteryTeacherDialog.cpp),
[GameplayPartyOverlayRenderer](../../game/ui/GameplayPartyOverlayRenderer.cpp) и списки
навыков в overlay/new-game/input. Нельзя ограничиться новой строкой в описаниях:
несколько списков имеют фиксированную каноническую номенклатуру.

Старым персонажам не выдавать автоматически уровни S; миграция может дать доступ к изучению
навыка без расхода старых навыков. Сам навык пока отсутствует. Сохранение knowledge и новые
поля spell transaction рассмотрены в [monster-knowledge](monster-knowledge.md).

## Диагностика и открытые решения

Расширить [GameplayDebugTrace](../../game/debug/GameplayDebugTrace.cpp) отдельным выключенным
по умолчанию конфигом `death_strike_trace` (предложение, ключ ещё не существует).
Одна запись: action ID, персонаж/level, S/rank/V/I и base/effective policy, known,
monster canonical/raw/runtime ID и level, category/revision, hp_before, hp_after_ordinary,
runtime_max_hp, P0/KL/KG/P, roll/result/skip_reason, terminal event ID.
Диагностика не делает дополнительный RNG trial и не меняет порядок существующих roll.

Открыто: учителя и требования; cap Minotaur/MinotaurLord по Death Strike;
base/effective уровни; bolster Level; боссовые script immunity; нулевой урон; контролируемые,
summoned и reanimated цели; перечень визуально спорных GIANT. Ни одно исключение ещё не внедрено.
В v0.2 нет иммунитета по Undead/Elemental/NoCorpse; вводить его без отдельного решения нельзя.
