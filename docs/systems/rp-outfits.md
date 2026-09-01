# Sparse RP outfits

Begin with `.rp outfit begin "Name"`, use the installed mod-transmog NPC, then `.rp outfit save`. The module records
the effective appearance baseline and saves only changed supported armor slots: head, shoulders, shirt, chest,
waist, legs, feet, wrists, hands, back and tabard. Weapons are excluded; absent slots are unmanaged.

Commands: `begin`, `save`, `cancel`, `list`, `info`, `apply`, `assign`, `clear`, and `delete`. `apply` is one-time;
`assign` persists a Playerbot GUID association. Login, equip and post-visible-slot hooks reapply controlled slots
through mod-transmog's `GetFakeEntry`/`SetFakeEntry` APIs, so gear and stats remain untouched and upgrades retain
the uniform. Targets must be loaded Playerbots; ordinary players and RP recruits are unsupported.

Tables: `rp_outfit`, `rp_outfit_slot`, `rp_playerbot_outfit`. `clear` and deletion leave current visuals in place.
No explicit hidden/clear delta is captured. Definitions are globally named/shared, with creator-only overwrite and
delete. Source: `rp_phase1.cpp`; migration: `rev_20260828010000.sql`.

