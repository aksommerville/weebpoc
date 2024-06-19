# weebpoc

2024-06-14

Sketching some things out in preparation for js13k, which I plan to participate in this year for the first time.

What does it take to write a web game that packs into a ZIP file smaller than 13 kB?

## Basic operation.

Give `min` an HTML template and it produces a single flattened and minified HTML file.

`<img>` and `<script>` tags can refer to relative files, and get expanded in place.

For the Javascript minifier, I'm using a fairly light touch:
- Whitespace and comments are eliminated, obviously.
- Imports are inlined in the order we find them. Don't depend on sequencing of a file's initial run.
- Most identifiers are preserved but aliased with something shorter.
- - eg `this.abra.cadabra();` becomes `const [a,b]="abra,cadabra".split(); ... this[a][b]();`.
- If the first appearance of an identifier is an obvious declaration (const, let, class, function), we replace its name hard.

TODO: We're not able to replace function parameters. What would that take?

We're not doing any generic tree-shaking. It does happen at the file level -- if you never import a file, it gets dropped.

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
