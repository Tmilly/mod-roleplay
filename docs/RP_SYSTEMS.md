# RP systems

`mod-roleplay` contains Lyon's Shadow Wand, sparse Playerbot outfits, mortal recruit creatures, Ironbelly Cooking,
validated Playerbot gender/appearance controls, and a safe in-game editor for PBC character cards. Their
implementations remain separated in `lyon_shadow.cpp`, `rp_phase1.cpp`, `rp_bot.cpp`, and `ironbelly_cooking.cpp`.

The outfit and recruit commands are player-level. Recruit ownership is enforced; outfit definitions are shared,
but only their creator can overwrite or delete them. Ironbelly learning is player-level but name-gated to Dorrin.
Playerbot customization and sheet commands also use player security. They require a currently loaded Playerbot,
but deliberately do not impose ownership or account checks.
