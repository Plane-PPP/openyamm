# Lethal Strike v0.4 — Смертельный удар

> **Known bug LS-001, 2026-10-16:** пользователь подтвердил отсутствие Lethal Strike у Herald Foestryke.
> Все три новые темы имеют пять колонок и пропускаются загрузчиком; обучение у NPC не завершено.
> [Причина и отложенное исправление](../research/lethal-strike-teacher-bug.md). Игровые ресурсы пока не исправлены.

Действующий контракт: ТЗ пользователя v0.4 от 2026-10-15. Прежнее проектное название Death Strike
и формула v0.2 заменены; второй способности нет. Старые расчёты доступны в истории Git и помеченных
исторических исследованиях. Identify Monster и бестиарий — **Deferred**, без изменений игрового поведения.

## Формула и порядок

S и V — **базовые** численные уровни Lethal Strike и Perception; предметы/нанятые NPC не добавляют
к ним бонусов. Ранг Perception не множит V. Отсутствующий Perception даёт V=0.

| Ранг | P0, проценты | Доступность |
| --- | --- | --- |
| Normal | `0.60*S` | Minotaur / MinotaurLord |
| Expert | `0.90*S + 0.56*V` | Minotaur / MinotaurLord |
| Master | `1.10*S + 0.64*V` | Minotaur / MinotaurLord |
| Grandmaster | `1.30*S + 0.72*V` | Только MinotaurLord после обучения |

`delta=PlayerLevel-MonsterLevel`.
При delta≥0: `KL=min(1.4,1+delta/60.0)`; иначе `KL=30/(30+1.5*abs(delta))`.
NORMAL: KG=1 при HP>0. GIANT: KG=1 только при `HP>0 && maxHP>0 && 5*HP<3*maxHP`, иначе KG=0.
Сравнение целочисленное int64; ровно60% запрещено. Штрафа вероятности раненому GIANT нет.
`P_percent=min(30.0,P0*KL*KG)`. Промежуточного округления/минимального шанса нет.
Один `uniform_real_distribution<double>(0,100)` trial; диагностика не добавляет RNG.
Identify Monster не участвует. Отдельных бонусов за продвижение, эффектов Master/GM нет.

PlayerLevel учитывает существующий levelModifier через общий расчёт движка. Пассивное сопротивление
по-прежнему использует прежнюю базовую формулу и не меняется. MonsterLevel — экспортированный
cappedTargetMonsterLevel того же bolster-расчёта, который масштабирует AC/обычный урон; без bolster —
шаблонный Level. Это не уровень, выведенный классификатором из HP.

Проверка находится в общем GameplayActionController, после обычного melee-урона/сопротивлений
и on-hit effects, до переключения активного персонажа. Любое разрешённое оружие, две руки и
Unarmed дают один trial. Нет trial для miss, world object, shooting, spells, уже мёртвой цели,
смерти от обычного урона или GIANT ≥60%. Успешное попадание с полностью резистированным уроном
остаётся допустимым. Нежить/элементали автоматически не исключаются.

## Получение, обучение и сохранения

Навык добавлен в конец canonical skills как ID40; ID0–39 сохранены. Character.skills — строковая
карта, не фиксированный массив. Новый Minotaur/MinotaurLord получает Normal1 сверх выбранных
навыков бесплатно. Общая синхронизация применяется к созданию, seed/roster, найму, резерву,
гидратации старых сохранений и смене класса. `try_emplace` не меняет существующий ранг/мастерство;
повторная загрузка идемпотентна. Монстры не являются Character и навыка не получают.
Формат save v85 хранит cached outdoor combat Level; старые v84 и ранее читаются, отсутствующий
уровень восстанавливается при restoreSnapshot через bolster. Строковая карта навыков не меняет
свою структуру; LS отсутствовал в эталонном v84 fixture и мигрирует в Normal1 без потери других навыков.
Доступность классов задаёт новая строка class_skills.txt: M/G/None; другие caps сохранены.
Рост численного уровня использует штатную возрастающую цену очков.

| Учитель MM8 | Место / house ID | Ранг | Навык / золото | Новый topic |
| --- | --- | --- | --- | --- |
| Herald Foestryke, NPC115 | Garrote Gorge, house540 | Expert | 4 / 2000 | 29000 |
| Jasp Hunter, NPC116 | Ravenshore, house907 | Master | Expert7 / 5000 | 29001 |
| Garic Senjac, NPC117 | Balthazar Lair, house635 | GM | Master10 / 8000 | 29002 |

Оригинальные Axe topics309–311 и остальные услуги сохранены. Это обычные дополнительные пункты
диалога; проверка продвижения/очков/золота выполняется стандартным MasteryTeacherDialog.
Доступ подтверждён map events: out05:event15 → house540; out02:event463 → house907 при !QBit57;
d24:event225 → house635 после QBit23 (спасение минотавров). Эти сюжетные ограничения сохранены.

## Смерть и интерфейс

World runtime получает явный `killPartyAttackActor`, не искусственный урон. Он использует существующую
terminal-ветку: dying/corpse, XP, reputation, MonsterKilled hooks, death sound и drops; повторный вызов
на мёртвую цель отвергается. Второй MonsterDamage/PainReflection/lifesteal не создаётся.
MM9 actor с canReceiveDamage=false сохраняет существующую защиту; не добавлены исключения по природе MM8.

Skills отображает уровень и мастерство, стандартную кнопку повышения. RMB содержит коэффициенты,
ограничения и динамическую `Base Lethal Strike Chance: XX.XX%` (P0 без cap).
Успех вызывает `<name> delivers a Lethal Strike!` через обычную status bar, High priority, 4 секунды,
включая show_hits=false. Обычные Normal-priority сообщения его не затирают. Звук/графика требуют
ручной проверки; автоматический тест не является прослушиванием/визуальной приёмкой.

Stats содержит Damage Resistance непосредственно под Armor Class. Значение каждый кадр берётся
из существующего GameMechanics::minotaurDamageResistancePercent; остальные классы имеют0%.
Нижние Condition/QuickSpell сдвинуты на18 logical pixels. RMB поясняет только физический урон,
уровень, бонус продвижения и максимум30%.

В проекте нет общей runtime-локализации этих UI-строк. Опция `[debug] minotaur_russian_text=true`
включает русские новые описания/название/боевое сообщение и запятую в P0; остальной интерфейс
не переводится. По умолчанию English. Это независимая текстовая опция, формулы не меняет.

## Проверки и дальнейшая работа

[Инструкции тестирования](../MINOTAUR_REBALANCE.md), [BUILD-NOTES](../../BUILD-NOTES.md),
[ROADMAP](../ROADMAP.md). Аналитика: [v0.4 CSV](../research/lethal-strike-probabilities-v0.4.csv),
[утверждённая классификация](../research/lethal-strike-classification.csv).
Результаты автоматических проверок и точный Windows SHA фиксируются в BUILD-NOTES.
Новый Windows-билд до подтверждения пользователя не считается Manually tested.
