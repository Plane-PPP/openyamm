**Русский** | [English](undead-condition-immunities.en.md)

# Иммунитеты игровых личей и вампиров: MM7, MM8 и OpenYAMM

Дата: 2026-10-31. Статус: исследование завершено; исправления НЕ реализованы.
Проверена ветка feature/minotaur-rebalance, HEAD07e79ebb5b5b34c45dc1c27070b909cba10c8b7d,
и официальный upstream v1.20,1add1d374848648887c08707199921335b4ba440.
Сборка, изменения кода/баланса/ресурсов/CI и миграция сохранений не выполнялись.

## Наблюдение пользователя и практический вывод

Удар наги отравил вампира и лича; лич получен настоящим квестовым продвижением в MM8.
Это боевой спецэффект, а не прямое назначение состояния сценарием объекта.

- Вампир: врождённый иммунитет к яду не предусмотрен; отравление само по себе не баг.
- Лич: обнаружен конкретный дефект OpenYAMM — превращение выдаёт Light/Dark вместо Mind/Body.
  Он оставляет лича без ожидаемой защиты Body и объясняет его уязвимость к ядовитым ударам в этой реализации.
- Болезнь не равна яду: при спецударах Disease защищается Endurance/Luck, а не Body.
- Иммунитет к урону школы, сопротивление спецэффекту и прямой запрет состояния — три разные проверки.
  Нельзя объявлять любую болезнь/отравление нежити багом или вводить общий запрет всех состояний.
- Даже правильное восстановление Mind/Body не доказывает, что оригинальный MM7/MM8 никогда не позволял
  получить Poison/Fear от специального удара. Политика абсолютных condition-иммунитетов требует отдельного решения.

## Что подтверждают оригинальные источники

[Оригинальное руководство MM8, раздел Resistances](https://manualmachine.com/gamespc/mightandmagicviiidayofthedestroyer/1119628-user-manual/)
обещает игровому вампиру полный иммунитет к Mind. Body/яд в качестве врождённого иммунитета ему не указаны.
Vampire/Nosferatu — игровые классы MM8; в MM7 нет соответствующего игрового класса, хотя есть монстры-вампиры.
Их монстровые таблицы нельзя переносить на героя.

[Авторский changelog GrayFace MM8 Patch](https://grayface.github.io/mm/MM8PatchReadMe/), раздел2.5,
отдельно исправляет неработавший Mind-иммунитет вампиров. Поэтому «так было в непатченой игре» не всегда
означает задуманное правило. Этот changelog не объявляет вампиров невосприимчивыми к яду или болезни.

[GrayFace: механика MM7/MM8](https://grayface.github.io/mm/mechanics/#Chance-To-Hit) разделяет сопротивление
спецударам: Poison/Dead/Eradicated используют Body; Afraid/Insane/Paralyze — Mind; Disease/Sleep/Weak —
модификатор Endurance. Luck также участвует. Указанная формула успешного эффекта после попытки:
30/(30+LuckEffect+OtherEffect). Это не вероятность отравления на каждый взмах и не формула магического урона.

Для личей MM7/MM8 ожидаемый набор — Mind/Body, а не Light/Dark; четыре элементальных сопротивления при
превращении поднимаются как минимум до20. Прямое свидетельство механизма превращения:
[MMExtension ChangePlayerClass](https://grayface.github.io/mm/ext/ref/), флаг Lich запускает изменение лица и
выдачу иммунитетов. [Первичный hook GrayFace](https://github.com/GrayFace/MMExtension/blob/9d73c0ff0482bd231ea522f752f4fe9b26640925/Scripts/Core/events.lua#L2071)
различает обычную смену класса и превращение. Точный набор Mind/Body дополнительно согласуется с
[наблюдением игрока оригинальной MM8](https://www.reddit.com/r/MightAndMagic/comments/vqjvjo/), сравнившего
продвижение некромантов с готовыми Hevatia/Vetrinus. Последний источник — наблюдение пользователя, не
дизассемблирование и не самостоятельная достаточная проверка всех статусов. В том же сообщении отмечены
нулевые базовые сопротивления двух готовых личей; не следует смешивать это с текущим случаем продвижения.

## Почему нельзя написать «нежить вообще не получает Poison/Disease/Fear»

Проверен [справочный код реконструкции MM7 OpenEnroth](https://github.com/OpenEnroth/OpenEnroth/blob/a644b752f05af0cdbb1c5d96d120d6967bd8b790/src/Engine/Objects/Character.cpp):
CalculateIncommingDamage, ReceiveSpecialAttackEffect, GetActualResistance, SetCondition.
Отдельный путь спецудара берёт численное сопротивление и делает спасбросок; class-Lich запрета состояния
в нём нет. GetActualResistance ограничивает значение лича200. Отсюда для этого реконструированного пути
при Body200/LuckEffect0 остаётся30/230≈13,04% после попытки спецэффекта, до отдельной защиты предметом/баффом.
Это иллюстрация различия механизмов, НЕ измеренная вероятность оригинальной MM8 и НЕ утверждение о
точности всех деталей текущего OpenEnroth. В его расчёте урона остаются TODO о типах/Spirit.

[Таблица защиты состояний реконструкции](https://github.com/OpenEnroth/OpenEnroth/blob/a644b752f05af0cdbb1c5d96d120d6967bd8b790/src/Engine/Conditions.cpp)
проверяет предметы и Protection from Magic отдельно. Прямой SetCondition также не даёт общего class-Lich
запрета. Наличие иммунитета к магии нельзя превращать в обещание универсального condition-иммунитета.

Ограничение исследования: локальные reference/OpenEnroth-git, mmext-scripts и re_mm8/mm8_orig/MM8-Rel.exe
отсутствуют. [Руководство Rizin](../../re_mm8/RIZIN_GUIDE.md) прочитано; бинарную проверку выполнить не удалось.
По правилам проекта публичный OpenEnroth использован лишь после проверки отсутствия локального reference.
Оригинальные MM7/MM8 в этом исследовании не запускались; ни retail, ни GOG, ни GrayFace не помечены как
воспроизведённые. Нативная точность иммунитета от конкретного спецудара наги в MM8 остаётся открытой.

## Краткая матрица правил

| Персонаж/эффект | Что установлено |
|---|---|
| Вампир/Nosferatu MM8: Mind | Задуман иммунитет; в оригинале был дефект, исправленный GrayFace |
| Вампир/Nosferatu MM8: Poison | Врождённой полной защиты Body/Poison нет; отравление допустимо |
| Вампир/Nosferatu MM8: Disease | Общего иммунитета нет; боевой save использует Endurance/Luck |
| Лич MM7/MM8: школы | Ожидаются Mind/Body; это не Light/Dark и не иммунитет ко всем стихиям |
| Лич: Poison/Fear/Insane/Paralyze от спецудара | Участвуют Body/Mind; абсолютный запрет для всех оригинальных путей не подтверждён |
| Лич: Disease/Sleep/Weak | Один Mind/Body-иммунитет не блокирует эти состояния |
| Любая нежить: сценарный эффект объекта | Нужна отдельная проверка пути, который назначает состояние |
| Монстры Lich/Vampire | Отдельные monster definitions; не источник иммунитетов игрового персонажа |

## Подтверждённая реализация OpenYAMM1.20

Ссылки ниже закреплены на исследованной ревизии; пути в рабочем дереве те же.

1. [Party::applyLichState](https://github.com/Plane-PPP/openyamm/blob/07e79ebb5b5b34c45dc1c27070b909cba10c8b7d/game/party/Party.cpp#L1083)
   ставит четыре elemental base≥20, light/dark≥65000 и permanentImmunities.light/dark=true.
   Mind/Body и permanentConditionImmunities не выдаются. setMemberClassName вызывает это при настоящем
   превращении (classOnly=false); applyLichTransformation — тоже. Это соответствует уточнённому пути пользователя.
2. [Начало истории](https://github.com/pjasicek/openyamm/commit/5bace84609e90da75a6c8fac421a4a60722b4ea5):
   такие Light/Dark-флаги уже внесены upstream7мая2026 и присутствуют в официальном1.20.
   Последний порт Minotaur их не добавлял. В tests/ScriptedMapRegressionTests.cpp около5667 старый тест даже
   утверждает light65000/light+dark immunity — тест закрепляет эту ошибочную семантику, а не опровергает её.
3. [Характеристики/урон](https://github.com/Plane-PPP/openyamm/blob/07e79ebb5b5b34c45dc1c27070b909cba10c8b7d/game/gameplay/GameMechanics.cpp#L2548):
   Mind/Body Infinite берутся из соответствующих permanent/magical флагов. Сам по себе className=Lich или
   raceId=Vampire не создаёт их в Standard. Число65000 в другом поле не заменяет правильный флаг.
4. [Боевой путь](https://github.com/Plane-PPP/openyamm/blob/07e79ebb5b5b34c45dc1c27070b909cba10c8b7d/game/gameplay/GameplayCombatController.cpp#L1010):
   monsterSpecialAttackSaveBonus и memberImmuneToMonsterSpecialAttack выбирают нужную школу. Poison всех трёх
   степеней блокируется Body.infinite; Fear/Insane/Paralyze — Mind.infinite. Disease не попадает в этот список.
   Затем applyMonsterSpecialAttack проверяет прочие защиты и вызывает applyMemberCondition.
5. [Party::applyMemberCondition / hasConditionImmunity](https://github.com/Plane-PPP/openyamm/blob/07e79ebb5b5b34c45dc1c27070b909cba10c8b7d/game/party/Party.cpp#L1183)
   проверяют condition-флаги/предметы/эффекты, без общей проверки undead/Mind/Body. EVT-присваивания идут
   непосредственно сюда: EventRuntime.cpp около3381/3680. Это другой путь, чем подтверждённый удар наги.
6. Рождение нового персонажа сбрасывает permanentImmunities/conditionImmunities в GameApplication.cpp:2762.
   Для Standard не найден последующий выдающий Vampire Mind-иммунитет путь. RosterEntry копирует численные
   сопротивления, не иммунитеты класса. Например, Elsbeth имеет Mind10/Body10; Gethric/Infaustus/Artorius —0/0;
   Hevatia/Vetrinus —0/0. Это данные общей MMerge-таблицы проекта, не доказательство vanilla roster.
7. MAW проверяется отдельно: MawClassRules.cpp:85–104 уже содержит расовые Undead→Body+Mind и Vampire→Mind,
   выдаваемые через magicalImmunities в applyClassBonuses. Это не работает как исправление Standard, и нельзя
   повторно суммировать его при будущей работе. Состояния/легендарные предметы MAW имеют дополнительные правила.

### Конкретно наги в исследованной общей таблице

assets_dev/engine/data_tables/monster_data.txt:

| ID | Название | Level | Special |
|---|---|---:|---|
|169|Naga|20|Poison1x2|
|170|Naga Warrior|28|Poison1x3|
|171|Naga Queen|38|Poison2x4|

Это физический удар с отдельным poison-эффектом. shouldApplyMonsterSpecialAttack использует Level×множитель,
с ограничением попытки100%; далее идут иммунитет/save/предметы/Protection from Magic. В базовой таблице попытки
40%,84%,100%; это НЕ итоговые шансы отравления и они меняются с runtime-уровнем/bolster.
У продвинутого лича по нынешнему коду Body.infinite=false, поэтому проверка врождённой защиты не останавливает
этот путь. Вампир также не имеет Body-иммунитета, но для него это соответствует ожидаемому набору школ.

## Зафиксированные дефекты и следующие проверки — без реализации

- UND-001: Lich promotion назначает Light/Dark вместо Mind/Body; наследуется upstream. Исправить правильные
  школы и проверить магический урон, Stats и боевые состояния отдельно. Не выдавать иммунитет к Disease «заодно».
- UND-002: Standard Vampire/Nosferatu не получают заявленный Mind-иммунитет; не путать с допустимым Poison.
- UND-003: заранее созданные личи и старые saves нельзя исправить только новым вызовом promotion;
  происхождение неверных постоянных Light/Dark значений нужно учитывать. Не стирать легальные бонусы предметов
  и событий без явной проверяемой миграции. Ничего в существующих saves сейчас не менялось.

Перед кодом нужно выбрать целевую совместимость: воспроизведение ошибок непатченой игры, исправленные
классовые школы или дополнительно абсолютная защита от condition-спецударов. Последний вариант нельзя
называть автоматически подтверждённым vanilla-правилом: нынешний OpenYAMM Infinite блокирует спецэффект
без броска, тогда как реконструкция MM7 показывает численный save. Нужна точечная проверка оригинального MM8.

Минимальная проверка: обычный некромант, тот же после квеста, нанятый лич; Vampire и Nosferatu; Standard отдельно
от MAW. Проверить чистый Body/Mind урон, Poison1/2/3 и Fear/Insane/Paralyze через реальный удар, Disease отдельно;
затем предметные иммунитеты, Protection from Magic, EVT-эффекты и save/load. Для наги фиксировать реально
используемые Body/Mind, флаги Infinite и источник состояния. Отдельные запреты сна/слабости/болезни/проклятия
не добавлять по одному лишь лору нежити.
