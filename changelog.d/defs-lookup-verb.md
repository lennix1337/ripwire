### Added — `--defs=NAME[,NAME…]`: which definitions a name binds, with the spelling that picks each one alone

One call answers "which definitions does this name bind?" for up to 16 names in one index load. Each definition group (a C-family
prototype with the definition it declares is one; a TypeScript overload with its implementation is one) is a row with its
owner-qualified name, kind, `file:line`, language, `class=source|test` and its distinct graph callers (a floor), plus `sel=`: a
spelling every symbol verb accepts that the lookup re-resolved and proved binds that definition and nothing else (`name`,
`file:name`, `file:Owner.name`, then the `@file:line` seed; `sel_proven="0"` when none does). Source rows come first, then by
callers. A JSON/YAML key or a markdown heading of the same name is counted as `data_defs=` and never listed as code; a definition
the parser does not model (an assignment, `define_method`, `setattr`) is not one. More than 16 rows page with `--offset`; a single
name that binds nothing answers with the shared "not found" refusal. `--json` gives the same rows. The lookup lives in one header
(`src/defslookup.h`, `rw::lookupDefs`), reusing the definition-group rule the callers/impact tables use.
