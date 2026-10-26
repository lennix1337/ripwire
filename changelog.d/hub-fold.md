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
