### Added — a `--for` task that names a literal carries the literal's sites and the code that handles it

A `--for` task that carries a "double-quoted message" (two words or more) or a CONST_SHAPED word (an environment variable, an
error code or a setting name such as `FZF_DEFAULT_COMMAND`, `FST_ERR_DUPLICATED_ROUTE`, `PYTHONASYNCIODEBUG`) now ends with a
`<lit>` element, just before `</ctx>`: the grep verb's byte-exact scan of the index for that literal. Its `<ls>` rows are the
sites in source files, labelled by the parse (`k="string"` inside a string literal, `k="code"` outside one, `k="?"` a file not
classified) with the definition holding each (`in=`, `ink=`), sites inside a function first; comment and docstring sites, test
paths and doc, shell or config files are counted (`comment=`, `test=`, `doc=`), never listed as a handler. The function holding a
shown site is printed whole (`<b>`, at most three), or as an excerpt around the site when its body is over 3,000 bytes (`<x>`,
`next=` the whole body); a body the answer already printed is not repeated (`body="above"`). Several sites are always listed
with counts; a literal with no source site (found only in tests, docs or comments, or not at all) carries no block. The element carries its own legend and its size (`bytes=`, not in `est_tokens`);
everything before it is byte-identical to the answer without it (`RIPWIRE_NO_LITERAL_ANCHOR=1`); `--json`, `--columnar`,
`--candidates` and an explicit `--token-budget` carry none. A `--flag` spelling is not read as a literal. Gate:
`test/forliteralcheck.sh`.
