DELETE FROM `item_template`
WHERE (`entry` = 900000);

INSERT INTO `item_template` (
    `entry`,
    `class`,
    `subclass`,
    `SoundOverrideSubclass`,
    `name`,
    `displayid`,
    `Quality`,
    `Flags`,
    `BuyCount`,
    `BuyPrice`,
    `SellPrice`,
    `InventoryType`,
    `AllowableClass`,
    `AllowableRace`,
    `ItemLevel`,
    `RequiredLevel`,
    `maxcount`,
    `stackable`,
    `bonding`,
    `description`,
    `Material`,
    `VerifiedBuild`
)
VALUES (
    900000,
    2,          -- Weapon
    19,         -- Wand
    -1,
    'Shadow Wand',
    18356,
    4,          -- Epic quality
    32,
    1,
    0,
    0,
    26,         -- Ranged / Wand inventory type
    -1,
    -1,
    1,
    1,
    1,
    1,
    1,
    'Cold to the touch. Something within refuses to let go.',
    2,
    12340
);
