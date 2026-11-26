# Ветка `feature/medusa-ruleset`

Движковые изменения для Project Medusa поверх базы Minotaur (сейчас `integration/minotaur-v3.2-20261124`, OpenYAMM 3.2).
Принцип как у MAW: данные — в таблицах рулсета и мода, движок даёт **общие** вызовы, а `game/rules/medusa/` появляется
только для настоящих отличий правил. Ночной порт (Codex, `PORT_BRIEF.md`, шаг 3) переносит коммиты этой ветки на новую
базу.

Карты и документы кампании — в рабочем дереве Map Studio (`openyamm-mapstudio`, ветка `map-studio`,
`campaigns/medusa/`); строки таблиц Medusa — `campaigns/medusa/tables/`.

## Изменения

| Что | Где | Зачем |
|---|---|---|
| `evt.JoinParty(rosterId)` → `joined, outcome` | `game/events/EventRuntime.cpp` (`luaJoinParty`) | Скрипт любой карты и мира присоединяет персонажа из строки ростера (в MM8 наём привязан к темам NPC 601–649). `outcome`: `joined`, `inn` (партия полна: ждёт в таверне, как в MM8), `along` (уже в партии или в таверне), `solo` (в соло-забег никто не вступает), `unknown` (нет строки) |
| `Party::joinFromRoster`, `Party::setRosterTable` | `game/party/Party.{h,cpp}`; привязка в `GameApplication::bindPartyDependencies` | Общая логика вступления по номеру строки `roster.txt` (с рядами рулсетов и модов) |
| Тест | `tests/EventScriptingTests.cpp`: «evt.JoinParty brings a roster row into the party…» | Партия из одного, полная партия, повтор, соло, неизвестная строка, отсутствие таблицы |

Первый пользователь: друг героини в `med_school` — строка ростера 20001 (диапазон Medusa 20000–20999), её приносит мод
`medusa_slice`. Studio видит новый вызов сама: `openyamm-studio/cmake/EmbedEventFunctions.cmake` читает регистрации
`EventRuntime.cpp`.
