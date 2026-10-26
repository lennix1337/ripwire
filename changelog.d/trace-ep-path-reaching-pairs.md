### Changed — `--path` lists every reaching definition pair when a name has several definitions

`--path=A,B` used to print the first chain one search met when `A` or `B` names several definitions, with nothing to say
that another pair also reaches: `--path=Loop,extendedMatch` on fzf printed `Terminal.Loop`'s 4 hops while `Matcher.Loop`
reaches `extendedMatch` in 5 proven hops. When two or more definition pairs reach, the answer now carries
`pairs_reaching=N` and prints up to three `<pair hops= hedged=>` chains: an all-proven chain before any hedged one, then
fewest hedged hops, then fewest hops, in the order the endpoint tie-break of #432 (thanks @mpapis) set for `--connect`, which
now shares the one rank. `hedged=` counts the chain's `via=`/`prov="split"`/`anchor="missing"` rows, so a chain bound by name
never reads as proven. A to= definition no chain reaches may add one hedged bridge (`bridge="1"`). More than three pairs add
`alt=N`, one `<alt>` row per alternative and `next=` running the first; a chain whose tail repeats an earlier one stops where
it meets it (`joins=J`). The chain the old answer printed is always among the printed ones. One reaching pair, no reaching
pair and a glob endpoint keep their old bytes; MCP `path_between` prints the same rows. What it does NOT mean: the order is
not a claim that the first pair is the one meant — it lists, it does not pick. Measured on the 163 non-sealed rows: see
the lane report; 9 of the 15 stored `--path` argvs change.
