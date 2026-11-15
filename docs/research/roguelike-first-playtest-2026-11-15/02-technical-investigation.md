# Исследование реализации первого прототипа

Дата: 2026-11-15. Только чтение исходников, истории, документов и изображений.
Исполняемый код не изменялся; сборки, игровые запуски, CI, релизы и git-операции записи не выполнялись.

Обозначения: **наблюдение** — сообщение пользователя; **код** — установленный исходниками факт;
**гипотеза** — ещё нужна проверка исполнения; **предложение** — вариант следующей итерации.
Все относительные пути исходников ниже относятся к **R**, если явно не указано иначе.

## 1. Ревизии и источники

**C**, каталог сдачи документов: `/home/plane/projects/openyamm-medusa`, ветка `feature/project-medusa`,
HEAD `b096d52fefd3b36f7f4ff90016729803856fa320`. До работы был только untracked
`docs/research/full-2.3-windows-audit.md`; он сохранён без изменений.

**R**, фактические исходники прототипа: `/home/plane/projects/openyamm-medusa/.roguelike-v23`,
ветка `feature/roguelike-v0`, HEAD **`da1bb1624580ceb7caa3626d637c020727f5aa46`**, 2026-11-14.
`git status --porcelain` R пуст. Не переносить автоматически изменения в C: в нём другой набор кода.

Другие worktrees: `.roguelike` — `d3bc5643` (старый скелет),
`/home/plane/projects/openyamm-minotaur-v2.3` — `4788e9b5` (интеграционная линия, untracked
`INTEGRATION-STATUS.md`, `PREPARATION.md`), `/home/plane/projects/openyamm` — `54efa0e9`.
`git worktree list` подтверждает разные ветки одного checkout-набора; их HEAD нельзя смешивать.

Происхождение R по `git log`: upstream 2.3 `f4a6b8032759a218505532b2600ac56b6d80c7af` →
`530d75a3` интеграция Minotaur/Medusa/импортёра → `d3670e16` скелет → `6c46a840` правила v0 →
`e0e60c57` героиня и сброс choices при load → исправления CI/поставки `560b98aa`, `283871bc`,
`40c73c76`, `d653af68`, `da1bb162`. Последние коммиты преимущественно про упаковку, не исправление двери.
У C последние коммиты относятся к Medusa 0.2/0.2.1/0.2.2; HEAD C не является SHA тестового roguelike.

`/home/plane/projects/openyamm-roguelike-notes/как-играть.md` связывает Windows-пакет
`minotaur-2.3-da1bb1624580-46-1` с R и Actions run `43598461895`, artifact `14115874232`,
размером 9 944 108 882 байт. Это локальная запись поставки, не повторная проверка скачанного ZIP:
бинарник, его хеш и пользовательский лог теста здесь не получены. Нельзя доказывать SHA именно
запущенной пользователем установки только этим документом.

Актуальные источники:

- R `docs/research/roguelike-rules-v0.md`: реализация и исторические результаты Linux headless.
- R `docs/research/minotaur-2.3-integration.md`, `minotaur-2.3-roguelike.md`: происхождение и перенос скелета.
  Последний описывает прежние 8 остановок, а не текущую длину забега.
- Notes: `правила-v0.md`, `правила-v0-приложение.md`, `скелет.md`, `движок.md`, `исследование.md`,
  `статус.md`, `как-играть.md`; решения в `codex-review-2026-11-12/claude-answer.md` и `decisions.md`.
  Исторические планы не заменяют текущий run.yml.
- `/home/plane/projects/project-medusa`, HEAD `2e1244962b7201d16f3c486ea3fa001690f57f27`:
  `README.md`, `docs/DECISIONS.md`, контент `content/engine/rulesets/medusa/`.
  README ещё называет Alpha 0.1, но ниже указывает портреты v2 Alpha 0.2.2 — явная неоднородность документации.
- C `docs/research/full-2.3-windows-audit.md`: исторический аудит `5f44e9f1`.
  Его ограничения состава уже частично исправлены последующими commits R; не выдавать их за нынешние дефекты.

## 2. Что реально реализовано

| Подсистема | Факт и точка проверки |
|---|---|
| Подключение | `game/app/GameSession.cpp::setRuleset`, строки 409–427: run с rules создаёт RoguelikeRules и читает сохранённое состояние |
| Общий бой | `game/rules/roguelike/RoguelikeRules.cpp::RoguelikeRules`, 540–560: заменены Experience и Monsters, остальные интерфейсы взяты из standardGameRules этой сборки. Отдельной полной реализации Minotaur-боя здесь нет |
| Маршрут | `assets_dev/engine/rulesets/roguelike/run.yml`: route [1,2,3,3], 8 кандидатов; `game/content/DungeonRun.cpp::dungeonRunStop`: seeded выбор по tier без повторов |
| Старт | run.yml: 500 золота, 2 еды. `RoguelikeRules.cpp::Experience::prepareNewParty` устанавливает стартовые ресурсы |
| Зачистка | `RoguelikeRules.cpp::rewardMapProgress`, 414–450; `runClearFraction`: доля HP исходных монстров, порог строго больше 50%, не половина количества существ |
| Уровни | perStop [3,3,2,2], +4 уровня каждому, десять событий/герой, итог L41; последнее событие этапа при вызове стража, не при его смерти |
| Страж | `GameApplicationDungeonRun.cpp::placeDungeonRunGuardian`, 731–790: strongestMonsterId, у входа, HP×4; если spawn не удался, выход открывается без боя |
| Карточки | `RoguelikeRules.cpp::dealRunCards`, 178–275; `applyRunCard`: одна ступень N1/E4/M7/GM10 в рамках класса либо перк; известный навык + перк при наличии, остаток из общего пула |
| Книги | `GameApplicationDungeonRun.cpp::answerDungeonRunChoice`, 976–1017; `deliverDungeonRunBooks`, 792–861: 3/2 книги, исключение известных/имеющихся/должных, очередь при полном инвентаре |
| Продвижение | `promoteDungeonRunParty`, 686–729, и `updateDungeonRunRules`, 1056–1059: следующий класс после второго стража |
| Camp | `holdMapMoveForDungeonRunCamp`, 643–682; `openDungeonRunCamp`, 618–641: пять домов и продолжение; после четвёртого Camp нет |
| Восстановление | `restoreDungeonRunParty`, 571–596: HP/SP живым, без еды; состояния, мёртвые и сломанные вещи сохраняются; при redirect восстановление повторяется |
| Часы Camp | holdMapMove… переводит время на 09:00, если вне [09:00,16:00); штатные часы домов продолжают действовать |
| Конец | `redirectMapMoveForDungeonRun`, 122–171: после последнего stop — pendingReturnToMainMenu и Victory |
| Medusa | run.yml unrestrictedCharacters [79]: карточки без class caps; класс создания всё ещё задаёт HP/SP и стартовые навыки; отдельный новый класс не создан |
| HUD | `RoguelikeRules.cpp::Monsters`, 503–520: заполнение до порога, завершение после смерти стража. Сохранить визуально удачную полосу |

Текущие перки run.yml: шесть характеристик +15, HP+30, SP+25, AC+8, сопротивления+10,
melee/bow damage+4 (обычно 3 ранга), Explosive arrows (один ранг, нужен Bow).
Пул навыков включает оружие, броню, служебные навыки и семь школ; Light/Dark не включены.
Случайность детерминирована seed, позицией героя и числом взятых карт; reload не даёт бесплатного reroll.

## 3. P0: точная цепочка выхода и установленный дефект

### 3.1 Штатный путь и перехват

1. Сгенерированный `assets_dev/worlds/mm6/events/maps/6d01.lua`, событие 51, строки 384–386:
   MoveToMap в oute3.odm. **Не редактировать этот Lua** для исправления общего перехода.
2. `game/gameplay/GameplayDialogController.cpp`, MapTransitionConfirm, 3346–3427:
   копирует transitionMapMove в pendingMapMove, выставляет shouldCloseActiveDialog=true.
3. `game/gameplay/GameplayScreenRuntime.cpp::executeActiveDialogAction`, 2514–2560:
   при pendingMapMove не закрывает окно немедленно; сохраняет callback в m_pendingMapMoveDialogClose,
   передаёт move в session, очищает runtime.pendingMapMove. Это намеренно сохраняет фон ухода до загрузки.
4. `game/app/GameApplication.cpp::processPendingMapMove`, 14850–14925:
   получает move, synchronizeSessionFromRuntime, проверяет sealed exit, затем **до autosave/закрытия UI**
   вызывает holdMapMoveForDungeonRunCamp. При true сразу возвращает false.
5. `GameApplicationDungeonRun.cpp::holdMapMoveForDungeonRunCamp`, 643–682:
   для убитого стража и межкарточного перехода восстанавливает HP/SP, меняет время при необходимости,
   сохраняет move в m_dungeonRunCampMove и выставляет m_dungeonRunCampOpen=true.
   Вызывает openDungeonRunCamp(), **игнорирует результат**, возвращает true.
6. `openDungeonRunChoice`, 863–897: отклоняет открытие при activeEventDialog.isActive либо pendingDialogueContext.
   Штатный exit-dialog из шага 3 ещё активен, следовательно Camp здесь не откроется.
7. Нормальное закрытие лежит позже: `GameApplication.cpp::closeTransientGameplayUiForMapMove`,
   15154–15181, вызывает `GameplayScreenRuntime::closeActiveDialogForMapMove`, 587–600;
   processPendingMapMove достигает его на 14952/14992 лишь после раннего возврата Camp.

**Подтверждено чтением:** нарушен контракт передачи владения диалогом между штатным переходом и Camp;
move уже потреблён, состояние Camp и ресурсы уже изменены, но предыдущий UI не закрыт.
Это конкретный дефект, а не общее предположение «мышь не работает».

После ручного Close `updateDungeonRunRules`, 1062–1087, видит !dialogBusy и CampOpen,
повторно вызывает openDungeonRunCamp — это соответствует наблюдению OK→Close→Camp.
В момент первичного OK entered ещё не обязан меняться: redirect с setDungeonRunEntered находится позже,
но HP/SP, время и внутреннее состояние Camp уже меняются. Ответ на вопрос о невидимом изменении состояния — да.

### 3.2 Что этим НЕ доказано

- Вся Windows-последовательность, включая немедленный Close→«обычный выход», здесь не воспроизведена.
  Нужны trace action.kind, pendingMapMove/session move, activeEventDialog, pendingDialogueContext,
  CampOpen/CampLeaving/CampMove и entered до/после каждого клика. Уточнить фактическую карту после Close.
- Сам по себе этот дефект объясняет застревание штатного диалога и отсроченный Camp;
  ещё не доказывает, почему пункт продолжения **уже открытого** Camp не сработал у пользователя.
- Неверный seed/отсутствующая карта/ошибка загрузки — возможные дополнительные причины, доказательств нет.
  Код маршрута умеет выдавать второй stop; Linux-сценарий исторически проходил его.

### 3.3 Состояние, карты и пробел тестов

`answerDungeonRunChoice`, 932–966: выбор магазина открывает HouseService; любой положительный ответ выше
числа магазинов трактуется как продолжение, возвращает сохранённый move и ставит CampLeaving=true.
`holdMapMove…` один раз пропускает его; далее autosave и redirect загружают следующий stop.
Состояние выбора и CampMove — оперативная память, не весь интерфейсный процесс сериализован.
`redirectMapMoveForDungeonRun`, 160–167 увеличивает entered и сбрасывает guardian **до успешной загрузки**.
При ошибке восстановления карты processPendingMapMove (около 15000–15035) не обеспечивает симметричный
откат entered/guardian. Это отдельный риск пропуска этапа, отмеченный и прежним handoff.

`tests/scenarios/rulesets/roguelike_run.yml`, 23–24, 76–90, 118–119 использует `run leave` и `run pick`.
`registerDungeonRunConsoleCommand`, GameApplicationDungeonRun.cpp 317–381:
run leave обходит событие двери и его confirmation; run pick вручную очищает диалог перед обработкой.
Поэтому 117 исторически успешных шагов не проверяли критическую комбинацию штатной двери и отложенного close.
Успешный клик карточки в этом сценарии не является успешным кликом OK у двери.

### 3.4 Минимальное исправление для следующей задачи

Перед перехватом в Camp корректно завершать диалог ухода через имеющийся API закрытия для map move;
затем ставить новый ruleset-диалог. Не закрывать произвольный новый диалог старым отложенным callback.
Обрабатывать отказ openDungeonRunCamp явно, сохраняя восстанавливаемое состояние; не терять move.
Уменьшить раздвоение session/runtime pending move. Продвижение entered/сброс guardian делать при подтверждённом
переходе либо с явным rollback на load failure. Нужен review изменения state/save-пути.
Отдельно реализовать согласованный Next Dungeon / Camp / Rest; это изменение поведения, не часть доказательства бага.

Проверки: настоящая event 51→OK без debug bypass; Close без OK; Escape; повторный клик;
Camp→магазин→Close→Camp→Next; autosave on/off; save/load перед дверью и после Camp;
ошибка загрузки следующей карты без изменения stop; последний выход и defeat; обычный MM6 exit без roguelike.

## 4. Подсказки и Add Portrait

`game/ui/screens/NewGameScreenPartySheet.cpp::drawCreationAttributes`, 655–726 уже берёт
StatInspectEntry.description и добавляет расовую строку. `drawCreationHelp`, 1464–1525 выводит **и** body,
**и** extra lines; утверждать, что тело забыто в renderer, неверно.
Та же логика чтения и вывода присутствует в локальном upstream `f4a6b803` (проверено git show).

Gameplay: `game/ui/GameplayHudOverlaySupport.cpp`, 804–830 дополнительно вызывает
rules.skills.statTooltip; `game/rules/SkillRules.cpp::statTooltip`, 153 и далее вызывает
GameMechanics::buildPrimaryStatInspectSupplement и вычисляет фактические эффекты. Создание этот шаг пропускает.
Это подтверждённое расхождение путей, но не полное объяснение исчезновения базового описания.
Гипотезы: другой поставленный stats.txt/перевод, маленькое содержимое description, различие бинарника,
либо пользователь навёлся на иной элемент. Проверить текст getStat и asset pack/language на реальной установке.

Предложение: переиспользовать существующий formatter с корректно построенным preview Character/Party и
creation rules, либо общий чистый formatter описания; не дублировать боевые формулы в меню.
Показывать игровой смысл плюс стоимость расы. Проверить все семь статов, RU/EN, Standard/Roguelike/MAW.
Не заявлять воспроизведённый upstream-баг до чистой установки; [04](04-upstream-bug-report.md).

`drawPortraitPicker`, 1541–1640: mockup X=70,Y=40,W=1460,H=820. World filters на Y+38 высотой34
занимают [Y+38,Y+72]; Add Portrait на Y+64 высотой26 занимает [Y+64,Y+90].
При полном наборе миров worldX начинается X+676, ширины с gap8: 132/104/134/108/112;
последний заканчивается X+1298. Кнопка при минимальной ширине170 начинается X+1246 и заканчивается X+1416.
**Есть пересечение 52×8 mockup px** с последним фильтром; более длинная локализация расширяет кнопку влево.
Подзаголовок Y+74 высотой16, viewport Y+92: для добавочной строки места не зарезервировано.

Предложение: отдельный ряд для import, выделить высоту подзаголовку и сдвинуть viewport с уменьшением высоты;
либо разделить верхние controls по измеренным ширинам с переносом. Сохранить sheetPx/labelButtonWidth conversion.
Проверить 4:3, 16:9, ultrawide, RU/EN, DPI и все пять фильтров; не лечить только ручным X-сдвигом.

## 5. Гильдии Camp

`game/gameplay/HouseInteraction.cpp::magicGuildMembershipType`, 381–430, requiredGuildMembershipType,
hasGuildMembership 433–475; проверки на 1081, 1119–1121, 1671–1672 закрывают услуги без членства.
`GameplayDialogController.cpp`, 1910–1948: обычное членство хранится в party event var 0x8000|guildType
и runtime var 0x80000000|guildType. Camp открывает штатный дом, но эти права не даёт.

Минимальный вариант — при старте roguelike явно выдавать членство только нужных типов штатными party vars,
с сохранением в run save и миграцией уже начатого забега. Это меньше нового UI, но права останутся во всём забеге.
Если доступ должен действовать строго внутри Camp — общий policy-hook доступа к услугам, проверяющий активный
ruleset и allowlist домов Camp; применить к показу и исполнению услуги, не только к кнопке.
Не добавлять безусловный true в hasGuildMembership и не менять обычные NPC-вступления.
Выбор варианта зависит от согласованного охвата услуг: книги, обучение, цены, пределы мастерства.

## 6. Экономика и боссовые предметы

В `RoguelikeRules.cpp::rewardMapProgress`, 414–450 сейчас нет особой награды золотом/редкостью за guardian;
его тип наследует стандартный loot. `game/gameplay/CorpseLootRuntime.cpp`, 285–295 кидает goldDiceRolls
раз по [1,goldDiceSides]; доход зависит от выбранных карт, обыска, сундуков и продаж, а не только этапа.
`LootContainerRuntime.cpp::generatedGoldAmount`, 27–39: случайные кучки T1=50–100,T2=100–200,
T3=200–500,T4=500–1000,T5=1000–2000,T6=2000–5000. Это диапазоны одной кучки, не гарантированный доход карты.

`assets_dev/engine/data_tables/house_data.txt`, строки 24/64/99/191/1216: ранние дома имеют multiplier1.5;
строки30/31: Weapons поздних Camp multiplier2.5, ассортимент L3 против раннего L2.
`game/items/PriceCalculator.cpp::itemBuyingPrice`, 192–205: max(realValue, merchantDiscount(realValue×multiplier)).
`itemSellingPrice`, 216–246: value/(multiplier+2)+merchant term, ограничено resale cap, broken=1.
Цены включают зачарования (`itemValue` → ItemEnchantRuntime), единой цены «меча» нет.

**Предложение тестового бюджета, не утверждённый баланс:**

| Босс | Бонус золота партии | Расчёт целевой покупки без Merchant | Редкость по пожеланию |
|---|---:|---|---|
| 1 | 1500 | 2 покупки с value≈500 ×1.5 | Artifact |
| 2 | 3000 | 2 покупки value≈600 ×2.5 | Artifact |
| 3 | 5000 | 2 покупки value≈1000 ×2.5 | Relic |
| 4 | 7500 условно | терминальная награда; тратить сейчас негде | Relic |

Цены value500/600/1000 — **бюджетные ориентиры**, не измеренные средние stock и не доказательство
покупательной способности конкретного save. Сумма 17 000 бонуса на партию плюс старт500 и обычная добыча.
Перед балансировкой записать на нескольких seed gold до/после dungeon и Camp, доход продаж,
реальные предложения/покупки/остаток. Если штатная добыча уже закрывает бюджет — уменьшать бонус,
а не молча повышать цены. У четвёртой награды сначала определить назначение; метапрогрессия сейчас отсутствует.

`game/tables/ItemTable.h::ItemRarity` различает Artifact и Relic;
`ItemTable.cpp` 292–299 разбирает соответствующие строки. Но `ItemRuntime.cpp::isArtifact`, 129–132
объединяет обе категории, поэтому его одного недостаточно для наград 1–2 против 3–4.
`ItemGenerator.cpp::chooseRandomRareItem`, 77–110: исключает NoRandom, zero id, уже найденные;
`ItemRuntime.cpp::isUniquelyGeneratedRareItem`, 124–126 требует Artifact/Relic с value>0.
`shopPoolAllows`, 56–65 учитывает world pool; `shouldTryGenerateRareItem`, 69–74 запрещает shop rolls
и требует treasureLevel≥6. Обычный rare roll не гарантирует нужную точную редкость.

Для гарантии нужен явный детерминированный выбор по точной rarity с теми же ограничениями,
нормальное создание item runtime, учёт найденных и полного инвентаря. Решить world pool, пригодность классу,
повторы, NoRandom/quest exclusions, выдачу при обыске/смерти. Exactly-once reward marker обязателен при save/load;
не добавлять предмет повторно при обнаружении GuardianSlain каждый кадр. Изменение inventory требует review.
Ранний Artifact может перекрыть несколько будущих покупок и иметь сильный эффект; варианты контроля для обсуждения:
пул умеренных ранних артефактов, случайность с гарантией пригодности хотя бы одному герою, либо принять высокую
дисперсию roguelike. Не вводить автоматическое ослабление артефактов без согласования.

## 7. Guardian и четыре карточки

Имя стража сейчас не меняется: placeDungeonRunGuardian передаёт monsterId и healthMultiplier, без label.
`game/ui/GameplayPartyOverlayRenderer.cpp`, 12078–12081 выводит actor inspect title как обычную строку,
используя displayNameOverride либо localizedActorName. **Поддержка inline цветовой разметки этим путём
не установлена**; не вклеивать ANSI/HTML/legacy escapes в monster table.
Предложение: слой представления конкретного actor (по guardianActor текущего run) рисует цветной span Guardian
и обычное имя отдельно. Проверить также hover/status имя, сохранение/загрузку, отсутствие окраски других
Goblin King. Цвет/длина/локализация обсуждаются; не менять общий MonsterTable entry данного вида.

`dealRunCards`, 269: cards.size()<3 — точка перехода на4; takeRandom удаляет выбранную запись из пула,
поэтому остаётся отсутствие повторов одного кандидата при уникальных входных skills/perks.
Сохранять первые две квоты (known/perk), затем выбирать ещё два из rest; если кандидатов меньше4,
выдавать доступное количество. Проверить уникальность data ids, class caps, исчерпанный пул.
UI openDungeonRunChoice принимает vector answers, жёсткого лимита3 здесь нет; Camp уже использует6.
Нужна проверка четырёх длинных подписей на реальном экране и keyboard/mouse navigation.
Число повышений и выбор одной карты не меняются; вероятность найти нужную комбинацию возрастает,
так что менять одновременно частоту повышений без измерений не следует.

Важный save-риск: perk level хранится как **0xF100 + индекс в YAML**, а применённая числовая прибавка
остаётся в полях героя (`RoguelikeRules.h`, 30–34; applyRunCard).
Удаление/перестановка перков переинтерпретирует старые saves. Нужны stable ids/версия rules и явная миграция,
либо явно объявленный новый забег; не объявлять совместимость только из-за неизменного бинарного формата.

## 8. Направления развития карточек — варианты для обсуждения

1. **Мастерство как основа, небольшое число меняющих действие перков.** Сохранить N/E/M/GM,
   заменить основные статы эффектами условного боя (по типу уже существующих Explosive arrows).
   Плюсы: узнаваемый MM, мало новых подсистем, быстрый первый проход. Риски: редкие комбинации не успеют
   раскрыться за десять выборов; class caps до promotion могут ограничивать цель. Вопросы: сколько сильных
   перков к финалу, нужны ли prerequisites и гарантированный выбор для стиля?
2. **Мастерство плюс теги синергии.** Перки взаимодействуют через небольшие теги оружия/школы/контроля,
   предложения случайны, но часть доступна нескольким стилям. Плюсы: неожиданные сочетания и разный бой.
   Риски: экспоненциальные взаимодействия, proc loops, AoE и восстановление ломают баланс.
   Вопросы: пределы срабатываний, общий или личный эффект, приемлемый уровень дисперсии?
3. **Поздние мощные развилки.** Обычные карты ведут к сильным возможностям, доступным на поздних этапах
   или высоком мастерстве. Плюсы: выраженная кульминация четырёх dungeon; понятный рост силы.
   Риски: случайность может лишить подходящего финала, ранняя игра остаётся скучной; этапные гарантии
   меняют нынешний алгоритм предложения. Вопросы: можно ли гарантировать сильную карту, допустимы ли
   преобразования ранее выбранных умений и насколько строгими будут требования?

Это не утверждённый каталог карт. Числовые сопротивления сохранить во всех вариантах.
Не переносить MAW целиком: использовать уже подходящие shared hooks после аудита конкретных эффектов.

## 9. Baseline, обновления, совместимость

Первый прототип уже committed в R; «сохранить текущую версию» не требует пустого нового commit.
Предложение: после разрешения отдельно commit только отчёт в C; baseline самого прототипа указывает на R SHA,
а не на commit этих документов. Отдельно сохранить original ZIP/hash, CI metadata/index/content_id,
configure/release version, receipts Medusa, тестовые saves и Windows logs. Код SHA недостаточен для
восстановления внешнего контента Medusa и точного пакета. Annotated tag вроде
`roguelike-v0-playtest1` — только после отдельного разрешения, без автоматического upstream vX.Y publish.
Naming: «Roguelike v0 / first-playtest prototype on OpenYAMM 2.3, progression blocked after dungeon1»;
следующие версии alpha/iteration, не «полностью играбельная 2.3».

Штатный updater уже существует:

- `packaging/RELEASE.md`, разделы What the workflow builds / One-time setup: CI собирает, подписывает,
  публикует приложения и паки; manifests последними, reuse неизменных пакетов. Не повторять старые пункты
  UPDATER_PLAN про Flatpak/полные ZIP как текущее устройство: верхние revisions их отменяют.
- `game/app/GameUpdater.cpp::InstallationUpdater`, 31–175 и `GameUpdater::create`, 355–374:
  check/download/activate в игре, версия установки, языки, keep_modified_packs отключает обновление.
- `engine/update/AppUpdater.h::AppUpdater`: baseUrl, транспорт, trusted keys;
  `ReleaseManifest.cpp::releaseManifestPublicKeys`, 307–314 и verifyManifestSignature, 316–362:
  Ed25519, известные slots. Смена только URL недостаточна: собственный канал должен доверять своим ключам.
- `game/launcher/`, `engine/update/`, `game/app/ContentSetup.*`: versioned app/current,
  подготовка загрузок, запуск/rollback и первоначальная установка контента — переиспользовать.

Мод сейчас имеет иной канал: `packaging/minotaur/README.md`, `Apply-MinotaurUpdate.ps1`,
`tools/minotaur_updates.py`, `.github/workflows/minotaur-windows.yml`.
README прямо запрещает включать stock online updater мода. Offline installer использует versioned layout,
проверку базы/хешей, staging, app/current/previous и сохраняет пользовательские данные.
Workflow 114–136 отделяет full от update/both; 545–556 создаёт patch либо новую baseline.
Текущий `packaging/minotaur/latest-baseline.json` всё ещё ссылается на
`minotaur-1.21-c9949bbc7301-28-1`, **не** на новый full da1bb. Это не доказательство готовности нового патча.

Предложение следующего этапа: собственный подписанный mod channel поверх штатного publisher/AppUpdater,
с собственным URL и доверенными ключами, тем же индексом полного desktop-состава MM6–MM9/languages,
rulesets/importer/Medusa. Сначала staging-проверка, затем bootstrap-переход для существующей full установки.
Не подключать upstream channel: он может заменить mod binary/engine packs. Не создавать новый downloader.
Без URL/хостинга/ключей online пока не готов. Промежуточный вариант — штатный launcher с явным выбором
локального подписанного пакета, переиспользующий проверки; это отдельная небольшая разработка UX,
а не разрешение запускать PowerShell автоматически без проверки. Пользователь не должен вручную
вызывать applyminotaurupdate после каждого выпуска.

`game/maps/SaveGame.cpp`: SaveVersion180/Upstream179 (26–27), saveSchemaVersion (313 и далее),
старый mod155→upstream154. Roguelike использует named globals и Character eventVariables;
`RoguelikeRules.h::RoguelikeRunState`, `readRunState/writeRunState` 140–160 .cpp;
очередь книг — run.books/run.book.N. Неизменный контейнер saves не гарантирует семантическую совместимость.
При будущем upstream save bump нужны mapping и frozen fixture предыдущего production формата.
Смена perk ids, route, наград, Guardian actor index требует явной политики миграции/сброса run,
при этом обычные сохранения и кампании не должны сбрасываться.

Оставлять rules/content/награды/доступ Camp в roguelike package/policy, shared UI и map hooks общими.
Не добавлять отдельную реализацию боя в IndoorGameView; не править общие таблицы для режима;
не смешивать Medusa campaign старт Nighon с доступной в run героиней79.

Документация правил RU/EN следующей итерации: одинаковые разделы «статус/ревизия, старт/партия,
маршрут/зачистка, страж/награда, уровни/карты/мастерство, книги, Camp/Rest/цены, конец,
сохранения/миграции, известные ограничения». Каждому правилу — источник YAML/функция, признак
implemented/tested/proposed, тестовые числа отдельно от решений. Обновлять обе версии вместе и
`python3 tools/check_doc_i18n.py` перед будущим commit. Этот пакет исследования не подменяет основной RU/EN guide.

## 10. Границы проверки

Выполнены read-only git status/branch/rev-parse/log/worktree list/show, rg/sed/nl/cat и просмотр PNG.
Существующие материалы не менялись. Тяжёлые тесты и реальная Windows-игра не запускались.
Исторические «15 run cases / 117 steps / regressions» взяты из R roguelike-rules-v0.md,
не заявляются новым выполненным прогоном. Два заявленных bug screenshots получены при продолжении задачи и просмотрены (см.01).
По-прежнему нет Windows log, save перед выходом и хеша реально запущенного пакета. Полная upstream runtime-проверка подсказок отсутствует.

Проверки подготовленных документов:

- `python3 tools/check_doc_i18n.py` — exit0, 8 pairs, 0 errors, 8 warnings о существующих расхождениях
  ссылок/примеров в прежних RU/EN парах. Эти четыре research-файла не являются новыми основными bilingual guides.
- Одноразовая проверка через `python3 -` (pathlib/re): все4 файла, баланс code fences,
  относительные Markdown-ссылки и trailing whitespace — 0 ошибок. Дополнительный файл проверки не создавался.
- Финальный `git status --short`: добавлена только папка этих четырёх документов;
  прежний untracked full-2.3-windows-audit.md остаётся. Ничего не staged/committed.

Дальнейшие решения: политика Camp/Rest, наград/пула, перков, миграции run и собственного update channel
остаются за пользователем; конкретный список для обсуждения находится в03. Не проверены Windows/GPU,
реальные переходы и экономика; разработку и её тесты выполняет следующая сессия.


## 11. Дополнение после загрузки скриншотов и сохранение в проекте Minotaur

По просьбе пользователя весь пакет из четырёх документов сохранён в
`/home/plane/projects/openyamm-minotaur-v2.3/docs/research/roguelike-first-playtest-2026-11-15/`.
Это дополненная копия; прежние документы в openyamm-medusa не удалены и не перезаписаны.
Каталог назначения имеет HEAD `4788e9b51049b27ad8280c3a8860af7a4b4f2b52`, а исследованные
исходники v0 по-прежнему находятся в R на `da1bb1624580ceb7caa3626d637c020727f5aa46`.
Сохранение отчёта в другой worktree не переносит туда реализацию и не меняет её ревизию.
До сохранения там были untracked `INTEGRATION-STATUS.md` и `PREPARATION.md`; они не изменялись.

Оба PNG находятся в `/home/plane/projects/project-medusa/test_screenshots/`, точные имена в01.
Они визуально подтверждают состав exit UI и overlap Add Portrait; это усиливает визуальные доказательства,
но не превращает анализ исходников в выполненный runtime-тест. Видимые пять героев требуют отдельной
проверки поддержки размера партии: обещанный ориентир40 выборов относится к четырём героям.
Скриншот сам по себе не устанавливает текущий ruleset или причину наличия пятого героя.

Проверки продолжения: просмотр двух PNG, `git status --short` в обоих проектах,
`git -C /home/plane/projects/openyamm-minotaur-v2.3 rev-parse HEAD`, проверка Markdown-ссылок,
баланса fences и отсутствия trailing whitespace. Сборки и игровые тесты не запускались;
commit/tag/push/release не выполнялись. Добавлена только папка отчёта.
