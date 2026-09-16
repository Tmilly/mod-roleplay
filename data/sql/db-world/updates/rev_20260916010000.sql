-- Retire the module-owned Runeblade Mentor: managed DK progression is learned automatically.
-- Keep shared trainer 13 and its spell rows intact for normal Death Knight trainers.
DELETE FROM `creature` WHERE `id1` = 900120;
DELETE FROM `creature_default_trainer` WHERE `CreatureId` = 900120;
DELETE FROM `creature_template_model` WHERE `CreatureID` = 900120;
DELETE FROM `creature_template` WHERE `entry` = 900120;
