# Материалы автора OpenYAMM

Копии для чтения (2026-11-18): в этом checkout их каталоги вне sparse-шаблонов, а часть объектов отсутствует в
частичном клоне. Не запускать как есть — переносить по [ТЗ](../README.md).

| Файл | Оригинал (upstream `pjasicek/openyamm`) | Что в нём |
|---|---|---|
| `mm9_pc_dolls-README.md` | `level_generation/ui/hud_inventory/mm9_pc_dolls/README.md` | метод «новая голова на готовом теле» для 20 героев MM9; тело и руки донора без изменений |
| `mm9_pc_dolls-dolls.py` | `…/mm9_pc_dolls/dolls.py` | `prepare` (кроп донора ×4, полоса личности, бриф) и `extract` (регистрация по телу, перенос тона кожи в Lab, маска разницы, шов) |
| `mm9_pc_dolls-pc02-brief.md` | `…/mm9_pc_dolls/pc02/brief.md` | бриф Codex для тела `pc02bod`, результат — `9pc02bod` |
| `mm9_pc_dolls-pc02-codex_last_message.md` | `…/mm9_pc_dolls/pc02/codex_last_message.md` | ответ Codex: 1254×1254 RGBA, одна корректирующая перегенерация |
| `mm9_pc_portraits-PLAYBOOK.md` | `…/mm9_pc_portraits/PLAYBOOK.md` (GitHub, `master`) | уроки генерации портретов в Codex: самопроверке прозрачности не верить, не больше 5 входных картинок, одна перегенерация, повтор при перегрузке, несимметричные приметы называть со стороны персонажа |
| `hesper_paperdoll-CODEX_BRIEF.md` | `level_generation/ui/hesper_paperdoll/CODEX_BRIEF.md` | генерация — только предложение, точную форму задаёт скрипт |

Швы пересадок автора по всем донорам — в [каталоге](../dolls.md), столбец «Шов».
