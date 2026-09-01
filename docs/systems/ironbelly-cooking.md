# Ironbelly Cooking Phase 1

The skillet is item `901000`, a Bind-on-Equip armor/misc held-offhand item with verified Frying Pan display
`28866`. This avoids granting Dorrin a weapon proficiency. It is tradeable before equip and remains a reusable tool
in equipped inventory, backpack, or normal bags through dedicated TotemCategory `901`.

Run `.rp ironbelly learn` as the configured character (default Dorrin) after learning Cooking. This grants spells
`901100-901107`; no recipe scrolls or discovery tracking exist. Recipes are native Cooking skill-line rows, so core
crafting validates output storage before consuming reagents and never consumes the skillet.

| Spell | Skill | Dish | Ingredients |
|---:|---:|---|---|
| 901100 | 125 | Spiced Raptor Haunch (`901001`) | Raptor Flesh `12184` x2, Hot Spices `2692` x1 |
| 901101 | 100 | Crocolisk Hunter's Stew (`901002`) | Crocolisk Meat `2924` x2, Mild Spices `2678` x1 |
| 901102 | 100 | Spider Leg Broth (`901003`) | Gooey Spider Leg `2251` x2, Spring Water `159` x1 |
| 901103 | 125 | Dwarven Trail Supper (`901004`) | Bear Meat `3173` x2, Mild Spices `2678` x1 |
| 901104 | 75 | Boar & Ale Fry-Up (`901005`) | Chunk of Boar Meat `769` x2, Flask of Port `2593` x1 |
| 901105 | 100 | Pan-Seared Field Catch (`901006`) | Raw Bristle Whisker Catfish `6308` x2, Mild Spices x1 |
| 901106 | 125 | Buzzard Field Skewer (`901007`) | Buzzard Wing `3404` x2, Hot Spices x1 |
| 901107 | 150 | Mystery Meat Surprise (`901008`) | Mystery Meat `12037` x2, Hot Spices x1 |

Phase 1 dishes use verified level-35 Food spell `5007`: health restoration plus the modest stamina/spirit Well Fed
effect `19709`. Boar & Ale has a 3% and Mystery Meat Surprise a 5% chance to trigger verified alcohol spell `11008`;
the reaction is temporary and harmless. Distinct stat variants and the optional field kitchen remain Phase 1.x.
