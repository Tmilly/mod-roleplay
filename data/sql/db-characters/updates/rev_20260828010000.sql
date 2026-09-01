CREATE TABLE IF NOT EXISTS `rp_recruit` (
  `id` bigint unsigned NOT NULL AUTO_INCREMENT,
  `owner_guid` int unsigned NOT NULL,
  `creature_entry` int unsigned NOT NULL,
  `rp_name` varchar(64) NOT NULL DEFAULT '',
  `map_id` smallint unsigned NOT NULL,
  `position_x` float NOT NULL,
  `position_y` float NOT NULL,
  `position_z` float NOT NULL,
  `orientation` float NOT NULL,
  `behavior_state` tinyint unsigned NOT NULL DEFAULT '0',
  `lifecycle_state` tinyint unsigned NOT NULL DEFAULT '0',
  `creation_token` bigint unsigned NOT NULL,
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `died_at` timestamp NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uq_rp_recruit_creation_token` (`creation_token`),
  KEY `idx_rp_recruit_owner_state` (`owner_guid`, `lifecycle_state`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `rp_outfit` (
  `id` bigint unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(64) NOT NULL,
  `creator_guid` int unsigned NOT NULL,
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uq_rp_outfit_name` (`name`),
  KEY `idx_rp_outfit_creator` (`creator_guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `rp_outfit_slot` (
  `outfit_id` bigint unsigned NOT NULL,
  `equipment_slot` tinyint unsigned NOT NULL,
  `appearance_entry` int unsigned NOT NULL,
  `operation` tinyint unsigned NOT NULL DEFAULT '1',
  PRIMARY KEY (`outfit_id`, `equipment_slot`),
  CONSTRAINT `fk_rp_outfit_slot_outfit` FOREIGN KEY (`outfit_id`)
    REFERENCES `rp_outfit` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `rp_playerbot_outfit` (
  `playerbot_guid` int unsigned NOT NULL,
  `outfit_id` bigint unsigned NOT NULL,
  `assigned_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`playerbot_guid`),
  KEY `idx_rp_playerbot_outfit_id` (`outfit_id`),
  CONSTRAINT `fk_rp_playerbot_outfit_outfit` FOREIGN KEY (`outfit_id`)
    REFERENCES `rp_outfit` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
