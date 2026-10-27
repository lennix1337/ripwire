### Fixed — TypeScript: `this.f.m()` on a field the class types never binds the caller's own `m`, and never vanishes

A call such as `this.pub.provide( x )` inside `class Executor` with `pub: Provider` was bound by NAME to `Executor.provide`
itself — a false caller and callee row — and inside `provide` the same call on a `#private` field (`this.#provider.provide()`)
disappeared: the caller's own method was the one same-file candidate, the self-loop was then dropped, and `--callees` said the
method calls nothing (kysely `DefaultQueryExecutor`). Now a member call whose receiver is `this.<field>` (`self.<field>` in
Python) and whose field the class states as an in-tree class or interface other than the caller's class or one of its bases
never binds the caller's own definition: an interface answers through the dispatch tier as a `<dc gate="iface">` row listing its
implementors (one structural candidate included, as `cands="1"`), a class through its own member. `#private` field declarations,
`this.#f = …` and `field!: T` carry their declared type, and an `import type { T }` names T's module, so an interface it imports
is found. An untyped field, one typed outside the tree, or one typed by a TypeScript `type` alias (a union, an alias of a base class:
it may hold the caller's own class) keeps the by-name rows (a stated floor). Measured on the 163 stored
S1 answers: 36 change, Rule C 122 → 123 and legacy 134 → 135 (kysely-06 ✗ → ✓), none lower. Gate: `test/receiverfixescheck.sh` (T).
