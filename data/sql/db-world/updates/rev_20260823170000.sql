INSERT INTO `spell_custom_attr` (`spell_id`, `attributes`)
VALUES (33335, 0x10)
ON DUPLICATE KEY UPDATE `attributes` = `attributes` | VALUES(`attributes`);
