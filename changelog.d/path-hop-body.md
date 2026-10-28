### Added — a short `--path` answer prints the body of its middle hop

A reached `--path` (and MCP `path_between`) whose chain has 2 hops now prints, after its hop rows, the body of its
middle hop inside `<bodies of="hops">`: the same `<b>` element `--expand` prints, without callee rows. The endpoints never get a
body. Each body is whole up to 4000 bytes; a longer one is cut at a line end and says so on the element (`truncated="1"
lines="1-K/T" next=` the call that serves the rest). A chain of more hops prints no body, and its root carries
`next="--expand=…"` naming the first middle hop. Inside a `--for` answer's graph-verb block, a middle hop whose body the
answer already prints is not printed twice. `RIPWIRE_NO_HOP_BODY=1` turns it off.

Why: `--path=fetch,compose` on hono named `Hono.fetch -> Hono.#dispatch -> compose` and stopped there, while the whole request
flow (path extraction, router match, Context, the one-handler fast path, compose, error handling) lives in `#dispatch`'s body.
Measured on the 163-row S1 replay: hono-12 goes from 0 to 6 of 7 required items (✗ -> ◐ by Rule C, ✗ -> ✓ legacy), 0 rows
lower; 9 of the 15 `--path` rows change, +3.1 KB in total (hono-12 +2.1 KB, kysely-14 +0.5 KB, six longer chains +~90 B of
`next=` each). The 2-hop gate is measured: 3 hops would add 1.6 KB on one already-complete row, 4 hops 13 KB on two more, and no
item. On LARGE-DEV one `--for` graph-verb path block gains a middle body (zed `get_any_active_multi_workspace`, +1.0 KB) and seven
gain only `next=`. Gate: `test/hopbodycheck.sh`.
