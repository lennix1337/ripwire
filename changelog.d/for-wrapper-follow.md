### Added — the `--for` body fold follows a one-line wrapper into the function it calls

When a body the `--for` fold serves is a thin delegator (one proven callee, no other call, at most 3 statement lines, such as
`fetch` returning `this.#dispatch(...)`), the callee's body is now printed right after it as `<b follow_of="fetch">`, up to two
levels (`handle -> fetch -> #dispatch`). A callee bound by name alone, a body with a second call or a fourth statement line, and
a body the answer already carries are never followed. The followed bodies ride outside the fold's `budget_bytes=`, each whole up
to 4000 bytes (a longer one is cut and says so). `RIPWIRE_NO_HOP_BODY=1` turns it off.

Measured: hono's default `--for "How does a request flow from app.fetch to a handler's Response?"` now carries `#dispatch`'s body
(router match, Context, the one-handler fast path, `compose`, error handling) after `fetch` (+1.9 KB). Over the in-sample tables
the follow fires on 3 of 163 S1 rows (+4.3 KB, all already complete), 2 of 39 MF1-DEV rows (+3.0 KB; ninja `Plan::AddTarget ->
AddSubTarget`) and 3 of 56 LARGE-DEV rows (+5.7 KB, one body cut at the 4000-byte guard); no row loses an item.
Gate: `test/hopbodycheck.sh`.
