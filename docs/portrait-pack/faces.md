# Лицо: 56 кадров портрета

Справочник к [ТЗ](README.md), этапы B–C. Обобщение ТЗ портретов v2 проекта Medusa: по нему получен набор героини,
который принят и работает в игре. Инструмент — `tools/portrait_pack/portrait_kit.py`.

## 1. Как устроены оригинальные портреты

Замеры по pc01–pc29 и 7pc01–7pc25:

- Голову видно сквозь **неподвижное мягкое яйцевидное окно**, которое касается нижнего края кадра. Окно одинаково во
  всех кадрах (IoU силуэта с 01 — 0,90–0,99): голова поворачивается и кивает внутри него, её размер не меняется. Окно
  опускается только в 35 (на 25 % высоты) и 36 (на 52 %, видна макушка).
- Под подбородком у многих героев короткая шея, которая уходит в низ окна. Плеч, ключиц и горизонтального среза нет.
- Генератор перерисовывает картинку целиком, выделения у него нет. Поэтому для кадров выражений `compose` берёт из
  сгенерированного кадра **только** глаза, рот или лицо и вклеивает их в мастер. Волосы и силуэт остаются от мастера
  пиксель в пиксель.
- Тайминги (`portrait_frame_data.txt`, единица 1/16 с): моргание, речь, «нет» и «да» — по 125 мс. Любой скачок размера
  или тона между соседними кадрами заметен сразу.

## 2. Холст и геометрия (проверяет `qa`)

Холст **944×1264** (×8 от игровых 118×158), RGBA. Генератор отдаёт картинку своего размера; на холст её переносит
`compose`, а не генератор.

| Что | На 944×1264 | Допуск |
|---|---|---|
| Окно | эллипс, центр (472, 640), полуоси 430×622 | задаёт `compose` |
| Зрачки | y 620 (49 %), x 330 и 614 | ±25 по y, ±30 по x |
| Кончик носа | y 800, x 472 | ±40 |
| Линия рта | y 935 | ±40 |
| Подбородок | **FAIL** только вне y 1065–1245; ориентир по полу — женщины ≈ 1110 (`pc02`, `pc12`), мужчины ≈ 1200 (`pc05` ≈ 1215, `pc09` ≈ 1200) | дальше 45 от ориентира — WARN |
| Шея | от подбородка до низа кадра, без плеч и среза | зазор под шеей ≤ 12 px |
| Волосы | заполняют окно до краёв, лишнее обрезается окном | — |

**Подбородок задаёт лицо персонажа, а не кадрирование.** `compose` ставит зрачки в канонические точки, то есть
масштабирует голову по расстоянию между зрачками. Где окажется подбородок, решают пропорции лица: у длинного мужского
лица он ниже. Промптом это не исправить: если генератор «поднимет подбородок», уменьшив лицо, `compose` растянет его
обратно по зрачкам. Из-за подбородка не перегенерировать. Ориентиры по полу измерены по сетке на оригиналах игры
(2026-11-19): женщины ≈ 1110, мужчины ≈ 1200. Прежний допуск 1110 ± 45 был снят с женского `pc02`, и мужские
оригиналы `pc05` и `pc09` его не проходили.

## 3. Тексты для промптов

Все три блока вставляются в промпты дословно. CHARACTER LOCK собирается из `card.yml` (README, этап A) один раз и
дальше не меняется.

```text
CHARACTER LOCK — identical in every image:
<age> <sex> <race look>, <build and overall look>.
Face: <face shape, cheekbones, jaw, chin>. Eyes: <shape, iris colour, pupils>. Brows: <shape, colour>.
Nose: <shape>. Lips: <shape, colour>.
Skin: <tone in words and approximate hex>, <texture>.
Hair: <colour>, <length relative to the jaw and shoulders>, <parting, fringe or none>, <style>.
Marks: <scars, tattoos, freckles, scales …; each asymmetric mark names the side in the character's own terms
("on her own left cheek") and says it never mirrors>. Head accessories: <none, or exactly what>.
Short neck visible under the chin; the neck continues straight down and is cut off by the bottom edge of the
image (do not round it off or fade it out). No shoulders, no collarbones, no clothing<, no jewellery unless listed>.
```

```text
STYLE LOCK — identical in every image:
Late-1990s Might and Magic VIII party portrait, remastered: pre-rendered 3D CG head, smooth broad shading, clear
readable facial planes, restrained specular highlights, soft frontal studio light slightly from the upper left,
warm neutral tone. Not a photograph, not a painting, no brush strokes, no outline.
Front view, camera at eye level. The head fills a tall portrait frame: top of the hair near the top edge, pupils
just below the middle of the height, the face in its natural proportions, chin near the bottom of the frame, short
neck below it.
Transparent background. One head only: no sheet, no grid, no text, no frame, no guide lines, no shadow.
```

```text
NEGATIVE:
shoulders, collarbones, clothing, background scene, vignette, drop shadow, text, watermark, frame, border, guide
lines, sprite sheet, cartoon, anime, painterly, photo, dark moody lighting, different hairstyle, different face,
changed head size<, character-specific additions from card.yml negatives>.
Never use pure magenta #FF00FF or pure cyan #00FFFF.
```

## 4. Мастер — кадр 01 (этап B)

Четыре кандидата: четыре **отдельных** вызова с одним и тем же промптом M. Вложения (не больше пяти):

1. образ персонажа — картинка или фото пользователя из `input/` (если их несколько — лучший анфас; второй ракурс можно
   добавить вместо пункта 3);
2. два оригинальных кадра 01 того же пола, ×4, из `$PACKS/_cache/reference/x4/`: женщины — `pc02-01`, `pc12-01`;
   мужчины — `pc05-01`, `pc09-01`. Для нечеловеческой расы один из них заменить кадром 01 строки этой расы;
3. схема `$PACKS/_cache/templates/layout-guide-944x1264.png` (только пропорции).

Промпт M:

```text
Create ONE image: frame 01 (the MASTER) of a 56-frame RPG party portrait set. Every later frame is made from it.
[CHARACTER LOCK]
[STYLE LOCK]
Expression: calm neutral face, relaxed closed lips, eyes looking straight at the viewer.
The first attachment shows WHO this character is (a picture or photo of them): keep the face shape, eyes, brows,
nose, lips, hair, skin tone and marks recognisable, but render them as a late-1990s pre-rendered 3D game portrait,
not as a photo or painting, and do not copy its background, clothing, lighting or camera angle.
The original game portraits show ONLY the rendering style, the tall oval head framing that fills the image and how
a short neck sits under the chin; do not copy their faces or hair. The grey layout guide shows only proportions;
never draw its lines.
Do not: [NEGATIVE]
```

Каждого кандидата: `portrait_kit.py grid` → зрачки по сетке → `compose --kind master --pupils LX LY RX RY`.
Лист для пользователя `face/candidates/masters.png`: четыре кандидата в размере игры 118×158 и ×2, рядом с образом из
`input/` и `pc02-01`/`pc05-01`, под каждым — яркость головы и заполнение окна. **Остановка 1.**

После выбора: `face/final/01.png`, `face/final/landmarks.json` по `grid`:

```json
{"pupil_left": [330, 620], "pupil_right": [614, 620], "brow_y": 545, "nose_tip": [472, 800],
 "mouth_left": [380, 935], "mouth_right": [565, 935], "chin": [472, 1110]}
```

`regions` — области «глаза» и «рот» не задевают волосы; `qa face/final --partial` — FAIL 0.

## 5. Ракурсы (не кадры, образцы для поз)

Четыре изображения с мастером во вложении: голова повёрнута на 35° вправо по изображению, на 35° влево, поднята на
15°, опущена на 25° (вид на макушку). Лежат в `face/turnarounds/`, в набор не входят.

## 6. Остальные 55 кадров (этап C)

Направления — **по изображению**, как видит зритель. Образец силы выражения и позы — тот же номер кадра у оригинала
того же пола: женщины `pc02-NN` (для 17–20 также `7pc05-NN`), мужчины `pc05-NN`, ×4 из `_cache/reference/x4/`.

**Выражения и цвет** (`eyes`, `mouth`, `face`, `head`). Вложения: `face/final/01.png`, образец. Промпт E:

```text
Edit the attached master portrait. Return the SAME image: same person, same head position, size and angle, same
hair, same lighting and transparent background. Change ONLY the facial expression: <CHANGE>.
The second attachment is an original game portrait showing ONLY how strong this expression should be; do not copy
its face.
[CHARACTER LOCK]
Do not: [NEGATIVE]
```

Для `head` (08, 09, 12, 54) вместо «Change ONLY the facial expression» — «Change the colour and surface of the whole
head as described, keep the outline». Сборка: `compose RAW --kind <вид> --master face/final/01.png --out
face/final/NN.png`. Если `registration score` < 0,60 — генератор изменил причёску или наклон: повторить с фразой
«the head and hair must be pixel-identical to the attached master» или сузить вид (`face` → `eyes`, затем `mouth`).

**Позы** (`pose`). Вложения: мастер, подходящий ракурс из раздела 5, образец. Промпт P:

```text
Create ONE image: frame NN of the same character's portrait set. The first attachment is the master frame of this
exact character: the same person, the same render style, lighting and head size.
[CHARACTER LOCK]
[STYLE LOCK]
Pose change (the only difference from the master): <CHANGE>. The head stays the same size and stays inside the same
tall oval framing; only its angle changes. The original game portrait attached shows ONLY the pose and its
strength; do not copy its face.
Do not: [NEGATIVE]
```

Сборка: `compose RAW --kind pose --master face/final/01.png --frame NN --out face/final/NN.png`. Последовательности
делать подряд и сразу смотреть анимацией: 25–29, 30–33, 34–36. Кадр 36 — тот же исходник, что 35, собранный с
`--frame 36`.

Каждый вызов — запись в `face/prompts.jsonl` (кадр, попытка, вложения, промпт, score, решение). На кадр — не больше
четырёх попыток; потом лучший вариант в набор, кадр — в список «на доработку» в `face/review.md`.

### Таблица кадров

| № | Движок | Вид | CHANGE (в промпт) |
|---|---|---|---|
| 01 | Normal | master | промпт M |
| 02 | Cursed | face | gloomy cursed look: heavy half-lowered eyelids, eyes looking down, slight frown, faint grey shadows under the eyes |
| 03 | Weak | face | exhausted and weak: eyelids almost closed, sad drooping brows, mouth slightly open and slack |
| 04 | Sleep | pose | asleep: eyes fully closed, face relaxed, lips slightly parted; head rolled about 15° so the crown leans to the LEFT side of the image, chin slightly down |
| 05 | Fear | face | terrified: eyes wide open, eyebrows raised high, mouth open in a gasp |
| 06 | Drunk | face | tipsy: lopsided silly half-smile, unfocused half-closed eyes, light pink flush on the cheeks |
| 07 | Insane | face | manic: wide toothy grin, wild staring eyes, brows raised unevenly |
| 08 | Poisoned | head | poisoned: sickly yellow-green skin clearly yellower than normal, sweaty sheen, nauseous half-closed eyes, mouth slightly open |
| 09 | Diseased | head | diseased: red inflamed blotches and small pustules on the forehead and cheeks, puffy skin, one eye squinting, miserable |
| 10 | Paralyzed | face | paralyzed: frozen blank stare, eyes wide open and fixed, face completely slack |
| 11 | Unconscious | pose | unconscious: same head roll as frame 04, eyes closed, mouth slack, dark purple bruise around the character's own right eye (the eye on the LEFT side of the image) |
| 12 | Petrified | head | the whole head — skin, hair and anything worn on the head — is matte grey granite, no colour, eyes closed, serene |
| 13 | Blink | eyes | both eyes closed naturally, nothing else changes |
| 14 | Wink | face | the character's own right eye (LEFT side of the image) closed in a wink, faint playful smile at one mouth corner |
| 15 | MouthOpenRandom | mouth | lips slightly parted |
| 16 | PurseLipsRandom | mouth | lips pressed together and slightly pursed |
| 17 | LookUp | eyes | eyes looking up, upper eyelids raised, head unchanged |
| 18 | LookRight | pose | head turned about 15° toward the RIGHT side of the image, eyes looking further RIGHT |
| 19 | LookLeft | pose | head turned about 15° toward the LEFT side of the image, eyes looking further LEFT |
| 20 | LookDown | eyes | eyes looking down, upper lids lowered, pupils still visible (eyes NOT closed) |
| 21 | Talk | mouth | speaking, mouth slightly open as for "eh" |
| 22 | MouthOpenWide | mouth | speaking, mouth open as for "ah" |
| 23 | MouthOpenA | mouth | speaking with a slight smile, mouth open as for "ee", upper teeth visible |
| 24 | MouthOpenO | mouth | speaking, lips rounded as for "oo" |
| 25 | No, крайний | pose | head turned (yaw) about 35° toward the RIGHT side of the image, mild disapproval, mouth closed |
| 26 | No | pose | head turned about 15° toward the RIGHT side of the image, mild disapproval |
| 27 | No, центр | mouth | slight disapproving press of the lips |
| 28 | No | pose | head turned about 15° toward the LEFT side of the image, mild disapproval |
| 29 | No, крайний | pose | head turned about 35° toward the LEFT side of the image, mild disapproval |
| 30 | Yes, вниз | pose | head nodded DOWN about 12° (forehead toward the viewer), eyes still on the viewer, pleasant agreeable look |
| 31 | Yes | pose | head nodded down about 5°, pleasant agreeable look |
| 32 | Yes | pose | chin raised about 3°, pleasant agreeable look |
| 33 | Yes, вверх | pose | chin raised about 8°, pleasant agreeable look |
| 34 | AvoidDamage | pose | dodging: head pulled back and turned about 20° toward the RIGHT side of the image, brows knitted, mouth open |
| 35 | AvoidDamage | pose | ducking: head bowed forward about 25°, eyes open glaring up from under the brows |
| 36 | AvoidDamage | pose | исходник кадра 35, `--frame 36` |
| 37 | DamageReceivedMinor | pose | hit: head knocked about 25° toward the LEFT side of the image and tilted, eyes squeezed, mouth open in a grimace, a little blood at the lip corner |
| 38 | DamageReceivedModerate | pose | hit hard: head knocked down and toward the RIGHT side of the image, eyes shut tight, bruise on the cheek, blood from the nose and lips |
| 39 | DamageReceivedMajor | pose | severe hit: head thrown back about 20°, mouth open in pain, purple bruises around one eye and the cheek, blood at the mouth |
| 40 | Smile | face | warm closed-mouth smile, friendly eyes |
| 41 | WideSmile | face | wide happy open smile showing upper teeth, eyes crinkled |
| 42 | Sad | face | displeased and sad: brows drawn together and down, mouth turned down, eyes lowered slightly |
| 43 | CastSpell | face | concentrating on a spell: eyes closed, brows slightly knitted, lips calm |
| 44 | Portrait41 | face | shocked pain: eyes wide, brows raised, mouth open in "ah!" |
| 45 | Portrait42 | face | wincing "ooh": lips pushed forward, brows raised, eyes half-closed |
| 46 | Portrait43 | face | puzzled: one eyebrow raised, the other lowered, lips pressed to one side |
| 47 | Portrait44 | face | pleading: eyes closed, brows raised in the middle, sad soft mouth |
| 48 | Portrait45 | face | smug: confident closed-mouth smirk, eyelids slightly lowered |
| 49 | Scared | face | alarmed shout: brows drawn down and together, eyes wide, mouth wide open yelling |
| 50 | Portrait47 | face | proud triumph: big bright open smile, eyes shining |
| 51 | Portrait48 | face | annoyed disappointment: scowl, brows knitted, mouth tight |
| 52 | Portrait49 | face | skeptical: eyes glancing toward the RIGHT side of the image, one eyebrow raised |
| 53 | Portrait50 | face | angry glare: brows low, eyes narrowed, lips tight |
| 54 | Portrait51 | head | straining with effort: face flushed red, teeth clenched and bared, eyes squeezed nearly shut |
| 55 | Portrait52 | face | cheerfully drunk laugh: big open laughing smile, happy eyes, light pink cheeks |
| 56 | Portrait53 | face | surprised awe: eyes wide, brows high, mouth open round "oh!" |

Итого: master 1; eyes 3 (13, 17, 20); mouth 7 (15, 16, 21–24, 27); face 23; head 4 (08, 09, 12, 54); pose 17
(04, 11, 18, 19, 25, 26, 28–35, 37–39) и 36 из исходника 35.

## 7. Проверка набора

`portrait_kit.py qa face/final --reference $PACKS/_cache/reference` → **FAIL 0**. Что проверяется: 56 файлов
944×1264 RGBA, нет `#FF00FF`/`#00FFFF`, ничего за окном, геометрия мастера (раздел 2), волосы и контур кадров
выражений совпадают с 01, IoU силуэта ≥ 0,90, площадь и тон в допусках, 35/36 опущены, 08 желтее, 09 краснее,
12 серый, 54 покраснел. Яркость мастера 95–125 рассчитана на светлую кожу: у тёмной кожи WARN допустим, его надо
объяснить в `review.md`.

Листы в `face/final/qa/`: `sheet-game-size.png`, `sheet-x2.png`, `diff-vs-01.png`, `anim-*.gif` с таймингами игры.

`face/review.md` — ответы «да/нет» с номерами кадров, каждое «нет» переделать:

1. На `sheet-game-size.png` все 56 кадров — один и тот же персонаж (лицо, глаза, кожа, причёска, приметы)?
2. Приметы (шрамы, татуировки, украшения) на своих местах и не отзеркалены в повёрнутых кадрах?
3. Каждое выражение читается в 118×158 так же уверенно, как у оригинала под ним?
4. `anim-blink`, `anim-talk`: двигаются только веки или рот, без вспышек и сдвигов?
5. `anim-no`, `anim-yes`: плавно, без скачков размера, лицо то же, что в 01?
6. `anim-dodge`: голова уходит вниз за край окна и возвращается без рывка вбок?
7. `anim-damage`: урон сильнее от 37 к 39, голова не выходит за окно?
8. 18 смотрит вправо по изображению, 19 — влево; 52 косится вправо?
9. Нет шва по краю вклейки: ни ореола, ни ступеньки тона у бровей и рта?
10. Нигде не видно плеч, одежды, фона, текста, линий схемы; низ — мягкий край окна?
11. Тон 01 не темнее оригиналов и лицо не теряется на тёмной рамке HUD?
12. Сходство с образом из `input/` сохранилось во всех кадрах, а не только в 01?
