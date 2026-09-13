-- Level-1 DK base stats: supply only missing rows, using this realm's warrior progression.
-- ObjectMgr adds race stat modifiers itself. Existing DK tuning and level 55+ stay intact.
-- No playercreateinfo, quest, spell_area, IP, auth or existing character rows are changed.
INSERT INTO `player_class_stats`
    (`Class`, `Level`, `BaseHP`, `BaseMana`, `Strength`, `Agility`, `Stamina`, `Intellect`, `Spirit`)
SELECT 6, `w`.`Level`, `w`.`BaseHP`, 0, `w`.`Strength`, `w`.`Agility`, `w`.`Stamina`, `w`.`Intellect`, `w`.`Spirit`
FROM `player_class_stats` AS `w`
LEFT JOIN `player_class_stats` AS `dk` ON `dk`.`Class` = 6 AND `dk`.`Level` = `w`.`Level`
WHERE `w`.`Class` = 1 AND `w`.`Level` BETWEEN 1 AND 54 AND `dk`.`Class` IS NULL;

-- Module-owned world mentor. Retain stock trainer 13, including IP modifications to its spells.
INSERT INTO `creature_template`
    (`entry`, `name`, `subname`, `minlevel`, `maxlevel`, `faction`, `npcflag`, `unit_class`, `type`,
     `unit_flags`, `ScriptName`)
VALUES (900120, 'Runeblade Mentor', 'Death Knight Trainer', 60, 60, 35, 17, 1, 7, 2,
    'npc_roleplay_death_knight_trainer')
ON DUPLICATE KEY UPDATE `name` = VALUES(`name`), `subname` = VALUES(`subname`),
    `minlevel` = VALUES(`minlevel`), `maxlevel` = VALUES(`maxlevel`), `faction` = VALUES(`faction`),
    `npcflag` = VALUES(`npcflag`), `ScriptName` = VALUES(`ScriptName`);

DELETE FROM `creature_template_model` WHERE `CreatureID` = 900120;
INSERT INTO `creature_template_model` (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`)
SELECT 900120, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`
FROM `creature_template_model` WHERE `CreatureID` = 28472;

DELETE FROM `creature_default_trainer` WHERE `CreatureId` = 900120;
INSERT INTO `creature_default_trainer` (`CreatureId`, `TrainerId`) VALUES (900120, 13);

DELETE FROM `trainer_spell` WHERE `TrainerId` = 13 AND `SpellId` IN
    (45477, 45462, 45902, 47541, 49576, 48266, 674);
INSERT INTO `trainer_spell`
    (`TrainerId`, `SpellId`, `MoneyCost`, `ReqSkillLine`, `ReqSkillRank`, `ReqAbility1`, `ReqAbility2`, `ReqAbility3`, `ReqLevel`)
VALUES
    (13, 45477, 10, 0, 0, 0, 0, 0, 1),
    (13, 45462, 10, 0, 0, 0, 0, 0, 1),
    (13, 45902, 100, 0, 0, 0, 0, 0, 4),
    (13, 47541, 500, 0, 0, 0, 0, 0, 6),
    (13, 49576, 1000, 0, 0, 0, 0, 0, 8),
    (13, 48266, 2000, 0, 0, 0, 0, 0, 10),
    (13, 674, 300, 0, 0, 0, 0, 0, 10);

-- Let MySQL allocate spawn GUIDs. Delete only our template's spawns on a reinstall.
DELETE FROM `creature` WHERE `id1` = 900120;
INSERT INTO `creature`
    (`id1`, `map`, `zoneId`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`)
SELECT DISTINCT 900120, `map`, `zone`, 1, `position_x` + 3, `position_y` + 2, `position_z`, `orientation`, 120
FROM `playercreateinfo`
WHERE (`class` = 1 AND `race` IN (1, 2, 3, 4, 5, 6, 7, 8, 11)) OR (`class` = 2 AND `race` = 10);

INSERT INTO `creature`
    (`id1`, `map`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`)
VALUES
(900120, 0, 1, -8824.0, 628.0, 94.1, 3.4, 120),
(900120, 0, 1, -4921.0, -953.0, 501.5, 5.4, 120),
(900120, 1, 1, 1597.0, -4409.0, 8.1, 4.0, 120),
(900120, 0, 1, 1639.0, 240.0, -43.1, 3.1, 120),
(900120, 1, 1, 9951.0, 2280.0, 1341.4, 3.0, 120),
(900120, 1, 1, -1276.0, 123.0, 131.3, 5.0, 120),
(900120, 530, 1, -4007.0, -11878.0, -1.5, 1.0, 120),
(900120, 530, 1, 9472.0, -7275.0, 14.3, 4.0, 120);
