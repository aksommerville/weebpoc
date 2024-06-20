# weebpoc

2024-06-14

Sketching some things out in preparation for js13k, which I plan to participate in this year for the first time.

What does it take to write a web game that packs into a ZIP file smaller than 13 kB?

## Basic operation.

Give `min` an HTML template and it produces a single flattened and minified HTML file.

`<img>` and `<script>` tags can refer to relative files, and get expanded in place.

The Javascript minifier inlines all imports and removes whitespace.
That's about it.
You're on the hook for keeping identifiers small, and keeping things in general small.

- [ ] It remains possible to track identifiers and inline simple global constants, I think. That could be a pretty big reduction.

Use comments `/*IGNORE{*/` and `/*}IGNORE*/` to fence portions of code that should be dropped during minification.
That's needed for the synthesizer, if you want it to work pre-minify too.

We're not doing any generic tree-shaking. It does happen at the file level -- if you never import a file, it gets dropped.

Assuming that there will be just one `<img>`.
If you need multiple, you'll have to give them IDs or something.
That may require a change to the HTML minifier, to preserve the IDs.

## Audio

Background music should be sourced from MIDI files.
Each channel is a distinct hard-coded instrument.
Aftertouch, pitch wheel, program changes, and control changes are all discarded.

Include each song in the HTML: `<song href="./mySong.mid"></song>`

Running your app from source, you'll need conditional logic to turn MIDI into our format. See src/www/Audio.js.

After minification, the song tags turn into `<song name="mySong">...COMPILED SONG DATA...</song>`

Compiled songs are plain text.
Whitespace is unused, you must strip it before feeding to the synthesizer.

0x27..0x37 `'()*+,-./01234567` are opcodes.
0x3f..0x7e are data bytes (0..63).

Starts with one data byte for the tempo: N+1 is ms per tick, 1..64.

Apostophe followed by a data byte is a delay, for N+1 ticks.

Other opcode bytes correspond to one MIDI channel, and indicate one note on that channel.
`OPCODE NOTEID DURATION` where NOTEID and DURATION are each one data byte.
NOTEID, add 0x23 for the MIDI note id. Range is B1..D7. (0x23 happens to be the start of GM drums, in case that ever comes up).

## Standard Violations

No doubt we're going to violate standards in hundreds of ways I don't even notice.
But I'll list what I know.

- *HTML*
- - Leading and trailing whitespace may be eliminated from text nodes.
- - Singleton tags must be marked with a trailing slash.
- - Behavior around a robust DOM is undefined. I'm assuming there will be minimal HTML.
- - CDATA not supported.
- - Tag and attribute names must be lower-case.
- - Character entities not permitted in anything machine-readable.
- - Extra attributes on `link`, `style`, `script`, and `img` may be lost.
- *CSS*
- - TODO Identify violations.
- *JS*
- - Identifiers must be `/[a-zA-Z_$][0-9a-zA-Z_$]*/`.
- - `export` is ignored (everything is implicitly exported).
- - `import` must be phrased `import { IDENTIFIERS... } from "RELATIVE_PATH";`. (No "as" and no external paths).
- - String escapes not permitted in import path.
