### Added — an unreached `--path` prints the bodies of the route it names

A `reachable="0"` `--path` (and MCP `path_between`) that names a route it could not trace — the bridge rows (a poster, the queue
consumer, the dispatcher) and the `<handler_rule>` readers — now prints those functions' bodies in `<bodies of="route"
proven="0">`: the bridge rows within 2 hops of `from=` (never `to=`), then the readers, at most 4 bodies (past that `capped="1"`
and `next=` the first one left out), each whole up to 4000 bytes. Every body is a hedge: one under a crossing carries that hop's
`via=` and `cands=` (`via="queue" cands="3"`), a reader's carries `via="name-key"`, and none of it enters `reachable=` or `hops=`.
Inside a `--for` answer's graph-verb block it stays off (that answer serves its own bodies; the one in-sample fire there, a
vscode `--for`, added 7.7 KB that named nothing the question asked). `RIPWIRE_NO_HOP_BODY=1` turns it off.

Why: textual's `--path=post_message,on_*` named `post_message -> _process_messages_loop -> _dispatch_message` and the reader
`_on_message`, but the answer to "how does a posted message reach its handler" lives in their bodies (the queue put, the loop's
get, the bubble to `_parent`). Measured: textual-11 goes from 4 to 6 of 6 required items by Rule C (◐ -> ✓), +7.0 KB.

### Fixed — the middle-hop body and the wrapper follow say what they prove

- A middle hop reached by name (`obj.process(x)` with two `process` definitions) prints its body with `via="name" cands="2"` on
  the `<b>`, never as if it were the proven callee (review F1).
- A `--for` wrapper follow needs a function or method whose text holds exactly one call site of any kind, that call its last
  statement: a declined, external or undefined call beside it, logic around it, a loop aggregator or a class field initializer
  is no wrapper (F2, F5, F6). A C `*out = v;` line counts as a statement (F3); a Python docstring does not (F4).
- `follow_of=` names the wrapper as its row does (`ChildRunner.launch`), and one answer follows at most 3 bodies; past that
  `<follow_cut n= next=>` counts the wrappers left and names the first callee.
