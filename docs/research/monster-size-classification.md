# Утверждённая классификация Lethal Strike v0.4

Статус: **Approved / Implemented**, ТЗ пользователя 2026-10-15. Предварительная ревизия
69bed40 сохранена в истории Git. Категории утверждены пользователем; нового автоматического
пересмотра по HP/Level/геометрии нет. Размеры, HP и другие характеристики монстров не менялись.

658 шаблонов: **610 NORMAL / 48 GIANT**. Исходные198 MM8: **177 NORMAL / 21 GIANT**.
Кампанийные свидетельства184 остаются отдельной выборкой, не основанием удалять остальные шаблоны.
У всех GIANT шанс разрешён только при `0 < HP < 0.60*maxHP`, затем полный KG=1.
Unknown/custom ID по умолчанию NORMAL; включённый lethal_strike_trace предупреждает об этом.

[Авторитетный реестр CSV](lethal-strike-classification.csv) использует merged template IDs,
не имя/локализацию, actor index или уровень. [Полные характеристики](monster-stats.csv) повторяют
эту категорию в сохранённой колонке category_proposed (историческое название колонки).
Реальный compiled registry: [LethalStrikeMonsters.h](../../game/gameplay/LethalStrikeMonsters.h),
генератор: [generate_lethal_strike_research.py](../../tools/generate_lethal_strike_research.py).

Окончательные переопределения: NORMAL28–33,58–63,76–81,91–93,106–108;
GIANT130–132 и220–222. Указанные ID проверены по picture/family, включая исходное `Pesant`.
Все остальные решения предварительного реестра приняты без изменений. Неизвестные новые записи
не наследуют категорию по похожему названию; добавление в явный реестр требует отдельной правки.

## Все семейства и A/B/C

| Семейство | ID | Категория |
| --- | --- | --- |
| Lizardmen Male (Peasant) | 1, 2, 3 | NORMAL |
| Lizardmen Warrior | 4, 5, 6 | NORMAL |
| Couatl (winged snake) | 7, 8, 9 | NORMAL |
| Pirate Warrior Male | 10, 11, 12 | NORMAL |
| Pirate Crossbowman (Female) | 13, 14, 15 | NORMAL |
| Pirate Mage Male | 16, 17, 18 | NORMAL |
| Dark Elf Pesant Male | 19, 20, 21 | NORMAL |
| Dark Elf Pesant Female | 22, 23, 24 | NORMAL |
| Dark Elf Warrior | 25, 26, 27 | NORMAL |
| Ogre Pesant Male | 28, 29, 30 | NORMAL |
| Ogre Warrior | 31, 32, 33 | NORMAL |
| Wererat Human Form Male | 34, 35, 36 | NORMAL |
| Wererat Ratman Form Male | 37, 38, 39 | NORMAL |
| Wererat Rat Form Male | 40, 41, 42 | NORMAL |
| Dragon Hunter | 43, 44, 45 | NORMAL |
| Cleric (human male) | 46, 47, 48 | NORMAL |
| Vampire Female Peasant | 49, 50, 51 | NORMAL |
| Vampire (monster) | 52, 53, 54 | NORMAL |
| Necromancer (monster) | 55, 56, 57 | NORMAL |
| Troll | 58, 59, 60 | NORMAL |
| Troll Pesant | 61, 62, 63 | NORMAL |
| Minotaur | 64, 65, 66 | NORMAL |
| Minotaur Pesant | 67, 68, 69 | NORMAL |
| Dragon | 70, 71, 72 | GIANT |
| Fire Elemental | 73, 74, 75 | NORMAL |
| Water Elemental | 76, 77, 78 | NORMAL |
| Earth Elemental | 79, 80, 81 | NORMAL |
| Air Elemental | 82, 83, 84 | NORMAL |
| Dire Wolf | 85, 86, 87 | NORMAL |
| Thunder Lizard | 88, 89, 90 | NORMAL |
| Cyclops | 91, 92, 93 | NORMAL |
| Serpentmen | 94, 95, 96 | NORMAL |
| Will ' O Wisp | 97, 98, 99 | NORMAL |
| Centaur | 100, 101, 102 | NORMAL |
| Efreeti | 103, 104, 105 | NORMAL |
| Ogre Mage | 106, 107, 108 | NORMAL |
| Unicorn | 109, 110, 111 | NORMAL |
| Crystal Dragon | 112, 113, 114 | GIANT |
| Thunderbird | 115, 116, 117 | NORMAL |
| Wasp Warrior | 118, 119, 120 | NORMAL |
| Wyvern | 121, 122, 123 | NORMAL |
| Gorgon | 124, 125, 126 | NORMAL |
| Salamander | 127, 128, 129 | NORMAL |
| Phoenix | 130, 131, 132 | GIANT |
| Triton | 133, 134, 135 | NORMAL |
| Dragon Turtle | 136, 137, 138 | GIANT |
| Raven Man | 139, 140, 141 | NORMAL |
| Malevolent Boulder | 142, 143, 144 | NORMAL |
| Plane Guardian | 145, 146, 147 | NORMAL |
| Crystal Walker | 148, 149, 150 | NORMAL |
| Nightmare | 151, 152, 153 | NORMAL |
| Ether Knight | 154, 155, 156 | NORMAL |
| Juggernaut | 157, 158, 159 | GIANT |
| Gog | 160, 161, 162 | NORMAL |
| Bone Dragon | 163, 164, 165 | GIANT |
| Bedraggled Human (peasant) | 166, 167, 168 | NORMAL |
| Naga | 169, 170, 171 | NORMAL |
| Skeletons Archer | 172, 173, 174 | NORMAL |
| Dark Dwarf | 175, 176, 177 | NORMAL |
| School of Fish (non-combatant) | 178, 179, 180 | NORMAL |
| Wimpy Pirate Warrior Male | 181, 182, 183 | NORMAL |
| Wimpy Pirate Crossbowman (Female) | 184, 185, 186 | NORMAL |
| Wimpy Pirate Mage Male | 187, 188, 189 | NORMAL |
| Wimpy Dragon | 190, 191, 192 | GIANT |
| Human Mercenary | 193, 194, 195 | NORMAL |
| Wimpy Plane Guardian | 196, 197, 198 | NORMAL |
| Angel | 199, 200, 201 | NORMAL |
| Archer | 202, 203, 204 | NORMAL |
| Bat | 205, 206, 207 | NORMAL |
| Behemoth | 208, 209, 210 | GIANT |
| Beholder | 211, 212, 213 | NORMAL |
| Cleric Moon | 214, 215, 216 | NORMAL |
| Cleric Sun | 217, 218, 219 | NORMAL |
| Devil | 220, 221, 222 | GIANT |
| ADragon | 223, 224, 225 | GIANT |
| Dragonfly | 226, 227, 228 | NORMAL |
| Dwarf | 229, 230, 231 | NORMAL |
| Elemental Air | 232, 233, 234 | NORMAL |
| Elemental Earth | 235, 236, 237 | NORMAL |
| Elemental Fire | 238, 239, 240 | NORMAL |
| Elemental Light | 241, 242, 243 | NORMAL |
| Elemental Water | 244, 245, 246 | NORMAL |
| Elf Archer | 247, 248, 249 | NORMAL |
| Elf Spearman | 250, 251, 252 | NORMAL |
| Fighter Chain | 253, 254, 255 | NORMAL |
| Fighter Leather | 256, 257, 258 | NORMAL |
| Fighter Plate | 259, 260, 261 | NORMAL |
| Gargoyle | 262, 263, 264 | NORMAL |
| Genie | 265, 266, 267 | NORMAL |
| Ghost | 268, 269, 270 | NORMAL |
| Goblin | 271, 272, 273 | NORMAL |
| AGog | 274, 275, 276 | NORMAL |
| Golem | 277, 278, 279 | GIANT |
| Griffin | 280, 281, 282 | NORMAL |
| Harpy | 283, 284, 285 | NORMAL |
| Hydra | 286, 287, 288 | GIANT |
| Lich | 289, 290, 291 | NORMAL |
| Mage | 292, 293, 294 | NORMAL |
| Manticore | 295, 296, 297 | NORMAL |
| Medusa | 298, 299, 300 | NORMAL |
| AMinotaur | 301, 302, 303 | NORMAL |
| Monk | 304, 305, 306 | NORMAL |
| Necromancer | 307, 308, 309 | NORMAL |
| Ooze | 310, 311, 312 | NORMAL |
| Peasant Dwarf FemaleA | 313, 314, 315 | NORMAL |
| Peasant Dwarf FemaleB | 316, 317, 318 | NORMAL |
| Peasant Dwarf FemaleC | 319, 320, 321 | NORMAL |
| Peasant Dwarf MaleA | 322, 323, 324 | NORMAL |
| Peasant Dwarf MaleB | 325, 326, 327 | NORMAL |
| Peasant Dwarf MaleC | 328, 329, 330 | NORMAL |
| Peasant Elf FemaleA | 331, 332, 333 | NORMAL |
| Peasant Elf FemaleB | 334, 335, 336 | NORMAL |
| Peasant Elf FemaleC | 337, 338, 339 | NORMAL |
| Peasant Elf MaleA | 340, 341, 342 | NORMAL |
| Peasant Elf MaleB | 343, 344, 345 | NORMAL |
| Peasant Elf MaleC | 346, 347, 348 | NORMAL |
| Peasant Human1 FemaleA | 349, 350, 351 | NORMAL |
| Peasant Human1 FemaleB | 352, 353, 354 | NORMAL |
| Peasant Human1 FemaleC | 355, 356, 357 | NORMAL |
| Peasant Human1 MaleA | 358, 359, 360 | NORMAL |
| Peasant Human1 MaleB | 361, 362, 363 | NORMAL |
| Peasant Human1 MaleC | 364, 365, 366 | NORMAL |
| Peasant Human2 MaleA | 367, 368, 369 | NORMAL |
| Peasant Human2 MaleB | 370, 371, 372 | NORMAL |
| Peasant Human2 MaleC | 373, 374, 375 | NORMAL |
| Peasant Human2 FemaleA | 376, 377, 378 | NORMAL |
| Peasant Human2 FemaleB | 379, 380, 381 | NORMAL |
| Peasant Human2 FemaleC | 382, 383, 384 | NORMAL |
| Rat | 385, 386, 387 | NORMAL |
| Robot | 388, 389, 390 | NORMAL |
| Roc | 391, 392, 393 | NORMAL |
| Sea Monster | 394, 395, 396 | NORMAL |
| Skeleton Warrior | 397, 398, 399 | NORMAL |
| Spider | 400, 401, 402 | NORMAL |
| Swordsman | 403, 404, 405 | NORMAL |
| Thief | 406, 407, 408 | NORMAL |
| Titan | 409, 410, 411 | GIANT |
| Troglodyte | 412, 413, 414 | NORMAL |
| Vampire | 415, 416, 417 | NORMAL |
| Warlock | 418, 419, 420 | NORMAL |
| Wight | 421, 422, 423 | NORMAL |
| AWyvern | 424, 425, 426 | NORMAL |
| Zombie | 427, 428, 429 | NORMAL |
| Peasant Goblin female A | 430, 431, 432 | NORMAL |
| Peasant Goblin female B | 433, 434, 435 | NORMAL |
| Peasant Goblin female C | 436, 437, 438 | NORMAL |
| Peasant Goblin Male A | 439, 440, 441 | NORMAL |
| Peasant Goblin Male B | 442, 443, 444 | NORMAL |
| Peasant Goblin Male C | 445, 446, 447 | NORMAL |
| ATroll | 448, 449, 450 | NORMAL |
| Treant | 451, 452, 453 | NORMAL |
| Ghoul | 454, 455, 456 | NORMAL |
| zBlasterGuy | 457, 458, 459 | NORMAL |
| zUltra Dragon | 460, 461, 462 | GIANT |
| zcat a | 463 | NORMAL |
| zcat b | 464 | NORMAL |
| zcat c | 465 | NORMAL |
| zchicken a | 466 | NORMAL |
| zchicken b | 467 | NORMAL |
| zchicken c | 468 | NORMAL |
| zdog a | 469 | NORMAL |
| zdog b | 470 | NORMAL |
| zdog c | 471 | NORMAL |
| zrat a | 472 | NORMAL |
| zrat b | 473 | NORMAL |
| zrat c | 474 | NORMAL |
| BArcher | 475, 476, 477 | NORMAL |
| Barbarian | 478, 479, 480 | NORMAL |
| BBat | 481, 482, 483 | NORMAL |
| BBeholder | 484, 485, 486 | NORMAL |
| Bloodsucker | 487, 488, 489 | NORMAL |
| Cleric | 490, 491, 492 | NORMAL |
| Cobra | 493, 494, 495 | NORMAL |
| Cockatrice | 496, 497, 498 | NORMAL |
| DemonFly | 499, 500, 501 | NORMAL |
| Demon | 502, 503, 504 | NORMAL |
| DragonCave | 505, 506, 507 | NORMAL |
| BDragonFly | 508, 509, 510 | NORMAL |
| DragonLand | 511, 512, 513 | NORMAL |
| DragonCover | 514, 515, 516 | NORMAL |
| Druidess | 517, 518, 519 | NORMAL |
| BDwarf | 520, 521, 522 | NORMAL |
| ElemAir | 523, 524, 525 | NORMAL |
| ElemEarth | 526, 527, 528 | NORMAL |
| ElemFire | 529, 530, 531 | NORMAL |
| ElemWater | 532, 533, 534 | NORMAL |
| FighterChain | 535, 536, 537 | NORMAL |
| FighterLeath | 538, 539, 540 | NORMAL |
| BGargoyle | 541, 542, 543 | NORMAL |
| BGenie | 544, 545, 546 | NORMAL |
| BGhost | 547, 548, 549 | NORMAL |
| BGoblin | 550, 551, 552 | NORMAL |
| Guard | 553, 554, 555 | NORMAL |
| BHarpy | 556, 557, 558 | NORMAL |
| BHydra | 559, 560, 561 | GIANT |
| Jackalman | 562, 563, 564 | NORMAL |
| KnightPlate | 565, 566, 567 | NORMAL |
| BLich | 568, 569, 570 | NORMAL |
| LizardArch | 571, 572, 573 | NORMAL |
| BMedusa | 574, 575, 576 | NORMAL |
| Merchant | 577, 578, 579 | NORMAL |
| BMinotaur | 580, 581, 582 | NORMAL |
| BMonk | 583, 584, 585 | NORMAL |
| Nobleman | 586, 587, 588 | NORMAL |
| BOoze | 589, 590, 591 | NORMAL |
| Ogre | 592, 593, 594 | NORMAL |
| PeasantF1 | 595, 596 | NORMAL |
| Peasantf1 | 597 | NORMAL |
| PeasantF2 | 598, 599, 600 | NORMAL |
| PeasantF3 | 601, 602, 603 | NORMAL |
| PeasantF4 | 604, 605, 606 | NORMAL |
| PeasantM1 | 607, 608, 609 | NORMAL |
| PeasantM2 | 610, 611, 612 | NORMAL |
| PeasantM3 | 613, 614, 615 | NORMAL |
| PeasantM4 | 616, 617, 618 | NORMAL |
| BRat | 619, 620, 621 | NORMAL |
| BRobot | 622, 623, 624 | NORMAL |
| SeaSerpent | 625, 626, 627 | NORMAL |
| Skeleton | 628, 629, 630 | NORMAL |
| Sorcerer | 631, 632, 633 | NORMAL |
| BSpider | 634, 635, 636 | NORMAL |
| BThief | 637, 638, 639 | NORMAL |
| BTitan | 640, 641, 642 | GIANT |
| Werewolf | 643, 644, 645 | NORMAL |
| zDemonQueen | 646 | NORMAL |
| zReactor | 647, 648, 652 | NORMAL |
| Cactus | 649, 650, 651 | NORMAL |
| custom:crusader_green | 901 | NORMAL |
| custom:crusader_blue | 902 | NORMAL |
| custom:crusader_red | 903 | NORMAL |
| custom:pmn2_red | 904 | NORMAL |
| custom:pmn2_green | 905 | NORMAL |
| custom:pmn2_blue | 906 | NORMAL |

Hydra/Titan/Golem и другие merged-only ресурсы не становятся существами кампании MM8
от включения в общий реестр: происхождение/размещения описаны в [финальной ревизии](final-review.md).
Она является историческим исследованием до принятия v0.4. В частности, Juggernaut/Crystal Walker/
Malevolent Boulder — самостоятельные шаблоны, а не переименования общего Golem.
