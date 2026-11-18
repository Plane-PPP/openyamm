# ТЗ v3: голова героини на теле `pc02` — пересадка головы по методу автора OpenYAMM

Версия 3, 2026-11-18. Заменяет [v1](doll-generation-brief-v1.md) и [v2](doll-generation-brief-v2.md) (они остаются как
история). Импорт куклы в движке не меняется: `doll/body.png` в пакете портрета, `devdocs/CUSTOM_PORTRAITS.md`.

Пути без `/` в начале — от корня копии `/home/plane/projects/openyamm-minotaur-v2.3/output/portrait-doll-project-medusa`.

**Суть.** Тело и руки остаются от `pc02` без единого изменения. Codex перерисовывает **только голову прямо на
кропе донора** (та же фигура, тот же кадр, другая голова), а скрипт по методу автора вписывает её обратно: регистрация
по телу, перенос тона кожи, маска разницы, шов в полосе 24 px. Ничего не размывается.

Ориентиры результата:
- `test_screenshots/Ideal_Head_On_Doll_Implementation_Example.png` — как должно быть в игре (кукла автора `9pc04bod`);
- `portraits/reference/mm/dolls/9pc02bod.png` — пересадка автора **на то же тело `pc02bod`**: главный образец геометрии;
- `portraits/reference/doll-v3-analysis/heads-x3.png` — `pc02` | `9pc02` | `9pc04` | C | D | портрет 01, линии глаз,
  подбородка и шва.

## 0. Почему прошлые попытки не дали результата

| Попытка | Что сделано | Что вышло |
|---|---|---|
| B_v2 (ТЗ v1) | голова сгенерирована отдельно, вклеена по ориентирам | мельче на ~20 %, чётче тела, «наклеена» |
| C (ТЗ v2, путь B) | вся кукла одним кадром 1024×1536 | лицо ≈ 87 px на холсте, тело и руки перерисованы — руки не годятся |
| D (ТЗ v2, путь A) | голова из C01 + тело и руки `pc02` | «мыло», голова читается маленькой, ракурс чужой |
| E–H (`output/medusa-doll-E…H`) | ошибочная ветка | **не использовать ни как образец, ни как код** |

Причины D, по коду и по картинкам:
1. **Мыло задано самим ТЗ v2** («смягчить голову: уменьшить до x1 и вернуть увеличением»): `tools/doll_fallback.py`
   делает `166×112 → 83×56 (BOX) → 166×112 (BICUBIC)`. Эталон автора резкий: голова чётче тела, и это принято.
2. **Ракурс.** Голова C01 повёрнута на ¾ и смотрит вбок (опора на `turnaround-left`; в ТЗ v1/v2 ошибочно сказано, что
   `pc02` в ¾). У `pc02` лицо почти анфас, повёрнуто чуть влево по изображению; у `9pc02` автора — так же.
3. **Размер.** Линии глаз и подбородка у D совпадают с `pc02` (y ≈ 50 и ≈ 88), но лицо уже из-за поворота, а волосы
   кончаются у челюсти: там, где у `pc02` длинные волосы лежат на плечах, у D открыта длинная тонкая шея — голова
   читается маленькой, «на палочке». У `9pc02` волосы занимают место волос донора, видна только короткая шея.
4. **Ручные пиксели.** Плечи под волосами донора дорисованы полигонами (`ImageDraw.polygon`) — ступенька на левом
   плече; размеры головы взяты из ручной разметки, а не измерены.

Общие причины всех попыток:
- голову рисовали отдельно от тела и ставили скриптом по числам, а генератор числам (координатам в промпте) не следует;
- в запрос на куклу вставляли CHARACTER LOCK портрета с его кадрированием («no shoulders», «neck cut off by the bottom
  edge») — противоречие;
- исправления просили в той же длинной сессии свободным текстом — Codex дрейфовал.

## 1. Метод автора (переносим без изобретений)

Материалы — `docs/upstream-reference/` (README там же). Главное:

- Автор пересадил головы 20 персонажей MM9 на тела MM6–MM8. Тело, руки, смещения и тип куклы — донора, пиксель в
  пиксель; меняется только голова и верх шеи над швом. Поэтому всё снаряжение садится как на донора.
- `9pc02bod` = `pc02bod` с головой MM9 PC02: строки **y ≥ 134 совпадают с `pc02bod` побайтно**, изменения — x 9–136,
  y 1–133 (по замеру разницы: шов ≈ 122 при полосе 24; `dolls.json` автора в репозиторий не попал). Глаза и подбородок
  на линиях донора, лицо почти анфас и такой же ширины, волосы до плеч.
- Шаги автора: `prepare` — кроп головы-шеи-плеч донора ×4 LANCZOS, полоса «личности» из кадров портрета 01, 19, 26,
  28, бриф; Codex **только генерирует** («та же фигура в том же кадре, но с головой персонажа»); `extract` —
  регистрация по телу ниже шва, перенос тона кожи головы на кожу донора у шва (Lab), маска «где новая голова
  отличается от донора», смешивание в полосе, ниже полосы — строки донора побайтно.

Уроки генерации Codex у автора (`mm9_pc_portraits-PLAYBOOK.md`, `mm9_pc_dolls-README.md`) и как они учтены:

| Урок | В этом ТЗ |
|---|---|
| Codex возвращает квадрат и перекомпонует высокие кропы | донор на квадратном холсте; регистрация с широким поиском масштаба |
| Тело вокруг головы Codex копирует близко, но не точно | тело — только донора; из генерации берётся лишь то, что отличается от донора, и только выше шва |
| Самопроверка Codex о прозрачности ненадёжна | альфу проверяет скрипт и картинка на зелёном фоне |
| Не больше 5 входных картинок; черты образца протекают в результат | ровно 2 входа: донор и личность. `9pc02`/`9pc04` генератору **не показывать** (протекут светлые/кудрявые волосы и лицо) |
| Одна корректирующая перегенерация, первый результат сохраняется | правило 3 раздела 3 |
| Неудачу чинят причиной (бриф, параметры) и новым запуском, а не правкой картинки | раздел 9 |
| Codex делает кожу светлее и теплее, волосы кудрявее образца | цвета названы словами; итоговый тон кожи выставляет скрипт |

## 2. Что получаем

- `body.png` 166×594 RGBA: `pc02bod`, у которой заменены голова, волосы и шея выше шва. Ниже шва — `pc02bod` побайтно.
- В пакете **только `doll/body.png`**. Руки не кладём: игра берёт руки `pc02` (импорт допускает любой поднабор).
- Единственное масштабирование — уменьшение генерации до масштаба донора внутри регистрации (LANCZOS).

## 3. Правила для Codex

1. Работать только в копии: новый скрипт `tools/doll_graft.py`, рабочая папка `portraits/med01/doll-graft/`, пакет
   `portraits/packages/medusa_portrait_doll_graft/`. Копия лежит в `output/` (в `.gitignore`), графика игры в git не
   попадёт. Не трогать `/home/plane/projects/project-medusa`, `output/medusa-doll-E…H`, папки `doll-v1`, `doll-v2`,
   `doll-v3-fallback` и пакеты B–D. Не коммитить.
2. Пиксели меняет только `tools/doll_graft.py` по этому ТЗ. **Запрещено:** размытие, уменьшение-увеличение
   («смягчение»), рисование полигонами или кистью, ручные ориентиры, правка тела ниже шва, правка рук, ретушь
   результата генерации.
3. Генерация — встроенным инструментом Codex: **один вызов — один кандидат**, текст `prompt.txt` дословно, вложения
   ровно `donor.png` и `identity.png` в этом порядке, CHARACTER LOCK портрета не добавлять. Ответ сохранить без
   изменений. Корректирующая перегенерация — одна на кандидата и только если сломан кадр (фигура сдвинута или другого
   размера) или нет прозрачности: тот же промпт плюс одно предложение о дефекте; первый ответ — `gN_rejected.png`.
4. Если шаг не выполняется (нет cv2, регистрация не сходится, маска кожи пустая) — остановиться и написать, что и
   почему. Обходных путей не изобретать.
5. Новый раунд после замечаний пользователя — в **новой** сессии Codex (раздел 9). Старые файлы не перезаписывать:
   раунд 2 пишет в `round2/`, кандидаты продолжают нумерацию (g4, g5, …).
6. Python — отдельный venv вне репозитория: `pillow numpy scipy opencv-python-headless`. Скрипты запускать через
   `safe-heavy-run run --profile light -- <venv>/bin/python tools/doll_graft.py …`; если light (512 МиБ) не хватит —
   тот же запуск без `--profile`. Лимиты не повышать.

## 4. Скрипт `tools/doll_graft.py` — порт `dolls.py` автора

Перенести из `docs/upstream-reference/mm9_pc_dolls-dolls.py` функции `register`, `srgb_to_lab`, `lab_to_srgb`,
`match_skin`, `head_weight` и логику `extract` — без изменений, кроме перечисленных ниже. Настройки —
`portraits/med01/doll-graft/graft.json`:

```json
{
  "donor_body": "portraits/reference/mm/dolls/pc02bod.png",
  "head_box": [0, 0, 166, 200],
  "square_canvas": 200,
  "upscale": 5,
  "seam_y": 122,
  "seam_band": 24,
  "difference_threshold": 40,
  "skin_match_other": 0.15,
  "skin_match_strengths": [1.0, 0.65, 0.35],
  "identity_dir": "portraits/med01/v2/final",
  "identity_frames": [1, 19, 26, 28]
}
```

**`prepare`** → в `portraits/med01/doll-graft/`:
- `donor.png`: кроп `head_box` из `pc02bod` (166×200), по центру прозрачного квадрата 200×200 (сдвиг x = 17), ×5
  LANCZOS → 1000×1000;
- `identity.png`: кадры `identity_frames` из `identity_dir` (944×1264), каждый → 300×402 LANCZOS, полоса 1200×402 на
  прозрачном фоне;
- `seams.png`: кроп донора ×3 с линией шва и границами полосы. Самопроверка: шов ниже концов длинных волос `pc02`
  (около y 118) и проходит по плечам и груди. Если нет — остановиться;
- `prompt.txt`: текст раздела 5 дословно.

**`extract gN`** (`raw/gN.png` → кандидат):
- `register` как у автора: шаблон — кроп донора 166×200 без полей; генерация покрывает квадрат с полями, поиск
  масштаба это учитывает. В отчёт: `registration_error`, `scale_vs_height_fit`, `offset` и `sampling` — сколько
  пикселей генерации приходится на пиксель донора (должно быть ≥ 2, ожидается ≈ 5).
- **Маска кожи новой головы — без змей.** Цветом золотые брюшки змей от кожи героини не отделить (замер по кадру 01
  v2: щёки ≈ RGB 176, 149, 124, насыщенность 0,20–0,42, шея до 0,48; брюшки змей 0,33–0,58, и `skin_mask` автора их
  захватывает), а местом — легко: это тонкие полосы в волосах. Для генерации: `skin_mask` автора выше шва →
  `binary_opening` (1–2 итерации) → **только самая большая связная область** (лицо и шея). Для донора (тёплая кожа
  `pc02`, строки шов…шов+60) — `skin_mask` автора как есть. Меньше 150 пикселей в любой маске — ошибка с сообщением.
- **Сила переноса тона** `s`: `lab + s·w·(moved − lab)`. По телу на каждое значение `skin_match_strengths`:
  `bodies/gN_s100.png`, `bodies/gN_s065.png`, `bodies/gN_s035.png`. (D, который пользователь счёл приемлемым по тону,
  смешивал примерно 65 % тона донора.)
- Дальше в точности как у автора: вес строк по полосе шва, `head_weight`, смешивание в премультиплицированной альфе,
  строки от `seam_y + seam_band/2` вниз — копия донора, вставка кропа в полное тело.
- `qa/gN/mask.png`: `head_weight` (красным) и маска кожи (зелёным) поверх кандидата — волосы, змеи, чешуя и глаза не
  должны попасть в маску кожи; старые каштановые волосы `pc02` не должны остаться.

**`qa`** — раздел 7. **`package gN sXXX`** — раздел 8.

## 5. Промпт генерации (`prompt.txt`, дословно)

```text
Edit job: replace the head of a game paperdoll. Make one image with your built-in image generation tool.

Input 1, donor.png: TARGET. The head, neck and shoulders of an existing game paperdoll (a full-body figure in a fixed
pose) on a transparent background, enlarged 5x from the game asset and centred on a square canvas.
Input 2, identity.png: the NEW character, a young half-medusa woman, from her game portrait set at four head angles
(oval cut-outs; ignore the oval). Use it for identity only, not for head angle, gaze or framing.

Produce one square image with exactly the framing of input 1: the same figure at the same size and position, but
with the head of input 2's character.

- Replace the head: face, hair, ears and the neck above the shoulders. The new head has the same size, position,
  turn and tilt as the head in input 1: almost frontal, turned only slightly toward the image left. Her eyes look
  toward the viewer; no sideways glance. The face fills the same area as input 1's face: the same eye line, the same
  chin line, the same width across the cheeks. It joins the same neck at the same place, so it sits naturally on the
  unchanged body.
- Hair: very dark green-black with bottle-green highlights, forehead free, no fringe. Where input 1's long hair falls
  behind the neck and onto both shoulders, her hair takes that place, so the outline of head and hair is about the
  same as input 1's, neither smaller nor larger. Only a short part of the neck shows under the chin. No brown hair
  from input 1 remains.
- Snakes: three to five slender emerald-green snakes with pale gold bellies grow from the hair at the temples and on
  the crown; small heads, closed mouths. They stay inside the hair outline and never cover the eyes or the face.
- Identity from input 2: oval face, high cheekbones, straight slim nose, full lips in muted rose-mauve, almond-shaped
  amber-gold eyes with round human pupils, dark arched eyebrows, small patches of fine emerald scales at both temples
  and along the sides of the neck. Calm, slightly serious expression, mouth closed.
- Skin: her skin tone from input 2, as bright as input 1's neck and shoulders and lit the same way, so the neck
  meets the body without a visible step. No green body paint.
- Rendering matches input 1's body: the same pre-rendered 3D game look, smooth broad shading, light from the upper
  left, no outlines, no brush strokes, no photographic pores, not a glossy modern illustration. Clean and sharp, not
  blurred.
- Everything else (shoulders, chest, clothing, arms) stays exactly as in input 1: same pose, colours, shading and
  edges. It is replaced by the original afterwards, so do not restyle it.
- Genuinely transparent RGBA background (alpha 0) outside the figure; no backdrop, shadow, glow or halo. Square
  output, about 1024x1024. No text, marks or guide lines. Never pure magenta #FF00FF or pure cyan #00FFFF.
```

Почему так: это бриф автора для того же тела (`mm9_pc_dolls-pc02-brief.md`) плюс приметы героини. Размер, ракурс и
стык с шеей задаёт картинка донора, а не числа; от портрета берётся только лицо.

## 6. Раунд генерации

1. `prepare`; посмотреть `seams.png`.
2. Три кандидата g1–g3: три отдельных вызова с одним и тем же промптом. Ответы — `raw/g1.png` … `raw/g3.png` без
   изменений. `raw/runs.json` — по записи на вызов: файл, полный промпт, sha256 входов и выхода, размер, режим,
   `retry_reason` (пусто у первого), заметка в одну строку.
3. Каждый ответ проверить самому, а не по самоотчёту генератора: `raw/gN_on_green.png` (наложение на зелёный) — фон
   прозрачный, фигура в том же кадре, что `donor.png`.
4. `extract g1` … `extract g3`, затем `qa`.

## 7. Проверка (`qa`)

**FAIL** (блокирует):
- `body` 166×594, PNG 8-bit RGBA, ≤ 1 МБ;
- строки y ≥ `seam_y + seam_band/2` (134) побайтно равны `pc02bod`;
- нет видимых чистых `#FF00FF` / `#00FFFF`;
- строка y = 0 полностью прозрачна;
- `sampling` ≥ 2; обе маски кожи ≥ 150 пикселей.

**WARN** (показать пользователю с картинкой):
- `|scale_vs_height_fit − 1| > 0,08` или ошибка регистрации заметно больше, чем у других кандидатов — Codex
  перекомпоновал фигуру (корректирующая перегенерация);
- рамка альфы головы выше шва отличается от `pc02` (x 28–111, верх y 6) по ширине больше чем на 12 % или верх вне
  y 2–14 (`9pc02`: x 34–110, верх 12 — проходит);
- на `mask.png` видны остатки каштановых волос `pc02`.

**Листы** `qa/`:
1. `heads-x4.png` — главный: одинаковый кроп (x 10–150, y 0–150) ×4: `pc02` | `9pc02` | g1 | g2 | g3 | портрет 01;
   линии y 50 (глаза `pc02`), 88 (подбородок `pc02`), 122 (шов); кандидаты при силе тона 0,65.
2. `dolls-x2.png` — куклы целиком на `backdoll`, пустые руки (тело в (120, 94), руки `pc02` по `STATES` из
   `tools/doll_full.py`): `pc02` | `9pc02` | g1 | g2 | g3.
3. `skin-gN.png` — голова и шея ×4 при силе 1,0 / 0,65 / 0,35.
4. `neck-gN.png` — зона шва (y 95–140) ×6: `pc02` | `9pc02` | gN: нет ступеньки тона, лесенки на плечах, старых волос.
5. `states-gN.png` — три состояния рук (пустые, оружие и щит, двуручное).
6. `report.json` на кандидата и `summary.md` — таблица: кандидат, регистрация, масштаб, WARN, файлы.

**Самопроверка** — `review.md`, по каждому кандидату честные ответы, сравнивая с `9pc02` на `heads-x4.png`:
1. Лицо такого же размера, как у `pc02`/`9pc02` (глаза и подбородок на линиях, ширина по скулам)?
2. Ракурс как у донора (почти анфас), взгляд на зрителя, без косого взгляда?
3. Волосы лежат на месте длинных волос `pc02` до плеч, шея короткая?
4. Резкость как у `9pc02`, без мыла?
5. Та же героиня, что на портрете 01: янтарные глаза, чешуя, тёмно-зелёные волосы, змеи читаются?

Сверх правила 3 сам не перегенерировать.

**Остановка (единственная):** показать пользователю `heads-x4.png`, `dolls-x2.png`, `skin-gN.png` и `summary.md`.
Пользователь выбирает кандидата и силу тона — или отклоняет всех (раздел 9).

## 8. Пакет

1. **Ошибка упаковщика.** В `/home/plane/projects/openyamm-minotaur-v2.3/tools/make_portrait_package.py` цикл
   `for name in DOLL_FILES:` (строка 88) затирает `name` из строки 67, и с `--doll` в `portrait.yml` пишется
   `name: "hand_right_hold.png"`. В C это поправили руками в манифесте — так не делать. Переименовать переменную цикла
   (например, в `part`) в строках 88–96 и 111–113, не коммитить, сообщить пользователю.
2. `portraits/med01/doll-graft/package-doll/body.png` — выбранное тело, единственный файл в папке.
3. Сборка (кадры — утверждённый набор v2 118×158 из пакета D):

   ```sh
   python3 /home/plane/projects/openyamm-minotaur-v2.3/tools/make_portrait_package.py \
     portraits/packages/medusa_portrait_doll_D_fallback --id medusa --name Medusa --base-character 2 \
     --doll portraits/med01/doll-graft/package-doll \
     --output portraits/packages/medusa_portrait_doll_graft --zip
   ```

4. Проверить: `name: "Medusa"`; в `doll/` только `body.png`, по пикселям равный выбранному; 56 кадров побайтно равны
   кадрам пакета D.

## 9. Если ни один кандидат не подошёл: раунд 2 в новой сессии

Пользователь открывает новую сессию Codex и пишет по шаблону (без своих промптов для генератора):

```text
ТЗ: output/portrait-doll-project-medusa/docs/design/doll-generation-brief-v3.md, раунд 2.
Ближе всего: g2. Не так (по листу heads-x4): <одно-два наблюдения>.
Правки только по таблице раздела 9; пиксели вручную не трогать.
```

Codex вносит **одну** правку из таблицы (добавляет предложение в конец соответствующего пункта `prompt.txt` или меняет
параметр в `graft.json`), записывает её в `round2/changes.md` и повторяет разделы 6–7 для g4–g6.

| Симптом | Правка |
|---|---|
| Лицо меньше или уже, чем у `pc02` | в промпт: «The eyes sit exactly on input 1's eye line and the chin exactly on input 1's chin line.» Голову скриптом не масштабировать |
| Поворот ¾ или взгляд вбок | `identity_frames: [1]`; в промпт: «Head and eyes face the viewer as in input 1; never a three-quarter turn.» |
| Волосы короткие, шея длинная и тонкая | в промпт: «Her hair covers the sides of the neck and rests on both shoulders exactly where input 1's hair lies.» |
| Волосы или змеи шире и выше донора, упираются в край | в промпт: «Hair and snakes stay within input 1's head-and-hair outline.» |
| Мыло | убедиться, что кроме LANCZOS в регистрации ничего не масштабирует; в промпт: «Crisp, clean edges and features.» |
| Голова «современная», глянцевая, чужая телу | в промпт: «The same simple pre-rendered 3D look and soft lighting as input 1's shoulders; less micro-detail.» |
| Ступенька тона на шее | сила тона 1,0; `seam_band` 24 → 32 |
| Остались каштановые волосы `pc02` | `difference_threshold` 40 → 30; в промпт: «No brown hair from input 1 remains anywhere, also behind the shoulders.» |
| Фигура сдвинута или другого размера (WARN регистрации) | корректирующая перегенерация (правило 3) |
| Фон не прозрачный | корректирующая перегенерация (правило 3) |

Два раунда без успеха — остановиться и показать пользователю все листы.

## 10. Проверка в игре (пользователь, Windows)

Импорт пакета кнопкой **Add Portrait...** (после повторного импорта того же `id` — перезапуск игры):
- превью создания партии и экран персонажа рядом со снимком `Ideal_Head_On_Doll_Implementation_Example.png`: одна
  манера, голова не меньше эталона, шея короткая, шва нет;
- без снаряжения; кожаный и кольчужный доспех, плащ, шлем (голова под шлемом, змеи могут скрыться), меч и щит,
  двуручное оружие — всё садится как на `pc02` (тело и руки те же, так что расхождения возможны только у головы и шлема).

## 11. Готово, когда

- `qa` без FAIL; `review.md` и `summary.md` написаны; пользователь выбрал кандидата и силу тона;
- упаковщик исправлен, пакет `medusa_portrait_doll_graft` собран и проверен (раздел 8);
- проверка в игре — за пользователем (раздел 10).

## 12. Одежда (необязательно, после принятой головы)

Автор одежду не менял: у `9pc04` строки ниже 162 побайтно равны `pc04bod`. Зелёный наряд на эталонном скриншоте —
родной костюм тела `pc04`. Автор подбирал донора, костюм которого подходит персонажу. Отсюда два пути, оба отдельным
шагом: промпт головы (раздел 5) **не меняется**, одежду в него не вписывать.

Листы для выбора: `portraits/reference/doll-v3-analysis/female-type1-bodies.png` (все 11 женских тел типа 1 и их
строки) и `costume-options.png` (кукла сейчас | пробная перекраска в зелёный и бирюзовый | донор `pc04` | `9pc04`).

### 12.1. Перекраска ткани — без генерации

Команда `doll_graft.py recolor <body.png> --hue <градусы> [--sat <множитель>]` → `costume/<имя>.png`.
- Маска ткани берётся по `pc02bod` (ниже шва тело с ним совпадает): y ≥ `seam_y`, альфа > 0, тон 170–260°,
  насыщенность > 0,10 — сине-серые лиф и легинсы. Кожа, кожаные вставки, сапоги и золотая отделка в маску не попадают.
- Меняется только тон (и при `--sat` насыщенность), яркость остаётся — светотень и фактура ткани сохраняются.
- Варианты для выбора: `--hue 130` (зелёный), `155` (бирюзовый), `95` (оливковый).
- Проверки: альфа равна входной; пиксели вне маски равны входным; нет `#FF00FF`/`#00FFFF`. Листы:
  `qa/costume-mask.png` (маска поверх тела), `qa/costume-variants.png` (куклы на `backdoll` с руками).
- Пакет — раздел 8 с выбранным телом; `--id medusa` заменит текущую куклу, `--id medusa_green` добавит вторую.

### 12.2. Другой донор — другой дизайн одежды

Готовые зелёные костюмы того же типа куклы (снаряжение садится так же):

| Тело | Строка | Чем строка отличается от 2 (`pc02`) | Костюм |
|---|---|---|---|
| `pc04bod` | 4 | класс тот же (16), голос по умолчанию 3 вместо 1 | зелёный лиф и легинсы, кожаные перчатки и обмотки — костюм эталона, у автора есть `9pc04` |
| `pc08bod` | 8 | класс 4, голос 7 | зелёный лиф со шнуровкой, мятные брюки, тёмные сапоги |
| `grldbod` | 45 | класс 4, голос 44, мир MM6, фон превью `6backdol` | зелёные лиф и брюки, золотой пояс |

Рекомендуется `pc04`: тот же мир и класс, и есть образец автора `9pc04`. Порядок — разделы 3–8 в **новой** сессии, с
изменениями:
- `graft.json`: `donor_body` → `portraits/reference/mm/dolls/pc04bod.png`; `seam_y` — заново по `seams.png`. Волосы
  `pc04` длиннее: у `9pc04` изменения доходят до строки 161, так что шов будет около 150; проверить, что он ниже концов
  волос;
- голову генерировать заново (3 кандидата): она рисуется прямо на голове донора, а у `pc04` другие поворот, шея и
  чокер. Промпт раздела 5 без изменений;
- на листах `9pc04` вместо `9pc02`, руки `pc04*.png` (лежат в `portraits/reference/mm/dolls/`);
- пакет: `--base-character 4`, `--id medusa_pc04`; руки в пакет не класть — игра возьмёт руки `pc04` (с перчатками).

### 12.3. Новый дизайн одежды генерацией — не делать

Codex рисует костюмы хорошо (попытка C), но не держит контур и позу: в C понадобилась локальная деформация по ручным
точкам. Костюм пришлось бы вырезать по альфе `pc02`, сводить края с кожей рук и талии и проверять, что видно из-под
каждого доспеха. У автора такая подгонка (снаряжение MM9, `hesper_paperdoll`) — отдельный большой конвейер. В это ТЗ не
входит.
