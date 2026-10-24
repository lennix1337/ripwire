### Fixed — TS/JS `run=` runs from the package directory (#335)

A TS/JS test in a subdirectory package got a `run=` that could not run. The command was root-relative. Run from the root,
the runner found no package config. Run from the package, the path was root-relative. Now the command starts with
`cd <package> &&`, and the path after it is package-relative. A multi-root run has no `root=` and keeps the plain command. Paste it in a fresh shell at the root. The `cd` stays in a shell that is reused.

A bracketed path, such as a Next.js route (`app/[slug]/page.test.ts`), had `--` before it. vitest reads `--` as the end
of its options and drops the file filter, so the row ran the whole suite. vitest and jest now get one quoted path.

Two paths get no `run=` now. A path that starts with `-` has no spelling that runs it. Node's test runner globs its
file arguments, so a bracketed path under `node --test` matches no file and the runner exits 1. Both rows print `run_unknown="1"`.

jest reads its argument as a regular expression, so a path with a bracket or a parenthesis has those bytes escaped with a backslash.

A package at `root=` keeps its command. A path there that is not shell-safe loses the `--`. For `node --test`, a glob byte in the path gives `run_unknown="1"`.

Limit: jest escapes a path only when it has a byte a shell would quote. A plain dot stays a regex dot, so the filter may also match a sibling such as `route/test.ts`.
