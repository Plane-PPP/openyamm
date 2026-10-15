# Личные знания разновидностей и бестиарий

> **Deferred по ТЗ Lethal Strike v0.4, 2026-10-15.** Существующее поведение Identify Monster
> не меняется. Knowledge, новые caps/формулы/боевые бонусы, GM magic и бестиарий не реализуются
> в этой версии и не являются зависимостями Lethal Strike. Ниже сохранён исследовательский проект.


Уточнение 2026-10-15: [финальная ревизия](../research/final-review.md) отделяет
198 исходных descriptor records от 184 разновидностей с кампанийными свидетельствами,
уточняет caps/UI, корреляции и вопросы визуальной классификации. Прежние расчёты по 198
сохранены как инвентарный baseline; они не являются статистикой частоты встреч.


Статус: **Research / Proposed**. Назначение: персистентное личное знание A/B/C без замены
существующего Identify Monster. Этапы [ROADMAP](../ROADMAP.md#этапы-разработки): R3, R7.
Знание и бестиарий пока не реализованы; сохранения и UI не изменялись.

## Подтверждённые структуры

[Character в Party.h](../../game/party/Party.h) хранит class/race, rosterId, skills и прочие данные;
[Party.cpp](../../game/party/Party.cpp) переводит целого Character между активной партией и
`AdventurersInnMember`. `rosterId` может быть 0, поэтому карта только по rosterId ненадёжна.
[SaveGame.cpp](../../game/maps/SaveGame.cpp) использует бинарный magic OYSAVE1,
**SaveVersion=84**, version-gated readers и отдельно write/read Character.
Сериализуются и активные members, и adventurersInnMembers; latter сериализует вложенный Character.
Reader **отклоняет version > SaveVersion**: старый executable не сможет читать новую версию автоматически.
`namedGlobalVars` уже сохраняются, но это глобальные integer-пары, не личный registry.

В [MonsterTable](../../game/tables/MonsterTable.h) используются числовые ID, Picture/InternalName
и lookup maps; действующего immutable UUID для каждого шаблона нет.
Инвентаризация подтверждает 198 исходных MM8 ID, однако переупорядочивание merged-таблицы
или переиспользование номера может менять смысл. Архитектурное требование AGENTS.md — canonical
namespaced IDs с raw-ID aliases, а не признание любого текущего номера вечной идентичностью.

## Предложенная модель знания

Добавить к Character набор canonical monster-variant keys. Для текущего MM8 начальная mapping:
`mm8:monster:70`, `mm8:monster:71`, `mm8:monster:72` — три отдельных immutable registry keys.
Число здесь используется лишь при первичной выдаче ключа, не генерируется заново из row ID после
обновления таблиц. Хранить mapping key→current ID и aliases; при переезде row ключ сохраняется.
Разные display names одного рода и одинаковые имена Peasant не объединять. Другие миры/custom
имеют собственные namespaces и owner package. Предложенные CSV keys ещё не действующий registry.

Знание находится **в Character**, а не в UI, runtime actor, party slot или party best skill.
При копировании в Inn сохраняется вместе с Character, при возвращении доступно вновь.
Данные о жизни конкретного actor (HP, уникальное имя, control effects) не сохраняются как знание
шаблона. Уникальный босс может использовать отдельный metadata key при отдельном решении;
одного уникального display name недостаточно для такой идентичности.

Намеренное действие: игрок открывает прежний inspect, выбирает исследующего персонажа и
«Изучить». Старое RMB speech/просмотр продолжают работать без записи знания.
Проверка из [Identify Monster](identify-monster.md) либо добавляет ключ, либо сообщает неуспех
без записи. Already-known — no-op; знание не тратится, не теряется при выходе из партии.
Первый успех показывает ровно один раз:

- `Monster knowledge acquired!`
- `Новый вид монстра изучен!`

Сложность и сила могут отображаться до действия, поскольку проверка детерминированна.
Отсутствие навыка исключает study. Временные enchantments, canAct и эффективный MonsterLevel
при bolster требуют решения; эти правила нельзя выводить из старой речевой функции.

## Сохранения и миграция

| Вариант | Преимущества | Риски / вывод |
| --- | --- | --- |
| Поле knowledge в Character + version-gated reader | Естественный lifecycle, Inn/save/load/копии | Рекомендуется явный bump SaveVersion и пустое значение для старых saves |
| Расширяемый length-delimited блок | Удобен для будущих модов и неизвестных ключей | Текущий формат не имеет готовой гарантии пропуска такого поля; нужен отдельный форматный контракт |
| namedGlobalVars | Уже сериализуется, integer flags | Нет надёжного ключа для rosterId0; слабая типизация/коллизии; не основной storage |
| Sidecar рядом с save | Не меняет бинарный формат | Save As, перенос, облачная синхронизация, atomicity и rollback легко теряют/подмешивают знания; не рекомендуется |
| QBits партии | Существующий persistent механизм | Глобальные, не личные; 198×число персонажей и namespace collision; не подходит |

Рекомендуемая совместимость: **новый executable читает старые saves**, инициализируя knowledge
пустым; прежние навыки/HP/предметы сохраняются. Это не обещание двусторонней совместимости:
старый executable отклонит будущую SaveVersion. Не использовать padding и не менять существующие
enum ordinals. Не выдавать знания о всех ранее убитых/увиденных монстрах: такой истории нет и
это противоречит намеренному изучению. Старый Death Strike отсутствует; S автоматически не выдаётся.
Не исправлять class caps через миграцию старых навыков без отдельного решения пользователя.

Registry unknown keys сохранять round-trip как непрозрачные ключи, но не применять отсутствующие
шаблоны. При удалении content package знания не теряются. Для merged-ID изменений необходима
явная mapping migration; локализованное имя не используется для сопоставления.

В будущем интеграционные проверки: old v84 load→empty knowledge, save/load round-trip,
dismiss/recruit original/new/rosterId0 Character, разные A/B/C, unknown package keys,
повторный study, смена party source, save во время Shrapmetal и DoT.
**Текущий этап не пишет тестовый save и не меняет сериализатор.**

## Минимальный UI и дальнейшее расширение

Текущие точки:
[GameplayUiController::ActorInspectOverlayState](../../game/ui/GameplayUiController.h),
[GameplayPartyOverlayRenderer::renderActorInspectOverlay](../../game/ui/GameplayPartyOverlayRenderer.cpp),
[IndoorGameView](../../game/indoor/IndoorGameView.cpp)/[OutdoorGameView](../../game/outdoor/OutdoorGameView.cpp)
и общий overlay input. Существующая геометрия/шрифты/preview/layout сохраняются.

Первый релиз: компактная строка «Изучена: персонаж / не изучена», явное действие Study для
выбранного персонажа и строка применимых новых bonuses/источника. Имя разновидности берётся
из template key, текущий уникальный display name сохраняется отдельно. Не добавлять длинный
лор в маленький RMB popup; при нехватке места отдельная expandable панель. Desktop held-RMB
не должен закрывать popup до выбора Study: понадобится проверяемый закреплённый режим/input
action; touch Inspect должен вести к тому же shared controller. Новый UI ресурс только в следующем этапе.

Дальше: отдельная страница бестиария (семейства/A-B-C/персонаж/поиск), известные силы,
сопротивления, история и рекомендации. Старый inspect остаётся доступен независимо от нового
журнала. Исторические тексты — отдельный необязательный R7; боевые навыки могут выйти без них.

## Лор: источники, границы и публикация

Исследованы доступные категории источников и первичные издания; полный текстовый корпус не извлечён.
Длинные PDF руководств частично не доступны через web extractor, поэтому неподтверждённые
семейные биографии из них не заявляются прочитанными или проверенными.

| Источник | Что использовать | Что не переносить автоматически |
| --- | --- | --- |
| [MM6 manual](https://www.mocagh.org/nwc/mm6-manual.pdf) | Контекст мира/правила; исходное издание | MM6 характеристики и чужие monster descriptions как MM8 данные |
| [MM7 manual](https://draketungsten.org/arcade/pcmanuals/mightandmagic/7-manual.pdf) | Контекст/термины; сопоставление prototype | Ступени Identify Monster вместо текущего OpenYAMM behavior |
| [MM8 manual](https://www.mocagh.org/nwc/mm8-manual.pdf) | Канон Jadame, расы/сюжет | Непроверенные утверждения о конкретном семействе |
| [Prima MM8 guide](https://mocagh.org/nwc/mm8-hintbook.pdf) | Кандидат на справочник всех creatures | Дословные страницы и лицензия на перепубликацию |
| [Официальный HoMM3 manual в Steam](https://shared.akamai.steamstatic.com/store_item_assets/steam/apps/297000/manuals/BONUS_Heroes_of_Might_and_Magic_III_HDEdition_OldManual1999_EN.pdf) | Тематические параллели семейств/городов | Army abilities, иммунитеты, death stare и combat stats в MM8 |

Локальные текстовые ресурсы: engine `english/history.txt`, `mm7_history.txt`, `npc_topic_text.txt`,
`quests.txt`, `Global.txt`, event STR тексты и merged NPC диалоги. Они содержат сюжет и названия,
но отдельного полного monster-lore/bestiary registry в доступном наборе не найдено.
Документированное происхождение хранить с источником/страницей и game/world тегом.
В [семейной матрице](../research/monster-lore.md) сейчас есть **фактические MM8 атаки, AI,
иммунитеты и производные рекомендации для всех 66 групп**, а происхождение отмечено неподтверждённым.
Нельзя подменять эти пробелы придуманными легендами. Отдельный verified lore можно добавить позже.

Юридическая оценка ограничена опубликованными notices: [MM6 manual](https://www.mocagh.org/nwc/mm6-manual.pdf)
содержит запрет воспроизведения без согласия; [HoMM3 HD EULA](https://store.steampowered.com/eula/297000_eula_1)
ограничивает воспроизведение/производные работы. Бесплатность мода сама по себе не даёт лицензию
на тексты/картинки. HD EULA не считается автоматически лицензией оригинальной MM8 установки.
Право на включение конкретных ресурсов и применимые правила нужно подтвердить отдельно перед
публикацией; ссылки на руководства не равны разрешению копировать их содержание.

Рекомендуемый формат: **собственные короткие описания** на основе проверенных таблиц,
с источниками для лора; не копировать руководства/wiki/иллюстрации и не включать полные оригинальные
STR как новый публичный бестиарий. Механическую рекомендацию строить по конкретному варианту:
Imm запрещает соответствующий damage type; низкое сопротивление — лучше относительная альтернатива,
не «уязвимость ×2». Разные games получают отдельные facts/lore fields.

## Открытые вопросы

Канонический registry ownership и alias migration; действующие Minotaur caps; источник study;
preview input на desktop/touch; bosses/summons; сохранение незавершённых spell transactions;
доступность первичных руководств и права на отдельные ресурсы. Бестиарий не блокирует R3–R6.

Связи: [Identify Monster](identify-monster.md), [Death Strike](death-strike.md),
[размеры](../research/monster-size-classification.md), [баланс](../research/combat-balance.md).
