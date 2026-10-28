### Fixed — name-key hops: the class family holds in JS/TS and across files, and a wide or d=1-only lookup says so

Four answers about the name-keyed hop (`via="name-key"`) were wrong or silent, found in review. (1) A JavaScript or TypeScript
`this[ 'handle' + e ]` named every `handle*` method of the tree, not the class family it claimed (`cands=` and
`reaches_hedged=` were inflated); the family is now the class definition that encloses the call, its ancestors and its subclasses.
(2) Two classes of the same name in different files (every Django `Command`) counted as one family; a class is now told apart by its
definition, so `a.py`'s `Shell` no longer reaches `b.py`'s `Shell.do_rm`. (3) A lookup naming more than 16 definitions is not crossed
by the `--path` bridge, and the answer said nothing: the sink's `<near_sink>` row read `why="external"`, "no indexed way in". It now
reads `why="declined:N"` (the word a declined call already uses) with `via="name-key" cands="17"`, and so does a narrow lookup whose
dispatcher the search never reached. (4) `--impact` counts a dispatcher that is a direct name-key hop in `reaches_hedged=` with no
`<rh>` row to explain it; the root now carries `reaches_hedged_via="name-key"` (and the legend clause) exactly then. A `hasattr` test
and a JS/TS store `o[ 'p' + e ] = fn` made no call and no longer draw a hop (they remain `<pd>` clues); a function that holds
only such sites is not a dispatcher. Gate: `test/namekeyhopcheck.sh`.
