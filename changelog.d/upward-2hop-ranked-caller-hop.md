### Added — a `--for` task about an error carries the callers chain of the narrow-funnel function it ranks

A `--for` task that names an error (a whole word of error, exception, panic, raise, throw, fail, crash, segfault, Traceback and their inflections, or an error type written as one identifier such as `DependencyCycleError` or `dependency_cycle_error`; not a "How does ..." explanation question) but no function now
carries the same `<callers>` element the name-exact route carries, marked `by="rank"`, when one of the answer's r=1..3 rows is
ONE function (the callers verb's own resolution, exact or no row) with one to three callers: its 1-hop callers, then their
callers as `d="2"` rows (at most 4), `next=` the whole caller radius. The seed is read from the rows `<sigs>` emitted; a row
whose every caller is a test path, or whose every caller was matched by name alone (an interface or duck-typed call), is
skipped; among the rest the seed is the first whose own body holds a string literal with two of the task's words (the function
that raises the message), else the first, and only for a statement, not a question. And the element has its own present-only legend clause saying it is a lead found
by rank, not a function the task named. Everything outside the element and its clause is byte-identical to the answer without
it (`RIPWIRE_NO_CALLER_HOP=1`); `--json`, `--signatures-only` and an explicit `--token-budget` carry none. Measured on in-sample
tasks: three error-message questions (two Go large-repository rows, one C++) now name the chain above the function that
detects the error; it fires on 0 of 163, 1 of 39 and 5 of 56 questions in three in-sample sets (0 %, +0.15 % and +0.35 % bytes). Gate: `test/forcallerhopcheck.sh` (R arms).
