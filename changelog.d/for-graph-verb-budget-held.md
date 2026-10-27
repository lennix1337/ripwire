### Fixed — the `--for` graph-verb block obeys `--token-budget` / MCP `budget_tokens`

With a token budget set, the `<graph_verb>` block no longer appends the verb's answer (it was priced outside `est_tokens`): it is a
self-closed tag, `held="budget"`, whose `verb=` is the one call to run, and its legend says so. Gate: `test/forroutecheck.sh` (b1a, b2).
