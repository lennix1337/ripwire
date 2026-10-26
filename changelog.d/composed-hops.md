### Added — hedged hops compose: `--impact` counts who reaches a hedged caller, tables called by index join both walks

`--impact`'s `reaches_hedged=` (the functions that MAY reach the answer through a call the graph holds no edge for) now
composes. It adds every function that reaches one of those hedged callers through ordinary proven calls (only past hops
whose join names at most 16 calling functions), takes a third struct-field hop when every hop on the chain is a field call
(`<ring d="3">`, `<rh … d="3">`), and follows a new hop kind, `container`: a call through a table element or a called
parameter (`handlers[i]()`), the caller that `--callers` already printed as `called_by=`. Each `<rh>` row now prints its
call's forward width (`slot_fns=` for a field, `cands=` for a container). On tmux, `--impact=cmd_split_window_exec` went
from `reaches_hedged="1"` to `"10"` (cmdq_fire_command and its 9 proven callers) and `--impact=window_copy_cmd_cancel`
from 2 to 12; `reaches=` did not change on any of the 235 slot-bound functions measured. `--path`'s hedged `<bridge>` crosses
a container call (`via="container" cands=K`, at most 16) and lets a declined or container crossing with at most 4
candidates be one of the three crossings a slot-only chain could already take.

What it does NOT mean: every one of these is a hedge. A chain with one hedged hop is hedged, so none of them is ever
counted in `reaches=`, `reachable=`, `hops=`, `by_depth=` or `radius_*`, and `<bridge proven="0">` stays. A wider crossing
is not followed and nothing says "not reached" because of it. Gate: `test/composedhopscheck.sh`.
