# PBC character-sheet management

The installed PBC is the `mod-playerbots-characters` checkout. Its configured `PBC.CharacterCardsPath` contains
freeform UTF-8 files named exactly `<CharacterName>.card.txt`. PBC loads those files into memory and expands its
normal template variables. `mod-roleplay` edits those canonical files; it does not implement personality or LLM
behavior itself.

## Commands

The selected target form is:

```text
.rp bot sheet create
.rp bot sheet show
.rp bot sheet append <text>
.rp bot sheet set <text>
.rp bot sheet clear
```

The explicit loaded-bot form is:

```text
.rp bot sheet Dorrin create
.rp bot sheet Dorrin show
.rp bot sheet Dorrin append Personality: Gruff but warm-hearted.
.rp bot sheet Dorrin set <replacement card text>
.rp bot sheet Dorrin clear
```

`create` refuses to overwrite an existing card. Its starter text populates name, race, class, and gender from the
live Playerbot, then leaves Background, Personality, Speaking Style, Motivations, Likes, Dislikes, Quirks, and
Role in the Party blank.

`append` preserves existing contents. `set` and `clear` first copy the current card to
`<Name>.card.txt.bak`; the latest destructive edit replaces that backup. An individual edit is capped at 4,000
characters and a card at 128 KiB. `show` sends 220-character chunks and truncates display after 4,000 characters
to avoid flooding WoW chat; the file itself is not truncated.

Only a name obtained from a validated, currently loaded Playerbot is used. The name must be alphanumeric. The
configured directory is normalized to an absolute path, and the resulting file's parent must equal that directory,
preventing `..`, absolute-path, or arbitrary-file access.

The installed PBC has no declared stable cross-module card-reload API. After create, append, set, or clear, run:

```text
.chars reload
```

That existing command requires GM security or may be run from the worldserver console. Until it runs, PBC keeps
using the previously loaded in-memory card. PBC cards are freeform; the starter headings are a convenience, not a
new file format.
