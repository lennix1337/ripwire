### Added — hedged hops compose: `--impact` counts who reaches a hedged caller, tables called by index join both walks

`--impact`'s `reaches_hedged=` (the functions that MAY reach the answer through a call the graph holds no edge for) now
composes. It adds every function that reaches one of those hedged callers through ordinary proven calls (only past hops
whose join names at most 16 calling functions), takes a third struct-field hop when every hop on the chain is a field call
(`<ring d="3">`, `<rh … d="3">`), and follows a new hop kind, `container`: a call through a table element
(`handlers[i]()`, or a parameter's default), the caller that `--callers` already printed as `called_by=`. Each `<rh>` row now prints its
call's forward width (`slot_fns=` for a field, `cands=` for a container). On tmux, `--impact=cmd_split_window_exec` went
from `reaches_hedged="1"` to `"10"` (cmdq_fire_command and its 9 proven callers) and `--impact=window_copy_cmd_cancel`
from 2 to 12; `reaches=` did not change on any of the 235 slot-bound functions measured. `--path`'s hedged `<bridge>` crosses
a container call (`via="container" cands=K`, at most 16) and lets a declined or container crossing with at most 4
candidates be one of the three crossings a slot-only chain could already take.

What it does NOT mean: every one of these is a hedge. A chain with one hedged hop is hedged, so none of them is ever
counted in `reaches=`, `reachable=`, `hops=`, `by_depth=` or `radius_*`, and `<bridge proven="0">` stays. A wider crossing
is not followed and nothing says "not reached" because of it. Gate: `test/composedhopscheck.sh`.

### Changed — `--impact` folds a hub in the hedged ring and orders the rows that stay

A hedged ring member with more than 16 proven callers behind it now prints one row instead of leaving its tail implicit:
`<hub n= via= d= behind= tested= sample= next="--impact=file:member">` (behind= the functions of `reaches_hedged=` that sit
behind it, sample= the first three by the ring order, next= lists them as proven rows). The root says how much is folded
(`reaches_hedged_hubs=K`) and its `next=` points at the widest hub when hubs hold more than half of `reaches_hedged=`
(tmux `--impact=window_copy_formats`: 779 of 780 behind `format_defaults_pane`). The `<rh>` rows (and the samples) are
ordered: one hedged hop before composed, narrow crossings first, then tested, entry points, the answer's own directory and
rank. Under an explicit `--limit`, the proven `<s>` rows keep the page and hedged rows are cut first, disclosed as
`ring_cut=N` with the replay in `next=`. What it does NOT mean: the fold changes rows, never counts (`reaches=` and
`reaches_hedged=` did not move on any of the 235 slot-bound functions measured), and behind= is still a hedge, never a
proven caller of the answer. Gate: `test/composedhopscheck.sh` (H1, O1, W1).
