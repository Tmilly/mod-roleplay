DELETE FROM `item_template` WHERE `entry` BETWEEN 901000 AND 901008;
INSERT INTO `item_template`
(`entry`, `class`, `subclass`, `SoundOverrideSubclass`, `name`, `displayid`, `Quality`, `BuyCount`,
 `InventoryType`, `AllowableClass`, `AllowableRace`, `ItemLevel`, `RequiredLevel`, `maxcount`, `stackable`,
 `spellid_1`, `spelltrigger_1`, `spellcharges_1`, `spellcooldown_1`, `spellcategorycooldown_1`, `bonding`,
 `description`, `Material`, `sheath`, `TotemCategory`, `FoodType`, `VerifiedBuild`)
VALUES
(901000, 4, 0, -1, 'The Ironbelly Skillet', 28866, 2, 1, 23, -1, -1, 30, 1, 1, 1,
 0, 0, 0, -1, -1, 2, 'Anything''s food if ye cook it long enough.', 1, 7, 901, 0, 12340),
(901001, 0, 5, -1, 'Spiced Raptor Haunch', 20803, 1, 1, 0, -1, -1, 35, 25, 0, 20,
 5007, 0, -1, -1, -1, 0, 'Best cooked before it starts looking back at ye.', -1, 0, 0, 1, 12340),
(901002, 0, 5, -1, 'Crocolisk Hunter''s Stew', 21473, 1, 1, 0, -1, -1, 35, 25, 0, 20,
 5007, 0, -1, -1, -1, 0, 'Tough enough to chew back.', -1, 0, 0, 1, 12340),
(901003, 0, 5, -1, 'Spider Leg Broth', 6347, 1, 1, 0, -1, -1, 35, 25, 0, 20,
 5007, 0, -1, -1, -1, 0, 'The legs are supposed to float. Probably.', -1, 0, 0, 1, 12340),
(901004, 0, 5, -1, 'Dwarven Trail Supper', 22194, 1, 1, 0, -1, -1, 35, 25, 0, 20,
 5007, 0, -1, -1, -1, 0, 'A proper meal for a long road and a cold watch.', -1, 0, 0, 1, 12340),
(901005, 0, 5, -1, 'Boar & Ale Fry-Up', 21327, 1, 1, 0, -1, -1, 35, 25, 0, 20,
 5007, 0, -1, -1, -1, 0, 'The ale goes in the pan. Most of it.', -1, 0, 0, 1, 12340),
(901006, 0, 5, -1, 'Pan-Seared Field Catch', 7176, 1, 1, 0, -1, -1, 35, 25, 0, 20,
 5007, 0, -1, -1, -1, 0, 'Fresh from the water, more or less.', -1, 0, 0, 1, 12340),
(901007, 0, 5, -1, 'Buzzard Field Skewer', 25469, 1, 1, 0, -1, -1, 35, 25, 0, 20,
 5007, 0, -1, -1, -1, 0, 'Game meat is a broad and useful term.', -1, 0, 0, 1, 12340),
(901008, 0, 5, -1, 'Mystery Meat Surprise', 22198, 1, 1, 0, -1, -1, 35, 25, 0, 20,
 5007, 0, -1, -1, -1, 0, 'The surprise is mostly in the asking.', -1, 0, 0, 1, 12340);

UPDATE `item_template` SET `ScriptName` = 'IronbellyFoodItemScript' WHERE `entry` IN (901005, 901008);
