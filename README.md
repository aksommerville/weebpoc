# weebpoc

2024-06-14

Sketching some things out in preparation for js13k, which I plan to participate in this year for the first time.

What does it take to write a web game that packs into a ZIP file smaller than 13 kB?

## Basic operation.

Give `min` an HTML template and it produces a single flattened and minified HTML file.

`<img>` and `<script>` tags can refer to relative files, and get expanded in place.

I'm not doing the same for `<link rel="stylesheet">`, only because I expect there to be only a very small amount of CSS.
Write your CSS directly in the template, and we'll minify it in place.

## Standard Violations

No doubt we're going to violate standards in hundreds of ways I don't even notice.
But I'll list what I know.

- HTML whitespace may be eliminated. (fore and aft, or whitespace-only nodes)
- HTML singleton tags *must* be marked with a trailing slash.
- Regex literals will be restricted (TODO determine exactly how)
- All globals must use 'window.' in the input.
- All instance members must be initialized in the constructor.
- Non-G0 characters in identifiers will be treated as punctuation.
- Order of static initialization is nondeterministic. Don't depend on static initialization.
- Imports must be phrased `import { IDENTIFIERS... } from "RELATIVE_PATH";`
- HTML should not contain any real DOM content. We're definitely not doing a full HTML parser.
- CDATA sections are not acknowledged.
- HTML tag and attribute names must be lowercase.
- HTML character entities pass thru as text, but must not appear in anything machine-readable. (eg `<script src>`)
- Extra attributes on link, style, script, or img tags will usually be lost when we rewrite them.
- HTML attributes must be phrased like XML: `key="value"`, no space, double-quote only.
- <script> tags can't share globals directly. You have to explicitly assign things to window to share them.
- Imports must end with a string, and the string is all we care about. No default or renaming of imports.
- Dotted member references must not have space or comments between the dot and the key.
- Class declarations only permitted at top level of a file.
- No async/await.
- No generator functions.
- No "var".
- Semicolons are mandatory.

...fuck it, this is crazy. I can't write a full AST minifier for Javascript.
