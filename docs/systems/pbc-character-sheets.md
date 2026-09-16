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

## Parsing and regression checks

The command remains available at `SEC_PLAYER`. Its registration uses `ChatCommands::Tail`: a typed
`std::string_view` argument consumes only one token and rejects the remaining words before the handler runs.
Only the sheet registration uses the new adapter. The internal parser still trims outer whitespace and
preserves internal whitespace, punctuation, and UTF-8 bytes. Missing/invalid operations, missing text, and
extra arguments on `create`, `show`, or `clear` print usage before resolving a card or changing files.
The client/core chat-message limit still applies; the 4,000-byte edit cap does not extend that limit.
The existing edit, file and display limits are measured in bytes, rather than Unicode characters.

Run `powershell -NoProfile -ExecutionPolicy Bypass -File tests/run-focused-tests.ps1` from this module on
Windows with MSVC installed. Override `-Core` and `-VsDevCmd` for another checkout/toolchain location.
The runner extracts production functions and the handler named by the actual `SEC_PLAYER` registration,
then invokes it through the installed core's real `CommandInvoker`/`Tail` implementation. Session lookup,
configured path resolution, starter-card metadata and display are test doubles. Real card I/O uses a new
temporary directory, never `PBC.CharacterCardsPath`. It also proves the old direct registration rejects
multiword/named commands. This is an isolated regression test, not a worldserver integration test.

In a test server, select a disposable online Playerbot named Testbot and run:

```text
.rp bot sheet create
.rp bot sheet show
.rp bot sheet append Background: A veteran of Lordaeron.
.rp bot sheet append Personality: Reserved, loyal, and stubborn.
.rp bot sheet set Name: Testbot. Background: A former soldier.
.rp bot sheet clear
```

Repeat with `Testbot` before every operation and another target selected. Repeat create and append; verify
create leaves existing text intact and append adds another copy. Append punctuation and UTF-8 text such as
`Quirks: café, 你好! 'Yes'; (no).` and verify the file bytes. Verify set/clear preserve the previous contents
in `.bak`. Try an empty command, a name without an operation, an unknown operation/name, missing append/set
text, and trailing text on create/show/clear; confirm no card or backup changes. Check configured paths,
filename rejection, display truncation, and `.chars reload` using only the disposable card. Do not send
overlong packets to bypass normal chat limits.

Other direct `std::string_view` registrations in recruit/outfit/appearance commands may have the same
argument-consumption issue. They are outside this repair and need separate investigation.
