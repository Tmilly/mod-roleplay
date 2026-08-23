DELETE FROM `spell_dbc` WHERE (`ID` = 900001);
INSERT INTO `spell_dbc` (`ID`, `Attributes`, `CastingTimeIndex`, `RecoveryTime`, `CategoryRecoveryTime`,
    `PowerType`, `ManaCost`, `RangeIndex`, `Speed`, `Effect_1`, `EffectBasePoints_1`, `ImplicitTargetA_1`,
    `SpellVisualID_1`, `SpellIconID`, `Name_Lang_enUS`, `Name_Lang_Mask`, `StartRecoveryCategory`,
    `StartRecoveryTime`, `MaxTargets`, `DefenseType`, `PreventionType`, `SchoolMask`, `EffectBonusMultiplier_1`)
VALUES (900001, 0, 1, 0, 0, 0, 0, 6, 0, 2, 0, 6, 0, 0, 'Death''s Reprisal', 16712190, 0, 0, 1, 1, 1, 32, 0);
