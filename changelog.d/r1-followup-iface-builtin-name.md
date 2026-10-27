### Fixed — `--path`: a `.match()` / `.get()` call on an interface-typed field no longer reads as the builtin when its implementors are one or more than sixteen

A call spelled like a builtin method (`this.router.match( … )`, `this.cache.get( … )`) on a field the class states as an in-tree
interface is not read as the builtin when the receiver is typed in the tree (the earlier receiver-typed lift). That lift required two to
sixteen implementors, so two shapes fell back to the builtin reading: an interface with ONE in-tree implementor (no hedged bridge
crossing and no `reachable="1"`, where the by-name edge of earlier builds had given one) and one with more than sixteen (the sink's
`<near_sink>` row said `why="external"`, "no indexed way in", although the declined call is one). Now the one-implementor field crosses
as a hedged `<bridge proven="0">` row `via="declined" cands="1"` (never proven: an open interface may have implementors outside the
tree), and a sink behind a seventeen-wide interface says `why="declined:N"` with the calling definition. A sixteen-wide set keeps its
bridge; a seventeen-wide one still has none (the bridge fans out to at most sixteen). A field typed `string` or outside the tree keeps
the builtin reading. Gate: `test/receiverfixescheck.sh` (F).
