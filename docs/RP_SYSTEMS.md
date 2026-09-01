# RP systems

`mod-roleplay` currently contains four independent systems: Lyon's carried Shadow Wand relic, sparse visual
outfits for Playerbots, mortal owner-bound recruit creatures, and Dorrin's Ironbelly Cooking recipes. Their
implementations remain separated in `lyon_shadow.cpp`, `rp_phase1.cpp`, and `ironbelly_cooking.cpp`.

The outfit and recruit commands are player-level. Recruit ownership is enforced; outfit definitions are shared,
but only their creator can overwrite or delete them. Ironbelly learning is player-level but name-gated to Dorrin.
