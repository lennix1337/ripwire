### Changed — `--path` lists the other definitions of an endpoint whose pick is not provably unique

`--path=fetch,compose` on hono printed `from_defs="7"`, bound one `fetch` and named none of the others, so an agent had to guess a
`file:name` spelling and retry. When an endpoint's name binds two or more definition groups, the answer (CLI and MCP
`path_between`) now ends with `<alts side="from|to" of=NAME>`: one compact row per other definition with `p=`, `class=` and a
pasteable `sel=`. Up to 16 rows; past that `alts_shown=K defs=N next=--defs=NAME` pages the rest. An endpoint that binds one
definition, or a `*` glob endpoint, adds nothing.
