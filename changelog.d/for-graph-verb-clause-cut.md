### Fixed — the `--for` graph-verb block reads its endpoints from the right sentence

The endpoints of the `<graph_verb endpoints="subject">` block (`path from A to B`, `callers of X`, `common callers of A and B`) are
now cut at the end of their own phrase: a word ending in `.` `;` `:` `!` `?` `,` (or the full-width `。；：！？，、`), or a clause word
(and, or, then, also, but, how, which, where, while, because, afterwards, explain, check, …). Before, the last endpoint ran to the end of the
task and the candidates were tried last-first, so `path from App.run to Widget.refresh. Afterwards summarise MessagePump.post_message`
ran the path to `MessagePump.post_message`. A phrase with no code-like token now fires nothing. `` `compose()` ``, `«Context.json»`
and `Context.json？` are read as endpoints, and `e.g.` / `i.e.` never are. Gate: `test/forroutecheck.sh` (n5, n6, n7, c3, s4a-c).
