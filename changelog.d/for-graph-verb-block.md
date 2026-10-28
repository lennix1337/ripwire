### Added — a `--for` task spelled as a graph question also gets that verb's answer

A `--for` (and MCP `for`) task written as `path from A to B`, `callers of X` or `common callers of A and B` now ends with the
`--path=A,B`, `--callers=X` or `--connect=A,B` answer, run on the index the `--for` run already built, inside
`<graph_verb endpoints="subject" verb=… from_span= to_span=|of_span= [and_span=] [tried=]>` just before `</ctx>`. The endpoints
are read from the task's own words: each span is cut at its first qualifier word (for/with/via/in/on/when/using/during/at/near)
and its code-like tokens (a `.`, `_`, `::`, `()` or an inner capital) are tried last-first, at most four combinations, the first
that the verb's own resolver finds runs; no plain word is an endpoint except `main`, so a description ("serialising a dict",
"spawning ripgrep") never fires. The block's legend says the endpoints are a name match read from prose. Everything before the
block is byte-identical to the answer without it, and nothing is appended when no combination resolves or the task is not one
of the three forms. The verb runs with its default caps and next= paging, so the block is the bare call's own bytes (CLI:
byte-identical, compact legend included). Measured on the LARGE-DEV rows: 7 rows move a tier (5 to complete), 13 of 15 missing
chain items recovered, none lost; the parser fires on 0 of 258 non-graph subjects. Gate: `test/forroutecheck.sh`.
