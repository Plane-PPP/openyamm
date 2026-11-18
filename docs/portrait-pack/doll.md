# Кукла: голова персонажа на теле донора

Справочник к [ТЗ](README.md), этап E. Метод автора OpenYAMM (`upstream-reference/mm9_pc_dolls-README.md`,
`upstream-reference/mm9_pc_dolls-dolls.py`), проверенный на Медузе: тело и руки донора не меняются ни на пиксель,
Codex перерисовывает **только голову прямо на кропе донора**, а скрипт вписывает её обратно. Инструмент —
`tools/portrait_pack/doll_graft.py`.

## 1. Что получаем

- `body.png` размера тела базовой строки ×1 в `icons_x2` (у `pc02bod` 166×594), RGBA. Ниже шва — тело донора побайтно.
- Руки в пак **не кладутся**: игра берёт руки базовой строки. Импорт проверяет, что `body.png` ровно вдвое больше
  логического размера тела базы (`game/party/CustomPortraits.cpp`), — у тела из `icons_x2` это выполняется само.
- Ничего не размывается и не масштабируется, кроме одного уменьшения генерации внутри регистрации (LANCZOS).
  Голова получается чётче мыльного тела — у автора так же, и это принято.

## 2. Настройки `doll/graft.json`

```json
{
  "base_row": 2,
  "donor_body": "$PACKS/_cache/dolls/pc02bod.png",
  "head_box": [0, 0, 166, 200],
  "square_canvas": 200,
  "upscale": 5,
  "seam_y": 122,
  "seam_band": 24,
  "difference_threshold": 40,
  "skin_match_other": 0.15,
  "skin_match_strengths": [1.0, 0.65, 0.35],
  "identity_dir": "$PACKS/<id>/face/final",
  "identity_frames": [1, 19, 26, 28],
  "benchmark": "$PACKS/_cache/dolls/9pc02bod.png"
}
```

Как заполнить для выбранного донора:

| Поле | Правило |
|---|---|
| `seam_y` | из столбца «Шов» [каталога](dolls.md); если там «—» — первая строка ниже концов волос, бороды или капюшона донора на плечах плюс 4. Проверить на `seams.png`: шов ниже всех старых волос и проходит по плечам или вороту |
| `head_box` | рамка головы донора (каталог, последний раздел) с отступом 10–20 px; низ — `seam_y` + 70…80, чтобы ниже шва осталось ≥ 70 строк тела для регистрации; не выходить за холст |
| `square_canvas` | `max(ширина, высота)` рамки: донор ставится по центру прозрачного квадрата — генератор возвращает квадрат и иначе перекомпонует кадр |
| `upscale` | целое, чтобы квадрат стал 900–1100 px |
| `benchmark` | пересадка автора на этот же донор (`9pcNNbod`, каталог), если есть; иначе не задавать |
| `skin_match_strengths` | как есть; итоговое значение выбирается в разделе 5 |

## 3. Вход генератору (`doll_graft.py prepare`)

- `donor.png`: кроп `head_box`, по центру прозрачного квадрата, ×`upscale` LANCZOS;
- `identity.png`: кадры `identity_frames` из `face/final` (944×1264) по 300×402 в полосу на прозрачном фоне;
- `seams.png`: кроп ×3 с линией шва и границами полосы;
- `prompt.txt`: шаблон ниже с подстановками из `card.yml`.

```text
Edit job: replace the head of a game paperdoll. Make one image with your built-in image generation tool.

Input 1, donor.png: TARGET. The head, neck and shoulders of an existing game paperdoll (a full-body figure in a fixed
pose) on a transparent background, enlarged from the game asset and centred on a square canvas.
Input 2, identity.png: the NEW character from their game portrait set at four head angles (oval cut-outs; ignore
the oval). Use it for identity only, not for head angle, gaze or framing.

Produce one square image with exactly the framing of input 1: the same figure at the same size and position, but
with the head of input 2's character.

- Replace the head: face, hair, ears, beard and anything worn on the head, and the neck above the shoulders. The new
  head has the same size, position, turn and tilt as the head in input 1. The eyes look the same way as input 1's;
  no sideways glance. The face fills the same area as input 1's face: the same eye line, the same chin line, the
  same width across the cheeks. It joins the same neck at the same place, so it sits naturally on the unchanged
  body.
- Hair: <hair from card.yml>. Where input 1's hair, beard or hood falls onto the shoulders or chest, the new
  character's own hair takes that place as far as it naturally reaches; nothing of input 1's old hair remains.
- Identity from input 2: <face, eyes, brows, nose, lips and marks from card.yml>. Calm neutral expression, mouth
  closed.
- Skin: the character's skin tone from input 2, as bright as input 1's neck and shoulders and lit the same way, so
  the neck meets the body without a visible step.
- Rendering matches input 1's body: the same pre-rendered 3D game look, smooth broad shading, light from the same
  side, no outlines, no brush strokes, no photographic pores, not a glossy modern illustration. Clean and sharp, not
  blurred.
- Everything else (shoulders, chest, clothing, arms) stays exactly as in input 1: same pose, colours, shading and
  edges. It is replaced by the original afterwards, so do not restyle it.
- Genuinely transparent RGBA background (alpha 0) outside the figure; no backdrop, shadow, glow or halo. Square
  output, about 1024x1024. No text, marks or guide lines. Never pure magenta #FF00FF or pure cyan #00FFFF.
```

Генератору ровно два входа. Пересадки автора (`9pcNNbod`) ему **не показывать**: их черты протекут в результат.

## 4. Кандидаты и сборка

1. Три кандидата g1–g3: три отдельных вызова с одним промптом. `doll/raw/gN.png` без изменений; `doll/raw/runs.json`
   — запись на вызов (файл, промпт, sha256 входов и выхода, размер, `retry_reason`). Корректирующая перегенерация —
   одна на кандидата и только если сломан кадр или нет прозрачности: тот же промпт плюс одно предложение о дефекте,
   первый ответ — `gN_rejected.png`.
2. Прозрачность проверять самому: `raw/gN_on_green.png`. Самоотчёту генератора не верить.
3. `extract gN` — как у автора: регистрация по телу ниже шва (масштаб и сдвиг, template match, cv2), перенос тона кожи
   в Lab, маска «где новая голова отличается от донора», смешивание в полосе шва, строки от `seam_y + seam_band/2`
   вниз — копия донора. Отличия от `dolls.py` автора:
   - маска кожи новой головы — `skin_mask` автора выше шва → `binary_opening` → **самая большая связная область** (лицо
     и шея): иначе в маску попадают золотые, рыжие и бежевые детали волос и украшений;
   - тело на каждую силу тона: `bodies/gN_s100.png`, `gN_s065.png`, `gN_s035.png`;
   - `qa/gN/mask.png`: маска разницы и маска кожи поверх кандидата.

## 5. Проверка и выбор кандидата (без остановки)

**FAIL:** размер тела = донору; 8-bit RGBA, ≤ 1 МБ; строки ниже `seam_y + seam_band/2` побайтно равны донору; нет
видимых `#FF00FF`/`#00FFFF`; верхняя строка прозрачна; на пиксель донора ≥ 2 пикселя генерации; маски кожи ≥ 150 px.

**WARN:** `|scale_vs_height_fit − 1| > 0,08` или ошибка регистрации заметно больше, чем у других кандидатов (Codex
перекомпоновал фигуру); рамка головы шире или уже донора больше чем на 12 %; на `mask.png` видны старые волосы донора.

Листы `doll/qa/`: `heads-x4.png` (донор | `benchmark` | g1 | g2 | g3 | кадр 01, линии глаз и подбородка донора и шва),
`dolls-x2.png` (куклы на фоне строки с пустыми руками), `skin-gN.png`, `neck-gN.png`, `states-gN.png` (три состояния
рук), при возможности `equipment-gN.png` (README, этап 0), `summary.md`.

Тело на фоне: `(2·Body X, −2·Body Y)` из строки `character_data.txt`, фон — столбец `Background`; руки — по смещениям
типа куклы из `doll_types.txt` (у типа 1: пустые руки `RHd` (50, 264) и `LHo` (234, 296); оружие и щит `RHu` и `RHb`
(86, 284), `LHd` (258, 296); двуручное `RHu` (86, 284), `LHu` (258, 296)).

`doll/review.md` по каждому кандидату: лицо того же размера, что у донора (глаза и подбородок на линиях)? Ракурс и
взгляд как у донора? Старых волос нет, шея естественной длины? Резкость как у `benchmark`? Тот же персонаж, что в
кадре 01?

**Выбор делает Codex:** без FAIL → меньше WARN → лучшие ответы `review.md`. Сила тона по умолчанию 0,65; 1,0 — если на
`neck-gN.png` видна ступенька тона. Решение и причина — в `review.md`; остальные кандидаты остаются в `bodies/`.

## 6. Если результат плохой

Одна правка за раунд: предложение в конец нужного пункта промпта или параметр `graft.json`. Запись в
`doll/changes.md`, кандидаты продолжают нумерацию (g4, g5, …). Раунд после замечаний пользователя — в новой сессии.

| Симптом | Правка |
|---|---|
| Остались старые волосы донора | `difference_threshold` 40 → 30, затем 25, повторить `extract` **без генерации**; не помогло — «No hair from input 1 remains anywhere, also behind the shoulders.» |
| Лицо меньше или уже, чем у донора | «The eyes sit exactly on input 1's eye line and the chin exactly on input 1's chin line.» Голову скриптом не масштабировать |
| Поворот или взгляд вбок | `identity_frames: [1]`; «Head and eyes face the same way as input 1; never a three-quarter turn.» |
| Волосы короткие, шея длинная и тонкая | «The hair covers the sides of the neck and rests on the shoulders where input 1's hair lies.» (если это не противоречит причёске персонажа) |
| Волосы шире и выше донора, упираются в край | «Hair stays within input 1's head-and-hair outline.» |
| Мыло | убедиться, что кроме LANCZOS в регистрации ничего не масштабирует; «Crisp, clean edges and features.» |
| Голова «современная», глянцевая | «The same simple pre-rendered 3D look and soft lighting as input 1's shoulders; less micro-detail.» |
| Ступенька тона на шее | сила тона 1,0; `seam_band` 24 → 32 |
| Фигура сдвинута или другого размера | корректирующая перегенерация |
| Фон не прозрачный | корректирующая перегенерация |

## 7. Цвет одежды (по просьбе пользователя)

Автор одежду не менял: другой костюм — это другой донор ([каталог](dolls.md)). Только цвет ткани меняется без
генерации: `doll_graft.py recolor <body.png> --hue <градусы> [--sat <множитель>] [--from <тон1>-<тон2>]`. Маска ткани
— по телу донора ниже шва: альфа > 0, тон в диапазоне `--from` (для сине-серой ткани `pc02` — 170–260°),
насыщенность > 0,10. Меняются только тон и, если задано, насыщенность; яркость, альфа и пиксели вне маски — как были.
Листы `qa/costume-mask.png` и `qa/costume-variants.png`. Новый дизайн одежды генерацией не делать: Codex не держит
контур и позу, подгонка снаряжения у автора — отдельный большой конвейер.
